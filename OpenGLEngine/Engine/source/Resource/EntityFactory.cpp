#include "Resource/EntityFactory.h"
#include <algorithm>
#include <vector>
#include "Ecs/Components/AABB.h"
#include "Ecs/Components/LocalBoundsComponent.h"
#include "Ecs/Components/AnimatorComponent.h"
#include "Ecs/Components/MaterialComponent.h"
#include "Ecs/Components/MeshComponent.h"
#include "Ecs/Components/MeshNameComponent.h"
#include "Ecs/Components/ShaderComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/EcsWorld.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Render/RenderEngine.h"
#include "Render/Texture.h"
#include "Resource/ModelCache.h"
#include "Resource/ModelLoader.h"
#include "Resource/SkinnedModelLoader.h"

template <typename VertexType> static void FillLocalBounds(EcsWorld& world, Entity entity, const std::vector<VertexType>& vertices)
{
    if (vertices.empty())
        return;

    Vector3 boundsMin = vertices[0].position;
    Vector3 boundsMax = vertices[0].position;

    for (const auto& vertex : vertices)
    {
        boundsMin.x = (std::min)(boundsMin.x, vertex.position.x);
        boundsMin.y = (std::min)(boundsMin.y, vertex.position.y);
        boundsMin.z = (std::min)(boundsMin.z, vertex.position.z);
        boundsMax.x = (std::max)(boundsMax.x, vertex.position.x);
        boundsMax.y = (std::max)(boundsMax.y, vertex.position.y);
        boundsMax.z = (std::max)(boundsMax.z, vertex.position.z);
    }

    auto& bounds = world.AddComponent<LocalBoundsComponent>(entity);
    bounds.min = boundsMin;
    bounds.max = boundsMax;
}

Entity EntityFactory::CreateModelEntity(EcsWorld& world, RenderEngine* renderEngine, const std::string& path, ShaderProgramPtr shader,
                                         std::vector<Entity>* outEntities)
{
    const CachedModel& model = ModelCache::Get(renderEngine, path);

    Entity result = 0;

    for (const auto& cachedMesh : model.meshes)
    {
        const auto entity = world.CreateEntity();
        world.AddComponent<TransformComponent>(entity);
        world.AddComponent<AABB>(entity);

        if (cachedMesh.hasBounds)
        {
            auto& bounds = world.AddComponent<LocalBoundsComponent>(entity);
            bounds.min = cachedMesh.boundsMin;
            bounds.max = cachedMesh.boundsMax;
        }

        auto& mesh = world.AddComponent<MeshComponent>(entity);
        mesh.vao = cachedMesh.vao;
        mesh.indexCount = cachedMesh.indexCount;

        if (!cachedMesh.name.empty())
            world.AddComponent<MeshNameComponent>(entity).name = cachedMesh.name;

        auto& shaderComp = world.AddComponent<ShaderComponent>(entity);
        shaderComp.shader = shader;

        if (cachedMesh.materialIndex >= 0 && cachedMesh.materialIndex < static_cast<int>(model.textures.size()))
        {
            auto& matComp = world.AddComponent<MaterialComponent>(entity);
            matComp.diffuseTexture = model.textures[cachedMesh.materialIndex];
            matComp.diffuseColor = model.diffuseColors[cachedMesh.materialIndex];
        }

        if (outEntities)
            outEntities->push_back(entity);

        if (result == 0)
            result = entity;
    }

    return result;
}

Entity EntityFactory::CreateSkinnedModelEntity(EcsWorld& world, RenderEngine* renderEngine, const std::string& path, ShaderProgramPtr shader)
{
    SkinnedModelData modelData = SkinnedModelLoader::Load(path);

    std::vector<TexturePtr> textures;
    for (auto& mat : modelData.materials)
    {
        if (!mat.diffuseTexturePath.empty())
            textures.push_back(Texture::LoadFromFile(mat.diffuseTexturePath));
        else
            textures.push_back(nullptr);
    }

    auto animatorState = std::make_shared<AnimatorState>();
    animatorState->skeleton = std::make_shared<Skeleton>(std::move(modelData.skeleton));
    animatorState->clips = std::make_shared<std::vector<AnimationClip>>(std::move(modelData.animations));
    animatorState->boneMatrices.assign(animatorState->skeleton->bones.size(), Matrix4());

    VertexAttributes attrs[] = {{sizeof(Vector3) / sizeof(float)}, {sizeof(Vector2) / sizeof(float)}, {sizeof(Vector3) / sizeof(float)}, {4}, {4}};

    Entity result = 0;

    for (auto& meshData : modelData.meshes)
    {
        const auto vao = renderEngine->CreateVertexArrayObject({static_cast<void*>(meshData.vertices.data()), sizeof(SkinnedVertex), static_cast<int>(meshData.vertices.size()), attrs, 5},
                                                                {static_cast<void*>(meshData.indices.data()), static_cast<int>(meshData.indices.size() * sizeof(unsigned int))});

        const auto entity = world.CreateEntity();
        world.AddComponent<TransformComponent>(entity);
        world.AddComponent<AABB>(entity);
        FillLocalBounds(world, entity, meshData.vertices);

        auto& mesh = world.AddComponent<MeshComponent>(entity);
        mesh.vao = vao;
        mesh.indexCount = static_cast<unsigned int>(meshData.indices.size());

        auto& shaderComp = world.AddComponent<ShaderComponent>(entity);
        shaderComp.shader = shader;
        shaderComp.shaderType = ShaderRenderType::Skinned;

        auto& animatorComp = world.AddComponent<AnimatorComponent>(entity);
        animatorComp.state = animatorState;

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
