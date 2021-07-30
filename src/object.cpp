#include "object.h"

#include <string>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include "scene.h"

Object::~Object() {
    if (shadow_displaylist) {
        glDeleteLists(shadow_displaylist, 1);
    }
}

void Object::load_transform() const {
    glMultMatrixf(&transform[0].x);
}

void Object::render_solid() const {
    if (!main_shape) return;

    if (texture) {
        glEnable(GL_TEXTURE_2D);
        texture->bind();
    }

    //glColor(colour);
    glPushMatrix();
    load_transform();
    main_shape->render_solid();
    glPopMatrix();

    if (texture) {
        texture->unbind();
        glDisable(GL_TEXTURE_2D);
    }
}

void Object::cache_shadow(const Light& light) {
    if (!reuse_shadow) return;
    if (shadow_displaylist) return;
    
    auto list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    render_shadow(light);
    glEndList();

    shadow_displaylist = list;
}

void Object::render_shadow(Light light) const {
    if (!shadow_shape) return;

    if (reuse_shadow && shadow_displaylist) {
        glCallList(shadow_displaylist);
        return;
    }

    auto pos = glm::vec4(light.pos, 0.0f);
    if (light.is_point) {
        pos.w = 1.0f;
    }
    pos = glm::inverse(transform) * pos;
    light.pos = glm::vec3(pos.x, pos.y, pos.z);
    if (!light.is_point) {
        light.vec = glm::normalize(light.vec);
    }

    glPushMatrix();
    load_transform();
    shadow_shape->render_shadow(light);
    glPopMatrix();
}

void Object::set_mass(float m) {
    assert(m > 0.0f);
    mass = m;
    inv_mass = 1.0f / m;
}

void Object::set_density(float density) {
    assert(density > 0.0f);
    assert(collider);
    mass = density * collider->get_volume();
    inv_mass = 1.0f / mass;
}

void Object::set_infinite_mass() {
    mass = -1.0f;
    inv_mass = 0.0f;
}

void Object::set_inertia_mass(float m) {
    assert(m > 0.0f);
    assert(collider);
    set_inertia((m / collider->get_volume()) * collider->get_inertia());
}

void Object::set_inertia_density(float density) {
    assert(density > 0.0f);
    assert(collider);
    set_inertia(density * collider->get_inertia());
}

void Object::set_inertia(glm::mat3 inertia) {
    local_inverse_inertia = glm::inverse(inertia);
    global_inverse_inertia = orientation * local_inverse_inertia * glm::transpose(orientation);
}

void Object::set_infinite_inertia() {
    local_inverse_inertia = global_inverse_inertia = glm::mat3(0.0f);
}

void Object::update_orientation() {
    orientation = glm::mat3_cast(glm::normalize(glm::quat_cast(orientation)));
    global_inverse_inertia = orientation * local_inverse_inertia * glm::transpose(orientation);
}

void Object::update_transform() {
    transform = glm::translate(position) * glm::mat4(orientation);

    update_bounds();
}

void Object::update(const Scene& scene, float step_size) {
    if (inv_mass != 0.0f) {
        linear_velocity += scene.gravity * step_size;
    }

    position += linear_velocity * step_size;
    auto angle = glm::length(angular_velocity) * step_size;
    if (angle != 0) {
        auto axis = angular_velocity / angle;
        orientation = glm::mat3(glm::rotate(angle, axis)) * orientation;
        update_orientation();
    }
    update_transform();
}

void Object::update_bounds() {
    if (collider) {
        if (collider->is_sphere_collider()) {
            physics_bounds = collider->get_bounds().translate(position);
        } else {
            physics_bounds = collider->get_bounds().apply_transform(transform);
        }
    }

    if (main_shape) {
        render_bounds = main_shape->get_bounds().apply_transform(transform);
    } else if (shadow_shape) {
        render_bounds = shadow_shape->get_bounds().apply_transform(transform);
    }
}