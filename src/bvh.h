#pragma once

#include <memory>

#include "aabb.h"

class RawBVHTree {
    public:
        AABB get_bounds() const {
            if (!root) {
                return NAN_BOUNDS;
            }
            return root->bounds;
        }

        void debug_draw() const;

    protected:
        void add_raw_node(const AABB& bounds, void* data);
        std::vector<void*> intersect_raw(const AABB& bounds) const;

    private:
        struct BVHNode {
            BVHNode(AABB bounds, void* data) : bounds(bounds), data(data) {}
            BVHNode(AABB bounds) : BVHNode(bounds, nullptr) {}
            

            bool is_leaf() const { return children[0] == nullptr; }

            AABB bounds;
            std::unique_ptr<BVHNode> children[2] = {nullptr, nullptr};
            void* data;
        };
        void insert_raw_node(std::unique_ptr<BVHNode>& parent, std::unique_ptr<BVHNode> child);

        std::unique_ptr<BVHNode> root;
};

template<class T>
class BVHTree: public RawBVHTree {
    public:
        void add_node(const AABB& bounds, T* data) {
            add_raw_node(bounds, const_cast<void*>(reinterpret_cast<const void*>(data)));
        }

        std::vector<T*> intersect(const AABB& bounds) const {
            return intersect_raw(bounds);
        }
};

