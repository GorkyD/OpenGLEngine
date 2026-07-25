#version 410 core

in float heightFactor;

uniform float time;

out vec4 outColor;

void main()
{
    float t = clamp(heightFactor, 0.0, 1.0);

    vec3 baseColor = vec3(1.0, 0.95, 0.55);
    vec3 tipColor = vec3(0.95, 0.15, 0.02);
    vec3 color = mix(baseColor, tipColor, t);

    float flicker = 0.85 + 0.15 * sin(time * 18.0 + heightFactor * 12.0);
    color *= flicker;

    float alpha = mix(0.95, 0.15, t);

    outColor = vec4(color, alpha);
}
