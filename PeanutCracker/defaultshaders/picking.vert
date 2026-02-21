#version 330 core
layout (location = 0) in vec3 aPos;

layout (std140) uniform CameraMatricesUBOData {
    mat4 projection;
    mat4 view;
    vec4 cameraPos;
};

uniform mat4 modelMat;

void main() {
	gl_Position = projection * view * modelMat * vec4(aPos, 1.0f);
}