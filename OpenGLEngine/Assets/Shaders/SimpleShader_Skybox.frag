#version 410 core

in vec3 texCoord;

uniform samplerCube skyboxTexture;
uniform vec3 fogColor;
uniform float fogHorizonSpread;
uniform float fogHorizonIntensity;

out vec4 outColor;

void main()
{
    vec4 skyColor = texture(skyboxTexture, texCoord);

    vec3 dir = normalize(texCoord);
    float haze = (1.0 - smoothstep(0.0, fogHorizonSpread, abs(dir.y))) * fogHorizonIntensity;

    outColor = vec4(mix(skyColor.rgb, fogColor, haze), skyColor.a);
}
