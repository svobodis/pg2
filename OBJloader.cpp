#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <GL/glew.h> 
#include <glm/glm.hpp>

#include "OBJloader.hpp"

bool loadOBJ(const std::filesystem::path& filename, std::vector<Vertex>& out_vertices, std::vector<GLuint>& out_indices)
{
    std::cout << "Nacitam model: " << filename.string() << std::endl;

    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec2> temp_uvs;
    std::vector<glm::vec3> temp_normals;

    out_vertices.clear();
    out_indices.clear();

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Chyba: Soubor se nepodarilo otevrit!" << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string lineHeader;
        iss >> lineHeader;

        if (lineHeader == "v") {
            glm::vec3 vertex;
            iss >> vertex.x >> vertex.y >> vertex.z;
            temp_vertices.push_back(vertex);
        }
        else if (lineHeader == "vt") {
            glm::vec2 uv;
            iss >> uv.x >> uv.y;
            temp_uvs.push_back(uv);
        }
        else if (lineHeader == "vn") {
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            temp_normals.push_back(normal);
        }
        else if (lineHeader == "f") {
            std::vector<Vertex> faceVertices;
            std::string vertexData;

            while (iss >> vertexData) {
                int vIdx = 0, uvIdx = 0, nIdx = 0;

                size_t firstSlash = vertexData.find('/');
                size_t secondSlash = vertexData.find('/', firstSlash + 1);

                if (firstSlash == std::string::npos) {
                    vIdx = std::stoi(vertexData); 
                }
                else if (secondSlash == std::string::npos) {
                    vIdx = std::stoi(vertexData.substr(0, firstSlash)); 
                    uvIdx = std::stoi(vertexData.substr(firstSlash + 1));
                }
                else {
                    vIdx = std::stoi(vertexData.substr(0, firstSlash)); 
                    if (secondSlash > firstSlash + 1) {
                        uvIdx = std::stoi(vertexData.substr(firstSlash + 1, secondSlash - firstSlash - 1));
                    }
                    nIdx = std::stoi(vertexData.substr(secondSlash + 1));
                }

                Vertex currentVertex;
                currentVertex.position = temp_vertices[vIdx - 1]; 

                if (uvIdx > 0) {
                    currentVertex.texCoords = temp_uvs[uvIdx - 1];
                }
                else {
                    currentVertex.texCoords = glm::vec2(0.0f, 0.0f);
                }

                if (nIdx > 0) {
                    currentVertex.normal = temp_normals[nIdx - 1];
                }
                else {
                    currentVertex.normal = glm::vec3(0.0f);
                }

                faceVertices.push_back(currentVertex);
            }

            if (faceVertices.size() >= 3 && faceVertices[0].normal == glm::vec3(0.0f)) {
                glm::vec3 edge1 = faceVertices[1].position - faceVertices[0].position;
                glm::vec3 edge2 = faceVertices[2].position - faceVertices[0].position;
                glm::vec3 calculatedNormal = glm::normalize(glm::cross(edge1, edge2));

                for (auto& v : faceVertices) {
                    v.normal = calculatedNormal;
                }
            }

            for (size_t i = 1; i < faceVertices.size() - 1; ++i) {
                Vertex tri[3] = { faceVertices[0], faceVertices[i], faceVertices[i + 1] };

                for (int j = 0; j < 3; j++) {
                    auto t = std::find(out_vertices.begin(), out_vertices.end(), tri[j]);
                    if (t == out_vertices.end()) {
                        out_vertices.push_back(tri[j]);
                        out_indices.push_back(out_vertices.size() - 1);
                    }
                    else {
                        out_indices.push_back(std::distance(out_vertices.begin(), t));
                    }
                }
            }
        }
    }

    std::cout << "Model nacten a zpracovan: " << filename.string() << std::endl;
    return true;
}