#include "headers/light.h"
#include "headers/shadowCasterComponent.h"
#include "headers/shader.h"

#include <glm/glm.hpp>


void BaseLight::setPositionUniform(const Shader& shader, const std::string& uniformName, const glm::vec3& position) const {
	shader.setVec3(uniformName + ".position", position);
}
void BaseLight::setDirectionUniform(const Shader& shader, const std::string& uniformName, const glm::vec3& direction) const {
	shader.setVec3(uniformName + ".direction", direction);
}
void BaseLight::setLightUniform(const Shader& shader, const std::string& uniformName, const Light& light) const {
	shader.setVec3(uniformName + ".light.ambient", light.ambient);
	shader.setVec3(uniformName + ".light.diffuse", light.diffuse);
	shader.setVec3(uniformName + ".light.specular", light.specular);
}
void BaseLight::setAttenuationUniform(const Shader& shader, const std::string& uniformName, const Attenuation& attenuation) const {
	shader.setFloat(uniformName + ".attenuation.constant", attenuation.constant);
	shader.setFloat(uniformName + ".attenuation.linear", attenuation.linear);
	shader.setFloat(uniformName + ".attenuation.quadratic", attenuation.quadratic);
}
void BaseLight::setCutoffValuesUniform(const Shader& shader, const std::string& uniformName, float inCosCutoff, float outCosCutoff) const {
	shader.setFloat(uniformName + ".inCosCutoff", inCosCutoff);
	shader.setFloat(uniformName + ".outCosCutoff", outCosCutoff);
}

// DIRECTIONAL LIGHT
DirectionalLight::DirectionalLight(
	const glm::vec3& i_direction,	// Direction
	const Light& i_light			// Light
) : direction(i_direction),
	light(i_light) {
	shadowCasterComponent = std::make_unique<ShadowCasterComponent>(glm::vec2(2048, 2048), Shadow_Map_Projection::ORTHOGRAPHIC, 50.f, 50.f);
}
std::string DirectionalLight::getUniformPrefix() const {
	return "directionalLight";
}
void DirectionalLight::setUniforms(const Shader& shader, const std::string& uniformName) const {
	setDirectionUniform(shader, uniformName, direction);
	setLightUniform(shader, uniformName, light);
}

// POINT LIGHT
PointLight::PointLight(
	const glm::vec3& i_position,		// Position
	const Light& i_light,				// Light
	const Attenuation& i_attenuation	// Attenuation
) : position(i_position),
	light(i_light),
	attenuation(i_attenuation) {
	shadowCasterComponent = std::make_unique<ShadowCasterComponent>(glm::vec2(2048, 2048), Shadow_Map_Projection::PERSPECTIVE, 1.0f, 50.0f, 50.0f);
}
std::string PointLight::getUniformPrefix() const {
	return "pointLight";
}
void PointLight::setUniforms(const Shader& shader, const std::string& uniformName) const {
	setPositionUniform(shader, uniformName, position);
	setLightUniform(shader, uniformName, light);
	setAttenuationUniform(shader, uniformName, attenuation);
}

// SPOT LIGHT
SpotLight::SpotLight(
	const glm::vec3& i_position,		// Position
	const glm::vec3&	i_direction,	// Direction
	const Light&		i_light,		// Light
	const Attenuation&	i_attenuation,	// Attenuation
	float				i_inCosCutoff,
	float				i_outCosCutoff
) : position(i_position),
	direction(i_direction),
	light(i_light),
	attenuation(i_attenuation),
	inCosCutoff(i_inCosCutoff),
	outCosCutoff(i_outCosCutoff) {
	shadowCasterComponent = std::make_unique<ShadowCasterComponent>(glm::vec2(2048, 2048), Shadow_Map_Projection::PERSPECTIVE, i_outCosCutoff, 50.0f, 50.0f);
}
std::string SpotLight::getUniformPrefix() const {
	return "spotLight";
}
void SpotLight::setUniforms(const Shader& shader, const std::string& uniformName) const {
	setPositionUniform(shader, uniformName, position);
	setDirectionUniform(shader, uniformName, direction);
	setLightUniform(shader, uniformName, light);
	setAttenuationUniform(shader, uniformName, attenuation);
	setCutoffValuesUniform(shader, uniformName, inCosCutoff, outCosCutoff);
}