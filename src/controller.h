#pragma once

#include <memory>

class Scene;

class Controller {
    public:
        virtual void update() {};
        virtual void press_key(int key) {};
        virtual ~Controller() {};

    protected:
        Controller(Scene& scene) : scene(scene) {}
        
        Scene& scene;
};

std::unique_ptr<Controller> make_controller(Scene&);