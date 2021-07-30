#pragma once

#include "object.h"

inline vec12 constexpr combined_velocity_vector(const Object& a, const Object& b) { 
    return vec12(a.linear_velocity, a.angular_velocity, b.linear_velocity, b.angular_velocity);
}

class MassMatrix { // Actually it's the inverse
    public:
        constexpr MassMatrix(const Object& a, const Object& b) 
            : inertia_a(a.global_inverse_inertia), inertia_b(b.global_inverse_inertia), mass_a(a.inv_mass), mass_b(b.inv_mass) {}
    
        vec12 operator*(const vec12& v) const {
            return vec12(
                mass_a    * glm::vec3(v[ 0], v[ 1], v[ 2]),
                inertia_a * glm::vec3(v[ 3], v[ 4], v[ 5]),
                mass_b    * glm::vec3(v[ 6], v[ 7], v[ 8]),
                inertia_b * glm::vec3(v[ 9], v[10], v[11])
            );
        }

    private:
        glm::mat3 inertia_a;
        glm::mat3 inertia_b;
        float mass_a;
        float mass_b;
};

template<size_t L>
glm::mat<(glm::length_t)L, (glm::length_t)L, float> compute_inverse_effective_mass(const std::array<vec12, L>& J, const MassMatrix& M) {
    std::array<vec12, L> MJ; // List of columns
    for (size_t i=0; i<L; i++) {
        MJ[i] = M * J[i];
    }

    glm::mat<L, L, float> JMJ;
    for (glm::length_t i=0; i<L; i++) {
        for (glm::length_t j=0; j<L; j++) {
            JMJ[j][i] = J[i].dot(MJ[j]); 
        }
    }
    return glm::inverse(JMJ);
}

inline float compute_inverse_effective_mass(const vec12& J, const MassMatrix& M) {
    return 1.0f / J.dot(M * J);
}

template<size_t L>
glm::vec<(glm::length_t)L, float> resolve_constraint(const std::array<vec12, L>& J, const MassMatrix& M, const vec12& V, const glm::vec<(glm::length_t)L, float> bias) {
    glm::vec<L, float> b = -bias;
    for (glm::length_t i = 0; i<L; i++) {
        b[i] -= J[i].dot(V);
    }
    return compute_inverse_effective_mass(J, M) * b;
}

inline float resolve_constraint(const vec12& J, const MassMatrix& M, const vec12& V, const float bias) {
    return compute_inverse_effective_mass(J, M) * (-bias - J.dot(V));
}

template<size_t L>
vec12 apply_constraint(const std::array<vec12, L>& J, const MassMatrix& M, const glm::vec<(glm::length_t)L, float> lambda) {
    auto total = vec12();
    for (size_t i = 0; i<L; i++) {
        total += J[i] * lambda[i];
    }
    return M * total;
}

inline vec12 apply_constraint(const vec12& J, const MassMatrix& M, const float lambda) {
    return M * (J * lambda);
}

inline void set_velocity(Object& a, Object& b, const vec12& v) {
    a.linear_velocity  = glm::vec3(v[ 0], v[ 1], v[ 2]);
    a.angular_velocity = glm::vec3(v[ 3], v[ 4], v[ 5]);
    b.linear_velocity  = glm::vec3(v[ 6], v[ 7], v[ 8]);
    b.angular_velocity = glm::vec3(v[ 9], v[10], v[11]);
}

class Constraint {
    public:
        virtual void apply(const Scene&, float step_size) = 0;

    protected:
        Constraint(Object& a, Object& b) : obj_a(a), obj_b(b) {}

        Object& obj_a;
        Object& obj_b;
};

class BallSocketJoint : public Constraint {
    public:
        BallSocketJoint(Object& a, Object& b, glm::vec3 local_a, glm::vec3 local_b) : Constraint(a, b), local_a(local_a), local_b(local_b) {}

        void apply(const Scene&, float step_size) override;
    private:
        glm::vec3 local_a;
        glm::vec3 local_b;
};

//class FixedJoint : public Constraint {
//    public:
//        FixedJoint(Object& a, Object& b, glm::vec3 local_a, glm::vec3 local_b) : Constraint(a, b), local_a(local_a), local_b(local_b) {}
//
//        void apply(const Scene&, float step_size) override;
//
//    private:
//        glm::vec3 local_a;
//        glm::vec3 local_b;
//};


class HingeJoint : public Constraint {
    public:
        HingeJoint(Object& a, Object& b, glm::vec3 local_a, glm::vec3 local_b, glm::vec3 local_vec_a, glm::vec3 local_vec_b)
            : Constraint(a, b), local_a(local_a), local_b(local_b), local_vec_a(local_vec_a), local_vec_b(local_vec_b) {}

        void apply(const Scene&, float step_size) override;

    private:
        glm::vec3 local_a;
        glm::vec3 local_b;

        glm::vec3 local_vec_a;
        glm::vec3 local_vec_b;
};

class ContactConstraint final : public Constraint {
    public:
        ContactConstraint(Object& a, Object& b) : Constraint(a, b) {}

        void add_contact(glm::vec3 local_a, glm::vec3 local_b, glm::vec3 normal);

        void apply(const Scene&, float step_size) override;

        void draw_debug() const;

        void dump() const {
            std::cout << "(" << obj_a.get_name() << "," << obj_b.get_name() << ") ";
            if (contact_count == 1) {
                std::cout << contacts[0].penetration << ", " << glm::to_string(contacts[0].offsets[0]) << glm::to_string(contacts[0].offsets[1]) << ", " << normal_impulse_sum.x;
            } else {
                std::cout 
                    << glm::to_string(glm::vec2(contacts[0].penetration, contacts[1].penetration)) << ", " 
                    << "(" << glm::to_string(contacts[0].offsets[0]) << glm::to_string(contacts[0].offsets[1]) << "," << glm::to_string(contacts[1].offsets[0]) << glm::to_string(contacts[1].offsets[1]) << "), "
                    << glm::to_string(normal_impulse_sum);
            }
            std::cout << std::endl;
        }

    private:
        struct Contact {
            glm::vec3 offsets[2];
            glm::vec3 normal;
            glm::vec3 tangents[2];
            float penetration;
            float closing_velocity;
            glm::vec2 tangent_impulse_sum = glm::vec2(0.0f);
        };
        Contact contacts[2];

        glm::vec2 normal_impulse_sum = glm::vec2(0.0f);
        size_t contact_count = 0;
};