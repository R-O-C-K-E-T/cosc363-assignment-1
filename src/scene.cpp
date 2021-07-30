#include "scene.h"

// TODO Remove
#include <iostream>
#include <glm/gtx/string_cast.hpp>

Scene::Scene() {}
Scene::~Scene() {};

void Scene::update() {
    update_single(max_step_size);
}

//void Scene::update(float step_size) {
//    size_t substeps = std::ceil(step_size / max_step_size);
//    for (size_t i = 0; i<substeps; i++) {
//        update_single(step_size / substeps);
//    }
//}

void Scene::update(float step_size) {
    step_size += remaining_step;
    size_t substeps = std::floor(step_size / max_step_size);

    remaining_step = step_size - (substeps * max_step_size);

    for (size_t i = 0; i<substeps; i++) {
        update();
    }
}

void Scene::update_single(float step_size) {
    if (controller) controller->update();

    contacts.clear();
    //std::cout << step_size << std::endl;
    for (size_t i = 0; i<objects.size(); i++) {
        auto& obj_a = *objects[i];
        if (!obj_a.get_collider()) continue;

        for (size_t j = 0; j<i; j++) {
            auto& obj_b = *objects[j];

            if (!obj_b.get_collider()) continue;
            if (!obj_a.get_physics_bounds().intersect(obj_b.get_physics_bounds())) continue;
            evaluate_contact(obj_a, obj_b);
        }
    }
    //std::cout << std::endl;
    for (size_t i = 0; i<solver_steps; i++) {
        for (auto& constraint : constraints) {
            constraint->apply(*this, step_size);
        }
        for (auto& constraint : contacts) {
            constraint.apply(*this, step_size);
        }
        //std::cout << std::endl;
        //for (auto& constraint : contacts) {
        //    constraint.dump();
        //}
    }
    // std::cout << std::endl;
    // for (auto& constraint : contacts) {
    //     constraint.dump();
    // }
    
    for (auto object : objects) {
        object->update(*this, step_size);
    }

    tick += step_size;
}

//void Scene::slow_start() {
//    for (size_t i = 0; i<3; i++) {
//        update_single(0.005);
//    }
//}

void Scene::evaluate_contact(Object& object_a, Object& object_b) {
    const auto& collider_a = *object_a.get_collider();
    const auto& collider_b = *object_b.get_collider();
    if (collider_a.is_shape_collider()) {
        if (collider_b.is_sphere_collider()) {
            evaluate_contact(
                object_a, 
                object_b, 
                reinterpret_cast<const ShapeCollider&>(collider_a), 
                reinterpret_cast<const SphereCollider&>(collider_b)
            );
        }
    } else if (collider_a.is_sphere_collider()) {
        if (collider_b.is_sphere_collider()) {
            evaluate_contact(
                object_a, 
                object_b, 
                reinterpret_cast<const SphereCollider&>(collider_a), 
                reinterpret_cast<const SphereCollider&>(collider_b)
            );
        } else if (collider_b.is_shape_collider()) {
            evaluate_contact(
                object_b, 
                object_a, 
                reinterpret_cast<const ShapeCollider&>(collider_b),
                reinterpret_cast<const SphereCollider&>(collider_a)
            );
        }
    }
}

void Scene::evaluate_contact(Object& object_a, Object& object_b, const SphereCollider& collider_a, const SphereCollider& collider_b) {
    auto vec = object_b.position - object_a.position;
    float dist = glm::length(vec);
    if (dist == 0.0f) return;

    float penetration = collider_a.get_radius() + collider_b.get_radius() - dist;
    if (penetration < 0.0f) return;

    ContactConstraint constraint(object_a, object_b);

    vec = glm::normalize(vec);

    constraint.add_contact(
        vec * collider_a.get_radius(),
        vec * (-collider_b.get_radius()),
        vec
    );

    contacts.push_back(constraint);
}

// void Scene::evaluate_contact(Object& object_a, Object& object_b, const ShapeCollider& collider_a, const SphereCollider& collider_b) {
//     auto& shape = collider_a.get_shape();

//     auto test_point = object_a.global_to_local(object_b.position);

//     glm::vec3 closest = glm::vec3();
//     float dist = std::numeric_limits<float>::infinity();
//     for (auto& face : shape.get_faces()) {
//         glm::vec3 new_closest = project_point_to_triangle(
//             test_point, 
//             shape.get_vertices()[face[0].vertex], 
//             shape.get_vertices()[face[1].vertex], 
//             shape.get_vertices()[face[2].vertex]
//         );

//         glm::vec3 vec = test_point - new_closest;
//         float new_dist = glm::length(vec);
        
//         if (new_dist < dist) {
//             dist = new_dist;
//             closest = new_closest;
//         }
//     }

//     if (dist > collider_b.get_radius()) return;

//     glm::vec3 vec = test_point - closest;
//     vec /= dist;

//     vec = object_a.local_to_global_vec(vec);

//     auto a = object_a.local_to_global_vec(closest);
//     auto b = vec * (-collider_b.get_radius());

//     ContactConstraint constraint(object_a, object_b);
//     constraint.add_contact(
//         a,
//         b,
//         vec
//     );

//     contacts.push_back(constraint);
// }

void Scene::evaluate_contact(Object& object_a, Object& object_b, const ShapeCollider& collider_a, const SphereCollider& collider_b) {
    auto& shape = collider_a.get_shape();

    auto test_point = object_a.global_to_local(object_b.position);

    glm::vec3 closest = glm::vec3();
    float dist = std::numeric_limits<float>::infinity();

    for (auto& face : shape.get_faces()) {
        std::array<glm::vec3, 3> face_vertices = {
            shape.get_vertices()[face[0].vertex], 
            shape.get_vertices()[face[1].vertex], 
            shape.get_vertices()[face[2].vertex],
        };

        auto normal = glm::cross(face_vertices[1] - face_vertices[0], face_vertices[2] - face_vertices[0]);

        if (glm::dot(normal, test_point - face_vertices[0]) <= 0) continue;

        glm::vec3 new_closest = project_point_to_triangle(
            test_point, 
            face_vertices[0],
            face_vertices[1],
            face_vertices[2]
        );

        glm::vec3 vec = test_point - new_closest;
        float new_dist = glm::length(vec);
        
        if (new_dist < dist) {
            dist = new_dist;
            closest = new_closest;
        }
    }

    if (dist > collider_b.get_radius()) return;

    glm::vec3 closest_vec = test_point - closest;
    closest_vec /= dist;

    glm::vec3 next_closest = glm::vec3();
    float next_dist = std::numeric_limits<float>::infinity();
    for (auto& face : shape.get_faces()) {
        std::array<glm::vec3, 3> face_vertices = {
            shape.get_vertices()[face[0].vertex], 
            shape.get_vertices()[face[1].vertex], 
            shape.get_vertices()[face[2].vertex],
        };

        auto normal = glm::cross(face_vertices[1] - face_vertices[0], face_vertices[2] - face_vertices[0]);

        if (glm::dot(normal, test_point - face_vertices[0]) <= 0) continue;

        glm::vec3 new_closest = project_point_to_triangle(
            test_point, 
            face_vertices[0],
            face_vertices[1],
            face_vertices[2]
        );

        if (glm::length(new_closest - closest) < 0.1) continue; // Points are too close

        glm::vec3 vec = test_point - new_closest;
        float new_dist = glm::length(vec);

        if (glm::dot(closest_vec, vec) > new_dist * 0.98) continue; // Vectors are within 11 degrees
        
        if (new_dist < next_dist) {
            next_dist = new_dist;
            next_closest = new_closest;
        }
    }

    closest_vec = object_a.local_to_global_vec(closest_vec);

    ContactConstraint constraint(object_a, object_b);

    {
        auto a = object_a.local_to_global_vec(closest);
        auto b = closest_vec * (-collider_b.get_radius());

        constraint.add_contact(
            a,
            b,
            closest_vec
        );
    }
    


    if (next_dist <= collider_b.get_radius()) {
        auto next_closest_vec = test_point - next_closest;
        next_closest_vec /= next_dist;

        next_closest_vec = object_a.local_to_global_vec(next_closest_vec);

        auto a = object_a.local_to_global_vec(next_closest);
        auto b = next_closest_vec * (-collider_b.get_radius());

        constraint.add_contact(
            a,
            b,
            next_closest_vec
        );
    }
    
    contacts.push_back(constraint);
}

void Scene::render_skybox() const {
    glDepthMask(GL_FALSE);
    glPushMatrix();

    glm::mat4 transform;
    glGetFloatv(GL_MODELVIEW_MATRIX, &transform[0].x);
    transform[3] = glm::vec4(0, 0, 0, 1); // Removes translation component
    glLoadMatrixf(&transform[0].x);

    glEnable(GL_TEXTURE_2D);
    glColor(1,1,1);

    // Front +X
    if (skybox[0]) {
        skybox[0]->bind();
        glBegin(GL_TRIANGLE_FAN);

        glTexCoord(0, 1);
        glVertex( 1, -1,  1);

        glTexCoord(0, 0);
        glVertex( 1,  1,  1);

        glTexCoord(1, 0);
        glVertex( 1,  1, -1);

        glTexCoord(1, 1);
        glVertex( 1, -1, -1);

        glEnd();
    }

    // Back -X
    if (skybox[1]) {
       skybox[1]->bind();
       glBegin(GL_TRIANGLE_FAN);

       glTexCoord(0, 1);
       glVertex(-1, -1, -1);
    
       glTexCoord(0, 0);
       glVertex(-1,  1, -1);
    
       glTexCoord(1, 0);
       glVertex(-1,  1,  1);
    
       glTexCoord(1, 1);
       glVertex(-1, -1,  1);
       glEnd();
    }

    // Top +Y
    if (skybox[2]) {
        skybox[2]->bind();
        glBegin(GL_TRIANGLE_FAN);
        
        glTexCoord(1, 0);
        glVertex(-1,  1, -1);

        glTexCoord(1, 1);
        glVertex( 1,  1, -1);

        glTexCoord(0, 1);
        glVertex( 1,  1,  1);

        glTexCoord(0, 0);
        glVertex(-1,  1,  1);

        glEnd();
    }

    // Bottom -Y
    if (skybox[3]) {
        skybox[3]->bind();
        glBegin(GL_TRIANGLE_FAN);

        glTexCoord(0, 1);
        glVertex(-1, -1,  1);

        glTexCoord(0, 0);
        glVertex( 1, -1,  1);

        glTexCoord(1, 0);
        glVertex( 1, -1, -1);

        glTexCoord(1, 1);
        glVertex(-1, -1, -1);
        
        glEnd();
    }

    // Left +Z
    if (skybox[4]) {
        skybox[4]->bind();
        glBegin(GL_TRIANGLE_FAN);
        
        glTexCoord(0, 0);
        glVertex(-1,  1,  1);

        glTexCoord(1, 0);
        glVertex( 1,  1,  1);

        glTexCoord(1, 1);
        glVertex( 1, -1,  1);

        glTexCoord(0, 1);
        glVertex(-1, -1,  1);

        glEnd();
    }

    // Right -Z
    if (skybox[5]) {
        skybox[5]->bind();
        glBegin(GL_TRIANGLE_FAN);
        
        glTexCoord(1, 1);
        glVertex(-1, -1, -1);

        glTexCoord(0, 1);
        glVertex( 1, -1, -1);

        glTexCoord(0, 0);
        glVertex( 1,  1, -1);

        glTexCoord(1, 0);
        glVertex(-1,  1, -1);

        glEnd();
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glDisable(GL_TEXTURE_2D);

    glPopMatrix();
    glDepthMask(GL_TRUE);
}

void Scene::ambient_pass(glm::vec3 colour) const {
    for (auto object : objects) {
        auto litColour = object->colour * colour;
        glColor(litColour);
        object->render_solid();
    }
}

void Scene::light_pass(const Light& light) const {
    glEnable(GL_STENCIL_TEST);


    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);

    glDepthFunc(GL_LEQUAL);

    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glStencilMask(0xFF);

    glClear(GL_STENCIL_BUFFER_BIT);

    glCullFace(GL_FRONT);
    glStencilOp(GL_KEEP, GL_INCR, GL_KEEP);

    for (auto object : objects) {
        object->cache_shadow(light);
    }

    GLuint displaylist = glGenLists(1);
    glNewList(displaylist, GL_COMPILE_AND_EXECUTE);
    for (auto object : objects) {
        object->render_shadow(light);
    }
    glEndList();

    glCullFace(GL_BACK);
    glStencilOp(GL_KEEP, GL_DECR, GL_KEEP);

    glCallList(displaylist);
    glDeleteLists(displaylist, 1);

    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    
    glStencilFunc(GL_GREATER, 2, 0xFF);
    

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_EQUAL);
    glCullFace(GL_BACK);

    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);

    glm::vec4 pos;
    if (light.is_point) {
        pos = {light.pos, 1.0f};
    } else {
        pos = {-light.vec, 0.0f};
    }
    glm::vec4 colour = {light.colour, 1.0f};
    glLightfv(GL_LIGHT0, GL_DIFFUSE, &colour.x);
    glLightfv(GL_LIGHT0, GL_POSITION, &pos.x);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    for (auto object : objects) {
        glColor(object->colour);
        object->render_solid();
    }

    glDisable(GL_BLEND);

    glDisable(GL_LIGHTING);
    
    glStencilMask(0);
    glDisable(GL_STENCIL_TEST);
    glDepthFunc(GL_LEQUAL);
}

void Scene::render() const {
    render_skybox();
    ambient_pass(ambient_colour);
    for (auto& light : lights) {
        light_pass(light);
    }

    
    //glDisable(GL_BLEND);
    //glDepthFunc(GL_LESS);
    //ambient_pass({1.0, 1.0, 1.0});
    //for (auto object : objects) {
    //    glColor(0.0, 0.0, 0.0);
    //    object->render_shadow(lights[0]);
    //}

    //glBegin(GL_LINES);
    //for (auto& light : lights) {
    //    glVertex(0,0,0);
    //    glVertex(light.pos);
    //}
    //glEnd();

    if (debug_mode) {
        glDepthMask(GL_FALSE);
        glDisable(GL_BLEND);
        glDepthFunc(GL_ALWAYS);
        glColor(0.3, 0.3, 0.3);
        for (auto object : objects) {
            object->render_physics_bounds();
        }

        glColor(1, 1, 0);
        for (auto& contact : contacts) {
            contact.draw_debug();
        }

        glDepthFunc(GL_LEQUAL);

        glColor(1, 1, 1);
        for (auto object : objects) {
            object->render_physics_bounds();
        }

        for (auto object : objects) {
            auto collider = object->get_collider();
            if (collider) {
                if (collider->is_shape_collider()) {
                    auto& shape_collider = reinterpret_cast<ShapeCollider&>(*collider);
                    if (shape_collider.is_bvh_shape_collider()) {
                        glPushMatrix();
                        object->load_transform();
                        reinterpret_cast<BVHShapeCollider&>(shape_collider).get_bvh_tree().debug_draw();
                        glPopMatrix();
                    }
                }
            }
        }

        glDepthMask(GL_TRUE);
    }
}

std::shared_ptr<Object> Scene::get_object(std::string name) {
    for (auto object : objects) {
        if (object->get_name() == name) {
            return object;
        }
    }
    return {};
}

void Scene::clear() {
    objects.clear();
    constraints.clear();
    contacts.clear();
    lights.clear();
}

bool Scene::is_empty(AABB bounds) const {
    for (auto& object : objects) {
        if (object->get_collider() && bounds.intersect(object->get_physics_bounds())) {
            return false;
        }
    }
    return true;
}