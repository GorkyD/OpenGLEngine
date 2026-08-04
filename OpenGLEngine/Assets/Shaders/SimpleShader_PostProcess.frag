#version 410 core

in vec2 fragUv;

uniform sampler2D sceneColor;
uniform float exposure;
uniform float vignetteStrength;
uniform float vignetteRadius;
uniform float vignetteSoftness;

out vec4 outColor;

vec3 ACESFilm(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
    vec3 color = texture(sceneColor, fragUv).rgb * exposure;
    color = ACESFilm(color);

    float dist = length(fragUv - vec2(0.5)) * 2.0;
    float vig = 1.0 - vignetteStrength * smoothstep(vignetteRadius - vignetteSoftness, vignetteRadius, dist);
    color *= vig;

    outColor = vec4(color, 1.0);
}
