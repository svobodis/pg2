#pragma once

#include <memory>
#include "Mesh.hpp"

std::shared_ptr<Mesh> generateCube();
std::shared_ptr<Mesh> generateSphere(unsigned int sectors = 36, unsigned int rings = 18);