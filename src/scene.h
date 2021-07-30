#pragma once

#include <array>

#include "object.h"
#include "constraint.h"
#include "controller.h"

class Scene {
    public:
        explicit Scene();
        ~Scene();

        void update();
        void update(float time_step);
        void update_single(float time_step);
        //void slow_start();

        void render() const;

        bool is_empty(AABB) const;

        float baumgarte_bias = 0.2f;
        float max_step_size = 1.0f / 180.0f;
        size_t solver_steps = 8;
        bool debug_mode = false;

        double tick = 0.0;
        
        std::unique_ptr<Controller> controller = {};

        glm::vec3 camera_pos = glm::vec3(0.0f);
        glm::vec3 camera_rot = glm::vec3(0.0f);

        glm::vec3 gravity = glm::vec3(0.0f);
        
        glm::vec3 ambient_colour = glm::vec3(0.2f);

        std::vector<std::shared_ptr<Object>> objects;
        std::vector<std::unique_ptr<Constraint>> constraints;
        std::vector<Light> lights;
        std::array<std::shared_ptr<Texture>, 6> skybox; // Same as OpenGL Cubemap format

        std::shared_ptr<Object> get_object(std::string);

        void clear();
    private:
        std::vector<ContactConstraint> contacts;
        float remaining_step = 0.0f;

        void render_skybox() const;
        void light_pass(const Light& light) const;
        void ambient_pass(glm::vec3 colour) const;

        void evaluate_contact(Object&, Object&);
        void evaluate_contact(Object&, Object&, const SphereCollider&, const SphereCollider&);
        void evaluate_contact(Object&, Object&, const ShapeCollider&, const SphereCollider&);
};