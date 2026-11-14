#version 410 core

in vec2 fragTexCoord;
in vec3 fragNormal;
in vec3 fragWorldPos;
in vec4 fragVertexColor;

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

uniform vec3 emissiveColor;
uniform float emissiveIntensity;

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

uniform int bypassSpotCone;

uniform sampler2D shadowMap;

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

vec3 FresnelSchlick(float cosTheta, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 f0, float roughness)
{
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(float nDotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float denom = nDotH * nDotH * (a2 - 1.0) + 1.0;
    return a2 / max(PI * denom * denom, 0.0001);
}

float GeometrySchlick(float nDotX, float roughness)
{
    float r = roughness + 1.0;
    float k = r * r / 8.0;
    return nDotX / (nDotX * (1.0 - k) + k);
}

vec3 EnvironmentRadiance(vec3 dir, float roughness, vec3 base)
{
    vec3 ceiling = base * 3.4 + vec3(0.05, 0.045, 0.04);
    vec3 horizon = base * 1.5;
    vec3 ground = base * 0.55;
    vec3 color = dir.y > 0.0 ? mix(horizon, ceiling, smoothstep(0.0, 0.75, dir.y)) : mix(horizon, ground, smoothstep(0.0, 0.5, -dir.y));
    float sharpness = (1.0 - roughness) * (1.0 - roughness);
    float bands = smoothstep(0.92, 1.0, sin(dir.x * 9.0 + dir.z * 4.0) * 0.5 + 0.5) * smoothstep(0.1, 0.5, dir.y);
    color += (base * 6.0 + vec3(0.08, 0.075, 0.07)) * bands * sharpness;
    vec3 average = (ceiling + horizon * 2.0 + ground) * 0.25;
    return mix(color, average, clamp(roughness * roughness * 1.4, 0.0, 1.0));
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

    if (texColor.a * diffuseColor.a < 0.5)
        discard;

    vec3 albedo = texColor.rgb * diffuseColor.rgb * fragVertexColor.rgb;

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
        int type = int(lightDirectionType[i].w);
        bool isDirectional = (type == 0);

        vec3 toLight = lightPositionRange[i].xyz - fragWorldPos;
        float dist = length(toLight);
        vec3 pointDir = toLight / max(dist, 0.0001);
        vec3 dirLightDir = normalize(lightDirectionType[i].xyz);
        vec3 lightDir = isDirectional ? dirLightDir : pointDir;

        float rangeAttenuation = clamp(1.0 - dist / max(lightPositionRange[i].w, 0.0001), 0.0, 1.0);
        float attenuation = isDirectional ? 1.0 : rangeAttenuation;

        if (type == 2 && bypassSpotCone == 0)
        {
            vec3 spotDir = normalize(lightDirectionType[i].xyz);
            float cosAngle = dot(-pointDir, spotDir);
            float spotFactor = clamp((cosAngle - lightCone[i].y) / max(lightCone[i].x - lightCone[i].y, 0.0001), 0.0, 1.0);
            attenuation *= spotFactor;
        }

        vec3 radiance = lightColorIntensity[i].rgb * lightColorIntensity[i].w * attenuation;

        vec3 halfVec = normalize(viewDir + lightDir);
        vec3 fresnel = FresnelSchlick(max(dot(halfVec, viewDir), 0.0), f0);

        vec3 kD = (vec3(1.0) - fresnel) * (1.0 - metallic);
        float nDotL = max(dot(norm, lightDir), 0.0);
        float nDotV = max(dot(norm, viewDir), 0.001);
        float nDotH = max(dot(norm, halfVec), 0.0);
        vec3 specular = DistributionGGX(nDotH, roughness) * GeometrySchlick(nDotV, roughness) * GeometrySchlick(nDotL, roughness) * fresnel / max(4.0 * nDotV * nDotL, 0.001);

        float shadowFactor = 1.0;
        if (i == shadowLightIndex)
        {
            shadowFactor = ShadowFactor(fragWorldPos, norm, nDotL);
            sunShadowFactor = shadowFactor;
        }

        lightingOut += (kD * albedo / PI + specular) * radiance * nDotL * shadowFactor;
    }

    vec3 environmentBase = ambientColor * ambientIntensity;
    float occlusion = ao * mix(shadowAmbientOcclusion, 1.0, sunShadowFactor);
    float viewCos = max(dot(norm, viewDir), 0.001);
    vec3 ambientFresnel = FresnelSchlickRoughness(viewCos, f0, roughness);
    vec3 ambientDiffuse = (vec3(1.0) - ambientFresnel) * (1.0 - metallic);
    vec3 reflected = reflect(-viewDir, norm);
    vec3 ambient = environmentBase * albedo * ambientDiffuse * occlusion;
    ambient += EnvironmentRadiance(reflected, roughness, environmentBase) * ambientFresnel * occlusion;
    vec3 emissive = emissiveColor * emissiveIntensity;
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
