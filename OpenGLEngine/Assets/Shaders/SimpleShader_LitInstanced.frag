#version 410 core

in vec2 fragTexCoord;
in vec3 fragNormal;
in vec3 fragWorldPos;
in vec4 fragDiffuseColor;
in vec3 fragEmissiveColor;
in float fragEmissiveIntensity;
in float fragRoughness;
in float fragMetallic;

uniform sampler2D diffuseTexture;
uniform int hasTexture;

uniform vec3 viewPos;
uniform vec3 ambientColor;
uniform float ambientIntensity;

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

out vec4 outColor;

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

void main()
{
    vec3 norm = normalize(fragNormal);

    vec3 albedo = fragDiffuseColor.rgb;
    if (hasTexture != 0)
        albedo *= texture(diffuseTexture, fragTexCoord).rgb;

    float roughness = clamp(fragRoughness, 0.045, 1.0);
    float metallic = clamp(fragMetallic, 0.0, 1.0);
    float ao = 1.0;

    vec3 viewDir = normalize(viewPos - fragWorldPos);
    vec3 f0 = mix(vec3(0.04), albedo, metallic);

    vec3 lightingOut = vec3(0.0);

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

        lightingOut += (kD * albedo / PI + specular) * radiance * nDotL;
    }

    vec3 ambient = ambientColor * ambientIntensity * albedo * ao;
    vec3 emissive = fragEmissiveColor * fragEmissiveIntensity;
    vec3 finalColor = ambient + lightingOut + emissive;

    float dist = length(viewPos - fragWorldPos);
    float fogFactor = clamp((fogEnd - dist) / max(fogEnd - fogStart, 0.0001), 0.0, 1.0);
    finalColor = mix(fogColor, finalColor, fogFactor);

    outColor = vec4(finalColor, 1.0);
}
