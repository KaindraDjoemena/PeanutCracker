#version 330 core
out vec4 FragColor;

in vec3 localPos;

uniform samplerCube environmentMap;
uniform float roughness;

const float PI = 3.141592f;

float radicalInverse_VDC(uint bits);
vec2  hammersley(uint i, uint n);
vec3  importanceSampleGGX(vec2 xi, vec3 normal, float roughness);
float distributionGGX(vec3 normal, vec3 halfVec, float roughness);

void main() {
	vec3 norm = normalize(localPos);
	vec3 r = norm;
	vec3 v = r;
	
	const uint SAMPLE_COUNT = 1024u;
	float totalWeight     = 0.0f;
	vec3 prefilteredColor = vec3(0.0f);
	for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
		vec2 xi      = hammersley(i, SAMPLE_COUNT);
		vec3 halfVec = importanceSampleGGX(xi, norm, roughness);
		vec3 l       = normalize(2.0f * dot(v, halfVec) * halfVec - v);
		
		float nDotL = max(dot(norm, l), 0.0f);
		if (nDotL > 0.0f) {
			float D = distributionGGX(norm, halfVec, roughness);
			float NdotH = max(dot(norm, halfVec), 0.0f);
			float HdotV = max(dot(v, halfVec), 0.0f);
			float pdf = D * NdotH / (4.0f * HdotV) + 0.0001f;
			
			float resolution = 512.0f; // Resolution of source cubemap (per face)
			float saTexel = 4.0f * PI / (6.0f * resolution * resolution);
			float saSample = 1.0f / (float(SAMPLE_COUNT) * pdf + 0.0001f);
			float mipLevel = roughness == 0.0f ? 0.0f : 0.5f * log2(saSample / saTexel);
			
			// Fixed: Use textureLod with calculated mip level
			prefilteredColor += textureLod(environmentMap, l, mipLevel).rgb * nDotL;
			totalWeight      += nDotL;
		}
	}
	
	prefilteredColor /= totalWeight;
	
	FragColor = vec4(prefilteredColor, 1.0f);
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

float distributionGGX(vec3 normal, vec3 halfVec, float roughness) {
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(normal, halfVec), 0.0f);
	float NdotH2 = NdotH * NdotH;
	
	float nom   = a2;
	float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
	denom = PI * denom * denom;
	
	return nom / denom;
}