#pragma once

#include <glm/glm.hpp>
#include "shadowCasterComponent.h"


struct Light {
    glm::vec3 color      = glm::vec3(1.0f);
    float	  power      = 10.0f;
    float     normalBias = 0.0005f;
    float     depthBias  = 0.00005f;

    Light() = default;

    Light(const glm::vec3& i_color, float i_power) : color(i_color), power(i_power) {}

    Light(const Light& other) = default;
};


struct DirectionalLight {
    glm::vec3 position;
    glm::vec3 direction;
    Light light;
    float range;
    bool  isVisible = true;
    ShadowCasterComponent shadowCasterComponent;

    DirectionalLight()
        : DirectionalLight(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), Light(), 200.0f)
    {
    }

    DirectionalLight(
        const glm::vec3& i_position,
        const glm::vec3& i_direction,
        const Light&     i_light,
        float            i_range)
        : position(i_position)
        , direction(i_direction)
        , light(i_light)
        , range(i_range)
        , shadowCasterComponent(1024, Shadow_Map_Projection::ORTHOGRAPHIC, i_range, 0.01f, i_range * 2.0f)
    {
    }
};


struct PointLight {
    glm::vec3 position;
    Light light;
    float radius;
    bool  isVisible = true;
    ShadowCasterComponent shadowCasterComponent;

    PointLight()
        : PointLight(glm::vec3(0.0f, 0.0f, 0.0f), Light(), 10.0f)
    {
    }

    PointLight(
        const glm::vec3& i_position,
        const Light&     i_light,
        float            i_radius)
        : position(i_position)
        , light(i_light)
        , radius(i_radius)
        , shadowCasterComponent(true, 1024, Shadow_Map_Projection::PERSPECTIVE, 90.0f, i_radius, 0.01f, i_radius)
    {
    }
};


struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    Light light;
    float range;
    float inCosCutoff;
    float outCosCutoff;
    bool  isVisible = true;
    ShadowCasterComponent shadowCasterComponent;

    SpotLight()
        : SpotLight(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), Light(), 10.0f, cosf(glm::radians(10.0f)), cosf(glm::radians(12.5f)))
    {
    }

    SpotLight(
        const glm::vec3& i_position,
        const glm::vec3& i_direction,
        const Light&     i_light,
        float            i_range,
        float			 i_inCosCutoff,  // Cosine value
        float			 i_outCosCutoff) // Cosine value
        : position(i_position)
        , direction(i_direction)
        , light(i_light)
        , range(i_range)
        , inCosCutoff(i_inCosCutoff)
        , outCosCutoff(i_outCosCutoff)
        , shadowCasterComponent(false, 1024, Shadow_Map_Projection::PERSPECTIVE, glm::degrees(acos(glm::clamp(i_outCosCutoff, -1.0f, 1.0f)) * 2.0f) + 2.0f, i_range, 0.01f, i_range)
        // NOTE: Current Implementation -> shadowCasterComponent.size == range
    {
    }
};