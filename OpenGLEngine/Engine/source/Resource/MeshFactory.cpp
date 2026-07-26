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

VertexArrayObjectPtr MeshFactory::CreateCube(RenderEngine* renderEngine, unsigned int& outIndexCount, float size)
{
    struct CubeVertex
    {
        Vector3 position;
        Vector2 uv;
        Vector3 normal;
    };

    const float h = size * 0.5f;

    const Vector3 facePositions[6][4] = {
        {{-h, -h, h}, {h, -h, h}, {h, h, h}, {-h, h, h}},     // +Z
        {{h, -h, -h}, {-h, -h, -h}, {-h, h, -h}, {h, h, -h}}, // -Z
        {{h, -h, h}, {h, -h, -h}, {h, h, -h}, {h, h, h}},     // +X
        {{-h, -h, -h}, {-h, -h, h}, {-h, h, h}, {-h, h, -h}}, // -X
        {{-h, h, h}, {h, h, h}, {h, h, -h}, {-h, h, -h}},     // +Y
        {{-h, -h, -h}, {h, -h, -h}, {h, -h, h}, {-h, -h, h}}, // -Y
    };
    const Vector3 faceNormals[6] = {{0, 0, 1}, {0, 0, -1}, {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}};
    const Vector2 faceUvs[4] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};

    std::vector<CubeVertex> vertices;
    vertices.reserve(24);

    std::vector<unsigned int> indices;
    indices.reserve(36);

    for (int face = 0; face < 6; face++)
    {
        const unsigned int base = static_cast<unsigned int>(vertices.size());
        for (int corner = 0; corner < 4; corner++)
            vertices.push_back({facePositions[face][corner], faceUvs[corner], faceNormals[face]});

        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }

    VertexAttributes attrs[] = {{3}, {2}, {3}};
    const auto vao = renderEngine->CreateVertexArrayObject({static_cast<void*>(vertices.data()), sizeof(CubeVertex), static_cast<int>(vertices.size()), attrs, 3},
                                                           {static_cast<void*>(indices.data()), static_cast<int>(indices.size() * sizeof(unsigned int))});

    outIndexCount = static_cast<unsigned int>(indices.size());
    return vao;
}

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

VertexArrayObjectPtr MeshFactory::CreateGem(RenderEngine* renderEngine, unsigned int& outIndexCount, int sides, float radius, float topHeight, float bottomHeight)
{
    struct GemVertex
    {
        Vector3 position;
        Vector2 uv;
        Vector3 normal;
    };

    std::vector<GemVertex> vertices;
    vertices.reserve(static_cast<size_t>(sides + 1) * 3);

    constexpr float pi = 3.14159265358979323846f;

    for (int r = 0; r <= 2; r++)
    {
        for (int s = 0; s <= sides; s++)
        {
            const float u = static_cast<float>(s) / static_cast<float>(sides);
            const float theta = u * 2.0f * pi;
            const float cosT = std::cos(theta);
            const float sinT = std::sin(theta);

            GemVertex vertex;
            if (r == 0)
            {
                vertex.position = {0.0f, topHeight, 0.0f};
                vertex.normal = Vector3::Normalize({topHeight * cosT, radius, topHeight * sinT});
                vertex.uv = {u, 0.0f};
            }
            else if (r == 1)
            {
                vertex.position = {cosT * radius, 0.0f, sinT * radius};
                vertex.normal = {cosT, 0.0f, sinT};
                vertex.uv = {u, 0.5f};
            }
            else
            {
                vertex.position = {0.0f, -bottomHeight, 0.0f};
                vertex.normal = Vector3::Normalize({bottomHeight * cosT, -radius, bottomHeight * sinT});
                vertex.uv = {u, 1.0f};
            }

            vertices.push_back(vertex);
        }
    }

    std::vector<unsigned int> indices;
    indices.reserve(static_cast<size_t>(sides) * 12);

    const int stride = sides + 1;
    for (int r = 0; r < 2; r++)
    {
        for (int s = 0; s < sides; s++)
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
    const auto vao = renderEngine->CreateVertexArrayObject({static_cast<void*>(vertices.data()), sizeof(GemVertex), static_cast<int>(vertices.size()), attrs, 3},
                                                           {static_cast<void*>(indices.data()), static_cast<int>(indices.size() * sizeof(unsigned int))});

    outIndexCount = static_cast<unsigned int>(indices.size());
    return vao;
}

VertexArrayObjectPtr MeshFactory::CreateDisc(RenderEngine* renderEngine, unsigned int& outIndexCount, int segments, float radius)
{
    struct DiscVertex
    {
        Vector3 position;
        Vector2 uv;
        Vector3 normal;
    };

    std::vector<DiscVertex> vertices;
    vertices.reserve(static_cast<size_t>(segments) + 2);
    vertices.push_back({{0.0f, 0.0f, 0.0f}, {0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}});

    constexpr float pi = 3.14159265358979323846f;
    for (int i = 0; i <= segments; i++)
    {
        const float angle = (2.0f * pi * static_cast<float>(i)) / static_cast<float>(segments);
        const float x = std::cos(angle) * radius;
        const float z = std::sin(angle) * radius;
        vertices.push_back({{x, 0.0f, z}, {0.5f + 0.5f * std::cos(angle), 0.5f + 0.5f * std::sin(angle)}, {0.0f, 1.0f, 0.0f}});
    }

    std::vector<unsigned int> indices;
    indices.reserve(static_cast<size_t>(segments) * 3);
    for (int i = 0; i < segments; i++)
    {
        indices.push_back(0);
        indices.push_back(static_cast<unsigned int>(i + 1));
        indices.push_back(static_cast<unsigned int>(i + 2));
    }

    VertexAttributes attrs[] = {{3}, {2}, {3}};
    const auto vao = renderEngine->CreateVertexArrayObject({static_cast<void*>(vertices.data()), sizeof(DiscVertex), static_cast<int>(vertices.size()), attrs, 3},
                                                           {static_cast<void*>(indices.data()), static_cast<int>(indices.size() * sizeof(unsigned int))});

    outIndexCount = static_cast<unsigned int>(indices.size());
    return vao;
}
