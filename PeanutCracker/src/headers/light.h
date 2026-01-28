#ifndef LIGHT_H
#define LIGHT_H

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

	void setPositionUniform(const Shader& shader, const std::string& uniformName, const glm::vec3& position) const {
		shader.setVec3(uniformName + ".position", position);
	}

	void setDirectionUniform(const Shader& shader, const std::string& uniformName, const glm::vec3& direction) const {
		shader.setVec3(uniformName + ".direction", direction);
	}

	void setLightUniform(const Shader& shader, const std::string& uniformName, const Light& light) const {
		shader.setVec3(uniformName + ".light.ambient", light.ambient);
		shader.setVec3(uniformName + ".light.diffuse", light.diffuse);
		shader.setVec3(uniformName + ".light.specular", light.specular);
	}

	void setAttenuationUniform(const Shader& shader, const std::string& uniformName, const Attenuation& attenuation) const {
		shader.setFloat(uniformName + ".attenuation.constant", attenuation.constant);
		shader.setFloat(uniformName + ".attenuation.linear", attenuation.linear);
		shader.setFloat(uniformName + ".attenuation.quadratic", attenuation.quadratic);
	}

	void setCutoffValuesUniform(const Shader& shader, const std::string& uniformName, float innerCutoff, float outerCutoff) const {
		shader.setFloat(uniformName + ".innerCutoff", innerCutoff);
		shader.setFloat(uniformName + ".outerCutoff", outerCutoff);
	}
};

class DirectionalLight : public BaseLight {
public:
	glm::vec3 direction;	// O(0, 0, 0) - direction
	Light	  light;

	std::unique_ptr<ShadowCasterComponent> shadowCasterComponent;

	DirectionalLight(const glm::vec3& i_direction,	// Direction
		const Light& i_light						// Light
	) : direction(i_direction),
		light(i_light) {
		shadowCasterComponent = std::make_unique<ShadowCasterComponent>(glm::vec2(2048, 2048), Shadow_Map_Projection::ORTHOGRAPHIC);
	}

	std::string getUniformPrefix() const override { return "directionalLight";}

	void setUniforms(const Shader& shader, const std::string& uniformName) const override {
		setDirectionUniform(shader, uniformName, direction);
		setLightUniform(shader, uniformName, light);
	}
};

class PointLight : public BaseLight {
public:
	glm::vec3	position;
	Light		light;
	Attenuation attenuation;

	std::unique_ptr<ShadowCasterComponent> shadowCasterComponent;

	PointLight(const glm::vec3& i_position, // Position
		const Light& i_light,				// Light
		const Attenuation& i_attenuation	// Attenuation
	) : position(i_position),
		light(i_light),
		attenuation(i_attenuation) {
		shadowCasterComponent = std::make_unique<ShadowCasterComponent>(glm::vec2(2048, 2048), Shadow_Map_Projection::PERSPECTIVE);
	}

	std::string getUniformPrefix() const override { return "pointLight"; }

	void setUniforms(const Shader& shader, const std::string& uniformName) const override {
		setPositionUniform(shader, uniformName, position);
		setLightUniform(shader, uniformName, light);
		setAttenuationUniform(shader, uniformName, attenuation);
	}
};

class SpotLight : public BaseLight {
public:
	glm::vec3	position;
	glm::vec3	direction;
	Light		light;
	Attenuation attenuation;
	float		innerCutoff;
	float		outerCutoff;

	std::unique_ptr<ShadowCasterComponent> shadowCasterComponent;

	SpotLight(const glm::vec3& i_position,	// Position
		const glm::vec3& i_direction,		// Direction
		const Light& i_light,				// Light
		const Attenuation& i_attenuation,	// Attenuation
		float					i_innerCutoff,
		float					i_outerCutoff
	) : position(i_position),
		direction(i_direction),
		light(i_light),
		attenuation(i_attenuation),
		innerCutoff(i_innerCutoff),
		outerCutoff(i_outerCutoff) {
		shadowCasterComponent = std::make_unique<ShadowCasterComponent>(glm::vec2(2048, 2048), Shadow_Map_Projection::PERSPECTIVE);
	}

	std::string getUniformPrefix() const override { return "spotLight"; }

	void setUniforms(const Shader& shader, const std::string& uniformName) const override {
		setPositionUniform(shader, uniformName, position);
		setDirectionUniform(shader, uniformName, direction);
		setLightUniform(shader, uniformName, light);
		setAttenuationUniform(shader, uniformName, attenuation);
		setCutoffValuesUniform(shader, uniformName, innerCutoff, outerCutoff);
	}
};


#endif