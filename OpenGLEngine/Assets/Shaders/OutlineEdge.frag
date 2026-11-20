#version 410 core

in vec2 fragUv;

uniform sampler2D mask;
uniform vec3 outlineColor;
uniform int radius;

out vec4 outColor;

void main()
{
    vec2 texel = 1.0 / vec2(textureSize(mask, 0));
    if (texture(mask, fragUv).r > 0.5)
        discard;
    float near = 0.0;
    for (int x = -radius; x <= radius; x++)
    {
        for (int y = -radius; y <= radius; y++)
        {
            if (x * x + y * y > radius * radius + 1)
                continue;
            near = max(near, texture(mask, fragUv + vec2(x, y) * texel).r);
        }
    }
    if (near < 0.5)
        discard;
    outColor = vec4(outlineColor, 1.0);
}
