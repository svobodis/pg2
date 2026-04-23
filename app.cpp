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
#include "OBJloader.hpp"
#include "Texture.hpp"

#include "gl_err_callback.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>



void resolveCollision(Camera& cam, glm::vec3 boxMin, glm::vec3 boxMax) {
    float r = 0.5f; 

    glm::vec3 camMin = cam.Position - glm::vec3(r);
    glm::vec3 camMax = cam.Position + glm::vec3(r);

    if (camMax.x > boxMin.x && camMin.x < boxMax.x &&
        camMax.y > boxMin.y && camMin.y < boxMax.y &&
        camMax.z > boxMin.z && camMin.z < boxMax.z) {

        float overlapX1 = camMax.x - boxMin.x;
        float overlapX2 = boxMax.x - camMin.x;
        float overlapX = std::min(overlapX1, overlapX2);

        float overlapY1 = camMax.y - boxMin.y;
        float overlapY2 = boxMax.y - camMin.y;
        float overlapY = std::min(overlapY1, overlapY2);

        float overlapZ1 = camMax.z - boxMin.z;
        float overlapZ2 = boxMax.z - camMin.z;
        float overlapZ = std::min(overlapZ1, overlapZ2);

        if (overlapX < overlapY && overlapX < overlapZ) {
            if (overlapX1 < overlapX2) cam.Position.x -= overlapX;
            else cam.Position.x += overlapX;
            cam.Velocity.x = 0; 
        }
        else if (overlapY < overlapX && overlapY < overlapZ) {
            if (overlapY1 < overlapY2) cam.Position.y -= overlapY;
            else cam.Position.y += overlapY;
            cam.Velocity.y = 0; 
        }
        else {
            if (overlapZ1 < overlapZ2) cam.Position.z -= overlapZ;
            else cam.Position.z += overlapZ;
            cam.Velocity.z = 0; 
        }
    }
}

struct Particle {
    Model model;
    glm::vec3 velocity; 
    float lifetime;   
};

std::vector<Particle> aktivni_castice;


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

    glfwWindowHint(GLFW_SAMPLES, 4); 
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

        glEnable(GL_MULTISAMPLE);

        glfwSetWindowUserPointer(window, this);

        glfwGetFramebufferSize(window, &win_width, &win_height);
        update_projection_matrix();

        if (ma_engine_init(NULL, &audio_engine) != MA_SUCCESS) {
            std::cerr << "Varovani: Nepodarilo se inicializovat zvukovy engine!\n";
        }
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
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    shader_library.emplace("basic", std::make_shared<ShaderProgram>(
        std::filesystem::path("resources/basic_core3.vert"),
        std::filesystem::path("resources/basic.frag")
    ));

    shader_library.emplace("basic_core", std::make_shared<ShaderProgram>(
        std::filesystem::path("resources/basic_core3.vert"),
        std::filesystem::path("resources/basic_core3.frag")
    ));

    shader_library.emplace("basic_uniform", std::make_shared<ShaderProgram>(
        std::filesystem::path("resources/basic_core3.vert"),
        std::filesystem::path("resources/basic_uniform.frag")
    ));

    shader_library.emplace("rainbow", std::make_shared<ShaderProgram>(
        std::filesystem::path("resources/basic_core3.vert"),
        std::filesystem::path("resources/GL_rainbow.frag")
    ));

    shader_library.emplace("advanced_lights", std::make_shared<ShaderProgram>(
        std::filesystem::path("resources/lights.vert"),
        std::filesystem::path("resources/lights.frag")
    ));

    auto shader = shader_library.at("basic");

    //GLint pos_loc = shader->getAttribLocation("aPos");
    //GLint col_loc = shader->getAttribLocation("aColor");

    //my_triangle = std::make_shared<Mesh>(triangle_vertices, GL_TRIANGLES);

    //my_triangle = generateCube(); 
    //my_triangle = generateSphere(36, 18);

    //my_model = std::make_shared<Model>("resources/plane_tri_vnt.obj", shader_library.at("basic"));

    //mesh_library.emplace("cube", generateCube());
    mesh_library.emplace("sphere", generateSphere(36, 18));

    std::vector<Vertex> box_vertices;
    std::vector<GLuint> box_indices;
    if (loadOBJ("resources/cube_quads.obj", box_vertices, box_indices)) {
        mesh_library.emplace("cube", std::make_shared<Mesh>(box_vertices, box_indices, GL_TRIANGLES));
    }

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
        my_bunny.addMesh(mesh_library.at("bunny"), shader_library.at("advanced_lights"));
        scene["Stanfordsky_Kralik"] = my_bunny;
    }

    if (loadOBJ("resources/triangle.obj", vertices, indices)) {
        mesh_library.emplace("triangle_obj", std::make_shared<Mesh>(vertices, indices, GL_TRIANGLES));
    }

    shader_library.emplace("texture_shader", std::make_shared<ShaderProgram>(
        std::filesystem::path("resources/tex.vert"),
        std::filesystem::path("resources/tex.frag")
    ));

    texture_library.emplace("mc_block", std::make_shared<Texture>(
        std::filesystem::path("resources/textures/box_rgb888.png"),
        Texture::Interpolation::nearest
    ));

    texture_library.emplace("muj_atlas", std::make_shared<Texture>(
        std::filesystem::path("resources/tex_256.png"),
        Texture::Interpolation::nearest
    ));

    Model textured_cube;
    textured_cube.addMesh(
        mesh_library.at("cube"),             
        //shader_library.at("texture_shader"),
        shader_library.at("advanced_lights"),
        texture_library.at("mc_block")       
    );

    //textured_cube.translate(glm::vec3(2.0f, 0.0f, -2.0f));

    //scene["Minecraft_Blok"] = textured_cube;

    /*for (int x = 0; x < 10; x++) {
        for (int y = 0; y < 10; y++) {
            Model wall_cube;
            wall_cube.addMesh(
                mesh_library.at("cube"),
                shader_library.at("advanced_lights"),
                texture_library.at("mc_block")
            );

            float pos_x = (x * 2.0f) - 10.0f;
            float pos_y = y * 2.0f;          
            float pos_z = -8.0f;             

            wall_cube.translate(glm::vec3(pos_x, pos_y, pos_z));

            std::string cube_name = "Zed_" + std::to_string(x) + "_" + std::to_string(y);

            scene[cube_name] = wall_cube;
        }
    }*/

    // 1. Vytvoříme a vygenerujeme 2D mapu
    mapa = cv::Mat(10, 25, CV_8U);
    genLabyrinth(mapa);

    // 2. Postavíme 3D svět na základě mapy!
    // Projdeme každý pixel (znak) v naší 2D matici
    for (int j = 0; j < mapa.rows; j++) {
        for (int i = 0; i < mapa.cols; i++) {

            // Pokud je na tomto políčku znak '#', postavíme tam 3D kostku
            if (getmap(mapa, i, j) == '#') {
                Model wall_cube;
                wall_cube.addMesh(
                    mesh_library.at("cube"),
                    shader_library.at("advanced_lights"),
                    texture_library.at("mc_block")
                );

                // UMÍSTĚNÍ: X bereme ze sloupce (i), Z bereme z řádku (j). 
                // Násobíme 2.0f, protože naše Minecraft kostka je velká 2 jednotky!
                wall_cube.translate(glm::vec3(i * 2.0f, 0.0f, j * 2.0f));

                // Uložíme do scény pod unikátním jménem
                scene["Zed_" + std::to_string(i) + "_" + std::to_string(j)] = wall_cube;
            }
            // Místo, kde je 'e', si můžeme předpřipravit např. pro cíl hry
            else if (getmap(mapa, i, j) == 'e') {
                // Tady bys mohl umístit třeba speciální zlatou minci nebo portál!
            }
        }
    }

    for (int i = 0; i < 3; i++) {
        Model ghost_sphere;
        ghost_sphere.addMesh(mesh_library.at("sphere"), shader_library.at("advanced_lights"));

        ghost_sphere.is_transparent = true;

        ghost_sphere.translate(glm::vec3(-2.0f + (i * 2.0f), 3.0f, -4.0f));
        scene["Duch_" + std::to_string(i)] = ghost_sphere;
    }


    for (int i = 0; i < 5; i++) {
        Model coin;
        coin.addMesh(
            mesh_library.at("cube"),
            shader_library.at("advanced_lights"),
            texture_library.at("mc_block")
        );

        coin.setScale(glm::vec3(0.3f, 0.3f, 0.3f));

        coin.translate(glm::vec3(-4.0f + (i * 2.5f), 0.5f, -4.0f));

        scene["Mince_" + std::to_string(i)] = coin;
    }


    // Vygenerujeme mesh terénu (Krok sítě 2 pro hezké detaily)
    mesh_library.emplace("terrain", GenHeightMap("resources/heights.png", 2));

    // Vytvoříme model a přiřadíme mu tu tvoji Minecraft texturu (protože kód používá 16x16 atlas)
    Model terrain_model;
    terrain_model.addMesh(
        mesh_library.at("terrain"),
        shader_library.at("advanced_lights"),
        texture_library.at("muj_atlas") // Zde musí být nějaký "texture atlas"
    );

    // Posuneme ho trochu dolů, ať nám neprochází očima
    terrain_model.translate(glm::vec3(-50.0f, -5.0f, -50.0f));

    scene["Terrain"] = terrain_model;





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
        bool is_pov_camera = false;


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
                ImGui::Text("Klavesa G = Skryt ImGui");
                ImGui::Separator();
                ImGui::Text("Vyber Shader:");
                if (ImGui::RadioButton("Basic", active_shader_name == "basic")) { active_shader_name = "basic"; }
                if (ImGui::RadioButton("Basic Core", active_shader_name == "basic_core")) { active_shader_name = "basic_core"; }
                if (ImGui::RadioButton("Basic Uniform", active_shader_name == "basic_uniform")) { active_shader_name = "basic_uniform"; }
                if (ImGui::RadioButton("Rainbow (ShaderToy)", active_shader_name == "rainbow")) { active_shader_name = "rainbow"; }
                ImGui::Separator();
                ImGui::End();
            }

            if (texture_library.count("mc_block") > 0) {
                auto tex = texture_library.at("mc_block");

                // Získáme rozměry a ID z tvé třídy
                int my_image_width = tex->get_width();
                int my_image_height = tex->get_height();
                GLuint mytex = tex->get_name();

                const float scale = 1.0f; // Trochu to zvětšíme, protože ta mc textura je maličká (asi 16x16)

                ImGui::Begin("Prohlizec Textur");
                ImGui::Text("Nactena textura: mc_block");
                // Trik pro předání OpenGL ID do ImGui
                ImGui::Image((ImTextureID)(intptr_t)mytex, ImVec2(my_image_width * scale, my_image_height * scale));
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

            static bool c_was_pressed = false;
            if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
                if (!c_was_pressed) {
                    is_pov_camera = !is_pov_camera;
                    c_was_pressed = true;
                }
            }
            else {
                c_was_pressed = false;
            }

            glm::mat4 v_m;

            if (is_pov_camera && scene.count("Stanfordsky_Kralik") > 0) {
                auto& bunny = scene.at("Stanfordsky_Kralik");

                glm::vec3 cameraOffset = glm::vec3(0.0f, 2.0f, 5.0f);
                glm::vec3 targetPosition = bunny.pivot_position + cameraOffset;

                camera.Position += (targetPosition - camera.Position) * 5.0f * static_cast<float>(previous_frame_render_time);

                v_m = glm::lookAt(camera.Position, bunny.pivot_position, glm::vec3(0.0f, 1.0f, 0.0f));

                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) bunny.translate(glm::vec3(0.0f, 0.0f, -2.0f * previous_frame_render_time));
                if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) bunny.translate(glm::vec3(0.0f, 0.0f, 2.0f * previous_frame_render_time));
                if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) bunny.translate(glm::vec3(-2.0f * previous_frame_render_time, 0.0f, 0.0f));
                if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) bunny.translate(glm::vec3(2.0f * previous_frame_render_time, 0.0f, 0.0f));

            }
            else {
                camera.ProcessInput(window, static_cast<float>(previous_frame_render_time));
                glm::vec3 wallMin(-11.0f, -1.0f, -9.0f);
                glm::vec3 wallMax(9.0f, 19.0f, -7.0f);

                resolveCollision(camera, wallMin, wallMax);
                v_m = camera.GetViewMatrix();
            }

            for (auto& [name, shader] : shader_library) {
                shader->use();
                shader->setUniform("uP_m", projection_matrix);
                shader->setUniform("uV_m", v_m);
            }

            if (shader_library.count("advanced_lights") > 0) {
                auto light_shader = shader_library.at("advanced_lights");
                light_shader->use();

                // 1. BATERKA NA KAMEŘE (SpotLight)
                // Protože počítáme ve View Space, baterka je fixně na 0,0,0 a svítí dopředu
                light_shader->setUniform("spotLight.position", glm::vec3(0.0f, 0.0f, 0.0f));
                light_shader->setUniform("spotLight.direction", glm::vec3(0.0f, 0.0f, -1.0f));
                light_shader->setUniform("spotLight.color", glm::vec3(1.0f, 1.0f, 0.8f)); // Lehce žluté světlo
                light_shader->setUniform("spotLight.intensity", 2.0f);
                light_shader->setUniform("spotLight.cutOff", glm::cos(glm::radians(15.0f))); // Úhel kužele 15 stupňů
                light_shader->setUniform("spotLight.spotExponent", 10.0f); // Rozmazání okrajů
                // Útlum do dálky
                light_shader->setUniform("spotLight.constant", 1.0f);
                light_shader->setUniform("spotLight.linear", 0.045f);
                light_shader->setUniform("spotLight.quadratic", 0.0075f);

                // 2. LÉTAJÍCÍ SVĚTLUŠKY (PointLights)
                float t = static_cast<float>(glfwGetTime());

                // Nadeklarujeme si 3 pozice ve World Space (světlušky krouží pomocí sin/cos)
                glm::vec3 pl_positions[3] = {
                    glm::vec3(sin(t) * 4.0f, 1.0f, cos(t) * 4.0f - 4.0f),       // Krouží kolem dokola
                    glm::vec3(sin(t * 1.5f) * 2.0f, 3.0f, cos(t * 1.5f) * 2.0f - 4.0f), // Krouží rychleji výše
                    glm::vec3(0.0f, sin(t * 2.0f) * 3.0f + 2.0f, -4.0f)           // Létá nahoru a dolů
                };

                glm::vec3 pl_colors[3] = {
                    glm::vec3(1.0f, 0.2f, 0.2f), // Červená
                    glm::vec3(0.2f, 1.0f, 0.2f), // Zelená
                    glm::vec3(0.2f, 0.2f, 1.0f)  // Modrá
                };

                for (int i = 0; i < 3; i++) {
                    glm::vec3 view_pos = glm::vec3(v_m * glm::vec4(pl_positions[i], 1.0f));

                    std::string base = "pointLights[" + std::to_string(i) + "].";

                    light_shader->setUniform(base + "position", view_pos);
                    light_shader->setUniform(base + "color", pl_colors[i]);
                    light_shader->setUniform(base + "intensity", 3.0f);
                    light_shader->setUniform(base + "constant", 1.0f);
                    light_shader->setUniform(base + "linear", 0.09f);
                    light_shader->setUniform(base + "quadratic", 0.032f);
                }
            }

            std::vector<Model*> transparent_models;
            transparent_models.reserve(scene.size());
            std::vector<std::string> sebrane_mince;

            for (auto& item : scene) {
                if (item.first == "Stanfordsky_Kralik") {
                    item.second.rotate(glm::vec3(0.0f, 1.0f, 0.0f));
                }

                if (item.first.find("Mince_") != std::string::npos) {
                    item.second.rotate(glm::vec3(0.0f, 3.0f, 0.0f));

                    float vzdalenost = glm::distance(camera.Position, item.second.getPosition());

                    if (vzdalenost < 1.5f) {
                        sebrane_mince.push_back(item.first); 
                    }
                }

                item.second.update(0.016f);

                if (!item.second.is_transparent) {
                    item.second.meshes[0].shader->use();
                    item.second.meshes[0].shader->setUniform("object_alpha", 1.0f);
                    item.second.draw();
                }
                else {
                    transparent_models.push_back(&item.second);
                }
            }

            for (const std::string& jmeno_mince : sebrane_mince) {
                glm::vec3 pozice = scene[jmeno_mince].getPosition();
                scene.erase(jmeno_mince);

                ma_engine_play_sound(&audio_engine, "resources/ouch.wav", NULL);

                std::cout << "\n==================================\n";
                std::cout << " CINK! Sebral jsi: " << jmeno_mince << "!\n";
                std::cout << "==================================\n";

                for (int p = 0; p < 10; p++) {
                    Particle castice;
                    castice.model.addMesh(mesh_library.at("cube"), shader_library.at("advanced_lights"), texture_library.at("mc_block"));

                    castice.model.translate(pozice);
                    castice.model.setScale(glm::vec3(0.05f, 0.05f, 0.05f));

                    float vx = ((rand() % 100) / 50.0f) - 1.0f; // od -1.0 do 1.0
                    float vy = ((rand() % 100) / 50.0f) + 1.0f; // od 1.0 do 3.0 (vždy nahoru)
                    float vz = ((rand() % 100) / 50.0f) - 1.0f; // od -1.0 do 1.0
                    castice.velocity = glm::vec3(vx, vy, vz) * 3.0f;

                    castice.lifetime = 1.5f; 
                    aktivni_castice.push_back(castice);
                }
            }

            std::sort(transparent_models.begin(), transparent_models.end(), [&](Model* a, Model* b) {
                return glm::distance(camera.Position, a->getPosition()) > glm::distance(camera.Position, b->getPosition());
                });

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);

            for (Model* p : transparent_models) {
                p->meshes[0].shader->use();
                p->meshes[0].shader->setUniform("object_alpha", 0.4f);
                glDisable(GL_CULL_FACE);
                p->draw();
                glEnable(GL_CULL_FACE);
            }

            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);

            for (auto it = aktivni_castice.begin(); it != aktivni_castice.end(); ) {
                float dt = static_cast<float>(previous_frame_render_time);

                it->lifetime -= dt;

                if (it->lifetime <= 0.0f) {
                    it = aktivni_castice.erase(it);
                }
                else {
                    it->velocity.y -= 9.81f * dt;

                    it->model.translate(it->velocity * dt);
                    it->model.rotate(glm::vec3(10.0f * dt, 20.0f * dt, 5.0f * dt));

                    it->model.meshes[0].shader->use();
                    it->model.meshes[0].shader->setUniform("object_alpha", it->lifetime / 1.5f);

                    glEnable(GL_CULL_FACE);
                    it->model.draw();

                    ++it; 
                }
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


uchar App::getmap(cv::Mat& map, int x, int y)
{
    x = std::clamp(x, 0, map.cols);
    y = std::clamp(y, 0, map.rows);

    //at(row,col)!!!
    return map.at<uchar>(y, x);
}

// Random map gen
void App::genLabyrinth(cv::Mat& map) {
    cv::Point2i start_position, end_position;

    // C++ random numbers
    std::random_device r; // Seed with a real random value, if available
    std::default_random_engine e1(r());
    std::uniform_int_distribution<int> uniform_height(1, map.rows - 2); // uniform distribution between int..int
    std::uniform_int_distribution<int> uniform_width(1, map.cols - 2);
    std::uniform_int_distribution<int> uniform_block(0, 15); // how often are walls generated: 0=wall, anything else=empty

    //inner maze 
    for (int j = 0; j < map.rows; j++) {
        for (int i = 0; i < map.cols; i++) {
            switch (uniform_block(e1))
            {
            case 0:
                map.at<uchar>(cv::Point(i, j)) = '#';
                break;
            default:
                map.at<uchar>(cv::Point(i, j)) = '.';
                break;
            }
        }
    }

    //walls
    for (int i = 0; i < map.cols; i++) {
        map.at<uchar>(cv::Point(i, 0)) = '#';
        map.at<uchar>(cv::Point(i, map.rows - 1)) = '#';
    }
    for (int j = 0; j < map.rows; j++) {
        map.at<uchar>(cv::Point(0, j)) = '#';
        map.at<uchar>(cv::Point(map.cols - 1, j)) = '#';
    }

    //gen start_position inside maze (excluding walls)
    do {
        start_position.x = uniform_width(e1);
        start_position.y = uniform_height(e1);
    } while (getmap(map, start_position.x, start_position.y) == '#'); //check wall

    //gen end different from start, inside maze (excluding outer walls) 
    do {
        end_position.x = uniform_width(e1);
        end_position.y = uniform_height(e1);
    } while (start_position == end_position); //check overlap
    map.at<uchar>(cv::Point(end_position.x, end_position.y)) = 'e';

    std::cout << "Start: " << start_position << std::endl;
    std::cout << "End: " << end_position << std::endl;

    //print map
    for (int j = 0; j < map.rows; j++) {
        for (int i = 0; i < map.cols; i++) {
            if ((i == start_position.x) && (j == start_position.y))
                std::cout << 'X';
            else
                std::cout << getmap(map, i, j);
        }
        std::cout << std::endl;
    }

    //set player position in 3D space (transform X-Y in map to XYZ in GL)
    // Násobíme dvěma, protože i kostky při stavění násobíme dvěma (jejich rozteč)
    camera.Position.x = start_position.x * 2.0f;
    camera.Position.z = start_position.y * 2.0f;
    // Výška očí hráče - např. 1.5 metru nad zemí
    camera.Position.y = 1.5f;
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
    ma_engine_uninit(&audio_engine);
}

App::~App() {
    destroy();
    std::cout << "Bye...\n";
}