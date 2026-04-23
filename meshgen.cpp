#include "meshgen.hpp"
#include <cmath>
#include <opencv2/opencv.hpp>
#include <algorithm>
#include <iostream>
#include <filesystem>

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



glm::vec2 get_subtex_st(const int x, const int y) {
    return glm::vec2(x * 1.0f / 16.0f, y * 1.0f / 16.0f);
}

// Přiřazení textury podle výšky (normalizované 0.0 - 1.0)
glm::vec2 get_subtex_by_height(float height) {
    if (height > 0.9f) return get_subtex_st(2, 11); // snow
    else if (height > 0.8f) return get_subtex_st(3, 11); // ice
    else if (height > 0.5f) return get_subtex_st(0, 14); // rock
    else if (height > 0.3f) return get_subtex_st(2, 15); // soil
    else return get_subtex_st(0, 11); // grass
}

// Hlavní generátor
std::shared_ptr<Mesh> GenHeightMap(const std::filesystem::path& hm_file, const unsigned int mesh_step_size) {
    // Načtení obrázku jako černobílý (GRAYSCALE)
    cv::Mat hmap = cv::imread(hm_file.string(), cv::IMREAD_GRAYSCALE);
    if (hmap.empty()) {
        throw std::runtime_error("ERR: Height map empty? File: " + hm_file.string());
    }

    std::cout << "Note: heightmap size: " << hmap.cols << "x" << hmap.rows << ", channels: " << hmap.channels() << std::endl;

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    for (unsigned int x_coord = 0; x_coord < (hmap.cols - mesh_step_size); x_coord += mesh_step_size) {
        for (unsigned int z_coord = 0; z_coord < (hmap.rows - mesh_step_size); z_coord += mesh_step_size) {

            // Získání Y pozice a roztažení hor (dělíme třeba 10.0f, ať nejsou hory moc špičaté)
            float y0 = hmap.at<uchar>(cv::Point(x_coord, z_coord)) / 10.0f;
            float y1 = hmap.at<uchar>(cv::Point(x_coord + mesh_step_size, z_coord)) / 10.0f;
            float y2 = hmap.at<uchar>(cv::Point(x_coord + mesh_step_size, z_coord + mesh_step_size)) / 10.0f;
            float y3 = hmap.at<uchar>(cv::Point(x_coord, z_coord + mesh_step_size)) / 10.0f;

            glm::vec3 p0(x_coord, y0, z_coord);
            glm::vec3 p1(x_coord + mesh_step_size, y1, z_coord);
            glm::vec3 p2(x_coord + mesh_step_size, y2, z_coord + mesh_step_size);
            glm::vec3 p3(x_coord, y3, z_coord + mesh_step_size);

            // Výpočet textury podle nejvyššího bodu dlaždice
            float max_h = std::max({ y0, y1, y2, y3 }) / (255.0f / 10.0f); // normalizace zpet na 0-1

            glm::vec2 tc0 = get_subtex_by_height(max_h);
            glm::vec2 tc1 = tc0 + glm::vec2(1.0f / 16.0f, 0.0f);
            glm::vec2 tc2 = tc0 + glm::vec2(1.0f / 16.0f, 1.0f / 16.0f);
            glm::vec2 tc3 = tc0 + glm::vec2(0.0f, 1.0f / 16.0f);

            // OPRAVA CHYBĚJÍCÍCH ZÁVOREK: Výpočet normál
            glm::vec3 n1 = glm::normalize(glm::cross(p1 - p0, p2 - p0));
            glm::vec3 n2 = glm::normalize(glm::cross(p2 - p0, p3 - p0));
            glm::vec3 navg = glm::normalize(n1 + n2);

            // OPRAVA INDEXŮ: Zjistíme, na jakém indexu zrovna jsme
            GLuint startIndex = vertices.size();

            // Přidáme vrcholy do pole
            vertices.push_back(Vertex{ p0, navg, tc0 });
            vertices.push_back(Vertex{ p1, n1,   tc1 });
            vertices.push_back(Vertex{ p2, navg, tc2 });
            vertices.push_back(Vertex{ p3, n2,   tc3 });

            // Správné přiřazení indexů pro dva trojúhelníky
            indices.push_back(startIndex + 0);
            indices.push_back(startIndex + 2);
            indices.push_back(startIndex + 1);
            indices.push_back(startIndex + 0);
            indices.push_back(startIndex + 3);
            indices.push_back(startIndex + 2);
        }
    }

    std::cout << "Note: height map vertices generated: " << vertices.size() << std::endl;

    // Vrátíme hotový Mesh s GL_TRIANGLES
    return std::make_shared<Mesh>(vertices, indices, GL_TRIANGLES);
}