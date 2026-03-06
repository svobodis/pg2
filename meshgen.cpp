#include "meshgen.hpp"
#include <cmath>

std::shared_ptr<Mesh> generateCube() {
    std::vector<Vertex> V{
        {{1, 1, 0}, {0,0,1}, {0,0}}, // [00]
        {{0, 1, 0}, {0,0,1}, {0,0}}, // [01]
        {{1, 1, 1}, {0,0,1}, {0,0}}, // [02]
        {{0, 1, 1}, {0,0,1}, {0,0}}, // [03]
        {{1, 0, 0}, {0,0,1}, {0,0}}, // [04]
        {{0, 0, 0}, {0,0,1}, {0,0}}, // [05]
        {{0, 0, 1}, {0,0,1}, {0,0}}, // [06]
        {{1, 0, 1}, {0,0,1}, {0,0}}, // [07]
    };

    std::vector<GLuint> I{ 0, 1, 4, 5, 6, 1, 3, 0, 2, 4, 7, 6, 2, 3 };

    return std::make_shared<Mesh>(V, I, GL_TRIANGLE_STRIP);
}

std::shared_ptr<Mesh> generateSphere(unsigned int sectors, unsigned int rings) {
    std::vector<Vertex> V{};
    std::vector<GLuint> I{};

    unsigned int totalSectors = sectors + 1;

    for (unsigned int r = 0; r <= rings; ++r) {
        float const R = (float)r / (float)rings;
        float const phi = -glm::pi<float>() / 2.0f + glm::pi<float>() * R;
        float const y = std::sin(phi);
        float const xz_radius = std::cos(phi);

        for (unsigned int s = 0; s <= sectors; ++s) {
            float const S = (float)s / (float)sectors;
            float const theta = 2.0f * glm::pi<float>() * S;
            float const x = std::cos(theta) * xz_radius;
            float const z = std::sin(theta) * xz_radius;

            glm::vec3 pos(x, y, z);
            glm::vec3 normal = glm::normalize(pos);
            glm::vec2 tex(S, R);

            V.push_back({ pos, normal, tex });
        }
    }

    for (unsigned int r = 0; r < rings; ++r) {
        if (r > 0) {
            I.push_back(r * totalSectors + sectors);
            I.push_back(r * totalSectors);
        }
        for (unsigned int s = 0; s <= sectors; ++s) {
            I.push_back(r * totalSectors + s);
            I.push_back((r + 1) * totalSectors + s);
        }
    }

    return std::make_shared<Mesh>(V, I, GL_TRIANGLE_STRIP);
}