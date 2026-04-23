#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <memory>
#include <cmath> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp> 

#include "assets.hpp"
#include "Mesh.hpp"
#include "ShaderProgram.hpp"
#include "Texture.hpp"

class Model {
public:
    glm::vec3 pivot_position{ 0.0f };
    glm::vec3 eulerAngles{ 0.0f };
    glm::vec3 scale{ 1.0f };

    glm::mat4 local_model_matrix{ 1.0f };

    struct mesh_package {
        std::shared_ptr<Mesh> mesh;
        std::shared_ptr<ShaderProgram> shader;
        std::shared_ptr<Texture> texture;

        glm::vec3 origin{ 0.0f };
        glm::vec3 eulerAngles{ 0.0f };
        glm::vec3 scale{ 1.0f };
    };
    std::vector<mesh_package> meshes;

private:
    glm::mat4 createMM(const glm::vec3& origin, const glm::vec3& eAng, const glm::vec3& obj_scale) {
        glm::vec3 eA{ wrapAngle(eAng.x), wrapAngle(eAng.y), wrapAngle(eAng.z) };

        glm::mat4 t = glm::translate(glm::mat4(1.0f), origin);
        glm::mat4 rotm = glm::yawPitchRoll(glm::radians(eA.y), glm::radians(eA.x), glm::radians(eA.z));
        glm::mat4 s = glm::scale(glm::mat4(1.0f), obj_scale);

        return t * rotm * s;
    }

    float wrapAngle(float angle) {
        angle = std::fmod(angle, 360.0f);
        if (angle < 0.0f) {
            angle += 360.0f;
        }
        return angle;
    }

public:
    Model() = default;

    bool is_transparent = false;

    glm::vec3 getPosition() const {
        return pivot_position;
    }

    void addMesh(std::shared_ptr<Mesh> mesh, std::shared_ptr<ShaderProgram> shader, std::shared_ptr<Texture> texture = nullptr) {
        meshes.push_back({ mesh, shader, texture, glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f) });
    }

    void setPosition(const glm::vec3& new_position) {
        pivot_position = new_position;
        local_model_matrix = createMM(pivot_position, eulerAngles, scale);
    }

    void setEulerAngles(const glm::vec3& new_eulerAngles) {
        eulerAngles = new_eulerAngles;
        local_model_matrix = createMM(pivot_position, eulerAngles, scale);
    }

    void setScale(const glm::vec3& new_scale) {
        scale = new_scale;
        local_model_matrix = createMM(pivot_position, eulerAngles, scale);
    }

    void setModelMatrix(const glm::mat4& modelm) {
        local_model_matrix = modelm;
    }

    void translate(const glm::vec3& offset) {
        pivot_position += offset;
        local_model_matrix = createMM(pivot_position, eulerAngles, scale);
    }

    void rotate(const glm::vec3& pitch_yaw_roll_offs) {
        eulerAngles += pitch_yaw_roll_offs;
        local_model_matrix = createMM(pivot_position, eulerAngles, scale);
    }

    void addScale(const glm::vec3& scale_offs) {
        scale *= scale_offs;
        local_model_matrix = createMM(pivot_position, eulerAngles, scale);
    }

    void update(float delta_t) {
    }

    void draw() {
        for (auto const& mesh_pkg : meshes) {
            mesh_pkg.shader->use();

            if (mesh_pkg.texture) {
                mesh_pkg.texture->bind();
                mesh_pkg.shader->setUniform("tex0", 0); 
            }

            glm::mat4 mesh_model_matrix = createMM(mesh_pkg.origin, mesh_pkg.eulerAngles, mesh_pkg.scale);
            glm::mat4 final_matrix = local_model_matrix * mesh_model_matrix;

            mesh_pkg.shader->setUniform("uM_m", final_matrix);

            mesh_pkg.mesh->draw();
        }
    }
}; 