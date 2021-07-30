#pragma once

#include <filesystem>
namespace fs = std::filesystem;

#include "scene.h"
#include "image.h"

class ResourceManager {
    public:
        static ResourceManager& the() {
            static ResourceManager instance;
            return instance;
        }

        ResourceManager(ResourceManager const&) = delete;
        void operator=(ResourceManager const&) = delete;

        void set_path_prefix(std::string path) {
            path_prefix = path;
        }

        void flush();

        std::shared_ptr<Shape> load_shape(std::string filename);
        std::shared_ptr<ShapeCollider> load_shape_collider(std::string filename);
        std::shared_ptr<Texture> load_texture(std::string filename);

        void load_scene(Scene&, std::string scene_file);

    private:
        ResourceManager() {}

        fs::path path_prefix = "";

        std::unordered_map<std::string, std::weak_ptr<Texture>> texture_store;
        std::unordered_map<std::string, std::weak_ptr<ShapeCollider>> shape_collider_store;
        std::unordered_map<std::string, std::weak_ptr<Shape>> shape_store;

        std::shared_ptr<Texture> load_skybox_texture(std::string filename);
};