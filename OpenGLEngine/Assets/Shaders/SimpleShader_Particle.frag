#version 410 core

in vec2 fragUv;
in float fragAlpha;

uniform vec3 particleColor;

out vec4 outColor;

void main()
{
    float dist = length(fragUv - vec2(0.5));
    float edge = smoothstep(0.5, 0.1, dist);
    outColor = vec4(particleColor, fragAlpha * edge);
}
