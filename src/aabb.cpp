#include "aabb.h"

// TODO Remove
#include <iostream>

#include "util.h"

#undef min
#undef max

AABB::AABB(const std::vector<glm::vec3>& points) {
    lower = glm::vec3(+std::numeric_limits<float>::infinity());
    upper = glm::vec3(-std::numeric_limits<float>::infinity());
    for (auto point : points) {
        lower = glm::min(lower, point);
        upper = glm::max(upper, point);
    }
}

AABB AABB::make_union(const AABB& other) const {
    return AABB(glm::min(lower, other.lower), glm::max(upper, other.upper));
}

AABB AABB::expand(float radius) const {
    return AABB(lower - radius, upper + radius);
}

bool AABB::contains(const AABB& other) const {
    return upper.x >= other.upper.x && lower.x <= other.lower.x && 
           upper.y >= other.upper.y && lower.y <= other.lower.y &&
           upper.z >= other.upper.z && lower.z <= other.lower.z;
}

bool AABB::intersect(const AABB& other) const {
    return upper.x > other.lower.x && lower.x < other.upper.x &&
           upper.y > other.lower.y && lower.y < other.upper.y &&
           upper.z > other.lower.z && lower.z < other.upper.z;
}

std::vector<glm::vec3> AABB::get_points() const {
    std::vector<glm::vec3> points = {
        glm::vec3(lower.x, lower.y, lower.z),
        glm::vec3(upper.x, lower.y, lower.z),
        glm::vec3(lower.x, upper.y, lower.z),
        glm::vec3(upper.x, upper.y, lower.z),
        glm::vec3(lower.x, lower.y, upper.z),
        glm::vec3(upper.x, lower.y, upper.z),
        glm::vec3(lower.x, upper.y, upper.z),
        glm::vec3(upper.x, upper.y, upper.z),
    };
    return points;
}

AABB AABB::apply_transform(const glm::mat4& transform) const {
    // Quite naive
    auto points = get_points();
    for (auto& point : points) {
        point = transform * glm::vec4(point, 1.0f);
    }
    return AABB(points);
}

AABB AABB::translate(const glm::vec3& offset) const {
    return AABB(lower + offset, upper + offset);
}

void AABB::render() const {
    auto points = get_points();

    glBegin(GL_LINES);
    for (size_t i = 0; i<4; i++) {
        glVertex(points[2*i + 0]);
        glVertex(points[2*i + 1]);

        glVertex(points[((i + 2) & ~2) + 0]);
        glVertex(points[((i + 2) & ~2) + 2]);

        glVertex(points[i + 0]);
        glVertex(points[i + 4]);
    } 
    glEnd();
}