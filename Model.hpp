#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <memory> 

#include <GL/glew.h>
#include <glm/glm.hpp> 

#include "assets.hpp"
#include "Mesh.hpp"
#include "ShaderProgram.hpp"
#include "OBJloader.hpp"

class Model {
public:
    // origin point of whole model
    glm::vec3 pivot_position{}; // [0,0,0] of the object
    glm::vec3 eulerAngles{};    // pitch, yaw, roll
    glm::vec3 scale{ 1.0f, 1.0f, 1.0f };

    // mesh related data
    struct mesh_package {
        std::shared_ptr<Mesh> mesh;
        std::shared_ptr<ShaderProgram> shader; 

        glm::vec3 origin;
        glm::vec3 eulerAngles;
        glm::vec3 scale;
    }; 

    std::vector<mesh_package> meshes;

    Model() = default;

    Model(const std::filesystem::path& filename, std::shared_ptr<ShaderProgram> shader) {
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;

        if (loadOBJ(filename, vertices, indices)) {
            auto loaded_mesh = std::make_shared<Mesh>(vertices, indices, GL_TRIANGLES);
            addMesh(loaded_mesh, shader);
        }
        else {
            std::cerr << "Chyba: Nepodarilo se nacist model " << filename << std::endl;
        }
    }

    void addMesh(std::shared_ptr<Mesh> mesh,
        std::shared_ptr<ShaderProgram> shader,
        glm::vec3 origin = glm::vec3(0.0f),
        glm::vec3 eulerAngles = glm::vec3(0.0f),
        glm::vec3 scale = glm::vec3(1.0f)) {

        meshes.push_back({ mesh, shader, origin, eulerAngles, scale });
    }

    void update(const float delta_t) {
    }

    void draw() {
        for (auto const& mesh_pkg : meshes) {
            mesh_pkg.shader->use();
            mesh_pkg.mesh->draw();
        }
    }
};