#pragma once

#include <vector>
#include <array>
#include <string>
#include <glm/glm.hpp>
#include <memory>

#include "util.h"
#include "image.h"
#include "shape.h"
#include "collider.h"
#include "vec12.h"

class MassMatrix;
class Scene;

class Object {
    public:
        // Explicit name
        explicit Object(std::string name, std::shared_ptr<Shape> main_shape, std::shared_ptr<Shape> shadow_shape, std::shared_ptr<Collider> collider) 
                : name(name), main_shape(main_shape), shadow_shape(shadow_shape), collider(collider) {
            update_bounds();
        }

        // Auto-assigned name
        explicit Object(std::shared_ptr<Shape> main_shape, std::shared_ptr<Shape> shadow_shape, std::shared_ptr<Collider> collider) 
                : main_shape(main_shape), shadow_shape(shadow_shape), collider(collider) {
            char buff[30];
            snprintf(buff, sizeof(buff), "object_%p", this);
            name = std::string(buff);
        }

        ~Object();

        glm::vec3 colour = glm::vec3(1.0);
        std::shared_ptr<Texture> texture = {};

        glm::vec3 linear_velocity = glm::vec3();
        glm::vec3 angular_velocity = glm::vec3();

        glm::vec3 position = glm::vec3();
        glm::mat3 orientation = glm::mat3(1.0f); // Local space to global space

        float friction = 0.5f;
        float restitution = 0.3f;

        bool reuse_shadow = false; // Only valid if light is non-moving

        std::string get_name() const {
            return name;
        }

        void load_transform() const;
        void update_orientation();
        void update_transform();

        void update_bounds();

        void render_solid() const;
        void render_shadow(Light light) const;
        void cache_shadow(const Light& light);
        void render_physics_bounds() const { physics_bounds.render(); }
        void render_render_bounds() const { render_bounds.render(); }

        AABB get_physics_bounds() const { return physics_bounds; }
        AABB get_render_bounds() const { return render_bounds; }

        void set_mass(float); // Doesn't update inertia
        void set_density(float);
        void set_infinite_mass();

        void set_inertia_mass(float);
        void set_inertia_density(float);
        void set_inertia(glm::mat3);
        void set_infinite_inertia();

        void update(const Scene& scene, float step_size);

        glm::vec3 local_to_global(glm::vec3 p) const { return orientation * p + position; }
        glm::vec3 global_to_local(glm::vec3 p) const { return glm::transpose(orientation) * (p - position); }
        glm::vec3 local_to_global_vec(glm::vec3 v) const { return orientation * v; }
        glm::vec3 global_to_local_vec(glm::vec3 v) const { return glm::transpose(orientation) * v; }

        std::shared_ptr<Shape> get_main_shape() { return main_shape; }
        std::shared_ptr<Shape> get_shadow_shape() { return shadow_shape; }
        std::shared_ptr<Collider> get_collider() { return collider; }

        friend vec12 constexpr combined_velocity_vector(const Object& a, const Object& b);
        friend void set_velocity(Object& a, Object& b, const vec12& v);
        friend MassMatrix;
    private:
        std::string name;

        glm::mat4 transform = glm::mat4(1.0);

        std::shared_ptr<Shape> main_shape;
        std::shared_ptr<Shape> shadow_shape;
        std::shared_ptr<Collider> collider;

        float mass = 0.0f;
        float inv_mass = 0.0f;

        glm::mat3 local_inverse_inertia = glm::mat3(0.0);
        glm::mat3 global_inverse_inertia = glm::mat3(0.0);

        AABB physics_bounds = NAN_BOUNDS;
        AABB render_bounds = NAN_BOUNDS;

        GLuint shadow_displaylist = 0;
};