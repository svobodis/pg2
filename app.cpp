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
#include <filesystem>
#include <algorithm>
#include <string>
#include <fstream>

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
#include "meshgen.hpp"
#include "Model.hpp"
#include "teapot_vec.hpp"

#include "gl_err_callback.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>



void test_time_measure();

App::App()
{
    std::cout << "Constructed...\n";
}


void App::init_glfw() {
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit()) {
        throw std::runtime_error("GLFW can not be initialized.");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    std::ifstream config_file("config.json");
    if (config_file.is_open()) {
        std::string content((std::istreambuf_iterator<char>(config_file)), std::istreambuf_iterator<char>());
        size_t w_pos = content.find("\"width\"");
        if (w_pos != std::string::npos) sscanf(content.c_str() + w_pos, "\"width\"%*[ : \t]%d", &win_width);
        size_t h_pos = content.find("\"height\"");
        if (h_pos != std::string::npos) sscanf(content.c_str() + h_pos, "\"height\"%*[ : \t]%d", &win_height);
    }

    window = glfwCreateWindow(win_width, win_height, "ICP Projekt", nullptr, nullptr);
    if (!window) throw std::runtime_error("GLFW window can not be created.");

    glfwSetWindowUserPointer(window, this);
    glfwMakeContextCurrent(window);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
    glfwSetKeyCallback(window, glfw_key_callback);
    glfwSetScrollCallback(window, glfw_scroll_callback);
    glfwSetCursorPosCallback(window, glfw_cursor_position_callback);
}

void App::init_glew() {
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) throw std::runtime_error("Failed to initialize GLEW");
    wglewInit();
    if (!GLEW_ARB_direct_state_access) throw std::runtime_error("No DSA :-(");
}

void App::init_gl_debug() {
    if (GLEW_ARB_debug_output) {
        glDebugMessageCallback(App::MessageCallback, 0);
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        std::cout << "GL_DEBUG enabled." << std::endl;
    }
    else {
        std::cout << "GL_DEBUG NOT SUPPORTED!" << std::endl;
    }
}

void App::init_opencv() {}

bool App::init() {
    try {
        std::cout << "Current working directory: " << std::filesystem::current_path().generic_string() << '\n';

        if (!std::filesystem::exists("resources"))
            throw std::runtime_error("Directory 'resources' not found.");

        init_opencv();
        init_glfw();
        init_glew();
        init_gl_debug();

        print_opencv_info();
        print_glfw_info();
        print_gl_info();
        print_glm_info();

        glfwSwapInterval(is_vsync_on ? 1 : 0);

        init_assets();
        init_imgui();

        glfwShowWindow(window);
    }
    catch (std::exception const& e) {
        std::cerr << "Init failed : " << e.what() << std::endl;
        throw;
    }
    return true;
}

void App::init_imgui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();
    std::cout << "ImGUI version: " << ImGui::GetVersion() << "\n";
}

void App::init_assets() {
    shader_library.emplace("basic", std::make_shared<ShaderProgram>(
        std::filesystem::path("resources/basic.vert"),
        std::filesystem::path("resources/basic.frag")
    ));

    shader_library.emplace("basic_core", std::make_shared<ShaderProgram>(
        std::filesystem::path("resources/basic_core.vert"),
        std::filesystem::path("resources/basic_core.frag")
    ));

    shader_library.emplace("basic_uniform", std::make_shared<ShaderProgram>(
        std::filesystem::path("resources/basic_core.vert"),
        std::filesystem::path("resources/basic_uniform.frag")
    ));

    shader_library.emplace("rainbow", std::make_shared<ShaderProgram>(
        std::filesystem::path("resources/basic_core.vert"),
        std::filesystem::path("resources/GL_rainbow.frag")
    ));

    auto shader = shader_library.at("basic");

    GLint pos_loc = shader->getAttribLocation("aPos");
    GLint col_loc = shader->getAttribLocation("aColor");

    //my_triangle = std::make_shared<Mesh>(triangle_vertices, GL_TRIANGLES);

    //my_triangle = generateCube(); 
    //my_triangle = generateSphere(36, 18);

    //my_model = std::make_shared<Model>("resources/plane_tri_vnt.obj", shader_library.at("basic"));

    mesh_library.emplace("cube", generateCube());
    mesh_library.emplace("sphere", generateSphere(36, 18));

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Vertex> bunny_vertices;
    std::vector<GLuint> bunny_indices;
    if (loadOBJ("resources/bunny_tri_vn.obj", bunny_vertices, bunny_indices)) {
        for (auto& v : bunny_vertices) {
            v.position *= 0.01f;

            v.position.y -= 0.5f;
        }
        mesh_library.emplace("bunny", std::make_shared<Mesh>(bunny_vertices, bunny_indices, GL_TRIANGLES));

        Model my_bunny;
        my_bunny.addMesh(mesh_library.at("bunny"), shader_library.at("basic"));
        scene["Stanfordsky_Kralik"] = my_bunny;
    }

    if (loadOBJ("resources/triangle.obj", vertices, indices)) {
        mesh_library.emplace("triangle_obj", std::make_shared<Mesh>(vertices, indices, GL_TRIANGLES));
    }

    //Model sphere_model;
    //sphere_model.addMesh(mesh_library.at("sphere"), shader_library.at("rainbow"));
    //sphere_model.pivot_position = glm::vec3(0.0f, 0.0f, 0.0f);

    //scene["Duhova_Koule"] = sphere_model;

    //Model cube_model;
    //cube_model.addMesh(mesh_library.at("cube"), shader_library.at("basic"));
    //scene["Moje_Kostka"] = cube_model;

}

int App::run(void) {
    try {
        double FPS = 0.0;

        double now = glfwGetTime();
        double frame_begin_timepoint = now;
        double previous_frame_render_time{};


        while (!glfwWindowShouldClose(window)) {
            if (show_imgui) {
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                ImGui::SetNextWindowPos(ImVec2(10, 10));
                ImGui::SetNextWindowSize(ImVec2(300, 150));
                ImGui::Begin("Herni Info", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
                ImGui::Text("V-Sync: %s", is_vsync_on ? "ON" : "OFF");
                ImGui::Text("FPS: %.1f", FPS);
                ImGui::Text("---------------------------");
                ImGui::Text("PRAVE TLACITKO = Odemknout mys");
                ImGui::Text("Klavesa V = Prepnout VSync");
                ImGui::Text("Klavesa D = Skryt ImGui");
                ImGui::Separator();
                ImGui::Text("Vyber Shader:");
                if (ImGui::RadioButton("Basic", active_shader_name == "basic")) { active_shader_name = "basic"; }
                if (ImGui::RadioButton("Basic Core", active_shader_name == "basic_core")) { active_shader_name = "basic_core"; }
                if (ImGui::RadioButton("Basic Uniform", active_shader_name == "basic_uniform")) { active_shader_name = "basic_uniform"; }
                if (ImGui::RadioButton("Rainbow (ShaderToy)", active_shader_name == "rainbow")) { active_shader_name = "rainbow"; }
                ImGui::Separator();
                ImGui::End();
            }

            shader_library.at("rainbow")->use();
            shader_library.at("rainbow")->setUniform("iTime", static_cast<float>(glfwGetTime()));

            if (shader_library.count("basic_uniform") > 0) {
                shader_library.at("basic_uniform")->use();
                shader_library.at("basic_uniform")->setUniform("ucolor", glm::vec4(r, g, b, 1.0f));
            }

            if (scene.count("Duhova_Koule") > 0) { 
                scene["Duhova_Koule"].meshes[0].shader = shader_library.at(active_shader_name);
            }


            glClearColor(bg_r, bg_g, bg_b, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            //if (my_model) {
            //    my_model->draw();
            //}

            for (auto& item : scene) {
                item.second.update(0.016f);
                item.second.draw();
            }

            auto current_shader = shader_library.at(active_shader_name);
            current_shader->use();

            if (active_shader_name == "rainbow") {
                current_shader->setUniform("iTime", static_cast<float>(glfwGetTime()));
            }
            else if (active_shader_name == "basic_uniform") {
                current_shader->setUniform("ucolor", glm::vec4(r, g, b, 1.0f));
            }

            //my_triangle->draw();
			//my_model->draw();

            if (show_imgui) {
                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }

            glfwSwapBuffers(window);
            glfwPollEvents();

            now = glfwGetTime();
            previous_frame_render_time = now - frame_begin_timepoint;
            frame_begin_timepoint = now;

            fps_counter.update();

            if (fps_counter.is_updated()) {
                FPS = fps_counter.get();

                std::string title = "ICP Projekt - FPS: " + std::to_string((int)FPS);
                glfwSetWindowTitle(window, title.c_str());
            }
        }
    }
    catch (std::exception const& e) {
        std::cerr << "App failed : " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
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
            if (saved_xpos >= mx && saved_ypos >= my) {
                current_monitor = monitors[i];
            }
        }

        const GLFWvidmode* mode = glfwGetVideoMode(current_monitor);
        glfwSetWindowMonitor(window, current_monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
    else {
        glfwSetWindowMonitor(window, nullptr, saved_xpos, saved_ypos, saved_width, saved_height, 0);
    }
}



void App::destroy() {
    if (ImGui::GetCurrentContext()) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
    cv::destroyAllWindows();
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}

App::~App() {
    destroy();
    std::cout << "Bye...\n";
}