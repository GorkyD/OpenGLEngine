#include "Resource/TerrainGenerator.h"
#include <vector>
#include "Ecs/Components/TerrainComponent.h"
#include "Math/Noise.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Render/RenderEngine.h"

namespace
{
struct TerrainVertex
{
    Vector3 position;
    Vector2 uv;
    Vector3 normal;
};
} // namespace

void TerrainGenerator::Generate(RenderEngine* renderEngine, const Params& params, TerrainComponent& outTerrain)
{
    const int resolution = params.resolution;
    const int stride = resolution + 1;
    const float half = params.worldSize * 0.5f;
    const float cellSize = params.worldSize / static_cast<float>(resolution);

    std::vector<float> heights(static_cast<size_t>(stride) * stride);
    for (int j = 0; j < stride; j++)
    {
        for (int i = 0; i < stride; i++)
        {
            const float worldX = -half + static_cast<float>(i) * cellSize;
            const float worldZ = -half + static_cast<float>(j) * cellSize;
            const float n = Noise::Fractal2D(worldX * params.noiseFrequency, worldZ * params.noiseFrequency, params.octaves, params.persistence, params.seed);
            heights[j * stride + i] = (n * 2.0f - 1.0f) * params.heightScale;
        }
    }

    auto heightAt = [&](int i, int j) -> float
    {
        i = i < 0 ? 0 : (i > resolution ? resolution : i);
        j = j < 0 ? 0 : (j > resolution ? resolution : j);
        return heights[j * stride + i];
    };

    std::vector<TerrainVertex> vertices(static_cast<size_t>(stride) * stride);
    for (int j = 0; j < stride; j++)
    {
        for (int i = 0; i < stride; i++)
        {
            const float worldX = -half + static_cast<float>(i) * cellSize;
            const float worldZ = -half + static_cast<float>(j) * cellSize;
            const float h = heights[j * stride + i];

            const float hL = heightAt(i - 1, j);
            const float hR = heightAt(i + 1, j);
            const float hD = heightAt(i, j - 1);
            const float hU = heightAt(i, j + 1);

            const float dX = (hR - hL) / (2.0f * cellSize);
            const float dZ = (hU - hD) / (2.0f * cellSize);
            const Vector3 normal = Vector3::Normalize({-dX, 1.0f, -dZ});

            const float u = (static_cast<float>(i) / static_cast<float>(resolution)) * params.uvTiling;
            const float v = (static_cast<float>(j) / static_cast<float>(resolution)) * params.uvTiling;

            vertices[j * stride + i] = {{worldX, h, worldZ}, {u, v}, normal};
        }
    }

    std::vector<unsigned int> indices;
    indices.reserve(static_cast<size_t>(resolution) * resolution * 6);
    for (int j = 0; j < resolution; j++)
    {
        for (int i = 0; i < resolution; i++)
        {
            const unsigned int a = j * stride + i;
            const unsigned int b = a + stride;

            indices.push_back(a);
            indices.push_back(a + 1);
            indices.push_back(b);

            indices.push_back(a + 1);
            indices.push_back(b + 1);
            indices.push_back(b);
        }
    }

    VertexAttributes attrs[] = {{3}, {2}, {3}};
    outTerrain.vao = renderEngine->CreateVertexArrayObject({static_cast<void*>(vertices.data()), sizeof(TerrainVertex), static_cast<int>(vertices.size()), attrs, 3},
                                                            {static_cast<void*>(indices.data()), static_cast<int>(indices.size() * sizeof(unsigned int))});

    outTerrain.indexCount = static_cast<unsigned int>(indices.size());
    outTerrain.resolution = resolution;
    outTerrain.worldSize = params.worldSize;
    outTerrain.heightScale = params.heightScale;
    outTerrain.heights = std::move(heights);
}
