#pragma once

#include <glm/glm.hpp>
#include "shadowCasterComponent.h"


struct Light {
	glm::vec3	color = glm::vec3(1.0f);
	float		power = 10.0f;

	Light() = default;

	Light(const glm::vec3& i_color, float i_power) : color(i_color), power(i_power) {}

	Light(const Light& other) = default;
};


struct DirectionalLight {
	glm::vec3 direction;	// O(0, 0, 0) - direction
	Light light;
	float shadowDist;
	ShadowCasterComponent shadowCasterComponent;

	DirectionalLight()
		: DirectionalLight(glm::vec3(0.0f, -1.0f, 0.0f), Light(), 20.0f)
	{
	}

	DirectionalLight(
		const glm::vec3& i_direction,
		const Light&     i_light,
		float            i_shadowDist)
		: direction(i_direction)
		, light(i_light)
		, shadowDist(i_shadowDist)
	{
		shadowCasterComponent = ShadowCasterComponent(1024, Shadow_Map_Projection::ORTHOGRAPHIC, i_shadowDist, 0.01f, i_shadowDist);
	};
};


struct PointLight {
	glm::vec3 position;
	Light light;
	float radius;
	ShadowCasterComponent shadowCasterComponent;

	PointLight()
		: PointLight(glm::vec3(0.0f, 0.0f, 0.0f), Light(), 20.0f)
	{
	}

	PointLight(
		const glm::vec3& i_position,
		const Light&     i_light,
		float            i_radius)
		: position(i_position)
		, light(i_light)
		, radius(i_radius)
	{
		shadowCasterComponent = ShadowCasterComponent(true, 1024, Shadow_Map_Projection::PERSPECTIVE, 90.0f, i_radius, 0.01f, i_radius);
	};
};


// SOMETHING WRONG WITH THIS
struct SpotLight {
	glm::vec3 position;
	glm::vec3 direction;
	Light light;
	float radius;
	float inCosCutoff;
	float outCosCutoff;
	ShadowCasterComponent shadowCasterComponent;

	SpotLight()
		: SpotLight(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), Light(), 20.0f, cosf(glm::radians(10.0f)), cosf(glm::radians(12.5f)))
	{
	}

	SpotLight(
		const glm::vec3& i_position,
		const glm::vec3& i_direction,
		const Light&     i_light,
		float            i_radius,
		float			 i_inCosCutoff,  // Cosine value
		float			 i_outCosCutoff) // Cosine value
		: position(i_position)
		, direction(i_direction)
		, light(i_light)
		, radius(i_radius)
		, inCosCutoff(i_inCosCutoff)
		, outCosCutoff(i_outCosCutoff)
	{
		shadowCasterComponent = ShadowCasterComponent(false, 1024, Shadow_Map_Projection::PERSPECTIVE, glm::degrees(acos(glm::clamp(i_outCosCutoff, -1.0f, 1.0f)) * 2.0f) + 2.0f, i_radius, 0.01f, i_radius);
	};
};