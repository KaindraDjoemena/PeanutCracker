#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

#define MAX_LIGHTS 8

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec4 DirectionalLightSpacePos[MAX_LIGHTS];
out vec4 SpotLightSpacePos[MAX_LIGHTS];

// LIGHT STRUCTS
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

layout (std140) uniform ShadowMatricesUBOData {
    mat4 directionalLightSpaceMatrices[MAX_LIGHTS];
    mat4 spotLightSpaceMatrices[MAX_LIGHTS];
} shadowMatricesBlock;

uniform mat4 model;
uniform mat4 normalMatrix;


void main()
{
    FragPos  = vec3(model * vec4(aPos, 1.0f));
    Normal   = mat3(normalMatrix) * aNormal;
    TexCoord = aTexCoords;

    gl_Position = projection * view * model * vec4(aPos, 1.0f);

	for (int i = 0; i < lightingBlock.numDirectionalLights; ++i) {
		DirectionalLightSpacePos[i] = shadowMatricesBlock.directionalLightSpaceMatrices[i] * model * vec4(aPos, 1.0f);
	}
    
	for (int i = 0; i < lightingBlock.numSpotLights; ++i) {
		SpotLightSpacePos[i] = shadowMatricesBlock.spotLightSpaceMatrices[i] * model * vec4(aPos, 1.0f);
	}
}