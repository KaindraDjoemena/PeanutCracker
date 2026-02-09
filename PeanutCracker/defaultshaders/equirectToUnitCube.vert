#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 localPos;

uniform mat4 projectionMat;
uniform mat4 viewMat;

void main() {
	localPos = aPos;
	gl_Position = projectionMat * viewMat * vec4(localPos, 1.0f);
}