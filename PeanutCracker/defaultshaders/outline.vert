#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

layout (std140) uniform CameraMatricesUBOData {
    mat4 projection;
    mat4 view;
    vec4 cameraPos;
};

uniform mat4 model;
uniform mat4 normalMatrix;
uniform float outlineThickness = 0.8f;

void main() {
    vec4 worldPos  = model * vec4(aPos, 1.0f);
	vec3 worldNorm = normalize(mat3(normalMatrix) * aNormal);

	float dist = distance(cameraPos.xyz, worldPos.xyz);

	worldPos.xyz += worldNorm * outlineThickness * (dist * 0.01f);

    gl_Position = projection * view * worldPos;
}