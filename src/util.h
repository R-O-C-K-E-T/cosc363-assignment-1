#include <array>

#include <iostream>
#include <glm/gtx/string_cast.hpp>

#include <glm/glm.hpp>

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

inline glm::mat4 gen_rotation_x(float angle) {
    float s = std::sin(angle);
    float c = std::cos(angle);
    return glm::mat4(1, 0, 0, 0,  0, c, s, 0,  0, -s, c, 0,  0, 0, 0, 1);
}

inline glm::mat4 gen_rotation_y(float angle) {
    float s = std::sin(angle);
    float c = std::cos(angle);
    return glm::mat4(c, 0, s, 0,  0, 1, 0, 0,  -s, 0, c, 0,  0, 0, 0, 1);
}

inline glm::mat4 gen_rotation_z(float angle) {
    float s = std::sin(angle);
    float c = std::cos(angle);
    return glm::mat4(c, s, 0, 0,  -s, c, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1);
}

inline glm::mat4 gen_rotation(const glm::vec3& rot) {
    return gen_rotation_z(rot.z) * gen_rotation_y(rot.y) * gen_rotation_x(rot.x);
}

inline float trace(const glm::mat4& mat) {
    return mat[0][0] + mat[1][1] + mat[2][2] + mat[3][3];
}

inline float trace(const glm::mat3& mat) {
    return mat[0][0] + mat[1][1] + mat[2][2];
}

inline float trace(const glm::mat2& mat) {
    return mat[0][0] + mat[1][1];
}

inline bool isclose(float a, float b, float epsilon=1e-5) {
    return std::abs(a - b) < epsilon;
}

inline std::array<glm::vec3, 2> compute_tangents(const glm::vec3& normal) {
    auto tangent = glm::cross(normal, glm::vec3(1.0, 0.0, 0.0));
    auto length = glm::length(tangent);

    if (isclose(length, 0.0f)) {
        tangent = glm::cross(normal, glm::vec3(0.0, 1.0, 0.0));
        length = glm::length(tangent);

        if (isclose(length, 0.0f)) {
            tangent = glm::cross(normal, glm::vec3(0.0, 0.0, 1.0));
            length = glm::length(tangent);
        }
    }
    tangent /= length;

    return { tangent, glm::normalize(glm::cross(normal, tangent)) };
}

inline glm::vec3 project_point_to_line(const glm::vec3& point, const glm::vec3& line_a, const glm::vec3& line_b) {
    auto dir = line_b - line_a;

    float v = glm::dot(dir, point - line_a);
    if (v <= 0.0f) return line_a;
    float m = glm::dot(dir, dir);
    if (v >= m) return line_b;

    return line_a + dir * (v / m);
}

inline glm::vec3 project_point_to_triangle(glm::vec3 point, const glm::vec3& tri_a, const glm::vec3& tri_b, const glm::vec3& tri_c) {
    auto vec_ba = tri_a - tri_b;
    auto vec_ac = tri_c - tri_a;
    auto vec_cb = tri_b - tri_c;

    //std::cout << glm::to_string(point) << glm::to_string(tri_a) << glm::to_string(tri_b) << glm::to_string(tri_c) << "\n";

    auto normal = glm::normalize(glm::cross(vec_ba, vec_ac));
    
    auto projected_point = point - normal * (glm::dot(normal, point) - glm::dot(normal, tri_a)); // Point projected onto triangle
    //std::cout << glm::to_string(normal) << glm::to_string(point) << glm::to_string(projected_point) << std::endl;

    float u = glm::dot(glm::cross(vec_ba, projected_point - tri_b), normal);
    float v = glm::dot(glm::cross(vec_ac, projected_point - tri_a), normal);
    float w = glm::dot(glm::cross(vec_cb, projected_point - tri_c), normal);

    //std::cout << u << "," << v << "," << w << std::endl;

    if (u <= 0 && v <= 0) return tri_a;
    if (u <= 0 && w <= 0) return tri_b;
    if (v <= 0 && w <= 0) return tri_c;

    if (u <= 0) return project_point_to_line(point, tri_a, tri_b);
    if (v <= 0) return project_point_to_line(point, tri_a, tri_c);
    if (w <= 0) return project_point_to_line(point, tri_b, tri_c);

    return projected_point;
}

inline void glVertex(const glm::vec3& v) {
    glVertex3fv(&v.x);
}

inline void glVertex(float x, float y, float z) {
    glVertex3f(x, y, z);
}

inline void glColor(const glm::vec3& v) {
    glColor3fv(&v.x);
}

inline void glColor(float x, float y, float z) {
    glColor3f(x, y, z);
}

inline void glNormal(const glm::vec3& v) {
    glNormal3fv(&v.x);
}

inline void glNormal(float x, float y, float z) {
    glNormal3f(x, y, z);
}

inline void glTexCoord(const glm::vec2& v) {
    glTexCoord2fv(&v.x);
}

inline void glTexCoord(float x, float y) {
    glTexCoord2f(x, y);
}