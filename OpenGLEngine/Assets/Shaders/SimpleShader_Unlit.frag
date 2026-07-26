#version 410 core

in vec2 fragTexCoord;
in vec3 fragNormal;
in vec3 fragWorldPos;

uniform sampler2D diffuseTexture;
uniform vec4 diffuseColor;
uniform int hasTexture;

out vec4 outColor;

void main()
{
    vec4 texColor = vec4(1.0);
    if (hasTexture != 0)
        texColor = texture(diffuseTexture, fragTexCoord);

    outColor = vec4(texColor.rgb * diffuseColor.rgb, diffuseColor.a);
}
