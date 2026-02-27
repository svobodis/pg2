// icp.cpp 
// author: JJ

#pragma once

#include "assets.hpp"
#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <glm/glm.hpp>
#include "fps_meter.hpp"

class App {
public:
    App();
    ~App();

    bool init(void);
    void init_imgui();
    int run(void);
    void destroy(void);
    void toggle_fullscreen();

private:
    GLFWwindow* window{ nullptr };
    bool is_vsync_on{ true };

    bool show_imgui{ true };

    bool is_mouse_locked{ true };
    int win_width{ 800 };
    int win_height{ 600 };
    bool is_fullscreen{ false };
    int saved_xpos{ 0 }, saved_ypos{ 0 };
    int saved_width{ 800 }, saved_height{ 600 };

    fps_meter fps_counter;

    void init_opencv();
    void init_glew(void);
    void init_glfw(void);
    void init_gl_debug();
    void init_assets(void);

    void print_opencv_info();
    void print_glfw_info(void);
    void print_glm_info();
    void print_gl_info();

    static void glfw_error_callback(int error, const char* description);
    static void glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

    static void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

    static void glfw_cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

    GLuint shader_prog_ID{ 0 };
    GLuint VBO_ID{ 0 };
    GLuint VAO_ID{ 0 };
    std::vector<vertex> triangle_vertices = {
        {{ 0.0f,  0.5f,  0.0f }},
        {{ 0.5f, -0.5f,  0.0f }},
        {{-0.5f, -0.5f,  0.0f }}
    };

    GLfloat r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
    GLfloat bg_r = 0.1f, bg_g = 0.1f, bg_b = 0.2f;
};
