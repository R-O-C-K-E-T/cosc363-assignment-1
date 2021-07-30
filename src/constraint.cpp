#include "constraint.h"

#include "scene.h"

#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

#undef max
#undef min

void BallSocketJoint::apply(const Scene& scene, float step_size) {
    glm::vec3 r0 = obj_a.local_to_global_vec(local_a);
    glm::vec3 r1 = obj_b.local_to_global_vec(local_b);

    std::array<vec12, 3> J = {
        vec12(-1.0f,  0.0f,  0.0f,   0.0f, -r0.z,  r0.y,  1.0f, 0.0f, 0.0f,   0.0f,  r1.z, -r1.y),
        vec12( 0.0f, -1.0f,  0.0f,   r0.z,  0.0f, -r0.x,  0.0f, 1.0f, 0.0f,  -r1.z,  0.0f,  r1.x),
        vec12( 0.0f,  0.0f, -1.0f,  -r0.y,  r0.x,  0.0f,  0.0f, 0.0f, 1.0f,   r1.y, -r1.x,  0.0f),
    };

    glm::vec3 bias = (scene.baumgarte_bias / step_size) * (obj_b.position + r1 - obj_a.position - r0);
    
    MassMatrix M(obj_a, obj_b);
    vec12 V = combined_velocity_vector(obj_a, obj_b);
    V += apply_constraint(J, M, resolve_constraint(J, M, V, bias));
    set_velocity(obj_a, obj_b, V);
}

// void FixedJoint::apply(const Scene& scene, float step_size) {
//     MassMatrix M(obj_a, obj_b);
//     vec12 V = combined_velocity_vector(obj_a, obj_b);
    
//     { // Constrain relative position
//         glm::vec3 r0 = obj_a.local_to_global_vec(local_a);
//         glm::vec3 r1 = obj_b.local_to_global_vec(local_b);

//         std::array<vec12, 3> J = {
//             vec12(-1.0f,  0.0f,  0.0f,   0.0f, -r0.z,  r0.y,  1.0f, 0.0f, 0.0f,   0.0f,  r1.z, -r1.y),
//             vec12( 0.0f, -1.0f,  0.0f,   r0.z,  0.0f, -r0.x,  0.0f, 1.0f, 0.0f,  -r1.z,  0.0f,  r1.x),
//             vec12( 0.0f,  0.0f, -1.0f,  -r0.y,  r0.x,  0.0f,  0.0f, 0.0f, 1.0f,   r1.y, -r1.x,  0.0f),
//         };

//         glm::vec3 bias = (scene.baumgarte_bias / step_size) * (obj_b.position + r1 - obj_a.position - r0);
        
//         V += apply_constraint(J, M, resolve_constraint(J, M, V, bias));
//     }

//     { // Constrain relative rotation
//         std::array<vec12, 3> J = {
//             vec12(0.0f, 0.0f, 0.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f),
//             vec12(0.0f, 0.0f, 0.0f,   0.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f),
//             vec12(0.0f, 0.0f, 0.0f,   0.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f),
//         };

//         glm::vec3 bias = (scene.baumgarte_bias / step_size) * glm::vec3(0.0f); // TODO Make bias correct

//         V += apply_constraint(J, M, resolve_constraint(J, M, V, bias));
//     }
    
//     set_velocity(obj_a, obj_b, V);
// }

void HingeJoint::apply(const Scene& scene, float step_size) {
    MassMatrix M(obj_a, obj_b);
    vec12 V = combined_velocity_vector(obj_a, obj_b);
    
    { // Position constraint
        glm::vec3 r0 = obj_a.local_to_global_vec(local_a);
        glm::vec3 r1 = obj_b.local_to_global_vec(local_b);

        std::array<vec12, 3> J = {
            vec12(-1.0f,  0.0f,  0.0f,   0.0f, -r0.z,  r0.y,  1.0f, 0.0f, 0.0f,   0.0f,  r1.z, -r1.y),
            vec12( 0.0f, -1.0f,  0.0f,   r0.z,  0.0f, -r0.x,  0.0f, 1.0f, 0.0f,  -r1.z,  0.0f,  r1.x),
            vec12( 0.0f,  0.0f, -1.0f,  -r0.y,  r0.x,  0.0f,  0.0f, 0.0f, 1.0f,   r1.y, -r1.x,  0.0f),
        };

        glm::vec3 bias = (scene.baumgarte_bias / step_size) * (obj_b.position + r1 - obj_a.position - r0);
        
        
        V += apply_constraint(J, M, resolve_constraint(J, M, V, bias));
    }
    { // Angle constraint
        glm::vec3 v0 = obj_a.local_to_global_vec(local_vec_a);
        glm::vec3 v1 = obj_b.local_to_global_vec(local_vec_b);

        auto tangents = compute_tangents(v1);
        std::array<vec12, 2> J = {
            vec12(glm::vec3(0.0f), glm::cross(v0, tangents[0]), glm::vec3(0.0f), glm::cross(tangents[0], v0)),
            vec12(glm::vec3(0.0f), glm::cross(v0, tangents[1]), glm::vec3(0.0f), glm::cross(tangents[1], v0)),
        };

        glm::vec2 bias = (scene.baumgarte_bias / step_size) * glm::vec2(glm::dot(v0, tangents[0]), glm::dot(v0, tangents[1]));

        V += apply_constraint(J, M, resolve_constraint(J, M, V, bias));
    }

    set_velocity(obj_a, obj_b, V);
}

void ContactConstraint::add_contact(glm::vec3 offset_a, glm::vec3 offset_b, glm::vec3 normal) {
    assert(contact_count < 2);
    auto& contact = contacts[contact_count++];

    contact.offsets[0] = offset_a;
    contact.offsets[1] = offset_b;
    contact.normal = normal;

    auto t = compute_tangents(normal);
    contact.tangents[0] = t[0];
    contact.tangents[1] = t[1];

    auto global_a = obj_a.position + offset_a;
    auto global_b = obj_b.position + offset_b;

    contact.penetration = glm::dot(global_b - global_a, normal);

    auto vel_a = obj_a.linear_velocity + glm::cross(obj_a.angular_velocity, offset_a); 
    auto vel_b = obj_b.linear_velocity + glm::cross(obj_b.angular_velocity, offset_b);
    contact.closing_velocity = 0.0f;// glm::dot(normal, vel_b - vel_a);
}

void ContactConstraint::apply(const Scene& scene, float step_size) {
    MassMatrix M(obj_a, obj_b);
    vec12 V = combined_velocity_vector(obj_a, obj_b);

    // Constraint that ensures that the relative velocity along the normal is 0
    auto compute_jacobian = [&](glm::vec3 normal, const Contact& contact) { 
        return vec12(-normal, glm::cross(normal, contact.offsets[0]), normal, glm::cross(contact.offsets[1], normal));
    };

    float friction = std::sqrt(obj_a.friction * obj_b.friction);
    float restitution = std::sqrt(obj_a.restitution * obj_b.restitution);
    
    // Normal component
    if (contact_count == 1) {
        auto J = compute_jacobian(contacts[0].normal, contacts[0]);

        float bias = (scene.baumgarte_bias / step_size) * contacts[0].penetration - contacts[0].closing_velocity * restitution;

        auto lambda = resolve_constraint(J, M, V, bias);

        float prev_impulse_sum = normal_impulse_sum.x;
        normal_impulse_sum.x = glm::max(prev_impulse_sum + lambda, 0.0f);

        V += apply_constraint(J, M, normal_impulse_sum.x - prev_impulse_sum);
    } else if (contact_count == 2) {
        std::array<vec12, 2> J = {
            compute_jacobian(contacts[0].normal, contacts[0]),
            compute_jacobian(contacts[1].normal, contacts[1]),
        };
        glm::vec2 bias = (scene.baumgarte_bias / step_size) * glm::vec2(contacts[0].penetration, contacts[1].penetration)
             - glm::vec2(contacts[0].closing_velocity, contacts[1].closing_velocity) * restitution;

        glm::vec2 prev_impulse_sum = normal_impulse_sum;
        normal_impulse_sum += resolve_constraint(J, M, V, bias);

        if (normal_impulse_sum.x < 0.0f && normal_impulse_sum.y < 0.0f) { // Both separating
            normal_impulse_sum = glm::vec2(0.0f);

            V += apply_constraint(J, M, -prev_impulse_sum);
        } else if (normal_impulse_sum.x < 0.0f) { // Contact 0 separating
            normal_impulse_sum.x = 0.0f;
            V += apply_constraint(J[0], M, -prev_impulse_sum.x);

            normal_impulse_sum.y = glm::max(normal_impulse_sum.y + resolve_constraint(J[1], M, V, bias.y), 0.0f);
            V += apply_constraint(J[1], M, normal_impulse_sum.y - prev_impulse_sum.y);
        } else if (normal_impulse_sum.y < 0.0f) { // Contact 1 separating
            normal_impulse_sum.y = 0.0f;
            V += apply_constraint(J[1], M, -prev_impulse_sum.y);

            normal_impulse_sum.x = glm::max(normal_impulse_sum.x + resolve_constraint(J[0], M, V, bias.x), 0.0f);
            V += apply_constraint(J[0], M, normal_impulse_sum.x - prev_impulse_sum.x);
        } else { // Both non-separating
            V += apply_constraint(J, M, normal_impulse_sum - prev_impulse_sum);
        }

        //std::cout << obj_a.get_name() << "," << obj_b.get_name() << " -> " << glm::to_string(prev_impulse_sum) << ", " << glm::to_string(normal_impulse_sum) << std::endl;
    } else {
        assert(false);
    }

    // Friction component
    for (size_t i = 0; i<contact_count; i++) {
    //for (size_t i = 0; i<1; i++) {
        auto& contact = contacts[i];
        std::array<vec12, 2> J = {
            compute_jacobian(contact.tangents[0], contact),
            compute_jacobian(contact.tangents[1], contact),
        };

        auto lambda = resolve_constraint(J, M, V, glm::vec2(0.0f));

        float maximum = normal_impulse_sum[i] * friction;
        glm::vec2 impulse_sum = glm::max(glm::min(contact.tangent_impulse_sum + lambda, maximum), -maximum);
        lambda = impulse_sum - contact.tangent_impulse_sum;
        contact.tangent_impulse_sum = impulse_sum;

        V += apply_constraint(J, M, lambda);
    }

    set_velocity(obj_a, obj_b, V);
}

void ContactConstraint::draw_debug() const {
    for (size_t i = 0; i<contact_count; i++) {
        auto& contact = contacts[i];
        
        auto point_a = contact.offsets[0] + obj_a.position;
        auto point_b = contact.offsets[1] + obj_b.position;

        glBegin(GL_POINTS);
        glVertex(point_a);
        glVertex(point_b);
        glEnd();

        glBegin(GL_LINES);
        auto centre = (point_a + point_b) * 0.5f;
        glVertex(centre);
        glVertex(centre + contact.normal * 0.1f);
        glEnd();
    }
}