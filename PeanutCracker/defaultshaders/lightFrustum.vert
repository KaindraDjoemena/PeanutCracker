#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;       // inverse light VP
uniform mat4 view;        // camera view
uniform mat4 projection;  // camera projection

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
