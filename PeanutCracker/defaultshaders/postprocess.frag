#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D hdrBuffer;
float exposure = 1.0f;

// ACES Filmic Tone Mapping
vec3 ACESFilm(vec3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);
}

void main() {             
    vec3 hdrColor = texture(hdrBuffer, TexCoords).rgb;

    hdrColor *= exposure;
    
    vec3 mapped = ACESFilm(hdrColor);        // Tone mapping
    mapped = pow(mapped, vec3(1.0f / 2.2f)); // Gamma correction
  
    FragColor = vec4(hdrColor, 1.0f);
}