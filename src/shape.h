#pragma once

#include <vector>
#include <array>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "aabb.h"
#include "bvh.h"

struct Light {
    union {
        glm::vec3 pos;
        glm::vec3 vec;
    };
    glm::vec3 colour;
    bool is_point;
    float shadow_length = 40.0f;
};

inline Light create_point_light(glm::vec3 pos, glm::vec3 colour) {
    return { pos, colour, true };
}
inline Light create_direction_light(glm::vec3 vec, glm::vec3 colour) {
    return { glm::normalize(vec), colour, false };
}

struct FaceVertex {
    int32_t vertex  = { -1 };
    int32_t texture = { -1 };
    int32_t normal  = { -1 };
};
typedef std::array<FaceVertex, 3> Face;

class Shape {
    public:
        explicit Shape(
            const std::vector<glm::vec3>& vertices, 
            const std::vector<glm::vec2>& texture_coords, 
            const std::vector<glm::vec3>& vertex_normals, 
            const std::vector<Face> faces
        ) : vertices(vertices), texture_coords(texture_coords), vertex_normals(vertex_normals), faces(faces) {
            update_bounds();
        }
        ~Shape();

        const std::vector<glm::vec3>& get_vertices() const {
            return vertices;
        }

        const std::vector<glm::vec2>& get_texture_coords() const {
            return texture_coords;
        }

        const std::vector<glm::vec3>& get_vertex_normals() const {
            return vertex_normals;
        }

        const std::vector<Face>& get_faces() const {
            return faces;
        }

        void update_bounds();
        AABB get_bounds() const { return bounds; }

        void render_solid() const;
        void render_wireframe() const;
        void render_shadow(const Light& light) const;
        void render_face_normals(float length) const;
        void render_normals(float length) const;

        void dump() const;
    private:
        std::vector<glm::vec3> vertices;
        std::vector<glm::vec2> texture_coords;
        std::vector<glm::vec3> vertex_normals;
        std::vector<Face> faces;

        AABB bounds = NAN_BOUNDS;

        mutable GLuint solid_displaylist = 0;
};

std::shared_ptr<Shape> load_shape_unmanaged(std::string filename);