#version 410 core

in vec2 fragUv;
in vec4 fragColor;

out vec4 outColor;

void main()
{
    float dist = length(fragUv - vec2(0.5));
    float edge = smoothstep(0.5, 0.1, dist);
    outColor = vec4(fragColor.rgb, fragColor.a * edge);
}
