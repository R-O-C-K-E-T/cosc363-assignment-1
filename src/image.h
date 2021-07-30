#pragma once

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

#include <memory>
#include <unordered_map>

class Texture;

std::shared_ptr<Texture> load_texture_unmanaged(std::string filename);

class Texture {
    friend std::shared_ptr<Texture> load_texture_unmanaged(std::string);

    public:
        Texture(Texture const&) = delete;
        void operator=(Texture const&) = delete;

        void bind() { glBindTexture(GL_TEXTURE_2D, texture_name); }
        void unbind() { glBindTexture(GL_TEXTURE_2D, 0); }

        ~Texture() { glDeleteTextures(1, &texture_name); }
    private:
        Texture(GLuint texture_name) : texture_name(texture_name) {}
        GLuint texture_name;
};