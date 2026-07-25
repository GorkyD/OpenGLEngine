#version 410 core

in vec3 texCoord;

uniform samplerCube skyboxTexture;

out vec4 outColor;

void main()
{
    outColor = texture(skyboxTexture, texCoord);
}
