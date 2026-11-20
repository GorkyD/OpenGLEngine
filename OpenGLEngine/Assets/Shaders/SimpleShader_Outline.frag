#version 410 core

uniform vec3 outlineColor;
uniform int useSceneDepth;
uniform sampler2D sceneDepth;
uniform vec2 screenSize;

in vec3 hullNormal;
in vec3 hullPosition;
in vec3 cameraPosition;

out vec4 outColor;

void main()
{
    if (dot(hullNormal, cameraPosition - hullPosition) > 0.0)
        discard;
    if (useSceneDepth != 0)
    {
        vec2 uv = gl_FragCoord.xy / screenSize;
        vec2 texel = 1.0 / vec2(textureSize(sceneDepth, 0));
        float farthest = 0.0;
        for (int x = -1; x <= 1; x++)
        {
            for (int y = -1; y <= 1; y++)
                farthest = max(farthest, texture(sceneDepth, uv + vec2(x, y) * texel).r);
        }
        if (gl_FragCoord.z > farthest + 0.00002)
            discard;
    }
    outColor = vec4(outlineColor, 1.0);
}
