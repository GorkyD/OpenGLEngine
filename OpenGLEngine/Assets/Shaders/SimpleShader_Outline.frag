#version 410 core

uniform vec3 outlineColor;
uniform int useSceneDepth;
uniform sampler2D sceneDepth;
uniform vec2 screenSize;
uniform int maskPass;

in vec3 hullNormal;
in vec3 hullPosition;
in vec3 cameraPosition;

out vec4 outColor;

void main()
{
    if (maskPass == 0 && dot(hullNormal, cameraPosition - hullPosition) > 0.0)
        discard;
    if (useSceneDepth != 0)
    {
        vec2 uv = gl_FragCoord.xy / screenSize;
        if (gl_FragCoord.z > texture(sceneDepth, uv).r + 0.00004)
            discard;
    }
    outColor = vec4(outlineColor, 1.0);
}
