#include <iostream>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "app.hpp"

void App::glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW error: " << description << std::endl;
}

void App::glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto this_inst = static_cast<App*>(glfwGetWindowUserPointer(window));

    if ((action == GLFW_PRESS) || (action == GLFW_REPEAT)) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            if (action == GLFW_PRESS) {
                if (this_inst->currentState == GameState::PLAYING) {
                    this_inst->currentState = GameState::MENU;
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                }
                else {
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }
            }
            break;
        case GLFW_KEY_V:
            if (action == GLFW_PRESS) {
                this_inst->is_vsync_on = !this_inst->is_vsync_on;
                glfwSwapInterval(this_inst->is_vsync_on ? 1 : 0);
                std::cout << "VSync: " << (this_inst->is_vsync_on ? "ON" : "OFF") << "\n";
            }
            break;
        case GLFW_KEY_G:
            if (action == GLFW_PRESS) {
                this_inst->show_imgui = !this_inst->show_imgui;
            }
            break;
        case GLFW_KEY_SPACE:
            if (action == GLFW_PRESS
                && this_inst->currentState == GameState::PLAYING
                && !this_inst->is_free_camera
                && this_inst->isGrounded)
            {
                this_inst->playerVelocityY = 6.0f;
                this_inst->isGrounded = false;
            }
            break;
        case GLFW_KEY_F11:
            if (action == GLFW_PRESS) {
                this_inst->toggle_fullscreen();
            }
            break;
        case GLFW_KEY_F:
            if (action == GLFW_PRESS) {
                this_inst->toggle_fullscreen();
            }
            break;
        default:
            break;
        }
    }
}


void App::glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    auto this_inst = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (this_inst) {
        this_inst->fov -= 5.0f * yoffset;
        this_inst->fov = std::clamp(this_inst->fov, 20.0f, 170.0f);
        this_inst->update_projection_matrix();
    }
}

void App::glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    auto this_inst = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (this_inst) {
        this_inst->win_width = width;
        this_inst->win_height = height;
        glViewport(0, 0, width, height);
        this_inst->update_projection_matrix();
    }
}

void App::update_projection_matrix() {
    if (win_height < 1) win_height = 1;
    float ratio = static_cast<float>(win_width) / win_height;
    projection_matrix = glm::perspective(glm::radians(fov), ratio, 0.1f, 20000.0f);
}

void App::toggle_fullscreen() {
    is_fullscreen = !is_fullscreen;
    if (is_fullscreen) {
        glfwGetWindowPos(window, &saved_xpos, &saved_ypos);
        glfwGetWindowSize(window, &saved_width, &saved_height);
        int monitors_count;
        GLFWmonitor** monitors = glfwGetMonitors(&monitors_count);
        GLFWmonitor* current_monitor = monitors[0];
        for (int i = 0; i < monitors_count; i++) {
            int mx, my;
            glfwGetMonitorPos(monitors[i], &mx, &my);
            if (saved_xpos >= mx && saved_ypos >= my)
                current_monitor = monitors[i];
        }
        const GLFWvidmode* mode = glfwGetVideoMode(current_monitor);
        glfwSetWindowMonitor(window, current_monitor, 0, 0,
            mode->width, mode->height, mode->refreshRate);
    }
    else {
        glfwSetWindowMonitor(window, nullptr,
            saved_xpos, saved_ypos,
            saved_width, saved_height, 0);
    }
}

void App::glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    auto this_inst = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (!this_inst) return;

    if (action == GLFW_PRESS) {
        switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            else {
                this_inst->shoot();
            }
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            break;
        default:
            break;
        }
    }
}


void App::glfw_cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    auto this_inst = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (this_inst) {
        if (this_inst->firstMouse) {
            this_inst->cursorLastX = xpos;
            this_inst->cursorLastY = ypos;
            this_inst->firstMouse = false;
        }

        double xoffset = xpos - this_inst->cursorLastX;
        double yoffset = this_inst->cursorLastY - ypos;

        this_inst->cursorLastX = xpos;
        this_inst->cursorLastY = ypos;

        if (this_inst->is_mouse_locked) {
            this_inst->camera.ProcessMouseMovement(xoffset, yoffset);
        }
    }
}

void GLAPIENTRY App::MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    if (type == GL_DEBUG_TYPE_ERROR) {
        std::cerr << "[OpenGL ERROR] " << message << std::endl;
    }
}