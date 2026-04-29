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
#include "OBJloader.hpp"
#include "Texture.hpp"

#include "gl_err_callback.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>


void  App::resolveCollision() {
    // Definujeme velikost hráče
    float playerRadius = 0.3f;

    // Převod 3D souřadnic na indexy
    float mapX = camera.Position.x / 2.0f;
    float mapZ = camera.Position.z / 2.0f;

    // Okolí hráče (9 políček kolem něj)
    int gridX = (int)std::round(mapX);
    int gridZ = (int)std::round(mapZ);

    for (int j = gridZ - 1; j <= gridZ + 1; j++) {
        for (int i = gridX - 1; i <= gridX + 1; i++) {

            // Pokud je na políčku zeď
            if (getmap(mapa, i, j) == '#') {
                // Skutečná 3D pozice středu této zdi
                float wallX = i * 2.0f;
                float wallZ = j * 2.0f;

                // Nejbližší bod na zdi k hráči
                float closestX = std::max(wallX - 1.0f, std::min(camera.Position.x, wallX + 1.0f));
                float closestZ = std::max(wallZ - 1.0f, std::min(camera.Position.z, wallZ + 1.0f));

                // Vzdálenost mezi hráčem a tímto nejbližším bodem
                float distanceX = camera.Position.x - closestX;
                float distanceZ = camera.Position.z - closestZ;
                float distanceSquared = (distanceX * distanceX) + (distanceZ * distanceZ);

                // Pokud je vzdálenost menší než poloměr hráče, došlo ke kolizi
                if (distanceSquared < (playerRadius * playerRadius)) {
                    float distance = std::sqrt(distanceSquared);

                    // Prevence dělení nulou
                    if (distance < 0.0001f) continue;

                    // Kolik musíme hráče vytlačit ven
                    float overlap = playerRadius - distance;

                    // Vytlačení ve směru od stěny
                    camera.Position.x += (distanceX / distance) * overlap;
                    camera.Position.z += (distanceZ / distance) * overlap;
                }
            }
        }
    }
}

void App::shoot() {
    if (is_free_camera || currentState != GameState::PLAYING) return;

    Projektil p;
    p.model.addMesh(mesh_library.at("sphere"), shader_library.at("advanced_lights"));
    p.model.is_transparent = true;
    p.model.setScale(glm::vec3(0.1f));
    p.model.translate(camera.Position + camera.Front * 0.5f);
    p.velocity = camera.Front * 20.0f;
    p.lifetime = 2.0f;
    aktivni_projektily.push_back(p);

    ma_engine_play_sound(&audio_engine, "resources/shoot.wav", NULL);
}


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

        if (ma_engine_init(NULL, &audio_engine) == MA_SUCCESS) {
            // Načteme songu jako STREAM (šetří RAM) a připravíme objekt bgMusic
            if (ma_sound_init_from_file(&audio_engine, "resources/background.mp3", MA_SOUND_FLAG_STREAM, NULL, NULL, &bgMusic) == MA_SUCCESS) {
                ma_sound_set_looping(&bgMusic, MA_TRUE); // pořád dokola
                ma_sound_set_volume(&bgMusic, 0.20f);   
                isBgMusicLoaded = true;
            }
        }
        else {
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
    mesh_library.emplace("sphere", generateSphere(36, 18));

    std::vector<Vertex> box_vertices;
    std::vector<GLuint> box_indices;
    if (loadOBJ("resources/cube_quads.obj", box_vertices, box_indices)) {
        mesh_library.emplace("cube", std::make_shared<Mesh>(box_vertices, box_indices, GL_TRIANGLES));
    }

    std::vector<Vertex> coin_vertices;
    std::vector<GLuint> coin_indices;
    if (loadOBJ("resources/coin.obj", coin_vertices, coin_indices)) {

        // překlopení modelu o 90 stupňů
        for (auto& v : coin_vertices) {
            float puvodni_y = v.position.y;
            v.position.y = v.position.z; // Postavíme ji
            v.position.z = -puvodni_y;   // Srovnáme hloubku
        }

        mesh_library.emplace("model_mince", std::make_shared<Mesh>(coin_vertices, coin_indices, GL_TRIANGLES));
        std::cout << "Model mince USPESNE nacten!\n";
    }

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Vertex> bunny_vertices;
    std::vector<GLuint> bunny_indices;

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

    texture_library.emplace("zlato", std::make_shared<Texture>(
        std::filesystem::path("resources/zlato.png"),
        Texture::Interpolation::nearest
    ));

    texture_library.emplace("zed", std::make_shared<Texture>(
        std::filesystem::path("resources/stone.png"),
        Texture::Interpolation::nearest
    ));

    texture_library.emplace("strop", std::make_shared<Texture>(
        std::filesystem::path("resources/strop.png"),
        Texture::Interpolation::nearest
    ));
}


void App::startNewGame() {
    scene.clear();
    aktivni_castice.clear();
    aktivni_projektily.clear();
    gameTimer = initialTime;
	sebrano_minci = 0;

    // Mapa podle nastavení z menu
    mapa = cv::Mat(mazeRows, mazeCols, CV_8U);
    genLabyrinth(mapa);

    // Postavíme 3D svět
    for (int j = 0; j < mapa.rows; j++) {
        for (int i = 0; i < mapa.cols; i++) {
            char policko = getmap(mapa, i, j);

            if (policko == '#') {
                // ZEĎ (Složená ze 3 kostek nad sebou)
                for (int vyska = 0; vyska < 3; vyska++) {
                    Model zed_blok;
                    zed_blok.addMesh(mesh_library.at("cube"), shader_library.at("advanced_lights"), texture_library.at("zed"));

                    zed_blok.translate(glm::vec3(i * 2.0f, vyska * 2.0f, j * 2.0f));
                    scene["Zed_" + std::to_string(i) + "_" + std::to_string(vyska) + "_" + std::to_string(j)] = zed_blok;
                }
            }
            else {
                // PRÁZDNÝ PROSTOR (Chodba)
                Model podlaha;
                podlaha.addMesh(mesh_library.at("cube"), shader_library.at("advanced_lights"), texture_library.at("strop"));
                podlaha.setScale(glm::vec3(1.0f, 0.1f, 1.0f));
                podlaha.translate(glm::vec3(i * 2.0f, -1.0f, j * 2.0f));
                scene["Podlaha_" + std::to_string(i) + "_" + std::to_string(j)] = podlaha;

                Model strop = podlaha;
                strop.translate(glm::vec3(0.0f, 6.0f, 0.0f));
                scene["Strop_" + std::to_string(i) + "_" + std::to_string(j)] = strop;

                if (policko == 'e') {
                    // CÍLOVÝ DUCH
                    Model duch;
                    //duch.addMesh(mesh_library.at("sphere"), shader_library.at("advanced_lights"));
                    duch.addMesh(mesh_library.at("sphere"), shader_library.at("rainbow"));
                    duch.is_transparent = true;
                    duch.setScale(glm::vec3(0.8f, 0.8f, 0.8f));
                    duch.translate(glm::vec3(i * 2.0f, 1.0f, j * 2.0f));
                    scene["Cilovy_Duch"] = duch;
                }
                // NÁHODNÉ MINCE
                else if (policko == '.' && (rand() % 100 < 5)) {
                    Model coin;
                    coin.addMesh(mesh_library.at("model_mince"), shader_library.at("advanced_lights"), texture_library.at("zlato"));
                    coin.setScale(glm::vec3(0.2f, 0.2f, 0.2f));


                    coin.translate(glm::vec3(i * 2.0f, 0.5f, j * 2.0f));
                    scene["Mince_" + std::to_string(i) + "_" + std::to_string(j)] = coin;
                }
            }
        }
    }

    currentState = GameState::PLAYING;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Schováme myš pro hru
	spocitej_mince();
    if (isBgMusicLoaded) {
        ma_sound_start(&bgMusic); // Odstartuje hudbu
    }
}

int App::run(void) {
    try {
        double FPS = 0.0;
        double now = glfwGetTime();
        double frame_begin_timepoint = now;
        double previous_frame_render_time{};

        while (!glfwWindowShouldClose(window)) {
            // Časování pro fyziku a pohyb
            now = glfwGetTime();
            previous_frame_render_time = now - frame_begin_timepoint;
            frame_begin_timepoint = now;
            float dt_float = static_cast<float>(previous_frame_render_time);

            // Vyčištění obrazovky
            glClearColor(0.005f, 0.005f, 0.01f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // STAV 1: HLAVNÍ MENU
            if (currentState == GameState::MENU) {
                // Odjistíme kurzor
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                // Okno menu uprostřed obrazovky
                ImGui::SetNextWindowPos(ImVec2(win_width / 2.0f - 150, win_height / 2.0f - 110), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(300, 220));

                ImGui::Begin("GHOST HUNTER - MENU", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
                ImGui::Text("Nastaveni bludiste:");
                ImGui::SliderInt("Sirka", &mazeCols, 5, 40);
                ImGui::SliderInt("Vyska", &mazeRows, 5, 40);
                ImGui::Separator();
                if (ImGui::Button("START HRY", ImVec2(280, 50))) {
                    startNewGame();
                }
                if (ImGui::Button("KONEC", ImVec2(280, 30))) {
                    glfwSetWindowShouldClose(window, true);
                }
                ImGui::End();

                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }
            // STAV 2: SAMOTNÁ HRA
            else if (currentState == GameState::PLAYING) {
                

                // ZAHÁJENÍ IMGUI PRO TENTO SNÍMEK
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                // Původní ImGui (Herní Info)
                if (show_imgui) {
                    ImGui::SetNextWindowPos(ImVec2(10, 10));
                    ImGui::SetNextWindowSize(ImVec2(300, 150));
                    ImGui::Begin("Herni Info", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
                    ImGui::Text("V-Sync: %s", is_vsync_on ? "ON" : "OFF");
                    ImGui::Text("FPS: %.1f", FPS);
                    ImGui::Text("---------------------------");
                    ImGui::Text("ESC = Zpet do MENU");
                    ImGui::Text("Klavesa V = Prepnout VSync");
                    ImGui::Text("Klavesa F11 = Fullscreen");
                    ImGui::Text("Klavesa G = Skryt ImGui");
                    ImGui::Separator();
                    ImGui::Checkbox("Free Camera (Noclip)", &is_free_camera);
                    ImGui::End();
                }

                // Shadery
                glm::mat4 v_m;

                gameTimer -= dt_float;
                if (gameTimer <= 0.0f) {
                    gameTimer = 0.0f;
                    currentState = GameState::GAMEOVER; // PROHRA NA ČAS
                    ma_sound_stop(&bgMusic); // Zastavíme hudbu
                }

                // Pohyb Kamery a Kolize
                if (is_free_camera) {
                    camera.ProcessInput(window, dt_float, true);
                }
                else {
                    camera.ProcessInput(window, dt_float, false); // Zakázat klouzání

                    // Vlastní gravitace a skok
                    playerVelocityY -= 15.0f * dt_float;
                    camera.Position.y += playerVelocityY * dt_float;

                    if (camera.Position.y <= 1.5f) {
                        camera.Position.y = 1.5f;
                        playerVelocityY = 0.0f;
                        isGrounded = true;
                    }
                    else {
                        isGrounded = false;
                    }

                    // SYSTÉM KROKŮ 
                    if (isGrounded) {
                        bool isMoving = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
                            glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
                            glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
                            glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

                        if (isMoving) {
                            footstepTimer -= dt_float; // Odečet času

                            if (footstepTimer <= 0.0f) {
                                // Zvuk podle toho, na které noze jsme
                                if (stepToggle) {
                                    ma_engine_play_sound(&audio_engine, "resources/step1.wav", NULL);
                                }
                                else {
                                    ma_engine_play_sound(&audio_engine, "resources/step2.wav", NULL);
                                }
                                stepToggle = !stepToggle; // Přepínání nohou
                                footstepTimer = 0.45f;    // Čas mezi kroky
                            }
                        }
                        else {
                            // Po zastavení se resetuje čas
                            footstepTimer = 0.0f;
                        }
                    }


                    resolveCollision();
                }
                v_m = camera.GetViewMatrix();

                // Odeslání matic do shaderů
                for (auto& [name, shader] : shader_library) {
                    shader->use();
                    shader->setUniform("uP_m", projection_matrix);
                    shader->setUniform("uV_m", v_m);
                }

                // Nastavení světel
                if (shader_library.count("advanced_lights") > 0) {
                    auto light_shader = shader_library.at("advanced_lights");
                    light_shader->use();

                    // SMĚROVÉ SVĚTLO A AMBIENT
                    // Globální směr světla v mapě
                    glm::vec3 world_dir = glm::vec3(-0.2f, -1.0f, -0.3f);

                    glm::vec3 view_dir = glm::mat3(v_m) * world_dir;

                    light_shader->setUniform("dirLight.direction", view_dir);
                    light_shader->setUniform("dirLight.ambient", glm::vec3(0.001f, 0.001f, 0.005f));

                    light_shader->setUniform("dirLight.diffuse", glm::vec3(0.01f, 0.015f, 0.02f));

                    // bez odlesku
                    light_shader->setUniform("dirLight.specular", glm::vec3(0.0f, 0.0f, 0.0f));
                    
                    // Baterka
                    light_shader->setUniform("spotLight.position", glm::vec3(0.0f, 0.0f, 0.0f));
                    light_shader->setUniform("spotLight.direction", glm::vec3(0.0f, 0.0f, -1.0f));
                    light_shader->setUniform("spotLight.color", glm::vec3(1.0f, 1.0f, 0.8f));
                    light_shader->setUniform("spotLight.intensity", 2.0f);
                    light_shader->setUniform("spotLight.cutOff", glm::cos(glm::radians(15.0f)));
                    light_shader->setUniform("spotLight.spotExponent", 10.0f);
                    light_shader->setUniform("spotLight.constant", 1.0f);
                    light_shader->setUniform("spotLight.linear", 0.045f);
                    light_shader->setUniform("spotLight.quadratic", 0.0075f);

                    if (scene.count("Cilovy_Duch") > 0) {
                        glm::vec3 duch_pos = scene.at("Cilovy_Duch").getPosition();

                        // Převod do View Space (světla z pohledu kamery)
                        glm::vec3 view_pos = glm::vec3(v_m * glm::vec4(duch_pos, 1.0f));

                        light_shader->setUniform("pointLights[0].position", view_pos);
                        light_shader->setUniform("pointLights[0].color", glm::vec3(0.6f, 0.2f, 1.0f)); 
                        light_shader->setUniform("pointLights[0].intensity", 0.5f); // Dosvit
                        light_shader->setUniform("pointLights[0].constant", 1.0f);
                        light_shader->setUniform("pointLights[0].linear", 0.09f);
                        light_shader->setUniform("pointLights[0].quadratic", 0.032f);
                    }
                    else {
                        // Pokud je duch zastřelený, jeho světlo zhasne
                        light_shader->setUniform("pointLights[0].intensity", 0.0f);
                    }

                    if (!aktivni_projektily.empty()) {
                        // Z první kulky se udělá světlo
                        glm::vec3 kulka_pos = aktivni_projektily[0].model.getPosition();
                        glm::vec3 view_pos_kulky = glm::vec3(v_m * glm::vec4(kulka_pos, 1.0f));

                        light_shader->setUniform("pointLights[1].position", view_pos_kulky);
                        light_shader->setUniform("pointLights[1].color", glm::vec3(1.0f, 0.5f, 0.0f));
                        light_shader->setUniform("pointLights[1].intensity", 3.0f);
                        light_shader->setUniform("pointLights[1].constant", 1.0f);
                        light_shader->setUniform("pointLights[1].linear", 0.09f);
                        light_shader->setUniform("pointLights[1].quadratic", 0.032f);
                    }
                    else {
                        light_shader->setUniform("pointLights[1].intensity", 0.0f);
                    }

                    light_shader->setUniform("pointLights[2].intensity", 0.0f);
                }

                // Vykreslování objektů a sbírání mincí
                std::vector<Model*> transparent_models;
                transparent_models.reserve(100);
                std::vector<std::string> sebrane_mince;

                // View Cone Culling (Zorný úhel)
                float render_distance = 30.0f;

                for (auto& item : scene) {
                    glm::vec3 pozice_objektu = item.second.getPosition();
                    glm::vec3 smer_k_objektu = pozice_objektu - camera.Position;
                    float vzdalenost = glm::length(smer_k_objektu);

                    // Zahození extrémů
                    if (vzdalenost > render_distance) {
                        continue;
                    }

                    // ZA POSTAVOU (Dot Product kamera vs objekt)
                    if (vzdalenost > 2.0f) {
                        glm::vec3 smer_normalizovany = smer_k_objektu / vzdalenost;
                        float uhel_pohledu = glm::dot(smer_normalizovany, camera.Front);

                        // Dot product: 1.0 = přesně uprostřed obrazovky, -1.0 = přesně za postavou
                        if (uhel_pohledu < 0.5f) {
                            continue; // Objekt je mimo obrazovku, vůbec ho neposíláme na grafiku
                        }
                    }

                    // Detekce mincí
                    if (item.first[0] == 'M' && item.first[1] == 'i') {

                        // Čistá rotace zleva doprava kolem svislé osy Y
                        item.second.rotate(glm::vec3(0.0f, 100.0f * dt_float, 0.0f));

                        if (vzdalenost < 1.5f) {
                            sebrane_mince.push_back(item.first);
                        }
                    }

                    item.second.update(dt_float);

                    // Samotné vykreslení
                    if (!item.second.is_transparent) {
                        item.second.meshes[0].shader->use();
                        item.second.meshes[0].shader->setUniform("object_alpha", 1.0f);
                        bool je_mince = (item.first[0] == 'M' && item.first[1] == 'i');

                        if (je_mince) {
                            // Mince bude ignorovat tmu a bude lehce zářit do žluta ze všech stran
                            item.second.meshes[0].shader->setUniform("dirLight.ambient", glm::vec3(0.3f, 0.3f, 0.1f));
                        }

                        item.second.draw();

                        if (je_mince) {
                            // Okamžitě po vykreslení mince se vrací tma
                            item.second.meshes[0].shader->setUniform("dirLight.ambient", glm::vec3(0.005f, 0.005f, 0.01f));
                        }
                    }
                    else {
                        // Průhledné objekty později
                        transparent_models.push_back(&item.second);
                    }
                }

                // Efekty po sebrání mince
                for (const std::string& jmeno_mince : sebrane_mince) {
                    glm::vec3 pozice = scene[jmeno_mince].getPosition();
                    scene.erase(jmeno_mince);

                    sebrano_minci++;

                    ma_engine_play_sound(&audio_engine, "resources/coins.wav", NULL);

                    for (int p = 0; p < 10; p++) {
                        Particle castice;
                        castice.model.addMesh(mesh_library.at("cube"), shader_library.at("advanced_lights"), texture_library.at("mc_block"));
                        castice.model.translate(pozice);
                        castice.model.setScale(glm::vec3(0.05f, 0.05f, 0.05f));

                        float vx = ((rand() % 100) / 50.0f) - 1.0f;
                        float vy = ((rand() % 100) / 50.0f) + 1.0f;
                        float vz = ((rand() % 100) / 50.0f) - 1.0f;
                        castice.velocity = glm::vec3(vx, vy, vz) * 3.0f;

                        castice.lifetime = 1.5f;
                        aktivni_castice.push_back(castice);
                    }
                }


                // Průhledné objekty (Malířův algoritmus)
                std::sort(transparent_models.begin(), transparent_models.end(), [&](Model* a, Model* b) {
                    return glm::length(camera.Position - a->getPosition()) > glm::length(camera.Position - b->getPosition());
                    });

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDepthMask(GL_FALSE);

                for (Model* p : transparent_models) {
                    p->meshes[0].shader->use();
                    p->meshes[0].shader->setUniform("object_alpha", 0.6f);
                    p->meshes[0].shader->setUniform("iTime", (float)glfwGetTime());
                    glDisable(GL_CULL_FACE);
                    p->draw();
                    glEnable(GL_CULL_FACE);
                }

                glDisable(GL_BLEND);
                glDepthMask(GL_TRUE);

                // AKTUALIZACE PROJEKTILŮ A DETEKCE ZÁSAHU
                for (auto it = aktivni_projektily.begin(); it != aktivni_projektily.end(); ) {
                    float dt_update = dt_float;
                    it->lifetime -= dt_update;

                    if (it->lifetime <= 0.0f) {
                        it = aktivni_projektily.erase(it); // Smazání, pokud letěla moc dlouho
                    }
                    else {
                        it->model.translate(it->velocity* dt_update);

                        // KOLIZE KULKY SE STĚNOU + EFEKT
                        float pMapX = it->model.getPosition().x / 2.0f;
                        float pMapZ = it->model.getPosition().z / 2.0f;
                        int gridX = (int)std::round(pMapX);
                        int gridZ = (int)std::round(pMapZ);

                        bool stenaHit = false;
                        if (gridX >= 0 && gridX < mapa.cols && gridZ >= 0 && gridZ < mapa.rows) {
                            if (getmap(mapa, gridX, gridZ) == '#') {
                                stenaHit = true;
                            }
                        }

                        // KONTROLA KOLIZE S DUCHEM (jen pokud jsme netrefili zeď)
                        bool zasah = false;
                        if (!stenaHit && scene.count("Cilovy_Duch") > 0) {
                            float vzdalenost = glm::length(it->model.getPosition() - scene.at("Cilovy_Duch").getPosition());
                            if (vzdalenost < 1.0f) {
                                zasah = true;
                                glm::vec3 pozice_ducha = scene["Cilovy_Duch"].getPosition();
                                scene.erase("Cilovy_Duch");
                                ma_engine_play_sound(&audio_engine, "resources/ouch.wav", NULL);

                                currentState = GameState::WIN; // Stav hry na výhru
                                ma_sound_stop(&bgMusic);

                                // Výbuch konfet
                                for (int p = 0; p < 30; p++) {
                                    Particle castice;
                                    castice.model.addMesh(mesh_library.at("cube"), shader_library.at("advanced_lights"), texture_library.at("mc_block"));
                                    castice.model.translate(pozice_ducha);
                                    castice.model.setScale(glm::vec3(0.1f, 0.1f, 0.1f));
                                    float vx = ((rand() % 100) / 50.0f) - 1.0f;
                                    float vy = ((rand() % 100) / 50.0f) + 1.0f;
                                    float vz = ((rand() % 100) / 50.0f) - 1.0f;
                                    castice.velocity = glm::vec3(vx, vy, vz) * 5.0f;
                                    castice.lifetime = 3.0f;
                                    aktivni_castice.push_back(castice);
                                }
                            }
                        }

                        // ROZBITÍ KULKY A EFEKTY 
                        if (stenaHit) {
                            // EFEKT NÁRAZU DO STĚNY
                            glm::vec3 hitPozice = it->model.getPosition();

                            // 10 malých částic
                            for (int p = 0; p < 10; p++) {
                                Particle úlomek;
                                úlomek.model.addMesh(mesh_library.at("cube"), shader_library.at("advanced_lights"), texture_library.at("mc_block"));
                                úlomek.model.translate(hitPozice);

                                // Velikost částic
                                úlomek.model.setScale(glm::vec3(0.03f, 0.03f, 0.03f));

                                // Náhodná rychlost ve všech směrech (sférický rozstřik)
                                float vx = ((rand() % 100) / 50.0f) - 1.0f; // -1.0f až 1.0f
                                float vy = ((rand() % 100) / 50.0f) - 0.5f; // -0.5f až 1.5f (víc nahoru)
                                float vz = ((rand() % 100) / 50.0f) - 1.0f; // -1.0f až 1.0f

								// Rychlost částic
                                úlomek.velocity = glm::vec3(vx, vy, vz) * 4.0f;

                                úlomek.lifetime = 0.8f; // Krátký život
                                aktivni_castice.push_back(úlomek);
                            }

                            it = aktivni_projektily.erase(it); // Kulka zmizí
                        }
                        else if (zasah) {
                            it = aktivni_projektily.erase(it); // Kulka zmizí po zásahu ducha
                        }
                        else {
                            // ZAPNOUT BLENDING (míchání barev)
                            glEnable(GL_BLEND);
                            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                            // NASTAVIT SHADER A UNIFORMU
                            it->model.meshes[0].shader->use();

                            // Poloprůhlednost
                            it->model.meshes[0].shader->setUniform("object_alpha", 0.5f);

                            it->model.meshes[0].shader->setUniform("dirLight.ambient", glm::vec3(2.0f, 1.0f, 0.0f));

                            // VYKRESLIT
                            it->model.draw();

                            it->model.meshes[0].shader->setUniform("dirLight.ambient", glm::vec3(0.005f, 0.005f, 0.01f));

                            // VYPNOUT BLENDING (aby neovlivnil ostatní pevné objekty)
                            glDisable(GL_BLEND);

                            ++it;
                        }
                    }
                }

                // Částice
                for (auto it = aktivni_castice.begin(); it != aktivni_castice.end(); ) {
                    it->lifetime -= dt_float;
                    if (it->lifetime <= 0.0f) {
                        it = aktivni_castice.erase(it);
                    }
                    else {
                        it->velocity.y -= 9.81f * dt_float;
                        it->model.translate(it->velocity * dt_float);
                        it->model.rotate(glm::vec3(10.0f * dt_float, 20.0f * dt_float, 5.0f * dt_float));

                        it->model.meshes[0].shader->use();
                        it->model.meshes[0].shader->setUniform("object_alpha", it->lifetime / 1.5f);

                        glEnable(GL_CULL_FACE);
                        it->model.draw();
                        ++it;
                    }
                }

                // HUD POČÍTADLO MINCÍ
                // Flags pro "neviditelné" okno v rohu
                ImGuiWindowFlags corner_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

                ImGui::SetNextWindowBgAlpha(0.35f); // Průhledné pozadí

                // Pozice:win_width - padding, padding. Pivot: 1.0f (pravý okraj), 0.0f (horní okraj)
                const float DISTANCE_FROM_EDGE = 10.0f;
                ImGui::SetNextWindowPos(ImVec2(win_width - DISTANCE_FROM_EDGE, DISTANCE_FROM_EDGE), ImGuiCond_Always, ImVec2(1.0f, 0.0f));

                // Vykreslení okna
                if (ImGui::Begin("CoinCounterHUD", nullptr, corner_flags))
                {
                    ImGui::SetWindowFontScale(1.8f);
                    ImGui::Text("Mince: %d / %d", sebrano_minci, celkem_minci_na_mape);

                    // ČAS
                    if (gameTimer < 10.0f) {
                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "CAS: %.1f s", gameTimer);
                    }
                    else {
                        ImGui::Text("CAS: %.1f s", gameTimer);
                    }

                    ImGui::SetWindowFontScale(1.0f);

                    // Pokrok
                    if (celkem_minci_na_mape > 0) {
                        float progress = (float)sebrano_minci / (float)celkem_minci_na_mape;
                        ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));
                    }
                    ImGui::End();
                }

                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                
            }

            else if (currentState == GameState::WIN) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                ImGui::SetNextWindowPos(ImVec2(win_width / 2.0f - 150, win_height / 2.0f - 80));
                ImGui::Begin("VITEZSTVI", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
                ImGui::Text("GRATULUJEME! DUCH JE MRTVY!");
                ImGui::Text("Zbyvajici cas: %.1f s", gameTimer);
                ImGui::Text("Sebrano minci: %d", sebrano_minci, "/", celkem_minci_na_mape);
                if (ImGui::Button("HLAVNI MENU", ImVec2(300, 50))) {
                    currentState = GameState::MENU;
                }
                ImGui::End();

                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }
            // STAV PROHRA (GAMEOVER)
            else if (currentState == GameState::GAMEOVER) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                ImGui::SetNextWindowPos(ImVec2(win_width / 2.0f - 150, win_height / 2.0f - 80));
                ImGui::Begin("KONEC HRY", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "CAS VYPRSEL!");
                ImGui::Text("Duch ti utekl...");
                if (ImGui::Button("Zkusit znovu", ImVec2(300, 50))) {
                    currentState = GameState::MENU;
                }
                ImGui::End();

                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }

            glfwSwapBuffers(window);
            glfwPollEvents();

            // FPS Counter
            fps_counter.update();
            if (fps_counter.is_updated()) {
                FPS = fps_counter.get();
                std::string title = "GHOST HUNTER - FPS: " + std::to_string((int)FPS);
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


void App::spocitej_mince() {
    celkem_minci_na_mape = 0;
    // Průchod celé OpenCV matice mapy
    for (int j = 0; j < mapa.rows; j++) {
        for (int i = 0; i < mapa.cols; i++) {
            if (scene.count("Mince_" + std::to_string(i) + "_" + std::to_string(j)) > 0) {
                celkem_minci_na_mape++;
            }
        }
    }
}


uchar App::getmap(cv::Mat& map, int x, int y)
{
    x = std::clamp(x, 0, map.cols);
    y = std::clamp(y, 0, map.rows);

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
    camera.Position.x = start_position.x * 2.0f;
    camera.Position.z = start_position.y * 2.0f;
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
    if (isBgMusicLoaded) ma_sound_uninit(&bgMusic);
    ma_engine_uninit(&audio_engine);
}

App::~App() {
    destroy();
    std::cout << "Bye...\n";
}