#version 330 core
out vec4 FragColor;


float NEAR_PLANE = 0.1f;
float FAR_PLANE  = 100.0f;

float linearizeDepth(float depth) {
	float z = depth * 2.0f - 1.0f;
	return (2.0 * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - z * (FAR_PLANE - NEAR_PLANE));
}

void main() {
	float depth = linearizeDepth(gl_FragCoord.z) / FAR_PLANE;
	FragColor	= vec4(vec3(depth), 1.0f);
}