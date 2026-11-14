#version 410 core

in vec2 fragUv;

uniform sampler2D sceneDepth;
uniform sampler2D shadowMap;
uniform mat4 invProjection;
uniform mat4 invView;
uniform mat4 lightSpaceMatrix;
uniform vec3 cameraPosition;
uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform float density;
uniform float maxDistance;
uniform int steps;
uniform float time;

out vec4 outColor;

float Hash(vec3 p)
{
    p = fract(p * 0.3183099 + vec3(0.1, 0.2, 0.3));
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float ValueNoise(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(Hash(i), Hash(i + vec3(1, 0, 0)), f.x), mix(Hash(i + vec3(0, 1, 0)), Hash(i + vec3(1, 1, 0)), f.x), f.y),
               mix(mix(Hash(i + vec3(0, 0, 1)), Hash(i + vec3(1, 0, 1)), f.x), mix(Hash(i + vec3(0, 1, 1)), Hash(i + vec3(1, 1, 1)), f.x), f.y), f.z);
}

float Lit(vec3 worldPosition)
{
    vec4 light = lightSpaceMatrix * vec4(worldPosition, 1.0);
    vec3 projected = light.xyz / light.w * 0.5 + 0.5;
    if (projected.x <= 0.0 || projected.x >= 1.0 || projected.y <= 0.0 || projected.y >= 1.0 || projected.z >= 1.0)
        return 0.0;
    return projected.z - 0.003 > texture(shadowMap, projected.xy).r ? 0.0 : 1.0;
}

void main()
{
    float depth = texture(sceneDepth, fragUv).r;
    vec4 clip = vec4(fragUv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 view = invProjection * clip;
    vec3 world = (invView * vec4(view.xyz / view.w, 1.0)).xyz;
    vec3 ray = world - cameraPosition;
    float rayLength = min(length(ray), maxDistance);
    vec3 direction = normalize(ray);

    float g = 0.55;
    float cosTheta = dot(direction, normalize(sunDirection));
    float phase = (1.0 - g * g) / (4.0 * 3.14159265 * pow(1.0 + g * g - 2.0 * g * cosTheta, 1.5));

    float jitter = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))) + time * 0.618);
    float stepLength = rayLength / float(steps);
    float scattered = 0.0;
    for (int i = 0; i < steps; i++)
    {
        vec3 position = cameraPosition + direction * (stepLength * (float(i) + jitter));
        float dust = 0.55 + 0.9 * ValueNoise(position * 7.0 + vec3(0.0, time * 0.05, time * 0.03));
        scattered += Lit(position) * dust * stepLength;
    }
    outColor = vec4(sunColor * scattered * density * phase * 12.566, 1.0);
}
