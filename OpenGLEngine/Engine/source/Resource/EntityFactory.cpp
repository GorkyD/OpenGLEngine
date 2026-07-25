#include "Resource/EntityFactory.h"

#include <vector>

#include "Ecs/Components/AABB.h"
#include "Ecs/Components/MaterialComponent.h"
#include "Ecs/Components/MeshComponent.h"
#include "Ecs/Components/ShaderComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/EcsWorld.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Render/RenderEngine.h"
#include "Render/Texture.h"
#include "Resource/ModelLoader.h"

Entity EntityFactory::CreateModelEntity(EcsWorld& world, RenderEngine* renderEngine, const std::string& path,
                                        ShaderProgramPtr shader)
{
    ModelData modelData = ModelLoader::Load(path);

    std::vector<TexturePtr> textures;
    for (auto& mat : modelData.materials)
    {
        if (!mat.diffuseTexturePath.empty())
            textures.push_back(Texture::LoadFromFile(mat.diffuseTexturePath));
        else
            textures.push_back(nullptr);
    }

    VertexAttributes attrs[] = {
        {sizeof(Vector3) / sizeof(float)}, {sizeof(Vector2) / sizeof(float)}, {sizeof(Vector3) / sizeof(float)}};

    Entity result = 0;

    for (auto& meshData : modelData.meshes)
    {
        const auto vao =
            renderEngine->CreateVertexArrayObject({static_cast<void*>(meshData.vertices.data()), sizeof(MeshVertex),
                                                   static_cast<int>(meshData.vertices.size()), attrs, 3},
                                                  {static_cast<void*>(meshData.indices.data()),
                                                   static_cast<int>(meshData.indices.size() * sizeof(unsigned int))});

        const auto entity = world.CreateEntity();
        world.AddComponent<TransformComponent>(entity);
        world.AddComponent<AABB>(entity);

        auto& mesh = world.AddComponent<MeshComponent>(entity);
        mesh.vao = vao;
        mesh.indexCount = static_cast<unsigned int>(meshData.indices.size());

        auto& shaderComp = world.AddComponent<ShaderComponent>(entity);
        shaderComp.shader = shader;

        if (meshData.materialIndex >= 0 && meshData.materialIndex < static_cast<int>(modelData.materials.size()))
        {
            auto& matComp = world.AddComponent<MaterialComponent>(entity);
            matComp.diffuseTexture = textures[meshData.materialIndex];
            matComp.diffuseColor = modelData.materials[meshData.materialIndex].diffuseColor;
        }

        if (result == 0)
            result = entity;
    }

    return result;
}
