// icp.cpp 
// author: JJ

#pragma once

#include "assets.hpp"
#include <GLFW/glfw3.h>
#include <vector>

class App {
public:
    App();

    bool init(void);
	void init_assets(void);
    int run(void);

    
    static void error_callback(int error, const char* description);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

    ~App();
private:
    bool vsync_enabled = true;
    //new GL stuff
    GLFWwindow* window = nullptr;

    GLuint shader_prog_ID{ 0 };
    GLuint VBO_ID{ 0 };
    GLuint VAO_ID{ 0 };

    std::vector<vertex> triangle_vertices =
    {
        {{0.0f,  0.5f,  0.0f}},
        {{0.5f, -0.5f,  0.0f}},
        {{-0.5f, -0.5f,  0.0f}}
    };

    GLfloat r = 1.0f;
    GLfloat g = 1.0f;
    GLfloat b = 1.0f;
    GLfloat a = 1.0f;

    
};

