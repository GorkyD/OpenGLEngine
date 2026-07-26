#include "Resource/MeshFactory.h"
#include <cmath>
#include <vector>
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Render/RenderEngine.h"

namespace
{
struct SphereVertex
{
    Vector3 position;
    Vector2 uv;
    Vector3 normal;
};
} // namespace

VertexArrayObjectPtr MeshFactory::CreateSphere(RenderEngine* renderEngine, unsigned int& outIndexCount, int rings, int sectors, float radius)
{
    std::vector<SphereVertex> vertices;
    vertices.reserve(static_cast<size_t>(rings + 1) * (sectors + 1));

    constexpr float pi = 3.14159265358979323846f;

    for (int r = 0; r <= rings; r++)
    {
        const float v = static_cast<float>(r) / static_cast<float>(rings);
        const float phi = v * pi;
        const float y = std::cos(phi);
        const float ringRadius = std::sin(phi);

        for (int s = 0; s <= sectors; s++)
        {
            const float u = static_cast<float>(s) / static_cast<float>(sectors);
            const float theta = u * 2.0f * pi;
            const float x = ringRadius * std::cos(theta);
            const float z = ringRadius * std::sin(theta);

            SphereVertex vertex;
            vertex.normal = {x, y, z};
            vertex.position = {x * radius, y * radius, z * radius};
            vertex.uv = {u, v};
            vertices.push_back(vertex);
        }
    }

    std::vector<unsigned int> indices;
    indices.reserve(static_cast<size_t>(rings) * sectors * 6);

    const int stride = sectors + 1;
    for (int r = 0; r < rings; r++)
    {
        for (int s = 0; s < sectors; s++)
        {
            const unsigned int a = r * stride + s;
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
    const auto vao = renderEngine->CreateVertexArrayObject({static_cast<void*>(vertices.data()), sizeof(SphereVertex), static_cast<int>(vertices.size()), attrs, 3},
                                                           {static_cast<void*>(indices.data()), static_cast<int>(indices.size() * sizeof(unsigned int))});

    outIndexCount = static_cast<unsigned int>(indices.size());
    return vao;
}
