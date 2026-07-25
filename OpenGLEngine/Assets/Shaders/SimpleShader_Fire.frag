#version 410 core

in float heightFactor;
in vec3 localPos;

uniform float time;

out vec4 outColor;

float hash(float n)
{
    return fract(sin(n) * 43758.5453123);
}

void main()
{
    float t = clamp(heightFactor, 0.0, 1.0);
    float angle = atan(localPos.z, localPos.x);

    vec3 coreColor = vec3(1.0, 0.97, 0.75);
    vec3 midColor = vec3(1.0, 0.55, 0.08);
    vec3 tipColor = vec3(0.7, 0.1, 0.02);

    vec3 color = mix(coreColor, midColor, smoothstep(0.0, 0.45, t));
    color = mix(color, tipColor, smoothstep(0.4, 1.0, t));

    float strandFast = hash(floor(angle * 3.0) + floor(time * 7.0));
    float strandSlow = hash(floor(angle * 2.0) + floor(time * 2.2));

    float flicker = 0.8 + 0.2 * sin(time * 20.0 + heightFactor * 14.0 + strandFast * 6.2831);
    color *= flicker;
    color *= mix(0.65, 1.25, strandFast);

    float tipFade = 1.0 - smoothstep(0.5, 1.0, t);
    float alpha = mix(0.2, 0.95, tipFade);
    alpha *= mix(0.3, 1.0, strandSlow);
    alpha *= mix(0.7, 1.0, strandFast);

    const float emissiveIntensity = 1.4;
    vec3 emissive = color * emissiveIntensity;

    outColor = vec4(emissive, alpha);
}
