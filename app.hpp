// icp.cpp 
// author: JJ

#pragma once
#include <algorithm>

#include "assets.hpp"
#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <glm/glm.hpp>
#include "fps_meter.hpp"
#include "camera.hpp"
#include <unordered_map>
#include <memory>
#include "ShaderProgram.hpp"
#include "Mesh.hpp"
#include "Model.hpp"

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
    Camera camera;
    double cursorLastX{ 0.0 };
    double cursorLastY{ 0.0 };
    bool firstMouse{ true };

    bool is_mouse_locked{ true };
    int win_width{ 800 };
    int win_height{ 600 };
    bool is_fullscreen{ false };
    int saved_xpos{ 0 }, saved_ypos{ 0 };
    int saved_width{ 800 }, saved_height{ 600 };
    glm::mat4 projection_matrix = glm::mat4(1.0f); 
    float fov = 60.0f;

    void update_projection_matrix();

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

    std::string active_shader_name = "basic";

    std::unordered_map<std::string, std::shared_ptr<ShaderProgram>> shader_library;
    std::unordered_map<std::string, std::shared_ptr<Mesh>> mesh_library;

    std::unordered_map<std::string, Model> scene;
    std::shared_ptr<Mesh> my_triangle;
    //std::vector<Vertex> triangle_vertices = {
    //    {{ 0.0f,  0.5f,  0.0f },     { 0.0f, 0.0f, 1.0f },                  { 0.5f, 1.0f }}, 
    //    {{ 0.5f, -0.5f,  0.0f },     { 0.0f, 0.0f, 1.0f },                  { 1.0f, 0.0f }}, 
    //    {{-0.5f, -0.5f,  0.0f },     { 0.0f, 0.0f, 1.0f },                  { 0.0f, 0.0f }}  
    //};

    GLfloat r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
    GLfloat bg_r = 0.1f, bg_g = 0.1f, bg_b = 0.2f;
};
