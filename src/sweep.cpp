#include "sweep.h"

#include "util.h"

std::shared_ptr<Shape> generate_sweep_surface(const std::vector<glm::vec3> points, size_t segments) {
    std::vector<glm::vec3> vertices = {};
    std::vector<glm::vec3> normals = {};
    std::vector<Face> faces = {};

    auto add_vertex = [&](glm::vec3 pos) {
        vertices.push_back(pos);
        return (int)vertices.size() - 1;
    };

    auto add_normal = [&](glm::vec3 normal) {
        normals.push_back(normal);
        return normals.size() - 1;
    };

    for (float i = 0; i<segments; i++) {
        float angle_a = (2.0f * glm::pi<float>()) * ((i + 0) / segments);
        float angle_b = (2.0f * glm::pi<float>()) * ((i + 1) / segments);

        auto rotation_mat_a = glm::mat3(gen_rotation_y(angle_a));
        auto rotation_mat_b = glm::mat3(gen_rotation_y(angle_b));

        for (size_t j = 0; j<points.size()-1; j++) {
            auto point_a = points[j + 0];
            auto point_b = points[j + 1];

            int points[] = {
                add_vertex(rotation_mat_a * point_a),
                add_vertex(rotation_mat_a * point_b),
                add_vertex(rotation_mat_b * point_b),
                add_vertex(rotation_mat_b * point_a),
            };

            int32_t normal_a = add_normal(glm::normalize(rotation_mat_a * glm::vec3(point_a.x, 0.0f, point_a.z)));
            int32_t normal_b = add_normal(glm::normalize(rotation_mat_b * glm::vec3(point_a.x, 0.0f, point_a.z)));

            faces.push_back({
                FaceVertex{
                    points[0],
                    -1,
                    normal_a,
                },
                FaceVertex{
                    points[1],
                    -1,
                    normal_a,
                },
                FaceVertex{
                    points[2],
                    -1,
                    normal_b,
                }
            });

            faces.push_back({
                FaceVertex{
                    points[0],
                    -1,
                    normal_a,
                },
                FaceVertex{
                    points[2],
                    -1,
                    normal_b,
                },
                FaceVertex{
                    points[3],
                    -1,
                    normal_b,
                }
            });
        }
    }

    return std::make_shared<Shape>(vertices, std::vector<glm::vec2>(), normals, faces);
}