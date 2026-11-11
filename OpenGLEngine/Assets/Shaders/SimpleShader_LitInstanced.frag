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

#define MAX_LIGHTS 32
#define PI 3.14159265359

layout(std140) uniform FrameData
{
    mat4 lightSpaceMatrix;
    vec4 lightColorIntensity[MAX_LIGHTS];
    vec4 lightPositionRange[MAX_LIGHTS];
    vec4 lightDirectionType[MAX_LIGHTS];
    vec4 lightCone[MAX_LIGHTS];
    vec3 viewPos;
    float ambientIntensity;
    vec3 ambientColor;
    float fogStart;
    vec3 fogColor;
    float fogEnd;
    vec3 fogRadialOrigin;
    float fogRadialStart;
    float fogRadialEnd;
    float fogGlowExponent;
    float fogGlowStrength;
    int fogRadialEnabled;
    int numLights;
    int shadowLightIndex;
    float shadowBias;
    float shadowAmbientOcclusion;
    float shadowNormalBias;
};

uniform sampler2D shadowMap;

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

    vec4 texColor = hasTexture != 0 ? texture(diffuseTexture, fragTexCoord) : vec4(1.0);
    if (texColor.a * fragDiffuseColor.a < 0.5)
        discard;

    vec3 albedo = fragDiffuseColor.rgb * texColor.rgb;

    float roughness = clamp(fragRoughness, 0.045, 1.0);
    float metallic = clamp(fragMetallic, 0.0, 1.0);
    float ao = 1.0;

    vec3 viewDir = normalize(viewPos - fragWorldPos);
    vec3 f0 = mix(vec3(0.04), albedo, metallic);

    vec3 lightingOut = vec3(0.0);
    float sunShadowFactor = 1.0;

    for (int i = 0; i < numLights; i++)
    {
        int type = int(lightDirectionType[i].w);
        bool isDirectional = (type == 0);

        vec3 toLight = lightPositionRange[i].xyz - fragWorldPos;
        float dist = length(toLight);
        vec3 pointDir = toLight / max(dist, 0.0001);
        vec3 dirLightDir = normalize(lightDirectionType[i].xyz);
        vec3 lightDir = isDirectional ? dirLightDir : pointDir;

        float rangeAttenuation = clamp(1.0 - dist / max(lightPositionRange[i].w, 0.0001), 0.0, 1.0);
        float attenuation = isDirectional ? 1.0 : rangeAttenuation;

        if (type == 2)
        {
            vec3 spotDir = normalize(lightDirectionType[i].xyz);
            float cosAngle = dot(-pointDir, spotDir);
            float spotFactor = clamp((cosAngle - lightCone[i].y) / max(lightCone[i].x - lightCone[i].y, 0.0001), 0.0, 1.0);
            attenuation *= spotFactor;
        }

        vec3 radiance = lightColorIntensity[i].rgb * lightColorIntensity[i].w * attenuation;

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

    if (fogRadialEnabled != 0)
    {
        float radialDist = length(fragWorldPos.xz - fogRadialOrigin.xz);
        float radialFactor = clamp((fogRadialEnd - radialDist) / max(fogRadialEnd - fogRadialStart, 0.0001), 0.0, 1.0);
        fogFactor = min(fogFactor, radialFactor);
    }

    vec3 fogTint = fogColor;
    if (shadowLightIndex >= 0)
    {
        vec3 glowDir = normalize(lightDirectionType[shadowLightIndex].xyz);
        float glow = pow(max(dot(-viewDir, glowDir), 0.0), fogGlowExponent) * fogGlowStrength;
        fogTint = mix(fogColor, lightColorIntensity[shadowLightIndex].rgb * lightColorIntensity[shadowLightIndex].w, clamp(glow, 0.0, 1.0));
    }

    finalColor = mix(fogTint, finalColor, fogFactor);

    outColor = vec4(finalColor, 1.0);
}
