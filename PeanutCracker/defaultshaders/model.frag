#version 330 core
out vec4 FragColor;

#define MAX_LIGHTS 8

const float PI 				   = 3.14159f;
const float MAX_REFLECTION_LOD = 4.0f;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoord;
    vec4 DirectionalLightSpacePos[MAX_LIGHTS];
    vec4 SpotLightSpacePos[MAX_LIGHTS];

    mat3 TBN;
} fs_in;

struct Material {
    sampler2D albedoMap;
    sampler2D normalMap;
    sampler2D metallicMap;
    sampler2D roughnessMap;
    sampler2D aoMap;
};

// === LIGHT STRUCTS ========================================================
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

uniform Material material;

// Shadows
uniform sampler2DShadow DirectionalShadowMap[MAX_LIGHTS];
uniform samplerCube     PointShadowMap[MAX_LIGHTS];
uniform sampler2DShadow SpotShadowMap[MAX_LIGHTS];

// IBL
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D   brdfLUT;

// Reflection Probes
uniform samplerCube refEnvMap[MAX_LIGHTS];

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

layout (std140) uniform ReflectionProbeUBOData {
    vec4 position[MAX_LIGHTS];
    mat4 worldMats[MAX_LIGHTS];
    mat4 invWorldMats[MAX_LIGHTS];
    vec4 proxyDims[MAX_LIGHTS];
    int  numRefProbes;
    int  padding0;
    int  padding1;
    int  padding2;
} refProbeBlock;


vec3  getNormal();
float distributionGGX(vec3 normal, vec3 halfVec, float roughness);
float geometryShlickGGX(float nDotv, float roughness);
float geometrySmithGGX(vec3 normal, vec3 viewDir, vec3 lightDir, float roughness);
vec3  fresnelSchlick(float hDotV, vec3 F0);
vec3  fresnelSchlickRoughness(float hDotV, vec3 F0, float roughness);
vec3  parallaxCorrect(vec3 R, float roughness);
bool  isInAABB(vec3 pos, vec3 dimensions);

float calcDirShadow(bool isLocalLight, vec4 fragPosLightSpace, sampler2DShadow shadowMap, vec3 normal, vec3 lightDir, float depthBias);
float calcOmniShadow(vec3 lightPos, samplerCube shadowMap, float farPlane, vec3 normal, float depthBias);

vec3 calcPBRDir(DirectionalLightStruct light, vec3 normal, vec3 viewDir, vec3 F0, vec3 albedo, float metallic, float roughness, int lightIndex);
vec3 calcPBRPoint(PointLightStruct light, vec3 normal, vec3 viewDir, vec3 F0, vec3 albedo, float metallic, float roughness, int lightIndex);
vec3 calcPBRSpot(SpotLightStruct light, vec3 normal, vec3 viewDir, vec3 F0, vec3 albedo, float metallic, float roughness, int lightIndex);


/* ======================================================== MAIN === */
void main() {
    vec3 viewDir = normalize(cameraPos.xyz - fs_in.FragPos);	// frag.xyz -> camera.xyz
    vec3 norm = getNormal();	// tangent space

    vec3  albedo    = texture(material.albedoMap, fs_in.TexCoord).rgb;
    float metallic  = texture(material.metallicMap, fs_in.TexCoord).b;
    float roughness = texture(material.roughnessMap, fs_in.TexCoord).g;
    float ao        = texture(material.aoMap, fs_in.TexCoord).r;

    vec3 F0 = vec3(0.04f);
    F0 = mix(F0, albedo, metallic);

    vec3 directLighting = vec3(0.0f);   // Lighting from light sources

	//--Direct lighting
    for (int i = 0; i < lightingBlock.numDirectionalLights; ++i) {
        directLighting += calcPBRDir(lightingBlock.directionalLight[i], norm, viewDir, F0, albedo, metallic, roughness, i);
    }
    for (int i = 0; i < lightingBlock.numPointLights; ++i) {
        directLighting += calcPBRPoint(lightingBlock.pointLight[i], norm, viewDir, F0, albedo, metallic, roughness, i);
    }
    for (int i = 0; i < lightingBlock.numSpotLights; ++i) {
        directLighting += calcPBRSpot(lightingBlock.spotLight[i], norm, viewDir, F0, albedo, metallic, roughness, i);
    }

    // IBL
	vec3 R  = reflect(-viewDir, norm);
	vec3 F  = fresnelSchlickRoughness(max(dot(norm, viewDir), 0.0f), F0, roughness);
	vec3 kS = F;
	vec3 kD = 1.0f - kS;
	kD *= 1.0f - metallic;

	//--Diffuse IBL
	vec3 irradiance = texture(irradianceMap, norm).rgb;
	vec3 diffuseIBL = irradiance * albedo;

	//--Specular IBL
	//vec3 prefilteredCol = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
	vec3 prefilteredCol = parallaxCorrect(R, roughness);
	vec2 brdf           = texture(brdfLUT, vec2(max(dot(norm, viewDir), 0.0f), roughness)).rg;
	vec3 specularIBL    = prefilteredCol * (F * brdf.x + brdf.y);
	
	// Color
	vec3 ambient = (kD * diffuseIBL + specularIBL) * ao;
	vec3 color   = ambient + directLighting;

    FragColor = vec4(color, 1.0f);
}

/* ===================== LIGHTING FUNCTIONS ===================== */
// DIRECTIONAL
vec3 calcPBRDir(DirectionalLightStruct light, vec3 normal, vec3 viewDir, vec3 F0, vec3 albedo, float metallic, float roughness, int lightIndex) {
    vec3 lightDir = normalize(-light.direction.xyz);
    vec3 halfVec  = normalize(viewDir + lightDir);
    
    vec3 radiance = light.color.rgb * light.power;

    // --Cook-Torrance BRDF
    float D = distributionGGX(normal, halfVec, roughness);             // Normal Distribution Func
    float G = geometrySmithGGX(normal, viewDir, lightDir, roughness);  // Geometry Func
    vec3  F = fresnelSchlick(max(dot(normal, viewDir), 0.0f), F0);    // fresnelShlick

    vec3  num   = D * G * F;
    float denom = 4.0f * max(dot(viewDir, normal), 0.0f) * max(dot(lightDir, normal), 0.0f) + 0.0001f;
    vec3  specular = num / denom;

    // Energy conservation
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0f - metallic;

    float nDotL  = max(dot(normal, lightDir), 0.0f);
    // --Shadow
    float shadowFactor = calcDirShadow(false, fs_in.DirectionalLightSpacePos[lightIndex], DirectionalShadowMap[lightIndex], normal, lightDir, light.depthBias);

	return (kD * albedo / PI + specular) * radiance * nDotL * (1.0f - shadowFactor);
}
// POINT
vec3 calcPBRPoint(PointLightStruct light, vec3 normal, vec3 viewDir, vec3 F0, vec3 albedo, float metallic, float roughness, int lightIndex) {
    vec3 lightDir = normalize(light.position.xyz - fs_in.FragPos);
    vec3 halfVec  = normalize(viewDir + lightDir);
    
    float dist = length(light.position.xyz - fs_in.FragPos);
    float attenuationFactor = 1.0f / (dist * dist);
	
	float window = pow(clamp(1.0f - pow(dist / light.radius, 4.0f), 0.0f, 1.0f), 2.0f);
	
    vec3  radiance = light.color.rgb * attenuationFactor * window * light.power; 

    // --Cook-Torrance BDRF
    float D = distributionGGX(normal, halfVec, roughness);             // Normal Distribution Func
    float G = geometrySmithGGX(normal, viewDir, lightDir, roughness);  // Geometry Func
    vec3  F = fresnelSchlick(max(dot(halfVec, viewDir), 0.0f), F0);    // fresnelShlick

    vec3  numerator   = D * G * F;
    float denominator = 4.0f * max(dot(viewDir, normal), 0.0f) * max(dot(lightDir, normal), 0.0f) + 0.0001f;
    vec3  specular    = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0f) - kS;
    kD *= 1.0 - metallic;

    float nDotL = max(dot(normal, lightDir), 0.0f);
    
    // --Shadow
    float shadowFactor = 0.0f;
    switch (lightIndex) {
        case 0: shadowFactor = calcOmniShadow(light.position.xyz, PointShadowMap[0], light.radius, normal, light.depthBias); break;
        case 1: shadowFactor = calcOmniShadow(light.position.xyz, PointShadowMap[1], light.radius, normal, light.depthBias); break;
        case 2: shadowFactor = calcOmniShadow(light.position.xyz, PointShadowMap[2], light.radius, normal, light.depthBias); break;
        case 3: shadowFactor = calcOmniShadow(light.position.xyz, PointShadowMap[3], light.radius, normal, light.depthBias); break;
        case 4: shadowFactor = calcOmniShadow(light.position.xyz, PointShadowMap[4], light.radius, normal, light.depthBias); break;
        case 5: shadowFactor = calcOmniShadow(light.position.xyz, PointShadowMap[5], light.radius, normal, light.depthBias); break;
    }
    
    return (kD * albedo / PI + specular) * radiance * nDotL * (1.0f - shadowFactor);
}
// SPOT
vec3 calcPBRSpot(SpotLightStruct light, vec3 normal, vec3 viewDir, vec3 F0, vec3 albedo, float metallic, float roughness, int lightIndex) {
    vec3 lightDir = normalize(light.position.xyz - fs_in.FragPos);
    vec3 halfVec  = normalize(viewDir + lightDir);
    
    float dist = length(light.position.xyz - fs_in.FragPos);
    float attenuation = 1.0f / (dist * dist); 
	
	float window = pow(clamp(1.0f - pow(dist / light.range, 4.0f), 0.0f, 1.0f), 2.0f);

    float theta     = dot(lightDir, normalize(-light.direction.xyz));
    float epsilon   = light.inCosCutoff - light.outCosCutoff;
    float intensity = clamp((theta - light.outCosCutoff) / epsilon, 0.0f, 1.0f) * window * light.power;

    vec3 radiance = light.color.rgb * attenuation * intensity; 

    // --Cook-Torrance BRDF
    float D = distributionGGX(normal, halfVec, roughness);             // Normal Distribution Func
    float G = geometrySmithGGX(normal, viewDir, lightDir, roughness);  // Geometry Func
    vec3  F = fresnelSchlick(max(dot(halfVec, viewDir), 0.0f), F0);    // fresnelShlick

    vec3  num   = D * G * F;
    float denom = 4.0f * max(dot(viewDir, normal), 0.0f) * max(dot(lightDir, normal), 0.0f) + 0.0001f;
    vec3  specular = num / denom;

    vec3 kS = F;
    vec3 kD = vec3(1.0f) - kS;
    kD *= 1.0 - metallic;

    float nDotL = max(dot(normal, lightDir), 0.0f);
    
    // --Shadow
    float shadowFactor = calcDirShadow(true, fs_in.SpotLightSpacePos[lightIndex], SpotShadowMap[lightIndex], normal, lightDir, light.depthBias);
    
    return (kD * albedo / PI + specular) * radiance * nDotL * (1.0 - shadowFactor);
}



/* ======================================= HELPER FUNCTIONS === */
vec3 getNormal() {
    vec3 tangentNormal = texture(material.normalMap, fs_in.TexCoord).rgb;
    tangentNormal = tangentNormal * 2.0f - 1.0f;
    return normalize(fs_in.TBN * tangentNormal);
}
float distributionGGX(vec3 normal, vec3 halfVec, float roughness) {
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(normal, halfVec), 0.0f);
    float NdotH2 = NdotH * NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;
	
    return num / denom;
}
float geometryShlickGGX(float nDotv, float roughness) {
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float num   = nDotv;
    float denom = nDotv * (1.0f - k) + k;
	
    return num / denom;
}
float geometrySmithGGX(vec3 normal, vec3 viewDir, vec3 lightDir, float roughness) {
    float NdotV = max(dot(normal, viewDir), 0.0f);
    float NdotL = max(dot(normal, lightDir), 0.0f);
    float ggx2  = geometryShlickGGX(NdotV, roughness);
    float ggx1  = geometryShlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}
vec3  fresnelSchlick(float hDotV, vec3 F0) {
	return F0 + (1.0f - F0) * pow(clamp(1.0f - hDotV, 0.0f, 1.0f), 5.0f);
}
vec3  fresnelSchlickRoughness(float hDotV, vec3 F0, float roughness) {
	return F0 + (max(vec3(1.0f - roughness), F0) - F0) * pow(clamp(1.0f - hDotV, 0.0f, 1.0f), 5.0f);
}

vec3 parallaxCorrect(vec3 R, float roughness) {
	for (int i = 0; i < refProbeBlock.numRefProbes; ++i) {
		vec3 localPos = vec3(refProbeBlock.invWorldMats[i] * vec4(fs_in.FragPos, 1.0f));
		
		if (isInAABB(localPos, refProbeBlock.proxyDims[i].xyz)) {
			// Slab intersection check from inside the AABB
			vec3 localR = vec3(refProbeBlock.invWorldMats[i] * vec4(R, 0.0f));
			
			vec3 t1 = (-refProbeBlock.proxyDims[i].xyz / 2 - localPos) / localR;
			vec3 t2 = ( refProbeBlock.proxyDims[i].xyz / 2 - localPos) / localR;
			
			vec3 tMin = min(t1, t2);
			vec3 tMax = max(t1, t2);
			float t = min(min(tMax.x, tMax.y), tMax.z);
		
			vec3 localHit = localPos + t * localR;
			vec3 worldHit = vec3(refProbeBlock.worldMats[i] * vec4(localHit, 1.0f));
		
			vec3 rPrime = worldHit - refProbeBlock.position[i].xyz;
		
			switch(i) {
			case 0: return textureLod(refEnvMap[0], rPrime, roughness * MAX_REFLECTION_LOD).rgb; break;
			case 1: return textureLod(refEnvMap[1], rPrime, roughness * MAX_REFLECTION_LOD).rgb; break;
			case 2: return textureLod(refEnvMap[2], rPrime, roughness * MAX_REFLECTION_LOD).rgb; break;
			case 3: return textureLod(refEnvMap[3], rPrime, roughness * MAX_REFLECTION_LOD).rgb; break;
			case 4: return textureLod(refEnvMap[4], rPrime, roughness * MAX_REFLECTION_LOD).rgb; break;
			case 5: return textureLod(refEnvMap[5], rPrime, roughness * MAX_REFLECTION_LOD).rgb; break;
			case 6: return textureLod(refEnvMap[6], rPrime, roughness * MAX_REFLECTION_LOD).rgb; break;
			case 7: return textureLod(refEnvMap[7], rPrime, roughness * MAX_REFLECTION_LOD).rgb; break;
			}
		}
	}
	return textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
}

bool isInAABB(vec3 pos, vec3 dimensions) {
    return all(greaterThanEqual(pos, -dimensions / 2)) && all(lessThanEqual(pos, dimensions / 2));
}

float calcDirShadow(bool isLocalLight, vec4 fragPosLightSpace, sampler2DShadow shadowMap, vec3 normal, vec3 lightDir, float depthBias) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5f + 0.5f;
    
    // Outside bounds: Local lights=shadowed, Directional=lit
    if(projCoords.x < 0.0f || projCoords.x > 1.0f ||
       projCoords.y < 0.0f || projCoords.y > 1.0f) {
        return isLocalLight ? 1.0f : 0.0f;
    }

    if (projCoords.z > 1.0f) {
        return isLocalLight ? 1.0f : 0.0f;
    }
    
    if (projCoords.z < 0.0f) {
        return isLocalLight ? 1.0f : 0.0f;
    }
    
	// Bias
    float cosTheta = clamp(dot(normal, lightDir), 0.0f, 1.0f);
    float bias     = mix(depthBias, depthBias * 5.0f, 1.0f - cosTheta);

    // 3x3 PCF
    float shadow = 0.0f;
    vec2 texelSize = 1.0f / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            vec3 coord = vec3(projCoords.xy + vec2(x,y) * texelSize, projCoords.z - bias);
            shadow += texture(shadowMap, coord);
        }
    }

    return 1.0f - (shadow / 9.0f);
}

float calcOmniShadow(vec3 lightPos, samplerCube shadowMap, float farPlane, vec3 normal, float depthBias) {
    vec3 fragToLight   = fs_in.FragPos - lightPos;
    float currentDepth = length(fragToLight);
    vec3 sampleDir     = normalize(fragToLight);
    
    // Bias based on surface angle
	vec3  lightDir = normalize(lightPos - fs_in.FragPos);
	float cosTheta = clamp(dot(normal, lightDir), 0.0f, 1.0f);
	float bias     = mix(depthBias, depthBias * 5.0f, 1.0f - cosTheta);

    // PCF
    vec3 gridSamplingDisk[20] = vec3[](
       vec3(1, 1,  1),  vec3(1, -1, 1),  vec3(-1, -1, 1),  vec3(-1, 1, 1), 
       vec3(1, 1, -1), vec3(1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
       vec3(1, 1,  0),  vec3(1, -1, 0),  vec3(-1, -1, 0),  vec3(-1, 1, 0),
       vec3(1, 0,  1),  vec3(-1, 0, 1),  vec3(1, 0, -1),   vec3(-1, 0, -1),
       vec3(0, 1,  1),  vec3(0, -1, 1),  vec3(0, -1, -1),  vec3(0, 1, -1)
    );
    float shadow = 0.0f;
    int samples = 20;
    float diskRadius = (1.0f + (currentDepth / farPlane)) / 25.0f; 
    for(int i = 0; i < samples; ++i) {
        float closestDepth = texture(shadowMap, sampleDir + gridSamplingDisk[i] * diskRadius).r;
        closestDepth *= farPlane;
        
        if(currentDepth - bias > closestDepth) {
            shadow += 1.0f;
        }
    }

    return shadow / float(samples);
}