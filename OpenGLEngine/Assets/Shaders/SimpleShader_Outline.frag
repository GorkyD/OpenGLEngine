#version 410 core

uniform vec3 outlineColor;

in vec3 hullNormal;
in vec3 hullPosition;
in vec3 cameraPosition;

out vec4 outColor;

void main()
{
    if (dot(hullNormal, cameraPosition - hullPosition) > 0.0)
        discard;
    outColor = vec4(outlineColor, 1.0);
}
