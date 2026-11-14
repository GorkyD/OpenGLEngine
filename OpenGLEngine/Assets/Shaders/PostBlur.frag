#version 410 core

in vec2 fragUv;

uniform sampler2D source;
uniform sampler2D sceneDepth;
uniform vec2 direction;
uniform int depthAware;
uniform int brightPass;
uniform float threshold;

out vec4 outColor;

vec3 Fetch(vec2 uv)
{
    vec3 color = texture(source, uv).rgb;
    if (brightPass != 0)
    {
        float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
        color *= clamp((luminance - threshold) / max(luminance, 0.0001), 0.0, 1.0);
    }
    return color;
}

void main()
{
    const float weights[5] = float[5](0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);
    vec3 sum = Fetch(fragUv) * weights[0];
    float total = weights[0];
    float centerDepth = depthAware != 0 ? texture(sceneDepth, fragUv).r : 0.0;
    for (int i = 1; i < 5; i++)
    {
        for (int side = -1; side <= 1; side += 2)
        {
            vec2 uv = fragUv + direction * float(i * side);
            float weight = weights[i];
            if (depthAware != 0)
            {
                float d = texture(sceneDepth, uv).r;
                weight *= exp(-abs(d - centerDepth) * 4000.0);
            }
            sum += Fetch(uv) * weight;
            total += weight;
        }
    }
    outColor = vec4(sum / total, 1.0);
}
