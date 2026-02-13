#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform vec3  color = vec3(1.0f, 1.0f, 0.0f);
uniform int   mode;  // {LINE, SDF}
uniform float thickness = 0.001f;
uniform float radius = 1.0f;

void main()
{
    // LINE
    if (mode == 0) {
        FragColor = vec4(color, 1.0);
        return;
    }

    // SDF
    if (mode == 1) {
        vec2 uv = TexCoords * 2.0 - 1.0;
        float dist = length(uv);
        float aa = fwidth(dist);
        
        float edgeDist = abs(dist - radius);
        
        float alpha = 1.0 - smoothstep(thickness - aa, thickness + aa, edgeDist);

        if (alpha < 0.01) discard;

        FragColor = vec4(color, alpha);
    }
    else {
        discard;
    }
}