#pragma once

#include "transform.h"
#include "cubemap.h"

struct RefProbe {
    Transform transform;
    Cubemap localEnvMap;

    glm::vec3 proxyDims = glm::vec3(30.0f);   // probe position is inside a 30x30x30 square
    float farPlane = 200.0f;
    bool toBeBaked = false;
    bool isVisible = true;

    RefProbe(
        const Shader& i_convolutionShader,
        const Shader& i_conversionShader,
        const Shader& i_prefilterShader)
        : localEnvMap(i_convolutionShader, i_conversionShader, i_prefilterShader)
    {
    }
};