// icp.cpp 
// author: JJ
//
// WARNING:
// In general, you can NOT freely reorder includes!
//

// C++
// include anywhere, in any order
#include <iostream>
#include <chrono>
#include <stack>
#include <random>

// OpenCV (does not depend on GL)
#include <opencv2\opencv.hpp>

// OpenGL Extension Wrangler: allow all multiplatform GL functions
#include <GL/glew.h> 
// WGLEW = Windows GL Extension Wrangler (change for different platform) 
// platform specific functions (in this case Windows)
#include <GL/wglew.h> 

// GLFW toolkit
// Uses GL calls to open GL context, i.e. GLEW __MUST__ be first.
#include <GLFW/glfw3.h>

// OpenGL math (and other additional GL libraries, at the end)
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>


#include "assets.hpp"

#include "app.hpp"

#include "gl_err_callback.h"

App::App()
{
    // default constructor
    // nothing to do here (so far...)
    std::cout << "Constructed...\n";
}

bool App::init()
{
    try {
        void print_gl_info();
        glfwSetErrorCallback(error_callback);

        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // open window (GL canvas) with no special properties
        // https://www.glfw.org/docs/latest/quick.html#quick_create_window
        // TODO: add error checking!
        window = glfwCreateWindow(800, 600, "OpenGL context", NULL, NULL);
        if (!window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }
        
        glfwSetWindowUserPointer(window, this);
        glfwSetKeyCallback(window, key_callback);
        glfwSetScrollCallback(window, scroll_callback);
        
        glfwMakeContextCurrent(window);
        glfwSwapInterval(vsync_enabled ? 1 : 0);

        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            throw std::runtime_error("Failed to initialize GLEW");
        }

        print_gl_info();

        if (GLEW_ARB_debug_output)
        {
            glDebugMessageCallback(MessageCallback, 0);
            glEnable(GL_DEBUG_OUTPUT);

            //default is asynchronous debug output, use this to simulate glGetError() functionality
            //glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

            std::cout << "GL_DEBUG enabled." << std::endl;
        }
        else
            std::cout << "GL_DEBUG NOT SUPPORTED!" << std::endl;

        wglewInit();

        if (!GLEW_ARB_direct_state_access)
            throw std::runtime_error("No DSA :-(");


        //TODO: get info about your GL context    
        // initialization code
        //...

        // some init
        // if (not_success)
        //  throw std::runtime_error("something went bad");
    }
    catch (std::exception const& e) {
        std::cerr << "Init failed : " << e.what() << std::endl;
        throw;
    }
    
    std::cout << "Initialized...\n";

	init_assets();
    
    return true;
}

void App::init_assets(void) {
    //
    // Initialize pipeline: compile, link and use shaders
    //


    //SHADERS - define & compile & link
    const char* vertex_shader =
        "#version 460 core\n"
        "in vec3 attribute_Position;"
        "void main() {"
        "  gl_Position = vec4(attribute_Position, 1.0);"
        "}";

    const char* fragment_shader =
        "#version 460 core\n"
        "uniform vec4 uniform_Color;"
        "out vec4 FragColor;"
        "void main() {"
        "  FragColor = uniform_Color;"
        "}";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_shader, NULL);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader, NULL);
    glCompileShader(fs);

    shader_prog_ID = glCreateProgram();
    glAttachShader(shader_prog_ID, fs);
    glAttachShader(shader_prog_ID, vs);
    glLinkProgram(shader_prog_ID);

    //now we can delete shader parts (they can be reused, if you have more shaders)
    //the final shader program already linked and stored separately
    glDetachShader(shader_prog_ID, fs);
    glDetachShader(shader_prog_ID, vs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    // 
    // Create and load data into GPU using OpenGL DSA (Direct State Access)
    //

    // Create VAO + data description (similar to container)
    glCreateVertexArrays(1, &VAO_ID);

    GLint position_attrib_location = glGetAttribLocation(shader_prog_ID, "attribute_Position");

    vertex v;

    glEnableVertexArrayAttrib(VAO_ID, position_attrib_location);
    glVertexArrayAttribFormat(VAO_ID, position_attrib_location, v.position.length(), GL_FLOAT, GL_FALSE, offsetof(vertex, position));
    glVertexArrayAttribBinding(VAO_ID, position_attrib_location, 0); // (GLuint vaobj, GLuint attribindex, GLuint bindingindex)

    // Create and fill data
    glCreateBuffers(1, &VBO_ID);
    glNamedBufferData(VBO_ID, triangle_vertices.size() * sizeof(vertex), triangle_vertices.data(), GL_STATIC_DRAW);

    // Connect together
    glVertexArrayVertexBuffer(VAO_ID, 0, VBO_ID, 0, sizeof(vertex)); // (GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride)
}




int App::run(void)
{
    try {
        //GLfloat r, g, b, a;
        //r = g = b = a = 1.0f; //white color

        // Activate shader program. There is only one program, so activation can be out of the loop. 
        // In more realistic scenarios, you will activate different shaders for different 3D objects.
        glUseProgram(shader_prog_ID);

        // Get uniform location in GPU program. This will not change, so it can be moved out of the game loop.
        GLint uniform_color_location = glGetUniformLocation(shader_prog_ID, "uniform_Color");
        if (uniform_color_location == -1) {
            std::cerr << "Uniform location is not found in active shader program. Did you forget to activate it?\n";
        }

        while (!glfwWindowShouldClose(window)) {
            // clear canvas
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            //set uniform parameter for shader
            // (try to change the color in key callback)          
            glUniform4f(uniform_color_location, r, g, b, a);

            //bind 3d object data
            glBindVertexArray(VAO_ID);

            // draw all VAO data
            glDrawArrays(GL_TRIANGLES, 0, triangle_vertices.size());

            // poll events, call callbacks, flip back<->front buffer
            glfwPollEvents();
            glfwSwapBuffers(window);
        }
    }
    catch (std::exception const& e) {
        std::cerr << "App failed : " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    std::cout << "Finished OK...\n";
    return EXIT_SUCCESS;
}

App::~App()
{
    // clean-up
    glDeleteProgram(shader_prog_ID);
    glDeleteBuffers(1, &VBO_ID);
    glDeleteVertexArrays(1, &VAO_ID);

    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();

    cv::destroyAllWindows();
    std::cout << "Bye...\n";
}


void App::error_callback(int error, const char* description) {
    std::cerr << "Error: " << description << std::endl;
}


void App::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (yoffset > 0.0) {
        std::cout << "wheel up...\n";
    }
    else if (yoffset < 0.0) {
        std::cout << "wheel down...\n";
    }
}

void App::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    
    App* app = (App*)glfwGetWindowUserPointer(window);

    if ((action == GLFW_PRESS) || (action == GLFW_REPEAT)) {
        switch (key) {

        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;

        case GLFW_KEY_V:
            if (app) {
                app->vsync_enabled = !app->vsync_enabled;
                glfwSwapInterval(app->vsync_enabled ? 1 : 0);
                std::cout << "VSync: " << (app->vsync_enabled ? "ZAPNUTO" : "VYPNUTO") << std::endl;
            }
            break;

        case GLFW_KEY_SPACE:
            if (app) {
                app->r = (float)rand() / RAND_MAX;
                app->g = (float)rand() / RAND_MAX;
                app->b = (float)rand() / RAND_MAX;
                std::cout << "Zmena barvy na: " << app->r << ", " << app->g << ", " << app->b << std::endl;
            }
            break;

        default:
            break;
        }
    }
}

