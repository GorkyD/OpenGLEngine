#pragma once

#include <algorithm>
#include <vector>
#include "Extension/Extension.h"

struct TerrainComponent
{
    VertexArrayObjectPtr vao;
    unsigned int indexCount = 0;

    int resolution = 128;
    float worldSize = 100.0f;
    float heightScale = 8.0f;

    std::vector<float> heights;

    float GetHeightAt(float worldX, float worldZ) const
    {
        if (heights.empty())
            return 0.0f;

        const float half = worldSize * 0.5f;
        float gx = (worldX + half) / worldSize * static_cast<float>(resolution);
        float gz = (worldZ + half) / worldSize * static_cast<float>(resolution);

        gx = std::clamp(gx, 0.0f, static_cast<float>(resolution));
        gz = std::clamp(gz, 0.0f, static_cast<float>(resolution));

        const int x0 = static_cast<int>(gx);
        const int z0 = static_cast<int>(gz);
        const int x1 = (std::min)(x0 + 1, resolution);
        const int z1 = (std::min)(z0 + 1, resolution);

        const float tx = gx - static_cast<float>(x0);
        const float tz = gz - static_cast<float>(z0);

        const int stride = resolution + 1;
        const float h00 = heights[z0 * stride + x0];
        const float h10 = heights[z0 * stride + x1];
        const float h01 = heights[z1 * stride + x0];
        const float h11 = heights[z1 * stride + x1];

        const float top = h00 + tx * (h10 - h00);
        const float bottom = h01 + tx * (h11 - h01);
        return top + tz * (bottom - top);
    }
};
