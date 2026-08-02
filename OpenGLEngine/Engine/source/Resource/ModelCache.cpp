#include "Resource/ModelCache.h"
#include <algorithm>
#include <unordered_map>
#include "Math/Vector2.h"
#include "Render/RenderEngine.h"
#include "Render/Texture.h"
#include "Resource/ModelLoader.h"

static std::unordered_map<std::string, CachedModel> models;

static void FillBounds(const std::vector<MeshVertex>& vertices, CachedMesh& mesh)
{
    if (vertices.empty())
        return;

    mesh.boundsMin = vertices[0].position;
    mesh.boundsMax = vertices[0].position;

    for (const auto& vertex : vertices)
    {
        mesh.boundsMin.x = (std::min)(mesh.boundsMin.x, vertex.position.x);
        mesh.boundsMin.y = (std::min)(mesh.boundsMin.y, vertex.position.y);
        mesh.boundsMin.z = (std::min)(mesh.boundsMin.z, vertex.position.z);
        mesh.boundsMax.x = (std::max)(mesh.boundsMax.x, vertex.position.x);
        mesh.boundsMax.y = (std::max)(mesh.boundsMax.y, vertex.position.y);
        mesh.boundsMax.z = (std::max)(mesh.boundsMax.z, vertex.position.z);
    }

    mesh.hasBounds = true;
}

const CachedModel& ModelCache::Get(RenderEngine* renderEngine, const std::string& path)
{
    const auto existing = models.find(path);
    if (existing != models.end())
        return existing->second;

    ModelData modelData = ModelLoader::Load(path);

    CachedModel cached;
    cached.textures.reserve(modelData.materials.size());
    cached.diffuseColors.reserve(modelData.materials.size());

    for (auto& material : modelData.materials)
    {
        cached.diffuseColors.push_back(material.diffuseColor);

        if (!material.embeddedTexture.empty())
        {
            if (material.embeddedHeight > 0)
                cached.textures.push_back(Texture::CreateFromPixels(material.embeddedTexture.data(), material.embeddedWidth, material.embeddedHeight, 4));
            else
                cached.textures.push_back(Texture::LoadFromMemory(material.embeddedTexture.data(), static_cast<int>(material.embeddedTexture.size())));
        }
        else if (!material.diffuseTexturePath.empty())
            cached.textures.push_back(Texture::LoadFromFile(material.diffuseTexturePath));
        else
            cached.textures.push_back(nullptr);
    }

    static VertexAttributes attrs[] = {{sizeof(Vector3) / sizeof(float)}, {sizeof(Vector2) / sizeof(float)}, {sizeof(Vector3) / sizeof(float)}};

    cached.meshes.reserve(modelData.meshes.size());
    for (auto& meshData : modelData.meshes)
    {
        CachedMesh mesh;
        mesh.vao = renderEngine->CreateVertexArrayObject(
            {static_cast<void*>(meshData.vertices.data()), sizeof(MeshVertex), static_cast<int>(meshData.vertices.size()), attrs, 3},
            {static_cast<void*>(meshData.indices.data()), static_cast<int>(meshData.indices.size() * sizeof(unsigned int))});
        mesh.indexCount = static_cast<unsigned int>(meshData.indices.size());
        mesh.materialIndex = meshData.materialIndex;
        mesh.name = meshData.name;
        FillBounds(meshData.vertices, mesh);

        cached.meshes.push_back(std::move(mesh));
    }

    return models.emplace(path, std::move(cached)).first->second;
}

void ModelCache::Clear()
{
    models.clear();
}
