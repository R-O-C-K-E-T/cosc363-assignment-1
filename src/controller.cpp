#include "controller.h"

#include <algorithm>

#include <GLFW/glfw3.h>

#include "scene.h"
#include "sweep.h"

class StandardController final: public Controller {
    public:
        StandardController(Scene& scene) : Controller(scene) {
            ball = scene.get_object("Sphere");
            seesaw = scene.get_object("SeesawBody");
            ramp_start = scene.get_object("Sphere.001")->position;

            add_sweep_surface();
        }
        ~StandardController() {}

        void press_key(int key) override {
            if (key == GLFW_KEY_SPACE && ball) {
                auto object = try_add_ball(scene.camera_pos);
                if (object) {
                    object->linear_velocity = glm::mat3(gen_rotation(scene.camera_rot)) * glm::vec3(0, 0, -10);
                }
            }
        }

        void update() override {
            // Remove objects that are too low
            scene.objects.erase(std::remove_if(scene.objects.begin(), scene.objects.end(), [&](const std::shared_ptr<Object>& object) {
                return object->position.y < -20.0f;
            }), scene.objects.end());

            if (seesaw) {
                auto offset = glm::dot(glm::vec3(1.0, 0.0, 0.0), seesaw->orientation * glm::vec3(0.0, 1.0, 0.0));
                seesaw->angular_velocity.z = seesaw->angular_velocity.z * 0.98 + offset * 0.5;
            }
            if (is_triggered) {
                if (scene.tick > trigger_time + wait) {
                    if (try_add_ball(ramp_start)) {
                        //std::cout << "Waited: " << wait << std::endl;
                        is_triggered = false;
                    }
                }
            } else {
                if (scene.tick > trigger_time + 6.0f) {
                    //std::cout << glm::to_string(ball->position) << std::endl;
                    //if (ball->position.x < -12.1f) {
                    if (ball->position.z > 17.5) {
                        trigger_time = scene.tick;
                        //wait = 0.1f;
                        wait = 0.185f;
                        auto velocity = glm::length(ball->linear_velocity);

                        auto velocity_diff = velocity - 6.19398f;

                        wait -= velocity_diff * 0.8f;
                        wait = std::max(wait, 0.0f);
                        //std::cout << velocity << std::endl;
                        is_triggered = true;
                    }
                }
            }
        }

    private:
        void add_sweep_surface() {
            std::vector<glm::vec3> points = {
                glm::vec3(0.00, 0.0, 0),
                glm::vec3(0.20, 0.0, 0),
                glm::vec3(0.30, 0.2, 0),  
                glm::vec3(0.35, 0.4, 0),  
                glm::vec3(0.38, 0.6, 0),  
                glm::vec3(0.35, 0.8, 0),
                glm::vec3(0.30, 1.0, 0),
                glm::vec3(0.20, 1.2, 0),
                glm::vec3(0.25, 1.3, 0),
            };
            auto initial_size = points.size();

            for (size_t i = 0; i<initial_size; i++) {
                auto point = points[initial_size - 1 - i];
                points.push_back(point + glm::vec3(-0.01f, 0.01f, 0.0f));
            }

            auto shape = generate_sweep_surface(points, 32);

            auto object = std::make_shared<Object>(shape, shape, nullptr);
            object->colour = glm::vec3(1.0, 0.0, 0.0);
            object->position = glm::vec3(0.0, 0.0, 25.0);
            object->reuse_shadow = true;
            scene.objects.push_back(object);
        }

        std::shared_ptr<Object> try_add_ball(glm::vec3 pos) {
            auto new_ball = std::make_shared<Object>(*ball);

            new_ball->position = pos;
            new_ball->update_transform();

            auto bounds = new_ball->get_physics_bounds();
            if (!scene.is_empty(bounds)) {
                return {};
            }

            new_ball->angular_velocity = new_ball->linear_velocity = glm::vec3(0.0f);

            scene.objects.push_back(new_ball);

            return new_ball;
        }

        std::shared_ptr<Object> ball;
        std::shared_ptr<Object> seesaw;
        glm::vec3 ramp_start;

        float trigger_time = 0.0f;
        float wait;
        bool is_triggered = false;
};

std::unique_ptr<Controller> make_controller(Scene& scene) {
    return std::make_unique<StandardController>(scene);
}