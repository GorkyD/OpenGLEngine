#version 410 core

in vec2 fragUv;

uniform sampler2D sceneColor;
uniform float exposure;
uniform float vignetteStrength;
uniform float vignetteRadius;
uniform float vignetteSoftness;

uniform vec2 psxResolution;
uniform float psxColorLevels;
uniform float psxDitherStrength;

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

float Bayer4(vec2 pixel)
{
    int x = int(mod(pixel.x, 4.0));
    int y = int(mod(pixel.y, 4.0));
    int index = x + y * 4;
    const float pattern[16] = float[16](0.0, 8.0, 2.0, 10.0, 12.0, 4.0, 14.0, 6.0, 3.0, 11.0, 1.0, 9.0, 15.0, 7.0, 13.0, 5.0);
    return pattern[index] / 16.0 - 0.5;
}

void main()
{
    vec2 uv = fragUv;
    vec2 virtualPixel = gl_FragCoord.xy;

    if (psxResolution.y > 0.0)
    {
        virtualPixel = floor(fragUv * psxResolution);
        uv = (virtualPixel + 0.5) / psxResolution;
    }

    vec3 color = texture(sceneColor, uv).rgb * exposure;
    color = ACESFilm(color);

    float dist = length(fragUv - vec2(0.5)) * 2.0;
    float vig = 1.0 - vignetteStrength * smoothstep(vignetteRadius - vignetteSoftness, vignetteRadius, dist);
    color *= vig;

    if (psxColorLevels > 1.0)
    {
        float steps = psxColorLevels - 1.0;
        color += Bayer4(virtualPixel) * psxDitherStrength / steps;
        color = floor(clamp(color, 0.0, 1.0) * steps + 0.5) / steps;
    }

    outColor = vec4(color, 1.0);
}
