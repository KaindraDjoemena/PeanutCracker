#pragma once

#include <glm/glm.hpp>
#include "shader.h"
#include "texture.h"
#include <string>
#include <memory>


struct Material {
	std::string name;

	std::shared_ptr<Texture> albedo;
	Texture metallic;
	Texture roughness;
	Texture ao;
};