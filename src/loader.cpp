#include "loader.h"

#include <fstream>
#include <iostream>

#include <glm/glm.hpp>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

namespace glm {
    template<length_t L, typename T>
    void to_json(nlohmann::json& j, const vec<L, T>& v) {
        j = json::array();
        for (length_t i = 0; i<L; i++) {
            j[i] = v[i];
        }
    }
    template<length_t L, typename T>
    void from_json(const nlohmann::json& j, vec<L, T>& v) {
        for (length_t i = 0; i<L; i++) {
            j.at(i).get_to(v[i]);
        }
    }

    // Serialised matrices are stored in row major order as opposed to column major
    template<length_t C, length_t R, typename T>
    void to_json(nlohmann::json& j, const mat<C, R, T>& m) {
        mat<R, C, T> transposed = transpose(m);
        j = json::array();
        for (length_t i = 0; i<R; i++) {
            j[i] = transposed[i];
        }
    }

    template<length_t C, length_t R, typename T>
    void from_json(nlohmann::json& j, const mat<C, R, T>& m) {
        mat<R, C, T> transposed = {};
        for (length_t i = 0; i<R; i++) {
            j.at(i).get_to(transposed[i]);
        }
        m = glm::transpose(transposed);
    }
}

std::shared_ptr<Shape> ResourceManager::load_shape(std::string filename) {
    filename = fs::canonical(path_prefix / filename).string();
    
    auto it = shape_store.find(filename);
    if (it != shape_store.end()) {
        auto ptr = it->second.lock();
        if (ptr) {
            return ptr; // There is a valid entry in the store
        }
    }

    auto result = load_shape_unmanaged(filename);
    shape_store[filename] = result;
    return result;
}

std::shared_ptr<ShapeCollider> ResourceManager::load_shape_collider(std::string filename) {
    std::string canonical_path = fs::canonical(path_prefix / filename).string();
    auto it = shape_collider_store.find(canonical_path);
    if (it != shape_collider_store.end()) {
        auto ptr = it->second.lock();
        if (ptr) {
            return ptr; // There is a valid entry in the store
        }
    }

    auto shape = load_shape(filename);

    std::shared_ptr<ShapeCollider> result;
    if (false) { //(shape->get_vertices().size() > 1000) {
        result = std::make_shared<BVHShapeCollider>(shape);
    } else {
        result = std::make_shared<ShapeCollider>(shape);
    }

    shape_collider_store[canonical_path] = result;
    return result;
}

std::shared_ptr<Texture> ResourceManager::load_texture(std::string filename) {
    filename = fs::canonical(path_prefix / filename).string();
    
    auto it = texture_store.find(filename);
    if (it != texture_store.end()) {
        auto ptr = it->second.lock();
        if (ptr) {
            return ptr; // There is a valid entry in the store
        }
    }

    auto result = load_texture_unmanaged(filename);
    texture_store[filename] = result;
    return result;
}

std::shared_ptr<Texture> ResourceManager::load_skybox_texture(std::string filename) {
    auto texture = load_texture_unmanaged(fs::canonical(path_prefix / filename).string());

    // Changes filters to prevent seams
    texture->bind();
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    texture->unbind();

    return texture;
}

void ResourceManager::load_scene(Scene& scene, std::string filename) {
    filename = (path_prefix / filename).string();

    std::ifstream file(filename);
    auto data = json::parse(file);

    scene.tick = 0.0f;

    try {
        data.at("gravity").get_to(scene.gravity);
    } catch (json::basic_json::out_of_range) {
        scene.gravity = {0.0, 0.0, 0.0};
    }

    try {
        data.at("ambient_colour").get_to(scene.ambient_colour);
    } catch (json::basic_json::out_of_range) {
        scene.ambient_colour = {0.2, 0.2, 0.2};
    }

    if (data.contains("skybox")) {
        for (size_t i = 0; i<scene.skybox.size(); i++) {
            //scene.skybox[i] = load_texture(data["skybox"].at(i));
            scene.skybox[i] = load_skybox_texture(data["skybox"].at(i));
        }
    }

    for (auto const& elem : data.at("objects")) {
        std::shared_ptr<Shape> main_shape;
        if (elem.contains("main_shape")) {
            main_shape = load_shape(elem["main_shape"]);
        }

        std::shared_ptr<Shape> shadow_shape = main_shape;
        if (elem.contains("shadow_shape")) {
            auto shadow_json = elem["shadow_shape"];
            if (shadow_json.is_null()) {
                shadow_shape = nullptr;
            } else {
                shadow_shape = load_shape(shadow_json);
            }
        }

        std::shared_ptr<Collider> collider;
        if (elem.contains("collider")) {
            auto collider_json = elem["collider"];
            
            std::string type = collider_json.at("type");
            if (type == "shape") {
                collider = load_shape_collider(collider_json.at("source"));
            } else if (type == "sphere") {
                collider = std::make_shared<SphereCollider>(collider_json.at("radius"));
            } else {
                assert(false);
            }
        }

        std::shared_ptr<Object> object;
        if (elem.contains("name")) {
            std::string name = elem["name"];
            object = std::make_shared<Object>(name, main_shape, shadow_shape, collider);
        } else {
            object = std::make_shared<Object>(main_shape, shadow_shape, collider);
        }

        if (elem.contains("position")) {
            elem["position"].get_to(object->position);
            object->update_transform();
        }

        if (elem.contains("linear_velocity")) {
            elem["linear_velocity"].get_to(object->linear_velocity);
        }

        if (elem.contains("angular_velocity")) {
            elem["angular_velocity"].get_to(object->angular_velocity);
        }

        if (elem.contains("reuse_shadow")) elem["reuse_shadow"].get_to(object->reuse_shadow);

        bool rotatable = true;
        if (elem.contains("rotatable")) elem["rotatable"].get_to(rotatable);

        bool moveable  = true;
        if (elem.contains("moveable")) elem["moveable"].get_to(moveable);

        if (elem.contains("mass")) {
            assert(!elem.contains("density"));

            float mass = elem["mass"];
            if (moveable)
                object->set_mass(mass);
            if (collider && rotatable)
                object->set_inertia_mass(mass);
        } else if (elem.contains("density")) {
            float density = elem["density"];
            if (moveable)
                object->set_density(density);
            if (collider && rotatable)
                object->set_inertia_density(density);
        }

        if (elem.contains("friction")) object->friction = elem["friction"];
        if (elem.contains("restitution")) object->restitution = elem["restitution"];

        if (elem.contains("texture")) {
            object->texture = load_texture(elem["texture"]);
        }

        if (elem.contains("colour")) {
            elem["colour"].get_to(object->colour);
        }

        scene.objects.push_back(object);
    }

    if (data.contains("constraints")) {
        for (auto const& elem : data["constraints"]) {
            std::string type = elem.at("type");
            auto obj_a = scene.get_object(elem.at("objects").at(0));
            auto obj_b = scene.get_object(elem.at("objects").at(1));

            assert(obj_a != obj_b);

            if (type == "ball_and_socket") {
                glm::vec3 local_a = elem.at("locals")[0];
                glm::vec3 local_b = elem.at("locals")[1];

                scene.constraints.push_back(std::move(std::make_unique<BallSocketJoint>(*obj_a, *obj_b, local_a, local_b)));
            } else if (type == "hinge") {
                glm::vec3 local_a = elem.at("locals")[0];
                glm::vec3 local_b = elem.at("locals")[1];

                glm::vec3 local_vec_a = elem.at("vectors")[0];
                glm::vec3 local_vec_b = elem.at("vectors")[1];

                scene.constraints.push_back(std::move(std::make_unique<HingeJoint>(*obj_a, *obj_b, local_a, local_b, local_vec_a, local_vec_b)));
            } else {
                assert(false);
            }
        }
    }

    for (auto const& elem : data.at("lights")) {
        std::string type = elem.at("type");
        glm::vec3 colour = elem.at("colour");
        if (type == "point") {
            glm::vec3 position = elem["position"];
            scene.lights.push_back(create_point_light(position, colour));
        } else if (type == "direction") {
            glm::vec3 direction = elem["direction"];
            scene.lights.push_back(create_direction_light(direction, colour));
        } else {
            assert(false);
        }
    }
}

void ResourceManager::flush() {
    shape_store.clear();
    shape_collider_store.clear();
    texture_store.clear();
}