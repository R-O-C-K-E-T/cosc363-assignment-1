#ifdef _WIN32
#include <windows.h>
#endif

#include <cmath>
#include <iostream>
#include <algorithm>
#include <memory>

#include <filesystem>
namespace fs = std::filesystem;

#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include "object.h"
#include "image.h"
#include "scene.h"
#include "loader.h"

// temp
#include <fstream>

const std::string TITLE = "Demo";
std::string scene_filename;

Scene scene;

int speed = 0;
bool cursor_captured = false;
bool paused = false;

void dump_stencil() {
    GLint display[4];
    char* data = new char[display[2] * display[3]];
    glReadPixels(0, 0, display[2], display[3], GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, data);
    std::ofstream file;
    file.open("stencil");
    file.write(data, display[2] * display[3]);
    file.close();

    delete data;
}

void key_callback(GLFWwindow*, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;

    if (key == GLFW_KEY_I) {
        speed++;
    } else if (key == GLFW_KEY_K) {
        speed--;
    } else if (key == GLFW_KEY_R) {
        ResourceManager::the().flush();
        scene.clear();
        ResourceManager::the().load_scene(scene, scene_filename);
        //scene.slow_start();
        scene.controller = make_controller(scene);
    } else if (key == GLFW_KEY_O) {
        scene.debug_mode = !scene.debug_mode;
    } else if (key == GLFW_KEY_P) {
        paused = !paused;
    } else if (key == GLFW_KEY_T) {
        if (paused) scene.update();
    } else {
        if (scene.controller) scene.controller->press_key(key);
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        cursor_captured = !cursor_captured;
        if (cursor_captured) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

GLFWwindow* setup_opengl(const glm::uvec2 display) {
    if (!glfwInit())
        return nullptr;

    glfwWindowHint(GLFW_SAMPLES, 4);

    /* Create a windowed mode window and its OpenGL context */
    GLFWwindow* window = glfwCreateWindow(display.x, display.y, TITLE.c_str(), NULL, NULL);
    if (!window) {
        glfwTerminate();
        return nullptr;
    }

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glClearColor( 0.4f, 0.3f, 0.4f, 0.0f );

    //glEnable(GL_MULTISAMPLE);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glClearStencil(0);
    glStencilMask(0);

    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    {
        float black[] = {0.0, 0.0, 0.0, 1.0};
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, black);
    }
    {
        float white[] = {1.0, 1.0, 1.0, 1.0};
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
    }
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0f);

    glPointSize(10.0);

    glMatrixMode(GL_PROJECTION);
    gluPerspective(90.0f, (float)display.x / display.y, 0.1, 200.0);
    glMatrixMode(GL_MODELVIEW);

    return window;
}

int main(int argc, char **argv) {
    auto window = setup_opengl({1600, 900});
    //auto window = setup_opengl({800, 800});
    if (!window) return 1;

    const float MOVE_SPEEDS[] = {12.0f, 60.0f, 300.0f};

    fs::path full_scene_path = "./res/scene.json";
    if (argc >= 2) {
        full_scene_path = argv[1];
    }
    scene_filename = full_scene_path.filename().string();
    ResourceManager::the().set_path_prefix(full_scene_path.parent_path().string());

    ResourceManager::the().load_scene(scene, scene_filename);
    //scene.slow_start();
    scene.controller = make_controller(scene);

    // vec3(-0.066511, 21.718731, -15.570836)vec3(-0.435000, 3.145001, 0.000000)
    scene.camera_pos = glm::vec3(0.0f, 22.0f, -15.5f);
    scene.camera_rot = glm::vec3(-0.4, glm::pi<float>(), 0.0f);

    glm::dvec2 prev_mouse_pos = glm::dvec2();

    double prev_time = glfwGetTime() - 1.0 / 60.0;
    double framerate = 0.0;
    while (!glfwWindowShouldClose(window)) {
        // Update framerate
        double current_time = glfwGetTime();
        
        double frame_time = current_time - prev_time;
        prev_time = current_time;

        framerate = (framerate * 0.9) + 0.1 / frame_time;

        {
            char buffer[50];
            if (scene.debug_mode) {
                snprintf(buffer, sizeof(buffer), "%s: %.2lf, %.2f", TITLE.c_str(), framerate, scene.tick);
            } else {
                snprintf(buffer, sizeof(buffer), "%s: %.2lf", TITLE.c_str(), framerate);
            }
            
            glfwSetWindowTitle(window, buffer);
        }
        

        // Handle input
        glm::vec3 pos_delta(0);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) pos_delta.x += 1;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) pos_delta.x -= 1;
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) pos_delta.y += 1;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) pos_delta.y -= 1;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) pos_delta.z += 1;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) pos_delta.z -= 1;

        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) pos_delta.x += 1;
        if (glfwGetKey(window, GLFW_KEY_LEFT ) == GLFW_PRESS) pos_delta.x -= 1;
        if (glfwGetKey(window, GLFW_KEY_DOWN ) == GLFW_PRESS) pos_delta.z += 1;
        if (glfwGetKey(window, GLFW_KEY_UP   ) == GLFW_PRESS) pos_delta.z -= 1;

        glm::vec3 rot_delta(0);
        if (glfwGetKey(window, GLFW_KEY_KP_8) == GLFW_PRESS) rot_delta.x += 1;
        if (glfwGetKey(window, GLFW_KEY_KP_2) == GLFW_PRESS) rot_delta.x -= 1;
        if (glfwGetKey(window, GLFW_KEY_KP_6) == GLFW_PRESS) rot_delta.y += 1;
        if (glfwGetKey(window, GLFW_KEY_KP_4) == GLFW_PRESS) rot_delta.y -= 1;
        if (glfwGetKey(window, GLFW_KEY_KP_7) == GLFW_PRESS) rot_delta.z += 1;
        if (glfwGetKey(window, GLFW_KEY_KP_9) == GLFW_PRESS) rot_delta.z -= 1;

        float move_speed = MOVE_SPEEDS[1 + (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) - (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)];

        rot_delta *= 0.1f * move_speed * frame_time;

        glm::dvec2 cursor_pos;
        glfwGetCursorPos(window, &cursor_pos.x, &cursor_pos.y);

        auto delta = cursor_pos - prev_mouse_pos;
        prev_mouse_pos = cursor_pos;

        if (cursor_captured) {
            delta *= 0.005f;
            rot_delta.x -= delta.y;
            rot_delta.y += delta.x;
        }

        
        move_speed *= frame_time;

        scene.camera_rot += rot_delta;

        auto rotated_delta = gen_rotation_y(scene.camera_rot.y) * glm::vec4(pos_delta * 0.1f * move_speed, 0.0);
        scene.camera_pos += glm::vec3(rotated_delta.x, rotated_delta.y, rotated_delta.z);
        
        scene.camera_rot.x = std::min(std::max(scene.camera_rot.x, -glm::pi<float>() / 2), glm::pi<float>() / 2);

        //std::cout << glm::to_string(scene.camera_pos) << glm::to_string(scene.camera_rot) << std::endl;

        // Do physics update
        if (!paused) {
            double speed_factor = std::pow(1.2, speed);
            scene.update((float)(std::min(frame_time, 0.1) * speed_factor));
            //scene.update(1.0f / 60.0f);
        }
        

        // Render
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        {
            auto mat = glm::inverse(glm::translate(scene.camera_pos) * gen_rotation(scene.camera_rot));
            glLoadMatrixf(&mat[0].x);
        }
        scene.render();

        // Flip buffers
        glfwSwapBuffers(window);

        // Get events
        glfwPollEvents();

        //break;
    }

    glfwTerminate();
    return 0;
}