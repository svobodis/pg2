#pragma once

#include <memory>
#include <filesystem>

#include "Mesh.hpp" 

std::shared_ptr<Mesh> generateCube();
std::shared_ptr<Mesh> generateSphere(unsigned int rings, unsigned int sectors);

std::shared_ptr<Mesh> GenHeightMap(const std::filesystem::path& hm_file, const unsigned int mesh_step_size);