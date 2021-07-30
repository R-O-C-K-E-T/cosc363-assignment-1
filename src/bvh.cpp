#include "bvh.h"

#include <functional>


void RawBVHTree::add_raw_node(const AABB& bounds, void* data) {
    auto node = std::make_unique<BVHNode>(bounds, data);

    if (!root) {
        root = std::move(node);
        return;
    }
    insert_raw_node(root, std::move(node));
}

void RawBVHTree::insert_raw_node(std::unique_ptr<BVHNode>& parent, std::unique_ptr<BVHNode> child) {
    if (parent->is_leaf()) {
        auto sibling = std::move(parent);

        parent = std::make_unique<BVHNode>(sibling->bounds.make_union(child->bounds));

        parent->children[0] = std::move(child);
        parent->children[1] = std::move(sibling);
    } else {
        const AABB bounds_a = parent->children[0]->bounds;
        const AABB bounds_b = parent->children[1]->bounds;

        float area_diff_a = bounds_a.make_union(child->bounds).volume() - bounds_a.volume();
        float area_diff_b = bounds_b.make_union(child->bounds).volume() - bounds_b.volume();

        if (area_diff_a < area_diff_b) {
            insert_raw_node(parent->children[0], std::move(child));
        } else {
            insert_raw_node(parent->children[1], std::move(child));
        }

        parent->bounds = parent->children[0]->bounds.make_union(parent->children[1]->bounds);
    }
}

std::vector<void*> RawBVHTree::intersect_raw(const AABB& bounds) const {
    std::vector<void*> result = {};

    std::function<void(BVHNode*)> visitor = [&](BVHNode* node) {
        if (!node->bounds.intersect(bounds)) return;

        if (node->is_leaf()) {
            result.push_back(node->data);
        } else {
            visitor(node->children[0].get());
            if (node->children[1]) {
                visitor(node->children[1].get());
            }
        }
    };

    if (root) visitor(root.get());

    return result;
}

void RawBVHTree::debug_draw() const {
    std::function<void(BVHNode*)> visitor = [&](BVHNode* node) {
        if (!node) return;  

        node->bounds.render();

        visitor(node->children[0].get());
        visitor(node->children[1].get());
    };

    visitor(root.get());
}