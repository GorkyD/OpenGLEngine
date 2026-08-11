#pragma once

class Noise
{
public:
    static float Fractal2D(float x, float z, int octaves, float persistence, int seed)
    {
        float total = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float maxValue = 0.0f;

        for (int i = 0; i < octaves; i++)
        {
            total += Value2D(x * frequency, z * frequency, seed + i * 101) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= 2.0f;
        }

        return maxValue > 0.0f ? total / maxValue : 0.0f;
    }

private:
    static float Hash(int x, int z, int seed)
    {
        unsigned int n = static_cast<unsigned int>(x * 374761393 + z * 668265263 + seed * 2246822519u);
        n = (n ^ (n >> 13)) * 1274126177u;
        n = n ^ (n >> 16);
        return (n & 0x7fffffff) / static_cast<float>(0x7fffffff);
    }

    static float Value2D(float x, float z, int seed)
    {
        const int x0 = static_cast<int>(x >= 0.0f ? x : x - 1.0f);
        const int z0 = static_cast<int>(z >= 0.0f ? z : z - 1.0f);
        const int x1 = x0 + 1;
        const int z1 = z0 + 1;

        const float tx = x - static_cast<float>(x0);
        const float tz = z - static_cast<float>(z0);

        const float h00 = Hash(x0, z0, seed);
        const float h10 = Hash(x1, z0, seed);
        const float h01 = Hash(x0, z1, seed);
        const float h11 = Hash(x1, z1, seed);

        const float sx = tx * tx * (3.0f - 2.0f * tx);
        const float sz = tz * tz * (3.0f - 2.0f * tz);

        const float top = h00 + sx * (h10 - h00);
        const float bottom = h01 + sx * (h11 - h01);
        return top + sz * (bottom - top);
    }
};
