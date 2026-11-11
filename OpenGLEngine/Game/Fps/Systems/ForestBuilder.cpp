#include "Systems/ForestBuilder.h"
#include <algorithm>
#include <cmath>
#include "Audio/AudioSystem.h"
#include "Ecs/Components/ColliderComponent.h"
#include "Ecs/Components/LightComponent.h"
#include "Ecs/Components/LocalBoundsComponent.h"
#include "Ecs/Components/MaterialComponent.h"
#include "Ecs/Components/MeshComponent.h"
#include "Ecs/Components/NameComponent.h"
#include "Ecs/Components/ParticleEmitterComponent.h"
#include "Ecs/Components/ShaderComponent.h"
#include "Ecs/Components/TerrainComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/EcsWorld.h"
#include "Engine/Engine.h"
#include "Math/Matrix4.h"
#include "Physics/PhysicsLayers.h"
#include "Render/Texture.h"
#include "Resource/EntityFactory.h"
#include "Resource/MeshFactory.h"

namespace
{
constexpr float DegToRad = 3.14159265f / 180.0f;

struct Random
{
    unsigned int state;

    float Next()
    {
        state = state * 1664525u + 1013904223u;
        return static_cast<float>((state >> 8) & 0xFFFFFF) / static_cast<float>(0xFFFFFF);
    }

    float Range(float minValue, float maxValue)
    {
        return minValue + (maxValue - minValue) * Next();
    }
};

const char* const TreeModels[] = {
    "Assets/Models/Nature/RetroNature/trees/tree01.glb", "Assets/Models/Nature/RetroNature/trees/tree02.glb", "Assets/Models/Nature/RetroNature/trees/tree03.glb",
    "Assets/Models/Nature/RetroNature/trees/tree04.glb", "Assets/Models/Nature/RetroNature/trees/tree05.glb", "Assets/Models/Nature/RetroNature/trees/tree06.glb",
    "Assets/Models/Nature/RetroNature/trees/tree07.glb", "Assets/Models/Nature/RetroNature/trees/tree08.glb", "Assets/Models/Nature/LowPolyTrees/big_tree.obj",
    "Assets/Models/Nature/LowPolyTrees/med_tree.obj",
};

const char* const BushModels[] = {
    "Assets/Models/Nature/RetroNature/bushes/bush01.glb", "Assets/Models/Nature/RetroNature/bushes/bush02.glb", "Assets/Models/Nature/RetroNature/bushes/bush03.glb",
    "Assets/Models/Nature/RetroNature/bushes/bush04.glb", "Assets/Models/Nature/RetroNature/bushes/bush05.glb", "Assets/Models/Nature/RetroNature/bushes/bush06.glb",
};

const char* const GrassModels[] = {
    "Assets/Models/Nature/RetroNature/grass/grass01.glb", "Assets/Models/Nature/RetroNature/grass/grass03.glb", "Assets/Models/Nature/RetroNature/grass/grass05.glb",
    "Assets/Models/Nature/RetroNature/grass/grass_bush.glb",
};
}

float ForestBuilder::GroundHeight(EcsWorld& world, float x, float z)
{
    for (auto& [entity, terrain] : world.GetPool<TerrainComponent>())
        return terrain.GetHeightAt(x, z);
    return 0.0f;
}

std::vector<Entity> ForestBuilder::SpawnProp(Engine& engine, const std::string& path, float x, float z, float targetSize, float yawDegrees, float sink, bool fitLargestSide)
{
    auto& world = engine.GetWorld();

    std::vector<Entity> created;
    if (EntityFactory::CreateModelEntity(world, engine.GetRenderEngine(), path, engine.GetShaders().lit, &created) == 0 || created.empty())
        return created;

    Vector3 boundsMin = {1e30f, 1e30f, 1e30f};
    Vector3 boundsMax = {-1e30f, -1e30f, -1e30f};
    for (const auto entity : created)
    {
        if (!world.HasComponent<LocalBoundsComponent>(entity))
            continue;
        const auto& bounds = world.GetComponent<LocalBoundsComponent>(entity);
        boundsMin = {(std::min)(boundsMin.x, bounds.min.x), (std::min)(boundsMin.y, bounds.min.y), (std::min)(boundsMin.z, bounds.min.z)};
        boundsMax = {(std::max)(boundsMax.x, bounds.max.x), (std::max)(boundsMax.y, bounds.max.y), (std::max)(boundsMax.z, bounds.max.z)};
    }

    const Vector3 extent = boundsMax - boundsMin;
    const float modelSize = fitLargestSide ? (std::max)(extent.x, (std::max)(extent.y, extent.z)) : extent.y;
    const float scale = modelSize > 0.0001f ? targetSize / modelSize : 1.0f;
    const float ground = GroundHeight(world, x, z);
    const Matrix4 rotation = Matrix4::FromAxisAngle({0.0f, 1.0f, 0.0f}, yawDegrees * DegToRad);
    const Vector3 centerOffset = rotation.TransformDirection({(boundsMin.x + boundsMax.x) * 0.5f * scale, 0.0f, (boundsMin.z + boundsMax.z) * 0.5f * scale});

    for (const auto entity : created)
    {
        auto& transform = world.GetComponent<TransformComponent>(entity);
        transform.position = {x - centerOffset.x, ground - boundsMin.y * scale - sink, z - centerOffset.z};
        transform.scale = {scale, scale, scale};
        transform.rotation = rotation;

        if (world.HasComponent<ShaderComponent>(entity))
            world.GetComponent<ShaderComponent>(entity).shaderType = ShaderRenderType::Lit;
    }

    return created;
}

void ForestBuilder::Bounds(EcsWorld& world, const std::vector<Entity>& parts, Vector3& outMin, Vector3& outMax)
{
    outMin = {1e30f, 1e30f, 1e30f};
    outMax = {-1e30f, -1e30f, -1e30f};

    for (const auto entity : parts)
    {
        if (!world.HasComponent<LocalBoundsComponent>(entity) || !world.HasComponent<TransformComponent>(entity))
            continue;

        const auto& bounds = world.GetComponent<LocalBoundsComponent>(entity);
        const Matrix4 model = world.GetComponent<TransformComponent>(entity).GetModelMatrix();
        for (int corner = 0; corner < 8; corner++)
        {
            const Vector3 local = {(corner & 1) ? bounds.max.x : bounds.min.x, (corner & 2) ? bounds.max.y : bounds.min.y, (corner & 4) ? bounds.max.z : bounds.min.z};
            const Vector3 point = model.TransformPoint(local);
            outMin = {(std::min)(outMin.x, point.x), (std::min)(outMin.y, point.y), (std::min)(outMin.z, point.z)};
            outMax = {(std::max)(outMax.x, point.x), (std::max)(outMax.y, point.y), (std::max)(outMax.z, point.z)};
        }
    }
}

Vector3 ForestBuilder::BoundsCenter(EcsWorld& world, const std::vector<Entity>& parts)
{
    Vector3 boundsMin;
    Vector3 boundsMax;
    Bounds(world, parts, boundsMin, boundsMax);
    return (boundsMin + boundsMax) * 0.5f;
}

Entity ForestBuilder::AddBoxCollider(Engine& engine, const std::vector<Entity>& parts, float shrink)
{
    auto& world = engine.GetWorld();

    Vector3 boundsMin;
    Vector3 boundsMax;
    Bounds(world, parts, boundsMin, boundsMax);
    if (boundsMin.x > boundsMax.x)
        return 0;

    const Entity entity = world.CreateEntity();
    world.AddComponent<TransformComponent>(entity).position = (boundsMin + boundsMax) * 0.5f;

    auto& collider = world.AddComponent<ColliderComponent>(entity);
    collider.size = {(std::max)(0.1f, boundsMax.x - boundsMin.x - shrink), boundsMax.y - boundsMin.y, (std::max)(0.1f, boundsMax.z - boundsMin.z - shrink)};
    collider.isStatic = true;
    collider.layer = PhysicsLayers::Default;
    return entity;
}

CampfireEntities ForestBuilder::BuildCampfire(Engine& engine, const Vector3& position, float intensity)
{
    auto& world = engine.GetWorld();
    CampfireEntities result;
    result.position = position;

    SpawnProp(engine, "Assets/Models/Camp/Campfire/Campfire_LOWPOLY.obj", position.x, position.z, 0.45f, 0.0f);

    unsigned int flameIndexCount = 0;
    const auto flameVao = MeshFactory::CreateFlame(engine.GetRenderEngine(), flameIndexCount);

    result.flame = world.CreateEntity();
    world.AddComponent<NameComponent>(result.flame).name = "Campfire Flame";
    auto& flameTransform = world.AddComponent<TransformComponent>(result.flame);
    flameTransform.position = position + Vector3(0.0f, 0.12f, 0.0f);
    flameTransform.scale = {0.45f, 0.95f, 0.45f};

    auto& flameMesh = world.AddComponent<MeshComponent>(result.flame);
    flameMesh.vao = flameVao;
    flameMesh.indexCount = flameIndexCount;
    flameMesh.castsShadow = false;

    auto& flameShader = world.AddComponent<ShaderComponent>(result.flame);
    flameShader.shader = engine.GetShaders().fire;
    flameShader.shaderType = ShaderRenderType::Fire;

    auto& smoke = world.AddComponent<ParticleEmitterComponent>(result.flame);
    smoke.localOffset = {0.0f, 0.9f, 0.0f};
    smoke.spawnRadius = 0.25f;
    smoke.velocityMin = {-0.3f, 0.6f, -0.3f};
    smoke.velocityMax = {0.3f, 1.3f, 0.3f};
    smoke.gravity = {0.05f, 0.08f, 0.0f};
    smoke.lifetimeMin = 2.0f;
    smoke.lifetimeMax = 3.5f;
    smoke.startSize = 0.3f;
    smoke.endSize = 1.6f;
    smoke.color = {0.35f, 0.35f, 0.38f, 0.18f};
    smoke.maxParticles = 64;

    result.sparks = world.CreateEntity();
    world.AddComponent<TransformComponent>(result.sparks).position = position + Vector3(0.0f, 0.4f, 0.0f);
    auto& sparks = world.AddComponent<ParticleEmitterComponent>(result.sparks);
    sparks.spawnRadius = 0.3f;
    sparks.velocityMin = {-0.5f, 1.2f, -0.5f};
    sparks.velocityMax = {0.5f, 3.0f, 0.5f};
    sparks.gravity = {0.0f, -1.5f, 0.0f};
    sparks.spawnRate = 14.0f;
    sparks.lifetimeMin = 0.5f;
    sparks.lifetimeMax = 1.3f;
    sparks.startSize = 0.05f;
    sparks.endSize = 0.02f;
    sparks.color = {1.0f, 0.55f, 0.12f, 1.0f};
    sparks.maxParticles = 40;
    sparks.additive = true;

    result.light = world.CreateEntity();
    world.AddComponent<NameComponent>(result.light).name = "Campfire Light";
    auto& light = world.AddComponent<LightComponent>(result.light);
    light.type = LightType::Point;
    light.color = {1.0f, 0.52f, 0.18f};
    light.range = 16.0f;
    light.intensity = intensity;
    light.serialized = false;
    light.position = position + Vector3(0.0f, 0.8f, 0.0f);

    if (auto* audio = engine.GetAudioSystem())
    {
        audio->RegisterGameObject(result.flame, "Campfire");
        audio->SetPosition(result.flame, position.x, position.y, position.z);
        audio->PlayEvent("Play_Fire", result.flame);
    }

    return result;
}

void ForestBuilder::PlaceCampItems(Engine& engine)
{
    const std::string survival = "Assets/Models/Items/SurvivalPSX/Models/Survival.fbx#";

    SpawnProp(engine, survival + "Shelter_tent", -4.2f, 3.2f, 1.5f, 160.0f);
    SpawnProp(engine, survival + "sleeping_bag", 3.6f, 2.4f, 1.9f, -25.0f, 0.0f, true);
    SpawnProp(engine, survival + "Backpack", -2.6f, 2.0f, 0.55f, 40.0f);
    SpawnProp(engine, survival + "Lantern", 2.2f, -1.9f, 0.35f, 0.0f);
    SpawnProp(engine, survival + "Trunk", -3.0f, 1.4f, 0.8f, 75.0f, 0.0f, true);
    SpawnProp(engine, survival + "Trunk_01", 0.6f, 2.4f, 1.6f, 10.0f, 0.05f, true);
    SpawnProp(engine, "Assets/Models/Camp/FireAxe/FireAxe_PSX.obj", 1.3f, -0.8f, 0.8f, 35.0f, 0.0f, true);
    SpawnProp(engine, "Assets/Models/Items/JerryCan/JerryCan.glb", -1.6f, -0.9f, 0.5f, 70.0f, 0.0f, true);
}

void ForestBuilder::ScatterForest(Engine& engine, unsigned int seed, float clearingRadius)
{
    Random random{seed};

    for (int i = 0; i < 170; i++)
    {
        const float angle = random.Range(0.0f, 6.2831853f);
        const float radius = clearingRadius + std::sqrt(random.Next()) * (67.0f - clearingRadius);
        const char* model = TreeModels[static_cast<int>(random.Next() * 9.999f)];
        SpawnProp(engine, model, std::cos(angle) * radius, std::sin(angle) * radius, random.Range(5.5f, 11.0f), random.Range(0.0f, 360.0f), 0.1f);
    }

    for (int i = 0; i < 60; i++)
    {
        const float angle = random.Range(0.0f, 6.2831853f);
        const float radius = random.Range(5.5f, 40.0f);
        const char* model = BushModels[static_cast<int>(random.Next() * 5.999f)];
        SpawnProp(engine, model, std::cos(angle) * radius, std::sin(angle) * radius, random.Range(0.6f, 1.4f), random.Range(0.0f, 360.0f), 0.05f);
    }

    for (int i = 0; i < 90; i++)
    {
        const float angle = random.Range(0.0f, 6.2831853f);
        const float radius = random.Range(2.5f, 25.0f);
        const char* model = GrassModels[static_cast<int>(random.Next() * 3.999f)];
        SpawnProp(engine, model, std::cos(angle) * radius, std::sin(angle) * radius, random.Range(0.25f, 0.5f), random.Range(0.0f, 360.0f));
    }

    for (int i = 0; i < 8; i++)
    {
        const float angle = random.Range(0.0f, 6.2831853f);
        const float radius = random.Range(4.0f, 30.0f);
        SpawnProp(engine, "Assets/Models/Nature/Rocks/Rock low poly.dae", std::cos(angle) * radius, std::sin(angle) * radius, random.Range(0.4f, 1.0f), random.Range(0.0f, 360.0f), 0.15f);
    }
}

void ForestBuilder::ApplyPointFiltering(Engine& engine)
{
    for (auto& [entity, material] : engine.GetWorld().GetPool<MaterialComponent>())
    {
        for (const TexturePtr* texture : {&material.diffuseTexture, &material.normalTexture, &material.roughnessTexture, &material.metallicTexture, &material.aoTexture})
        {
            if (*texture)
                (*texture)->SetPointFiltering(true);
        }
    }
}
