#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal; // We need normals for expansion!

layout (std140) uniform CameraMatricesUBOData {
    mat4 projection;
    mat4 view;
    vec4 cameraPos;
};


uniform mat4 model;
uniform float outlineThickness = 0.01f;

void main() {
    // Expand along normal in Model Space
    vec3 pos = aPos + aNormal * outlineThickness;
    gl_Position = projection * view * model * vec4(pos, 1.0);
}