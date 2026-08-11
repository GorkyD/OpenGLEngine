#version 410 core

in vec2 fragTexCoord;

uniform sampler2D diffuseTexture;
uniform int hasTexture;
uniform vec4 diffuseColor;

void main()
{
    vec4 sampledColor = texture(diffuseTexture, fragTexCoord);
    vec4 texColor = mix(vec4(1.0), sampledColor, float(hasTexture));

    if (texColor.a * diffuseColor.a < 0.5)
        discard;
}
