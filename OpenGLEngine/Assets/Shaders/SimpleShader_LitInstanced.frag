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
uniform float fogGlowExponent;
uniform float fogGlowStrength;

#define MAX_LIGHTS 32
#define PI 3.14159265359

uniform int numLights;
uniform int lightType[MAX_LIGHTS];
uniform vec3 lightColor[MAX_LIGHTS];
uniform float lightIntensity[MAX_LIGHTS];
uniform vec3 lightDirection[MAX_LIGHTS];
uniform vec3 lightPosition[MAX_LIGHTS];
uniform float lightRange[MAX_LIGHTS];
uniform float lightInnerCos[MAX_LIGHTS];
uniform float lightOuterCos[MAX_LIGHTS];

uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;
uniform int shadowLightIndex;
uniform float shadowBias;
uniform float shadowAmbientOcclusion;
uniform float shadowNormalBias;

out vec4 outColor;

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

    vec3 albedo = fragDiffuseColor.rgb;
    if (hasTexture != 0)
        albedo *= texture(diffuseTexture, fragTexCoord).rgb;

    float roughness = clamp(fragRoughness, 0.045, 1.0);
    float metallic = clamp(fragMetallic, 0.0, 1.0);
    float ao = 1.0;

    vec3 viewDir = normalize(viewPos - fragWorldPos);
    vec3 f0 = mix(vec3(0.04), albedo, metallic);

    vec3 lightingOut = vec3(0.0);
    float sunShadowFactor = 1.0;

    for (int i = 0; i < numLights; i++)
    {
        int type = lightType[i];
        bool isDirectional = (type == 0);

        vec3 toLight = lightPosition[i] - fragWorldPos;
        float dist = length(toLight);
        vec3 pointDir = toLight / max(dist, 0.0001);
        vec3 dirLightDir = normalize(lightDirection[i]);
        vec3 lightDir = isDirectional ? dirLightDir : pointDir;

        float rangeAttenuation = clamp(1.0 - dist / max(lightRange[i], 0.0001), 0.0, 1.0);
        float attenuation = isDirectional ? 1.0 : rangeAttenuation;

        if (type == 2)
        {
            vec3 spotDir = normalize(lightDirection[i]);
            float cosAngle = dot(-pointDir, spotDir);
            float spotFactor = clamp((cosAngle - lightOuterCos[i]) / max(lightInnerCos[i] - lightOuterCos[i], 0.0001), 0.0, 1.0);
            attenuation *= spotFactor;
        }

        vec3 radiance = lightColor[i] * lightIntensity[i] * attenuation;

        vec3 halfVec = normalize(viewDir + lightDir);
        vec3 fresnel = FresnelSchlick(max(dot(halfVec, viewDir), 0.0), f0);

        vec3 specular = vec3(0.0);

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
    vec3 emissive = fragEmissiveColor * fragEmissiveIntensity;
    vec3 finalColor = ambient + lightingOut + emissive;

    float dist = length(viewPos - fragWorldPos);
    float fogFactor = clamp((fogEnd - dist) / max(fogEnd - fogStart, 0.0001), 0.0, 1.0);

    vec3 fogTint = fogColor;
    if (shadowLightIndex >= 0)
    {
        vec3 glowDir = normalize(lightDirection[shadowLightIndex]);
        float glow = pow(max(dot(-viewDir, glowDir), 0.0), fogGlowExponent) * fogGlowStrength;
        fogTint = mix(fogColor, lightColor[shadowLightIndex] * lightIntensity[shadowLightIndex], clamp(glow, 0.0, 1.0));
    }

    finalColor = mix(fogTint, finalColor, fogFactor);

    outColor = vec4(finalColor, 1.0);
}
