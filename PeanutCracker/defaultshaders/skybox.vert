#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

layout (std140) uniform CameraMatricesUBOData {
    mat4 projection;
    mat4 view;
    vec4 cameraPos;
};

void main() {
	    TexCoords = vec3(aPos.x, aPos.y, aPos.z);
    	vec4 pos = projection * mat4(mat3(view)) * vec4(aPos, 1.0f);
    	gl_Position = pos.xyww;

} 