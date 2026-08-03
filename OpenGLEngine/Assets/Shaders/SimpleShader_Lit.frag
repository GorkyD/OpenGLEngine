#version 410 core

in vec2 fragTexCoord;
in vec3 fragNormal;
in vec3 fragWorldPos;

uniform sampler2D diffuseTexture;
uniform sampler2D normalTexture;
uniform sampler2D roughnessTexture;
uniform sampler2D metallicTexture;
uniform sampler2D aoTexture;
uniform vec4 diffuseColor;
uniform int hasTexture;
uniform int hasNormalMap;
uniform int hasRoughnessMap;
uniform int hasMetallicMap;
uniform int hasAoMap;

uniform float roughnessScalar;
uniform float metallicScalar;

uniform vec3 viewPos;
uniform vec3 ambientColor;
uniform float ambientIntensity;

uniform vec3 emissiveColor;
uniform float emissiveIntensity;

uniform vec3 fogColor;
uniform float fogStart;
uniform float fogEnd;

#define MAX_LIGHTS 32
#define PI 3.14159265359

uniform int numLights;
uniform int lightType[MAX_LIGHTS];
uniform vec3 lightColor[MAX_LIGHTS];
uniform float lightIntensity[MAX_LIGHTS];
uniform vec3 lightDirection[MAX_LIGHTS];
uniform vec3 lightPosition[MAX_LIGHTS];
uniform float lightRange[MAX_LIGHTS];

uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;
uniform int shadowLightIndex;
uniform float shadowBias;
uniform float shadowAmbientOcclusion;
uniform float shadowNormalBias;

out vec4 outColor;

mat3 CotangentFrame(vec3 n, vec3 p, vec2 uv)
{
    vec3 dp1 = dFdx(p);
    vec3 dp2 = dFdy(p);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);

    vec3 dp2perp = cross(dp2, n);
    vec3 dp1perp = cross(n, dp1);
    vec3 t = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 b = dp2perp * duv1.y + dp1perp * duv2.y;

    float invmax = inversesqrt(max(dot(t, t), dot(b, b)));
    return mat3(t * invmax, b * invmax, n);
}

float DistributionGGX(vec3 n, vec3 h, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float nDotH = max(dot(n, h), 0.0);
    float denom = (nDotH * nDotH) * (a2 - 1.0) + 1.0;
    return a2 / max(PI * denom * denom, 0.0001);
}

float GeometrySchlickGGX(float nDotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return nDotV / (nDotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 n, vec3 v, vec3 l, float roughness)
{
    float nDotV = max(dot(n, v), 0.0);
    float nDotL = max(dot(n, l), 0.0);
    return GeometrySchlickGGX(nDotV, roughness) * GeometrySchlickGGX(nDotL, roughness);
}

vec3 FresnelSchlick(float cosTheta, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float ShadowFactor(vec3 worldPos, vec3 normal, float nDotL)
{
    float normalOffset = shadowNormalBias * (1.0 - nDotL) + shadowNormalBias * 0.1;
    vec3 offsetWorldPos = worldPos + normal * normalOffset;

    vec4 posLightSpace = lightSpaceMatrix * vec4(offsetWorldPos, 1.0);
    vec3 proj = posLightSpace.xyz / posLightSpace.w;
    proj = proj * 0.5 + 0.5;

    if (proj.z > 1.0)
        return 1.0;

    float bias = max(shadowBias * (1.0 - nDotL), shadowBias * 0.2);
    vec2 texelSize = 0.6 / vec2(textureSize(shadowMap, 0));

    float shadow = 0.0;
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            float pcfDepth = texture(shadowMap, proj.xy + vec2(x, y) * texelSize).r;
            shadow += (proj.z - bias > pcfDepth) ? 0.0 : 1.0;
        }
    }
    return shadow / 9.0;
}

void main()
{
    vec3 norm = normalize(fragNormal);
    if (hasNormalMap != 0)
    {
        vec3 mapNormal = texture(normalTexture, fragTexCoord).xyz * 2.0 - 1.0;
        mat3 tbn = CotangentFrame(norm, fragWorldPos, fragTexCoord);
        norm = normalize(tbn * mapNormal);
    }

    vec4 sampledColor = texture(diffuseTexture, fragTexCoord);
    vec4 texColor = mix(vec4(1.0), sampledColor, float(hasTexture));
    vec3 albedo = texColor.rgb * diffuseColor.rgb;

    float roughness = mix(roughnessScalar, texture(roughnessTexture, fragTexCoord).r, float(hasRoughnessMap));
    float metallic = mix(metallicScalar, texture(metallicTexture, fragTexCoord).r, float(hasMetallicMap));
    float ao = mix(1.0, texture(aoTexture, fragTexCoord).r, float(hasAoMap));
    roughness = clamp(roughness, 0.045, 1.0);

    vec3 viewDir = normalize(viewPos - fragWorldPos);
    vec3 f0 = mix(vec3(0.04), albedo, metallic);

    vec3 lightingOut = vec3(0.0);
    float sunShadowFactor = 1.0;

    for (int i = 0; i < numLights; i++)
    {
        float isPoint = float(lightType[i]);

        vec3 toLight = lightPosition[i] - fragWorldPos;
        float dist = length(toLight);
        vec3 pointDir = toLight / max(dist, 0.0001);
        vec3 dirLightDir = normalize(lightDirection[i]);
        vec3 lightDir = mix(dirLightDir, pointDir, isPoint);

        float pointAttenuation = clamp(1.0 - dist / max(lightRange[i], 0.0001), 0.0, 1.0);
        float attenuation = mix(1.0, pointAttenuation, isPoint);
        vec3 radiance = lightColor[i] * lightIntensity[i] * attenuation;

        vec3 halfVec = normalize(viewDir + lightDir);
        float ndf = DistributionGGX(norm, halfVec, roughness);
        float geo = GeometrySmith(norm, viewDir, lightDir, roughness);
        vec3 fresnel = FresnelSchlick(max(dot(halfVec, viewDir), 0.0), f0);

        vec3 specular = (ndf * geo * fresnel) /
                        max(4.0 * max(dot(norm, viewDir), 0.0) * max(dot(norm, lightDir), 0.0), 0.0001);

        vec3 kD = (vec3(1.0) - fresnel) * (1.0 - metallic);
        float nDotL = max(dot(norm, lightDir), 0.0);

        float shadowFactor = 1.0;
        if (i == shadowLightIndex)
        {
            shadowFactor = ShadowFactor(fragWorldPos, norm, nDotL);
            sunShadowFactor = shadowFactor;
        }

        lightingOut += (kD * albedo / PI + specular) * radiance * nDotL * shadowFactor;
    }

    vec3 ambient = ambientColor * ambientIntensity * albedo * ao * mix(shadowAmbientOcclusion, 1.0, sunShadowFactor);
    vec3 emissive = emissiveColor * emissiveIntensity;
    vec3 finalColor = ambient + lightingOut + emissive;

    float dist = length(viewPos - fragWorldPos);
    float fogFactor = clamp((fogEnd - dist) / max(fogEnd - fogStart, 0.0001), 0.0, 1.0);
    finalColor = mix(fogColor, finalColor, fogFactor);

    outColor = vec4(finalColor, 1.0);
}
