#version 330 core
out vec4 FragColor;

#define MAX_LIGHTS 8

const float PI = 3.14159f;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
    vec4 DirectionalLightSpacePos[MAX_LIGHTS];
    vec4 SpotLightSpacePos[MAX_LIGHTS];

    mat3 TBN;
} fs_in;

struct Material {
    sampler2D albedoMap;
    sampler2D normalMap;
    sampler2D metallicMap;
    sampler2D roughnessMap;
    sampler2D aoMap;
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

// SHADOWS
uniform sampler2DShadow DirectionalShadowMap[MAX_LIGHTS];
uniform samplerCube     PointShadowMap[MAX_LIGHTS];
uniform float           omniShadowMapFarPlanes[MAX_LIGHTS];
uniform sampler2DShadow SpotShadowMap[MAX_LIGHTS];

// IBL
uniform samplerCube irradianceMap;

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
vec3  getNormal();
float distributionGGX(vec3 normal, vec3 halfVec, float roughness);
float geometryShlickGGX(float nDotv, float roughness);
float geometrySmithGGX(vec3 normal, vec3 viewDir, vec3 lightDir, float roughness);
vec3  fresnelSchlick(float hDotV, vec3 F0, float roughness);

float calcDirShadow(bool isLocalLight, vec4 fragPosLightSpace, sampler2DShadow shadowMap, vec3 normal, vec3 lightDir);
float calcOmniShadow(vec3 lightPos, samplerCube shadowMap, float farPlane, vec3 normal);

vec3 calcPBRDir(DirectionalLightStruct light, vec3 normal, vec3 viewDir, vec3 F0, vec3 albedo, float metallic, float roughness, int lightIndex);
vec3 calcPBRPoint(PointLightStruct light, vec3 normal, vec3 viewDir, vec3 F0, vec3 albedo, float metallic, float roughness, int lightIndex);
vec3 calcPBRSpot(SpotLightStruct light, vec3 normal, vec3 viewDir, vec3 F0, vec3 albedo, float metallic, float roughness, int lightIndex);


/* ======================================================== MAIN === */
void main() {
    vec3 viewDir = normalize(cameraPos.xyz - fs_in.FragPos);
    vec3 norm = getNormal();

    vec3  albedo    = texture(material.albedoMap, fs_in.TexCoord).rgb;
    float metallic  = texture(material.metallicMap, fs_in.TexCoord).b;
    float roughness = texture(material.roughnessMap, fs_in.TexCoord).g;
    float ao        = texture(material.aoMap, fs_in.TexCoord).r;

    vec3 F0 = vec3(0.04f);
    F0 = mix(F0, albedo, metallic);

    vec3 directLighting = vec3(0.0f);   // Lighting from light sources

    for (int i = 0; i < lightingBlock.numDirectionalLights; ++i) {
        directLighting += calcPBRDir(lightingBlock.directionalLight[i], norm, viewDir, F0, albedo, metallic, roughness, i);
    }
    for (int i = 0; i < lightingBlock.numPointLights; ++i) {
        directLighting += calcPBRPoint(lightingBlock.pointLight[i], norm, viewDir, F0, albedo, metallic, roughness, i);
    }
    for (int i = 0; i < lightingBlock.numSpotLights; ++i) {
        directLighting += calcPBRSpot(lightingBlock.spotLight[i], norm, viewDir, F0, albedo, metallic, roughness, i);
    }

    // Diffuse IBL
    vec3 F = fresnelSchlick(max(dot(norm, viewDir), 0.0f), F0, roughness);
    vec3 kS = F;
    vec3 kD = 1.0f - kS;
    kD *= 1.0f - metallic;	  
    
    vec3 irradiance = texture(irradianceMap, norm).rgb;
    vec3 diffuse    = irradiance * albedo;
    vec3 ambient    = (kD * diffuse) * ao; 

    // Final color
    vec3 color = ambient + directLighting;

    color = color / (color + vec3(1.0f));
    color = pow(color, vec3(1.0/2.2f)); 

    FragColor = vec4(color, 1.0f);
    //FragColor = vec4(texture(irradianceMap, vec3(0, 1, 0)).rgb, 1.0); 
    //FragColor = vec4(norm * 0.5 + 0.5, 1.0); 
    //FragColor = vec4(diffuse, 1.0);
}

/* ===================== LIGHTING FUNCTIONS ===================== */
vec3 calcPBRDir(DirectionalLightStruct light, vec3 normal, vec3 viewDir, vec3 F0, vec3 albedo, float metallic, float roughness, int lightIndex) {
    vec3 lightDir = normalize(-light.direction.xyz);
    vec3 halfVec  = normalize(viewDir + lightDir);
    
    vec3 radiance = light.diffuse.rgb;

    // --Cook-Torrance BRDF
    float D = distributionGGX(normal, halfVec, roughness);             // Normal Distribution Func
    float G = geometrySmithGGX(normal, viewDir, lightDir, roughness);  // Geometry Func
    vec3  F = fresnelSchlick(max(dot(normal, viewDir), 0.0f), F0, roughness);    // fresnelShlick

    vec3  num   = D * G * F;
    float denom = 4.0f * max(dot(viewDir, normal), 0.0f) * max(dot(lightDir, normal), 0.0f) + 0.0001f;
    vec3  specular = num / denom;

    // Energy conservation
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0f - metallic;

    float nDotL  = max(dot(normal, lightDir), 0.0f);
    // --Shadow
    float shadowFactor = calcDirShadow(false, fs_in.DirectionalLightSpacePos[lightIndex], DirectionalShadowMap[lightIndex], normal, lightDir);

    return (kD * albedo / PI + specular) * radiance * nDotL * (1.0f - shadowFactor);
}
vec3 calcPBRPoint(PointLightStruct light, vec3 normal, vec3 viewDir, vec3 F0, vec3 albedo, float metallic, float roughness, int lightIndex) {
    vec3 L = normalize(light.position.xyz - fs_in.FragPos);
    vec3 H = normalize(viewDir + L);
    
    float dist = length(light.position.xyz - fs_in.FragPos);
    float attenuationFactor = 1.0f / (dist * dist);
    vec3  radiance = light.diffuse.rgb * attenuationFactor * 50.0f; 

    // --Cook-Torrance BDRF
    float D = distributionGGX(normal, H, roughness);          
    float G = geometrySmithGGX(normal, viewDir, L, roughness);  
    vec3  F = fresnelSchlick(max(dot(normal, viewDir), 0.0f), F0, roughness);

    vec3  numerator   = D * G * F;
    float denominator = 4.0f * max(dot(viewDir, normal), 0.0f) * max(dot(L, normal), 0.0f) + 0.0001f;
    vec3  specular    = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0f) - kS;
    kD *= 1.0 - metallic;

    float nDotL = max(dot(normal, L), 0.0f);
    
    // --Shadow
    float shadowFactor = 0.0f;
    switch (lightIndex) {
        case 0: shadowFactor = calcOmniShadow(light.position.xyz, PointShadowMap[0], omniShadowMapFarPlanes[0], normal); break;
        case 1: shadowFactor = calcOmniShadow(light.position.xyz, PointShadowMap[1], omniShadowMapFarPlanes[1], normal); break;
        case 2: shadowFactor = calcOmniShadow(light.position.xyz, PointShadowMap[2], omniShadowMapFarPlanes[2], normal); break;
        case 3: shadowFactor = calcOmniShadow(light.position.xyz, PointShadowMap[3], omniShadowMapFarPlanes[3], normal); break;
        case 4: shadowFactor = calcOmniShadow(light.position.xyz, PointShadowMap[4], omniShadowMapFarPlanes[4], normal); break;
        case 5: shadowFactor = calcOmniShadow(light.position.xyz, PointShadowMap[5], omniShadowMapFarPlanes[5], normal); break;
    }
    
    return (kD * albedo / PI + specular) * radiance * nDotL * (1.0f - shadowFactor);
}
vec3 calcPBRSpot(SpotLightStruct light, vec3 normal, vec3 viewDir, vec3 F0, vec3 albedo, float metallic, float roughness, int lightIndex) {
    vec3 L = normalize(light.position.xyz - fs_in.FragPos);
    vec3 H = normalize(viewDir + L);
    
    float distance = length(light.position.xyz - fs_in.FragPos);
    float attenuation = 1.0f / (distance * distance); 

    float theta     = dot(L, normalize(-light.direction.xyz));
    float epsilon   = light.inCosCutoff - light.outCosCutoff;
    float intensity = clamp((theta - light.outCosCutoff) / epsilon, 0.0f, 1.0f) * 50.0f;

    vec3 radiance = light.diffuse.rgb * attenuation * intensity; 

    // --Cook-Torrance BRDF
    float D = distributionGGX(normal, H, roughness);          
    float G = geometrySmithGGX(normal, viewDir, L, roughness);  
    vec3  F = fresnelSchlick(max(dot(normal, viewDir), 0.0f), F0, roughness);

    vec3  num   = D * G * F;
    float denom = 4.0f * max(dot(viewDir, normal), 0.0) * max(dot(L, normal), 0.0f) + 0.0001f;
    vec3  specular = num / denom;

    vec3 kS = F;
    vec3 kD = vec3(1.0f) - kS;
    kD *= 1.0 - metallic;

    float nDotL = max(dot(normal, L), 0.0f);
    
    // --Shadow
    float shadowFactor = calcDirShadow(true, fs_in.SpotLightSpacePos[lightIndex], SpotShadowMap[lightIndex], normal, L);
    
    return (kD * albedo / PI + specular) * radiance * nDotL * (1.0 - shadowFactor);
}



/* ======================================= HELPER FUNCTIONS === */

vec3 getNormal() {
    vec3 tangentNormal = texture(material.normalMap, fs_in.TexCoord).rgb;
    tangentNormal = tangentNormal * 2.0f - 1.0f;
    return normalize(fs_in.TBN * tangentNormal);
}
float distributionGGX(vec3 normal, vec3 halfVec, float roughness) {
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(normal, halfVec), 0.0f);
    float NdotH2 = NdotH * NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}
float geometryShlickGGX(float nDotv, float roughness) {
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float num   = nDotv;
    float denom = nDotv * (1.0f - k) + k;
	
    return num / denom;
}
float geometrySmithGGX(vec3 normal, vec3 viewDir, vec3 lightDir, float roughness) {
    float NdotV = max(dot(normal, viewDir), 0.0f);
    float NdotL = max(dot(normal, lightDir), 0.0f);
    float ggx2  = geometryShlickGGX(NdotV, roughness);
    float ggx1  = geometryShlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}
vec3  fresnelSchlick(float hDotV, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - hDotV, 0.0, 1.0), 5.0);
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
    vec3 fragToLight = fs_in.FragPos - lightPos;
    float currentDepth = length(fragToLight);
    vec3 sampleDir = normalize(fragToLight);
    
    // Bias based on surface angle
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    float cosTheta = max(dot(normal, lightDir), 0.0f);
    float bias = max(0.005f * (1.0f - cosTheta), 0.001f);
    
    // PCF
    vec3 gridSamplingDisk[20] = vec3[](
       vec3(1, 1, 1),  vec3(1, -1, 1),  vec3(-1, -1, 1),  vec3(-1, 1, 1), 
       vec3(1, 1, -1), vec3(1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
       vec3(1, 1, 0),  vec3(1, -1, 0),  vec3(-1, -1, 0),  vec3(-1, 1, 0),
       vec3(1, 0, 1),  vec3(-1, 0, 1),  vec3(1, 0, -1),   vec3(-1, 0, -1),
       vec3(0, 1, 1),  vec3(0, -1, 1),  vec3(0, -1, -1),  vec3(0, 1, -1)
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