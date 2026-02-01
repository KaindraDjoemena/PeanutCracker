#version 330 core
out vec4 FragColor;

#define MAX_LIGHTS 8

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 DirectionalLightSpacePos[MAX_LIGHTS];
in vec4 SpotLightSpacePos[MAX_LIGHTS];

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    float shininess;
};

// === LIGHT STRUCTS ========================================================
struct DirectionalLightStruct {
    vec4 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
}; 

struct PointLightStruct {
    vec4 position;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    float constant;
    float linear;
    float quadratic;
    float _padding;
};

struct SpotLightStruct {
    vec4 position;
    vec4 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    float constant;
    float linear;
    float quadratic;
    float inCosCutoff;
    float outCosCutoff;
    float _padding0;
    float _padding1;
    float _padding2;
};

uniform Material material;
uniform sampler2DShadow DirectionalShadowMap[MAX_LIGHTS];
uniform samplerCube     PointShadowMap[MAX_LIGHTS];
uniform float           omniShadowMapFarPlanes[MAX_LIGHTS];
uniform sampler2DShadow SpotShadowMap[MAX_LIGHTS];

layout (std140) uniform CameraMatricesUBOData {
    mat4 projection;
    mat4 view;
    vec4 cameraPos;
};

layout (std140) uniform LightingUBOData {
    DirectionalLightStruct directionalLight[MAX_LIGHTS];
    PointLightStruct       pointLight[MAX_LIGHTS];
    SpotLightStruct        spotLight[MAX_LIGHTS];
    int numDirectionalLights;
    int numPointLights;
    int numSpotLights;
    int padding;
} lightingBlock;


// FUNCTION PROTOTYPES (Standardized to use 'light' as parameter name)
vec3 calcAmbient(vec3 ambientColor);
vec3 calcDiffuse(vec3 lightDir, vec3 normal, vec3 diffuseColor);
vec3 calcSpecular(vec3 lightDir, vec3 normal, vec3 viewDir, vec3 specularColor);
float calcDirShadow(bool isLocalLight, vec4 fragPosLightSpace, sampler2DShadow shadowMap, vec3 normal, vec3 lightDir);
float calcOmniShadow(vec3 lightPos, samplerCube shadowMap, float farPlane, vec3 normal);

vec3 calcDirectionalLight(DirectionalLightStruct light, vec3 normal, vec3 viewDir, int lightIndex);
vec3 calcPointLight(PointLightStruct light, vec3 normal, vec3 viewDir, int lightIndex);
vec3 calcSpotLight(SpotLightStruct light, vec3 normal, vec3 viewDir, int lightIndex);


/* ======================================================== MAIN === */
void main() {
    vec3 viewDir = normalize(cameraPos.xyz - FragPos);
    vec3 norm = normalize(Normal);
    vec3 result = vec3(0.0f);

    for (int i = 0; i < lightingBlock.numDirectionalLights; ++i) {
        result += calcDirectionalLight(lightingBlock.directionalLight[i], norm, viewDir, i);
    }
    for (int i = 0; i < lightingBlock.numPointLights; ++i) {
        result += calcPointLight(lightingBlock.pointLight[i], norm, viewDir, i);
    }
    for (int i = 0; i < lightingBlock.numSpotLights; ++i) {
        result += calcSpotLight(lightingBlock.spotLight[i], norm, viewDir, i);
    }
    FragColor = vec4(result, 1.0f);
}

/* ===================== LIGHTING FUNCTIONS ===================== */
vec3 calcDirectionalLight(DirectionalLightStruct light, vec3 normal, vec3 viewDir, int lightIndex) {
    vec3 lightDir = normalize(-light.direction.xyz);

    vec3 ambient  = calcAmbient(light.ambient.xyz);
    vec3 diffuse  = calcDiffuse(lightDir, normal, light.diffuse.xyz);
    vec3 specular = calcSpecular(lightDir, normal, viewDir, light.specular.xyz);
    
    float shadowFactor = calcDirShadow(false, DirectionalLightSpacePos[lightIndex], DirectionalShadowMap[lightIndex], normal, lightDir);
    
    return ambient + (diffuse + specular) * (1.0f - shadowFactor);
}
vec3 calcPointLight(PointLightStruct light, vec3 normal, vec3 viewDir, int lightIndex) {
    vec3 lightDir = normalize(light.position.xyz - FragPos);

    vec3 ambient  = calcAmbient(light.ambient.xyz);
    vec3 diffuse  = calcDiffuse(lightDir, normal, light.diffuse.xyz);
    vec3 specular = calcSpecular(lightDir, normal, viewDir, light.specular.xyz);

    float dist = length(light.position.xyz - FragPos);
    float attenuation = 1.0f / (light.constant + light.linear * dist + light.quadratic * (dist * dist));

    float shadowFactor;
    switch (lightIndex) {
        case 0: shadowFactor = calcOmniShadow(light.position.xyz, PointShadowMap[0], omniShadowMapFarPlanes[0], normal); break;
        case 1: shadowFactor = calcOmniShadow(light.position.xyz, PointShadowMap[1], omniShadowMapFarPlanes[1], normal); break;
        case 2: shadowFactor = calcOmniShadow(light.position.xyz, PointShadowMap[2], omniShadowMapFarPlanes[2], normal); break;
        case 3: shadowFactor = calcOmniShadow(light.position.xyz, PointShadowMap[3], omniShadowMapFarPlanes[3], normal); break;
        case 4: shadowFactor = calcOmniShadow(light.position.xyz, PointShadowMap[4], omniShadowMapFarPlanes[4], normal); break;
        case 5: shadowFactor = calcOmniShadow(light.position.xyz, PointShadowMap[5], omniShadowMapFarPlanes[5], normal); break;
    }
    
    return (ambient + (diffuse + specular) * (1.0f - shadowFactor)) * attenuation;
}

vec3 calcSpotLight(SpotLightStruct light, vec3 normal, vec3 viewDir, int lightIndex) {
    vec3 lightDir = normalize(light.position.xyz - FragPos);

    float theta     = dot(lightDir, normalize(-light.direction.xyz));
    float epsilon   = light.inCosCutoff - light.outCosCutoff;
    float intensity = clamp((theta - light.outCosCutoff) / epsilon, 0.0f, 1.0f);

    vec3 ambient  = calcAmbient(light.ambient.xyz);
    vec3 diffuse  = calcDiffuse(lightDir, normal, light.diffuse.xyz);
    vec3 specular = calcSpecular(lightDir, normal, viewDir, light.specular.xyz);

    float dist = length(light.position.xyz - FragPos);
    float attenuation = 1.0f / (light.constant + light.linear * dist + light.quadratic * (dist * dist));

    float shadowFactor = calcDirShadow(true, SpotLightSpacePos[lightIndex], SpotShadowMap[lightIndex], normal, lightDir);
    
    return (ambient + (diffuse + specular) * (1.0f - shadowFactor)) * attenuation * intensity;
}


/* ======================================= HELPER FUNCTIONS === */
vec3 calcAmbient(vec3 ambientColor) {
    return ambientColor * vec3(texture(material.texture_diffuse1, TexCoord));
}

vec3 calcDiffuse(vec3 lightDir, vec3 normal, vec3 diffuseColor) {
    float diff = max(dot(normal, lightDir), 0.0f);
    return diffuseColor * diff * vec3(texture(material.texture_diffuse1, TexCoord));
}

vec3 calcSpecular(vec3 lightDir, vec3 normal, vec3 viewDir, vec3 specularColor) {
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0f), material.shininess);
    return specularColor * spec * vec3(texture(material.texture_specular1, TexCoord));
}


float calcDirShadow(bool isLocalLight, vec4 fragPosLightSpace, sampler2DShadow shadowMap, vec3 normal, vec3 lightDir) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5f + 0.5f;
    
    // Outside bounds: Local lights=shadowed, Directional=lit
    if(projCoords.x < 0.0f || projCoords.x > 1.0f ||
       projCoords.y < 0.0f || projCoords.y > 1.0f) {
        return isLocalLight ? 1.0f : 0.0f;
    }

    if (projCoords.z > 1.0f) {
        return isLocalLight ? 1.0f : 0.0f;
    }
    
    if (projCoords.z < 0.0f) {
        return isLocalLight ? 1.0f : 0.0f;
    }
    
    // Bias
    float cosTheta = clamp(dot(normal, lightDir), 0.0f, 1.0f);
    float bias = max(0.005f * tan(acos(cosTheta)), 0.001f); 
    bias = clamp(bias, 0.0001f, 0.01f);

    // 3x3 PCF
    float shadow = 0.0f;
    vec2 texelSize = 1.0f / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            vec3 coord = vec3(projCoords.xy + vec2(x,y) * texelSize, projCoords.z - bias);
            shadow += texture(shadowMap, coord);
        }
    }

    return 1.0f - (shadow / 9.0f);
}

float calcOmniShadow(vec3 lightPos, samplerCube shadowMap, float farPlane, vec3 normal) {
    vec3 fragToLight = FragPos - lightPos;
    float currentDepth = length(fragToLight);
    vec3 sampleDir = normalize(fragToLight);
    
    // Bias based on surface angle
    vec3 lightDir = normalize(lightPos - FragPos);
    float cosTheta = max(dot(normal, lightDir), 0.0f);
    float bias = max(0.005f * (1.0f - cosTheta), 0.001f);
    
    // PCF
    vec3 gridSamplingDisk[20] = vec3[](
       vec3(1, 1, 1), vec3(1, -1, 1), vec3(-1, -1, 1), vec3(-1, 1, 1), 
       vec3(1, 1, -1), vec3(1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
       vec3(1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
       vec3(1, 0, 1), vec3(-1, 0, 1), vec3(1, 0, -1), vec3(-1, 0, -1),
       vec3(0, 1, 1), vec3(0, -1, 1), vec3(0, -1, -1), vec3(0, 1, -1)
    );
    float shadow = 0.0f;
    int samples = 20;
    float diskRadius = (1.0f + (currentDepth / farPlane)) / 25.0f; 
    for(int i = 0; i < samples; ++i) {
        float closestDepth = texture(shadowMap, sampleDir + gridSamplingDisk[i] * diskRadius).r;
        closestDepth *= farPlane;
        
        if(currentDepth - bias > closestDepth) {
            shadow += 1.0f;
        }
    }

    return shadow / float(samples);
}