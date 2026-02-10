#version 330 core
out vec2 FragColor;

in vec2 TexCoords;

uniform float roughness;

const float PI = 3.141592f;

float radicalInverse_VDC(uint bits);
vec2  hammersley(uint i, uint n);
vec3  importanceSampleGGX(vec2 xi, vec3 normal, float roughness);
float geometrySchlickGGX(float nDotV, float roughness);
float geometrySmith(vec3 normal, vec3 viewDir, vec3 lightDir, float roughness);
vec2 integrateBDRF(float nDotV, float roughness);

void main() {
	vec2 integratedBDRF = integrateBDRF(TexCoords.x, TexCoords.y);
	FragColor = integratedBDRF;
}

float radicalInverse_VDC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 hammersley(uint i, uint n) {
	return vec2(float(i) / float(n), radicalInverse_VDC(i));
}

vec3 importanceSampleGGX(vec2 xi, vec3 normal, float roughness) {
	float a = roughness * roughness;
	
	float phi = 2.0f * PI * xi.x;
	float cosTheta = sqrt((1.0f - xi.y) / (1.0f + (a*a - 1.0f) * xi.y));
	float sinTheta = sqrt(1.0f - cosTheta*cosTheta);
	
	vec3 halfVec = vec3(cos(phi) * sinTheta,
						sin(phi) * sinTheta,
						cosTheta);
						
	vec3 upVec        = abs(normal.z) < 0.999f ? vec3(0.0f, 0.0f, 1.0f) : vec3(1.0f, 0.0f, 0.0f);
	vec3 tangentVec   = normalize(cross(upVec, normal));
	vec3 bitangentVec = cross(tangentVec, normal);
	
	vec3 sampleVec = tangentVec * halfVec.x + bitangentVec * halfVec.y + normal * halfVec.z;
	return normalize(sampleVec);
}

float geometrySchlickGGX(float nDotV, float roughness) {
	float a = roughness;
	float k = (a * a) / 2.0f;
	
	float nom   = nDotV;
	float denom = nDotV * (1.0f - k) + k;
	
	return nom / denom;
}

float geometrySmith(vec3 normal, vec3 viewDir, vec3 lightDir, float roughness) {
	float nDotV = max(dot(normal, viewDir), 0.0f);
	float nDotL = max(dot(normal, lightDir), 0.0f);
	
	float ggx1 = geometrySchlickGGX(nDotV, roughness);
	float ggx2 = geometrySchlickGGX(nDotL, roughness);
	
	return ggx1 * ggx2;
}

vec2 integrateBDRF(float nDotV, float roughness) {
	vec3 v = vec3(sqrt(1.0f - nDotV*nDotV),
				  0.0f,
				  nDotV);
				  
	float A = 0.0f;
	float B = 0.0f;
	
	vec3 norm = vec3(0.0f, 0.0f, 1.0f);
	
	const uint SAMPLE_COUNT = 1024u;
	for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
		vec2 xi      = hammersley(i, SAMPLE_COUNT);
		vec3 halfDir = importanceSampleGGX(xi, norm, roughness);
		vec3 l       = normalize(2.0f * dot(v, halfDir) * halfDir - v);
		
		float nDotL = max(l.z, 0.0f);
		float nDotH = max(halfDir.z, 0.0f);
		float vDotH = max(dot(v, halfDir), 0.0f);
		
		if (nDotL > 0.0f) {
			float g    = geometrySmith(norm, v, l, roughness);
			float gVis = (g * vDotH) / (nDotH * nDotV);
			float fC   = pow(1.0f - vDotH, 5.0f);
			
			A += (1.0f - fC) * gVis;
			B += fC * gVis;
		}
	}
	
	A /= float(SAMPLE_COUNT);
	B /= float(SAMPLE_COUNT);
	
	return vec2(A, B);
}