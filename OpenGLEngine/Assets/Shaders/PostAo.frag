#version 410 core

in vec2 fragUv;

uniform sampler2D sceneDepth;
uniform mat4 invProjection;
uniform mat4 projection;
uniform vec2 texelSize;
uniform float radius;
uniform float intensity;
uniform float frame;

out vec4 outColor;

vec3 ViewPosition(vec2 uv)
{
    float depth = texture(sceneDepth, uv).r;
    vec4 clip = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 view = invProjection * clip;
    return view.xyz / view.w;
}

float Noise(vec2 pixel)
{
    return fract(52.9829189 * fract(dot(pixel, vec2(0.06711056, 0.00583715))));
}

void main()
{
    float depth = texture(sceneDepth, fragUv).r;
    if (depth >= 1.0)
    {
        outColor = vec4(1.0);
        return;
    }
    vec3 position = ViewPosition(fragUv);
    vec3 right = ViewPosition(fragUv + vec2(texelSize.x, 0.0)) - position;
    vec3 left = position - ViewPosition(fragUv - vec2(texelSize.x, 0.0));
    vec3 up = ViewPosition(fragUv + vec2(0.0, texelSize.y)) - position;
    vec3 down = position - ViewPosition(fragUv - vec2(0.0, texelSize.y));
    vec3 dx = abs(right.z) < abs(left.z) ? right : left;
    vec3 dy = abs(up.z) < abs(down.z) ? up : down;
    vec3 normal = normalize(cross(dy, dx));
    if (dot(normal, position) > 0.0)
        normal = -normal;

    vec4 projected = projection * vec4(position + vec3(radius, 0.0, 0.0), 1.0);
    vec4 center = projection * vec4(position, 1.0);
    float screenRadius = abs(projected.x / projected.w - center.x / center.w) * 0.5;
    screenRadius = clamp(screenRadius, texelSize.x * 2.0, 0.12);

    const int samples = 12;
    float rotation = Noise(gl_FragCoord.xy + frame * 7.0) * 6.2831853;
    float occlusion = 0.0;
    for (int i = 0; i < samples; i++)
    {
        float t = (float(i) + 0.5) / float(samples);
        float angle = rotation + float(i) * 2.3999632;
        vec2 offset = vec2(cos(angle), sin(angle)) * screenRadius * t;
        vec3 probe = ViewPosition(fragUv + offset);
        vec3 toSample = probe - position;
        float distanceSq = dot(toSample, toSample);
        float facing = max(dot(toSample * inversesqrt(distanceSq + 0.000001), normal) - 0.15, 0.0);
        float falloff = max(0.0, 1.0 - distanceSq / (radius * radius));
        occlusion += facing * falloff;
    }
    occlusion = clamp(1.0 - intensity * occlusion / float(samples) * 1.6, 0.0, 1.0);
    outColor = vec4(occlusion, occlusion, occlusion, 1.0);
}
