#version 410 core

in vec3 texCoord;

uniform sampler2D moonTexture;
uniform int hasMoonTexture;

uniform vec3 zenithColor;
uniform vec3 horizonColor;
uniform vec3 groundColor;

uniform vec3 lightDir;
uniform vec3 lightColor;

uniform float celestialSize;
uniform float celestialGlow;
uniform float celestialIntensity;

uniform int starsEnabled;
uniform float starDensity;
uniform float starIntensity;
uniform float starRotation;

out vec4 outColor;

float Hash(vec3 p)
{
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

vec3 RotateY(vec3 v, float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    return vec3(v.x * c + v.z * s, v.y, -v.x * s + v.z * c);
}

vec3 StarLayer(vec3 dir, float scale, float density, float brightness, float size)
{
    vec3 scaledDir = dir * scale;
    vec3 cellId = floor(scaledDir);
    vec3 total = vec3(0.0);

    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            for (int z = -1; z <= 1; z++)
            {
                vec3 neighbor = cellId + vec3(float(x), float(y), float(z));
                float h = Hash(neighbor);
                if (h < density)
                    continue;

                vec3 jitter = vec3(Hash(neighbor + 11.1), Hash(neighbor + 23.7), Hash(neighbor + 47.3));
                vec3 starDir = normalize((neighbor + jitter) / scale);

                float d = length(dir - starDir);
                float starSize = size * (0.5 + 0.3 * Hash(neighbor + 5.0));

                float core = smoothstep(starSize, starSize * 0.2, d);
                float tint = 0.9 + 0.1 * Hash(neighbor + 3.3);

                total += vec3(tint * 0.95, tint * 0.97, tint) * core * brightness;
            }
        }
    }

    return total;
}

void main()
{
    vec3 dir = normalize(texCoord);

    float skyBlend = smoothstep(-0.05, 0.35, dir.y);
    vec3 skyColor = mix(horizonColor, zenithColor, skyBlend);
    skyColor = mix(groundColor, skyColor, smoothstep(-0.2, 0.02, dir.y));

    vec3 forward = normalize(lightDir);
    float alignment = max(dot(dir, forward), 0.0);

    vec3 celestial = vec3(0.0);
    if (alignment > 0.05)
    {
        vec3 worldUp = abs(forward.y) > 0.99 ? vec3(1.0, 0.0, 0.0) : vec3(0.0, 1.0, 0.0);
        vec3 right = normalize(cross(worldUp, forward));
        vec3 up = cross(forward, right);

        float t = 1.0 / alignment;
        vec3 planePoint = dir * t;
        vec3 localOffset = planePoint - forward;
        vec2 uv = vec2(dot(localOffset, right), dot(localOffset, up)) / (2.0 * celestialSize) + 0.5;

        if (uv.x > 0.0 && uv.x < 1.0 && uv.y > 0.0 && uv.y < 1.0)
        {
            float distFromCenter = length(uv - vec2(0.5));
            float discMask = smoothstep(0.5, 0.47, distFromCenter);
            vec3 moonSample = hasMoonTexture != 0 ? texture(moonTexture, uv).rgb : vec3(1.0);
            celestial = moonSample * lightColor * celestialIntensity * discMask;
        }
    }

    float glow = pow(alignment, celestialGlow);
    celestial += lightColor * celestialIntensity * glow * 0.5;

    vec3 finalColor = skyColor + celestial;

    if (starsEnabled != 0)
    {
        vec3 stars = StarLayer(RotateY(dir, starRotation * 1.0), 55.0, starDensity, 1.6, 0.010);
        stars += StarLayer(RotateY(dir, starRotation * 0.55), 130.0, starDensity + 0.001, 1.0, 0.005);
        stars += StarLayer(RotateY(dir, starRotation * 1.4), 240.0, starDensity + 0.002, 0.65, 0.003);
        stars += StarLayer(RotateY(dir, starRotation * 0.25), 420.0, starDensity + 0.0028, 0.4, 0.0017);

        stars *= smoothstep(0.0, 0.3, dir.y);
        stars *= (1.0 - clamp(glow * 2.0, 0.0, 1.0));

        finalColor += stars * starIntensity;
    }

    outColor = vec4(finalColor, 1.0);
}
