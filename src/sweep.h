#pragma once

#include "shape.h"

std::shared_ptr<Shape> generate_sweep_surface(const std::vector<glm::vec3> points, size_t segments);