#version 330 core
out vec4 FragColor;

#define MAX_LIGHTS 8

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 DirectionalLightSpacePos[MAX_LIGHTS];
in vec4 PointLightSpacePos[MAX_LIGHTS];
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
	float innerCutoff;
	float outerCutoff;
	float _padding0;
	float _padding1;
	float _padding2;
};

uniform Material material;
uniform sampler2D DirectionalShadowMap[MAX_LIGHTS];

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

struct ShadowMatricesUBOData {					    // 1536 Bytes
	mat4 directionalLightSpaceMatrices[MAX_LIGHTS];	// 64 * 8  = 512
	mat4 pointLightSpaceMatrices[MAX_LIGHTS];		// 64 * 8  = 512
	mat4 spotLightSpaceMatrices[MAX_LIGHTS];		// 64 * 8  = 512
};


// FUNCTION PROTOTYPES (Standardized to use 'light' as parameter name)
vec3 calcDirectionalLight(DirectionalLightStruct light, vec3 normal, vec3 viewDir, int lightIndex);
vec3 calcPointLight(PointLightStruct light, vec3 normal, vec3 viewDir);
vec3 calcSpotLight(SpotLightStruct light, vec3 normal, vec3 viewDir);

vec3 calcDirectionalLight(DirectionalLightStruct light, vec3 normal, vec3 viewDir);

vec3 calcAmbient(vec3 ambientColor);
vec3 calcDiffuse(vec3 lightDir, vec3 normal, vec3 diffuseColor);
vec3 calcSpecular(vec3 lightDir, vec3 normal, vec3 viewDir, vec3 specularColor);
float calcDirShadow(vec3 lightDir, vec3 normal, int lightIndex);

/* ======================================================== MAIN === */
void main() {
	vec3 viewDir = normalize(cameraPos.xyz - FragPos);
	vec3 norm = normalize(Normal);
	vec3 result = vec3(0.0f);

	for (int i = 0; i < lightingBlock.numDirectionalLights; ++i) {
		result += calcDirectionalLight(lightingBlock.directionalLight[i], norm, viewDir, i);
	}
   
	for (int i = 0; i < lightingBlock.numPointLights; ++i) {
		result += calcPointLight(lightingBlock.pointLight[i], norm, viewDir);
	}
    
	for (int i = 0; i < lightingBlock.numSpotLights; ++i) {
		result += calcSpotLight(lightingBlock.spotLight[i], norm, viewDir);
	}

	FragColor = vec4(result, 1.0f);
}

/* ===================== LIGHTING FUNCTIONS ===================== */
vec3 calcDirectionalLight(DirectionalLightStruct light, vec3 normal, vec3 viewDir, int lightIndex) {
    vec3 lightDir = normalize(-light.direction.xyz);

    vec3 ambient  = calcAmbient(light.ambient.xyz);
    vec3 diffuse  = calcDiffuse(lightDir, normal, light.diffuse.xyz);
    vec3 specular = calcSpecular(lightDir, normal, viewDir, light.specular.xyz);

    float shadow = calcDirShadow(lightDir, normal, lightIndex);

    return ambient + (diffuse + specular) * shadow;
}
vec3 calcPointLight(PointLightStruct light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(light.position.xyz - FragPos);

    vec3 ambient  = calcAmbient(light.ambient.xyz);
    vec3 diffuse  = calcDiffuse(lightDir, normal, light.diffuse.xyz);
    vec3 specular = calcSpecular(lightDir, normal, viewDir, light.specular.xyz);

    float distance = length(light.position.xyz - FragPos);
    float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    return (ambient + diffuse + specular) * attenuation;
}

vec3 calcSpotLight(SpotLightStruct light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(light.position.xyz - FragPos);

    float theta     = dot(lightDir, normalize(-light.direction.xyz));
    float epsilon   = light.innerCutoff - light.outerCutoff;
    float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0f, 1.0f);

    vec3 ambient  = calcAmbient(light.ambient.xyz);
    vec3 diffuse  = calcDiffuse(lightDir, normal, light.diffuse.xyz);
    vec3 specular = calcSpecular(lightDir, normal, viewDir, light.specular.xyz);

    float distance = length(light.position.xyz - FragPos);
    float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    return ambient * attenuation + (diffuse + specular) * attenuation * intensity;
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

/* ===================== SHADOW MAPPING ===================== */
float calcDirShadow(vec3 lightDir, vec3 normal, int lightIndex) {
    vec3 projCoords = DirectionalLightSpacePos[lightIndex].xyz / DirectionalLightSpacePos[lightIndex].w;

    // NDC [-1, +1] → [0,1]
    projCoords = projCoords * 0.5f + 0.5f;

    // Outside shadow map = fully lit
    if (projCoords.x < 0.0f || projCoords.x > 1.0f ||
        projCoords.y < 0.0f || projCoords.y > 1.0f ||
        projCoords.z > 1.0f) {
        return 1.0f;
    }

    float closestDepth = texture(DirectionalShadowMap[lightIndex], projCoords.xy).r;
    float currentDepth = projCoords.z;

    float bias = max(0.05f * (1.0f - dot(normal, lightDir)), 0.005f);

    return (currentDepth - bias > closestDepth) ? 0.0f : 1.0f;
}
