#version 330 core
out vec4 FragColor;

in vec3 localPos;

uniform samplerCube environmentMap;

const float PI = 3.14159265359f;

void main() {		
    // The world vector is the normal of our 'tangent' surface
    vec3 normal = normalize(localPos);

    vec3 irradiance = vec3(0.0f);

    // Create a little coordinate system (TBN) around the normal
    vec3 up    = vec3(0.0f, 1.0f, 0.0f);
    vec3 right = normalize(cross(up, normal));
    up         = normalize(cross(normal, right));

    float sampleDelta = 0.025f;
    float nrSamples   = 0.0f; 
    
    // The convolution integral: double loop over the hemisphere
    for(float phi = 0.0f; phi < 2.0f * PI; phi += sampleDelta) {
        for(float theta = 0.0f; theta < 0.5f * PI; theta += sampleDelta) {
            // Spherical to Cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            
            // Tangent space to World space
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal; 

            // Sample the skybox and add it to our total
            // We multiply by cos(theta) and sin(theta) for the math weights (Lambert's law)
            irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    
    // Average out the samples and multiply by PI
    irradiance = PI * irradiance * (1.0f / nrSamples);
    
    FragColor = vec4(irradiance, 1.0f);
}