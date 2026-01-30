#pragma once

#include <glm/glm.hpp>
#include "shader.h"
#include "shadowCasterComponent.h"


struct Light {
	glm::vec3	ambient;
	glm::vec3	diffuse;
	glm::vec3	specular;

	Light()
		: ambient(glm::vec3(0.05f)),
		  diffuse(glm::vec3(1.0f)),
		  specular(glm::vec3(0.5f)) {}

	Light(const glm::vec3& i_ambient,
		  const glm::vec3& i_diffuse,
		  const glm::vec3& i_specular
	) : ambient(i_ambient),
		diffuse(i_diffuse),
		specular(i_specular) {}

	Light(const Light& other) = default;
};

struct Attenuation {
	float	constant;
	float	linear;
	float	quadratic;

	Attenuation()
		: constant(1.0f),
		linear(0.09f),
		quadratic(0.032f) {}

	Attenuation(float i_constant,
				float i_linear,
				float i_quadratic
	) : constant(i_constant),
		linear(i_linear),
		quadratic(i_quadratic) {}

	Attenuation(const Attenuation& other) = default;
};


class BaseLight {
public:
	virtual ~BaseLight() = default;
	virtual void setUniforms(const Shader& shader, const std::string& uniformName) const = 0;
	virtual std::string getUniformPrefix() const = 0;

	void setPositionUniform(const Shader& shader, const std::string& uniformName, const glm::vec3& position) const;

	void setDirectionUniform(const Shader& shader, const std::string& uniformName, const glm::vec3& direction) const;

	void setLightUniform(const Shader& shader, const std::string& uniformName, const Light& light) const;

	void setAttenuationUniform(const Shader& shader, const std::string& uniformName, const Attenuation& attenuation) const;

	void setCutoffValuesUniform(const Shader& shader, const std::string& uniformName, float inCosCutoff, float outCosCutoff) const;
};

class DirectionalLight : public BaseLight {
public:
	glm::vec3 direction;	// O(0, 0, 0) - direction
	Light	  light;

	std::unique_ptr<ShadowCasterComponent> shadowCasterComponent;

	DirectionalLight(
		const glm::vec3& i_direction,	// Direction
		const Light& i_light			// Light
	);

	std::string getUniformPrefix() const override;

	void setUniforms(const Shader& shader, const std::string& uniformName) const override;
};

class PointLight : public BaseLight {
public:
	glm::vec3	position;
	Light		light;
	Attenuation attenuation;

	std::unique_ptr<ShadowCasterComponent> shadowCasterComponent;

	PointLight(
		const glm::vec3& i_position,		// Position
		const Light& i_light,				// Light
		const Attenuation& i_attenuation	// Attenuation
	);

	std::string getUniformPrefix() const override;

	void setUniforms(const Shader& shader, const std::string& uniformName) const override;
};

class SpotLight : public BaseLight {
public:
	glm::vec3	position;
	glm::vec3	direction;
	Light		light;
	Attenuation attenuation;
	float		inCosCutoff;
	float		outCosCutoff;

	std::unique_ptr<ShadowCasterComponent> shadowCasterComponent;

	SpotLight(
		const glm::vec3& i_position,		// Position
		const glm::vec3&	i_direction,	// Direction
		const Light&		i_light,		// Light
		const Attenuation&	i_attenuation,	// Attenuation
		float				i_inCosCutoff,
		float				i_outCosCutoff
	);

	std::string getUniformPrefix() const override;

	void setUniforms(const Shader& shader, const std::string& uniformName) const override;
};