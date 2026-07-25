#version 410 core

in vec2 fragTexCoord;
in vec3 fragNormal;
in vec3 fragWorldPos;

uniform sampler2D diffuseTexture;
uniform sampler2D normalTexture;
uniform vec4 diffuseColor;
uniform int hasTexture;
uniform int hasNormalMap;

uniform vec3 ambientColor;
uniform float ambientIntensity;

#define MAX_LIGHTS 8

uniform int numLights;
uniform int lightType[MAX_LIGHTS];
uniform vec3 lightColor[MAX_LIGHTS];
uniform float lightIntensity[MAX_LIGHTS];
uniform vec3 lightDirection[MAX_LIGHTS];
uniform vec3 lightPosition[MAX_LIGHTS];
uniform float lightRange[MAX_LIGHTS];

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

void main()
{
    vec3 norm = normalize(fragNormal);
    if (hasNormalMap != 0)
    {
        vec3 mapNormal = texture(normalTexture, fragTexCoord).xyz * 2.0 - 1.0;
        mat3 tbn = CotangentFrame(norm, fragWorldPos, fragTexCoord);
        norm = normalize(tbn * mapNormal);
    }

    vec3 lighting = ambientColor * ambientIntensity;

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

        float diff = max(dot(norm, lightDir), 0.0);
        lighting += lightColor[i] * diff * lightIntensity[i] * attenuation;
    }

    vec4 sampledColor = texture(diffuseTexture, fragTexCoord);
    vec4 texColor = mix(vec4(1.0), sampledColor, float(hasTexture));

    outColor = vec4(texColor.rgb * diffuseColor.rgb * lighting, 1.0);
}
