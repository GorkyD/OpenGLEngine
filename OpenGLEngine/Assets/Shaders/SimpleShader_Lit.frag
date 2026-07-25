#version 410 core

in vec2 fragTexCoord;
in vec3 fragNormal;
in vec3 fragWorldPos;

uniform sampler2D diffuseTexture;
uniform vec4 diffuseColor;
uniform int hasTexture;

uniform vec3 ambientColor;
uniform float ambientIntensity;

#define MAX_LIGHTS 8

// LightType: 0 = Directional, 1 = Point
uniform int numLights;
uniform int lightType[MAX_LIGHTS];
uniform vec3 lightColor[MAX_LIGHTS];
uniform float lightIntensity[MAX_LIGHTS];
uniform vec3 lightDirection[MAX_LIGHTS];
uniform vec3 lightPosition[MAX_LIGHTS];
uniform float lightRange[MAX_LIGHTS];

out vec4 outColor;

void main()
{
    vec3 norm = normalize(fragNormal);
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
