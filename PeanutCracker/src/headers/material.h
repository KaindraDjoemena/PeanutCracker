#pragma once

#include <glm/glm.hpp>

#include <string>
#include <memory>

#include "shader.h"
#include "texture.h"


struct Material {
    std::string name;

    std::shared_ptr<Texture> albedoMap;
    std::shared_ptr<Texture> normalMap;
    std::shared_ptr<Texture> metallicMap;
    std::shared_ptr<Texture> roughnessMap;
    std::shared_ptr<Texture> aoMap;

    void bind(const Shader& shader) const {

        shader.use();

        if (albedoMap) {
            albedoMap->bind(MatTex::ALBEDO);
            shader.setInt("material.albedoMap", MatTex::ALBEDO);
        }
        if (normalMap) {
            normalMap->bind(MatTex::NORM);
            shader.setInt("material.normalMap", MatTex::NORM);
        }
        if (metallicMap) {
            metallicMap->bind(MatTex::METALLIC);
            shader.setInt("material.metallicMap", MatTex::ROUGHNESS);
        }
        if (roughnessMap) {
            roughnessMap->bind(MatTex::ROUGHNESS);
            shader.setInt("material.roughnessMap", MatTex::ROUGHNESS);
        }
        if (aoMap) {
            aoMap->bind(MatTex::AO);
            shader.setInt("material.aoMap", MatTex::AO);
        }
    }
};