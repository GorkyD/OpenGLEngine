#version 410 core

uniform vec3 outlineColor;

out vec4 outColor;

void main()
{
    outColor = vec4(outlineColor, 1.0);
}
