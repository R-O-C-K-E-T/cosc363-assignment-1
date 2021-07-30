#pragma once

#include "shape.h"

class Collider {
    public:
        virtual bool is_sphere_collider() const { return false; }
        virtual bool is_shape_collider() const { return false; }

        virtual float get_volume() const = 0;
        virtual glm::mat3 get_inertia() const = 0;
        virtual AABB get_bounds() const = 0;

        virtual ~Collider() {};
};

struct ShapeProperties {
    glm::mat3 inertia;
    glm::vec3 centre_of_mass;
    float volume;
};
ShapeProperties compute_shape_properties(const Shape& shape);

class ShapeCollider : public Collider {
    public:
        explicit ShapeCollider(std::shared_ptr<Shape> shape, const ShapeProperties& properties) : shape(shape), properties(properties) {
            assert(shape);
        };
        explicit ShapeCollider(std::shared_ptr<Shape> shape)
            : ShapeCollider(shape, compute_shape_properties(*shape)) {}
        ~ShapeCollider();

        bool is_shape_collider() const override { return true; }
        float get_volume() const override { return properties.volume; }
        glm::mat3 get_inertia() const override { return properties.inertia; }
        AABB get_bounds() const override { return shape->get_bounds(); }

        virtual bool is_bvh_shape_collider() const { return false; }
        Shape& get_shape() { return *shape; }
        const Shape& get_shape() const { return *shape; }

    private:
        std::shared_ptr<Shape> shape;
        ShapeProperties properties;
};

class BVHShapeCollider final : public ShapeCollider {
    public:
        explicit BVHShapeCollider(std::shared_ptr<Shape> shape) : ShapeCollider(shape) {
            rebuild_bvh();
        }

        explicit BVHShapeCollider(std::shared_ptr<Shape> shape, const ShapeProperties& properties) : ShapeCollider(shape, properties) {
            rebuild_bvh();
        };
        ~BVHShapeCollider();

        void rebuild_bvh();

        bool is_bvh_shape_collider() const override { return true; }

        const BVHTree<const Face>& get_bvh_tree() { return tree; }

        
    private:
        BVHTree<const Face> tree;
};

class SphereCollider final : public Collider {
    public:
        SphereCollider(float radius) : radius(radius) {}
        ~SphereCollider();

        bool is_sphere_collider() const override { return true; }
        float get_volume() const override { 
            return (4.0f / 3.0f * glm::pi<float>()) * (radius * radius * radius);
        }
        glm::mat3 get_inertia() const override {
            float moment = (2.0f/5.0f) * get_volume() * (radius * radius);
            return glm::mat3(moment);
        }
        AABB get_bounds() const { return AABB(glm::vec3(-radius), glm::vec3(radius)); }

        float get_radius() const { return radius; }
    private:
        float radius;
};