#version 410 core

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D atlas;

out vec4 outColor;

void main()
{
    float coverage = texture(atlas, fragTexCoord).r;
    outColor = vec4(fragColor.rgb, fragColor.a * coverage);
}
