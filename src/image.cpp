#include "image.h"

#include <GL/glu.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

std::shared_ptr<Texture> load_texture_unmanaged(std::string filename) {
    int width, height, channels;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 0);
    if (!data) {
        return {}; // Failed to load image :(
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    if (!texture) {
        stbi_image_free(data);
        return {}; // Failed to generate texture
    }
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (channels == 3) {
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
    } else if (channels == 4) {
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
    } else {
        assert(false);
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    struct make_shared_enabler : public Texture {
        make_shared_enabler(GLuint texture) : Texture(texture) {};
    };
    return std::make_shared<make_shared_enabler>(texture);
}