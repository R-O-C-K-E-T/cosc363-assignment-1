#include "collider.h"

#include "util.h"

ShapeProperties compute_shape_properties(const Shape& shape) {
    const auto canonical = glm::mat3(2, 1, 1,  1, 2, 1,  1, 1, 2) / 120.0f; // Covariance of tetrahedron (0,0,0), (1,0,0), (0,1,0), (0,0,1)

    auto covariance = glm::mat3(0.0);
    auto volume     = 0.0f;
    auto com        = glm::vec3(0.0);
    for (auto& face : shape.get_faces()) {
        auto a = glm::mat3(
            shape.get_vertices()[face[0].vertex], 
            shape.get_vertices()[face[1].vertex],
            shape.get_vertices()[face[2].vertex]
        );
        float det = glm::determinant(a);
        volume += det;
        com += (a[0] + a[1] + a[2]) * det; // The fourth vertex is excluded as it is (0,0,0)
        covariance += det * a * canonical * glm::transpose(a);
    }
    com /= 4;
    com /= volume;

    volume /= 6.0f;

    //covariance -= volume * glm::outerProduct(com, com);  
    
    auto inertia = (trace(covariance)*glm::mat3(1.0f) - covariance);
    return { inertia, com, volume };
}

ShapeCollider::~ShapeCollider() {}

void BVHShapeCollider::rebuild_bvh() {
    tree = {};
    
    for (auto& face : get_shape().get_faces()) {
        AABB bounds({
            get_shape().get_vertices()[face[0].vertex],
            get_shape().get_vertices()[face[1].vertex],
            get_shape().get_vertices()[face[2].vertex],
        });

        tree.add_node(bounds, &face);
    }
}

BVHShapeCollider::~BVHShapeCollider() {}

SphereCollider::~SphereCollider() {}