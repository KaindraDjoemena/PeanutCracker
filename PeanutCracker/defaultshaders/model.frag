#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

#define MAX_LIGHTS 8

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
	float padding;
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
	float padding0;
	float padding1;
	float padding2;
};

uniform Material material;

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
vec3 calcDirectionalLight(DirectionalLightStruct light, vec3 normal, vec3 viewDir);
vec3 calcPointLight(PointLightStruct light, vec3 normal, vec3 viewDir);
vec3 calcSpotLight(SpotLightStruct light, vec3 normal, vec3 viewDir);

vec3 calcAmbient(vec3 ambientColor);
vec3 calcDiffuse(vec3 lightDir, vec3 normal, vec3 diffuseColor);
vec3 calcSpecular(vec3 lightDir, vec3 normal, vec3 viewDir, vec3 specularColor);

void main() {
	vec3 viewDir = normalize(cameraPos.xyz - FragPos);
	vec3 norm = normalize(Normal);
	vec3 result = vec3(0.0f);

	for (int i = 0; i < lightingBlock.numDirectionalLights; ++i) {
		if (i >= lightingBlock.numDirectionalLights) break;
		result += calcDirectionalLight(lightingBlock.directionalLight[i], norm, viewDir);
	}
   
	for (int i = 0; i < lightingBlock.numPointLights; ++i) {
		if (i >= lightingBlock.numPointLights) break;
		result += calcPointLight(lightingBlock.pointLight[i], norm, viewDir);
	}
    
	for (int i = 0; i < lightingBlock.numSpotLights; ++i) {
		if (i >= lightingBlock.numSpotLights) break;
		result += calcSpotLight(lightingBlock.spotLight[i], norm, viewDir);
	}

	FragColor = vec4(result, 1.0f);
}

vec3 calcDirectionalLight(DirectionalLightStruct light, vec3 normal, vec3 viewDir) {
	vec3 lightDir = normalize(-light.direction.xyz);
    	
	vec3 ambient  = calcAmbient(light.ambient.xyz);
	vec3 diffuse  = calcDiffuse(lightDir, normal, light.diffuse.xyz);
	vec3 specular = calcSpecular(lightDir, normal, viewDir, light.specular.xyz);
	
	return (ambient + diffuse + specular);
}

vec3 calcPointLight(PointLightStruct light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(light.position.xyz - FragPos);

    vec3 ambient  = calcAmbient(light.ambient.xyz);
    vec3 diffuse  = calcDiffuse(lightDir, normal, light.diffuse.xyz);
    vec3 specular = calcSpecular(lightDir, normal, viewDir, light.specular.xyz);

    float distance = length(light.position.xyz - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    return (ambient + diffuse + specular) * attenuation;
}

vec3 calcSpotLight(SpotLightStruct light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(light.position.xyz - FragPos);

    float theta     = dot(lightDir, normalize(-light.direction.xyz));
    float epsilon   = light.innerCutoff - light.outerCutoff;
    float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);

    vec3 ambient  = calcAmbient(light.ambient.xyz);
    vec3 diffuse  = calcDiffuse(lightDir, normal, light.diffuse.xyz);
    vec3 specular = calcSpecular(lightDir, normal, viewDir, light.specular.xyz);

    float distance = length(light.position.xyz - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    return ambient * attenuation + (diffuse + specular) * attenuation * intensity;
}

vec3 calcAmbient(vec3 ambientColor) {
    return ambientColor * vec3(texture(material.texture_diffuse1, TexCoord));
}

vec3 calcDiffuse(vec3 lightDir, vec3 normal, vec3 diffuseColor) {
    float diff = max(dot(normal, lightDir), 0.0);
    return diffuseColor * diff * vec3(texture(material.texture_diffuse1, TexCoord));
}

vec3 calcSpecular(vec3 lightDir, vec3 normal, vec3 viewDir, vec3 specularColor) {
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    return specularColor * spec * vec3(texture(material.texture_specular1, TexCoord));
}