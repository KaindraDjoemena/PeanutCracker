#ifndef MATERIAL_H
#define MATERIAL_H

#include <glm/glm.hpp>
#include "shader.h"
#include "texture.h"


struct Material {
	Texture diffuse;
	Texture specular;
	float shininess;

	Material(const char* i_diffusePath,
			 const char* i_specularPath,
			 float i_shininess
	) : diffuse(i_diffusePath, 0),
		specular(i_specularPath, 1),
		shininess(i_shininess) {}

	Material(const Texture& i_diffuseTex,
		const Texture& i_specularTex,
		float i_shininess
	) : diffuse(i_diffuseTex),
		specular(i_specularTex),
		shininess(i_shininess) {}

	void bind() {
		diffuse.bind();
		specular.bind();
	}
};

#endif