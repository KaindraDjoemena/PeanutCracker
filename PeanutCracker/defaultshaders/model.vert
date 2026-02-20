#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

#define MAX_LIGHTS 8

out VS_OUT {
	vec3 FragPos;
	vec2 TexCoord;
	vec4 DirectionalLightSpacePos[MAX_LIGHTS];
	vec4 SpotLightSpacePos[MAX_LIGHTS];

	mat3 TBN;
} vs_out;

// LIGHT STRUCTS
struct DirectionalLightStruct {
	vec4  direction;	// 16
	vec4  color;        // 16
	float power;        // 4
	float range;        // 4
	float normalBias;   // 4
	float depthBias;    // 4
};						// 48 Bytes

struct PointLightStruct {
	vec4  position;   // 16
	vec4  color;      // 16
	float power;      // 4
	float radius;     // 4
	float normalBias; // 4
	float depthBias;  // 4 
};				 	  // 48 Bytes

struct SpotLightStruct {
	vec4  position;		// 16
	vec4  direction;	// 16
	vec4  color;        // 16
	float power;        // 4
	float range;		// 4
	float inCosCutoff;	// 4
	float outCosCutoff;	// 4
	float normalBias;   // 4
	float depthBias;    // 4
	float p0;		    // 4
	float p1;           // 4
};						// 80 Bytes

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


void main() {
    vs_out.FragPos  = vec3(model * vec4(aPos, 1.0f));
	vs_out.TexCoord = aTexCoords;

	// Tangent space matrix
	vec3 T = normalize(mat3(normalMatrix) * aTangent);
	vec3 N = normalize(mat3(normalMatrix) * aNormal);
	T      = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);
	vs_out.TBN = mat3(T, B, N);

	for (int i = 0; i < lightingBlock.numDirectionalLights; ++i) {
		vec3 offsetPos = vs_out.FragPos + N * lightingBlock.directionalLight[i].normalBias;
		vs_out.DirectionalLightSpacePos[i] = shadowMatricesBlock.directionalLightSpaceMatrices[i] * vec4(offsetPos, 1.0f);
	}

	for (int i = 0; i < lightingBlock.numSpotLights; ++i) {
		vec3 offsetPos = vs_out.FragPos + N * lightingBlock.spotLight[i].normalBias;
		vs_out.SpotLightSpacePos[i] = shadowMatricesBlock.spotLightSpaceMatrices[i] * vec4(offsetPos, 1.0f);
	}
	
	gl_Position = projection * view * model * vec4(aPos, 1.0f);
}