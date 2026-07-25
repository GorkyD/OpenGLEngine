#version 410 core

in vec2 fragTexCoord;

uniform sampler2D atlas;
uniform vec4 textColor;

out vec4 outColor;

void main()
{
    float coverage = texture(atlas, fragTexCoord).r;
    outColor = vec4(textColor.rgb, textColor.a * coverage);
}
