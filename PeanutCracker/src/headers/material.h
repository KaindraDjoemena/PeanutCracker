#pragma once

#include <glm/glm.hpp>
#include <string>
#include <memory>
#include "shader.h"
#include "texture.h"


struct Material {
	std::string name;

	std::shared_ptr<Texture> albedo;
	std::shared_ptr<Texture> metallic;
	std::shared_ptr<Texture> roughness;
	std::shared_ptr<Texture> ao;


};