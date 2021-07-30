#pragma once

#include <vector>

#include <glm/glm.hpp>

struct AABB {
    AABB(glm::vec3 lower, glm::vec3 upper) : lower(lower), upper(upper) {}
    AABB(const std::vector<glm::vec3>& points);

    AABB make_union(const AABB& other) const;
    AABB expand(float radius) const;

    AABB apply_transform(const glm::mat4& transform) const;
    AABB translate(const glm::vec3& offset) const;

    float volume() const { return (upper.x - lower.x) * (upper.y - lower.y) * (upper.z - lower.z); }

    bool contains(const AABB& other) const;
    bool intersect(const AABB& other) const;

    std::vector<glm::vec3> get_points() const;

    void render() const;

    glm::vec3 lower;
    glm::vec3 upper;
};

const AABB NAN_BOUNDS = { glm::vec3(std::nanf("")), glm::vec3(std::nanf("")) };