#include "Resource/SceneSerializer.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include "Ecs/Components/MaterialComponent.h"
#include "Ecs/Components/ColliderComponent.h"
#include "Ecs/Components/HitFlashComponent.h"
#include "Ecs/Components/MeshComponent.h"
#include "Ecs/Components/ParentComponent.h"
#include "Ecs/Components/AmbientLightComponent.h"
#include "Ecs/Components/GroupComponent.h"
#include "Ecs/Components/LightComponent.h"
#include "Ecs/Components/PrefabComponent.h"
#include "Ecs/Components/ShaderComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Components/TransformOffsetComponent.h"
#include "Ecs/Core/EcsWorld.h"
#include "Engine/Engine.h"
#include "Extension/Extension.h"
#include "Render/RenderEngine.h"
#include "Resource/EntityFactory.h"

std::string SceneSerializer::GetScenePath(const std::string& sceneName, const std::string& ownerName)
{
    if (ownerName.empty())
        return "Assets/Scenes/" + sceneName + ".scene";

    return "Assets/Scenes/" + ownerName + "/" + sceneName + ".scene";
}

std::string SceneSerializer::GetSourceScenePath(const std::string& sceneName, const std::string& ownerName, const std::string& sourceAssetsPath)
{
    if (!sourceAssetsPath.empty())
        return sourceAssetsPath + "/Scenes/" + ownerName + "/" + sceneName + ".scene";

#ifdef OPENGLENGINE_SOURCE_ASSETS
    return std::string(OPENGLENGINE_SOURCE_ASSETS) + "/Scenes/" + sceneName + ".scene";
#else
    return GetScenePath(sceneName, ownerName);
#endif
}

std::string SceneSerializer::MakeUniqueGroupName(EcsWorld& world, const std::string& assetPath)
{
    std::string base = std::filesystem::path(assetPath).stem().string();
    if (base.empty())
        base = "Object";

    std::unordered_set<std::string> taken;
    for (auto& [entity, group] : world.GetPool<GroupComponent>())
        taken.insert(group.name);

    for (int index = 1;; index++)
    {
        std::string candidate = base + " " + std::to_string(index);
        if (taken.find(candidate) == taken.end())
            return candidate;
    }
}

static std::vector<std::string> SplitFields(const std::string& line, char delimiter)
{
    std::vector<std::string> fields;
    std::string current;
    std::istringstream stream(line);

    while (std::getline(stream, current, delimiter))
        fields.push_back(current);

    return fields;
}

static void ParseVector3(const std::string& text, Vector3& outValue)
{
    std::istringstream stream(text);
    stream >> outValue.x >> outValue.y >> outValue.z;
}

static void WriteScene(EcsWorld& world, std::ostream& file, int& outObjects, int& outLights)
{
    outObjects = 0;
    outLights = 0;

    std::unordered_map<std::string, std::vector<Entity>> groups;
    std::vector<std::string> order;

    for (auto& [entity, prefab] : world.GetPool<PrefabComponent>())
    {
        if (prefab.assetPath.empty() || !world.HasComponent<TransformComponent>(entity))
            continue;

        const std::string key = world.HasComponent<GroupComponent>(entity) ? world.GetComponent<GroupComponent>(entity).name : prefab.assetPath + std::to_string(entity);

        if (groups.find(key) == groups.end())
            order.push_back(key);
        groups[key].push_back(entity);
    }

    for (const auto& key : order)
    {
        auto& entities = groups[key];
        std::sort(entities.begin(), entities.end());

        const Entity keyEntity = entities.front();
        const auto& prefab = world.GetComponent<PrefabComponent>(keyEntity);
        const auto& transform = world.GetComponent<TransformComponent>(keyEntity);

        Vector3 rotationDegrees = {0, 0, 0};
        if (world.HasComponent<TransformOffsetComponent>(keyEntity))
            rotationDegrees = world.GetComponent<TransformOffsetComponent>(keyEntity).rotationDegrees;

        std::string parentName;
        std::string attachBone;
        if (world.HasComponent<ParentComponent>(keyEntity))
        {
            const auto& parentComp = world.GetComponent<ParentComponent>(keyEntity);
            attachBone = parentComp.attachBone;
            if (parentComp.parent != INVALID_ENTITY && world.HasComponent<GroupComponent>(parentComp.parent))
                parentName = world.GetComponent<GroupComponent>(parentComp.parent).name;
        }

        std::string visibility;
        for (const auto entity : entities)
            visibility += (world.HasComponent<MeshComponent>(entity) && !world.GetComponent<MeshComponent>(entity).visible) ? '0' : '1';

        MaterialComponent material;
        if (world.HasComponent<MaterialComponent>(keyEntity))
            material = world.GetComponent<MaterialComponent>(keyEntity);

        file << "object|" << prefab.assetPath << '|' << key << '|' << parentName << '|' << attachBone << '|' << transform.position.x << ' ' << transform.position.y << ' '
             << transform.position.z << '|' << rotationDegrees.x << ' ' << rotationDegrees.y << ' ' << rotationDegrees.z << '|' << transform.scale.x << ' ' << transform.scale.y
             << ' ' << transform.scale.z << '|' << material.diffuseColor.x << ' ' << material.diffuseColor.y << ' ' << material.diffuseColor.z << ' ' << material.diffuseColor.w
             << ' ' << material.emissiveColor.x << ' ' << material.emissiveColor.y << ' ' << material.emissiveColor.z << ' ' << material.emissiveIntensity << ' '
             << material.roughness << ' ' << material.metallic << '|' << visibility << '|';

        if (world.HasComponent<ColliderComponent>(keyEntity))
        {
            const auto& collider = world.GetComponent<ColliderComponent>(keyEntity);
            file << collider.center.x << ' ' << collider.center.y << ' ' << collider.center.z << ' ' << collider.size.x << ' ' << collider.size.y << ' ' << collider.size.z
                 << ' ' << (collider.isStatic ? 1 : 0) << ' ' << (collider.isTrigger ? 1 : 0) << ' ' << collider.layer << ' ' << collider.collidesWith << ' '
                 << collider.restitution;
        }

        file << '\n';

        outObjects++;
    }

    for (auto& [entity, light] : world.GetPool<LightComponent>())
    {
        file << "light " << static_cast<int>(light.type) << ' ' << light.color.x << ' ' << light.color.y << ' ' << light.color.z << ' ' << light.direction.x << ' '
             << light.direction.y << ' ' << light.direction.z << ' ' << light.position.x << ' ' << light.position.y << ' ' << light.position.z << ' ' << light.intensity << ' '
             << light.range << ' ' << (light.castShadows ? 1 : 0) << ' ' << light.shadowOrthoSize << ' ' << light.shadowDistance << ' ' << light.shadowBias << ' '
             << light.shadowAmbientOcclusion << ' ' << light.shadowFocusDistance << ' ' << light.shadowNormalBias << ' ' << light.innerConeAngleDeg << ' '
             << light.outerConeAngleDeg << '\n';
        outLights++;
    }

    for (auto& [entity, ambient] : world.GetPool<AmbientLightComponent>())
    {
        file << "ambient " << ambient.color.x << ' ' << ambient.color.y << ' ' << ambient.color.z << ' ' << ambient.intensity << '\n';
        outLights++;
    }
}

bool SceneSerializer::Save(Engine& engine, const std::string& sceneName)
{
    const Engine::SceneEntry* entry = engine.FindSceneEntry(sceneName);
    const std::string ownerName = entry ? entry->ownerName : "";
    const std::string sourceAssets = entry ? entry->sourceAssetsPath : "";

    std::ostringstream buffer;
    int objectCount = 0;
    int lightCount = 0;
    WriteScene(engine.GetWorld(), buffer, objectCount, lightCount);

    const std::string contents = buffer.str();
    bool wroteAny = false;

    for (const std::string& path : {GetScenePath(sceneName, ownerName), GetSourceScenePath(sceneName, ownerName, sourceAssets)})
    {
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);

        std::ofstream file(path);
        if (!file.is_open())
        {
            OGL_WARNING("SceneSerializer | Cannot write: " << path)
            continue;
        }

        file << contents;
        wroteAny = true;
    }

    if (wroteAny)
        OGL_INFO("SceneSerializer | Saved " << objectCount << " objects and " << lightCount << " lights (" << sceneName << ")")

    return wroteAny;
}

SceneData SceneSerializer::ReadFile(const std::string& sceneName, const std::string& ownerName, const std::string& sourceAssetsPath)
{
    SceneData data;

    std::ifstream file(GetScenePath(sceneName, ownerName));
    if (!file.is_open())
        file.open(GetSourceScenePath(sceneName, ownerName, sourceAssetsPath));
    if (!file.is_open())
        return data;

    std::string line;
    while (std::getline(file, line))
    {
        if (line.rfind("object|", 0) == 0)
        {
            const auto fields = SplitFields(line, '|');
            if (fields.size() < 10)
                continue;

            PlacedObject object;
            object.assetPath = fields[1];
            object.name = fields[2];
            object.parentName = fields[3];
            object.attachBone = fields[4];

            ParseVector3(fields[5], object.position);
            ParseVector3(fields[6], object.rotationDegrees);
            ParseVector3(fields[7], object.scale);

            std::istringstream materialStream(fields[8]);
            float cr, cg, cb, ca, er, eg, eb, ei, roughness, metallic;
            if (materialStream >> cr >> cg >> cb >> ca >> er >> eg >> eb >> ei >> roughness >> metallic)
            {
                object.hasMaterial = true;
                object.diffuseColor = {cr, cg, cb, ca};
                object.emissiveColor = {er, eg, eb};
                object.emissiveIntensity = ei;
                object.roughness = roughness;
                object.metallic = metallic;
            }

            if (fields.size() > 9)
                object.visibility = fields[9];

            if (fields.size() > 10 && !fields[10].empty())
            {
                std::istringstream colliderStream(fields[10]);
                int isStatic = 0;
                if (colliderStream >> object.colliderCenter.x >> object.colliderCenter.y >> object.colliderCenter.z >> object.colliderSize.x >> object.colliderSize.y >>
                    object.colliderSize.z >> isStatic)
                {
                    object.hasCollider = true;
                    object.colliderStatic = isStatic != 0;

                    int isTrigger = 0;
                    if (colliderStream >> isTrigger >> object.colliderLayer >> object.colliderMask >> object.colliderRestitution)
                        object.colliderTrigger = isTrigger != 0;
                }
            }

            if (!object.assetPath.empty())
                data.objects.push_back(object);

            continue;
        }

        std::istringstream stream(line);
        std::string token;
        stream >> token;

        if (token == "object")
        {
            PlacedObject object;
            stream >> object.assetPath >> object.position.x >> object.position.y >> object.position.z >> object.rotationDegrees.x >> object.rotationDegrees.y >>
                object.rotationDegrees.z >> object.scale.x >> object.scale.y >> object.scale.z;

            float cr = 0, cg = 0, cb = 0, ca = 0, er = 0, eg = 0, eb = 0, ei = 0, roughness = 0, metallic = 0;
            if (stream >> cr >> cg >> cb >> ca >> er >> eg >> eb >> ei >> roughness >> metallic)
            {
                object.hasMaterial = true;
                object.diffuseColor = {cr, cg, cb, ca};
                object.emissiveColor = {er, eg, eb};
                object.emissiveIntensity = ei;
                object.roughness = roughness;
                object.metallic = metallic;
            }

            if (!object.assetPath.empty())
                data.objects.push_back(object);
        }
        else if (token == "light")
        {
            PlacedLight light;
            stream >> light.type >> light.color.x >> light.color.y >> light.color.z >> light.direction.x >> light.direction.y >> light.direction.z >> light.position.x >>
                light.position.y >> light.position.z >> light.intensity >> light.range;

            int castShadowsFlag = 0;
            if (stream >> castShadowsFlag >> light.shadowOrthoSize >> light.shadowDistance >> light.shadowBias)
            {
                light.castShadows = castShadowsFlag != 0;

                float ambientOcclusion = 0.0f;
                if (stream >> ambientOcclusion)
                {
                    light.shadowAmbientOcclusion = ambientOcclusion;

                    float focusDistance = 0.0f;
                    if (stream >> focusDistance)
                    {
                        light.shadowFocusDistance = focusDistance;

                        float normalBias = 0.0f;
                        if (stream >> normalBias)
                        {
                            light.shadowNormalBias = normalBias;

                            float innerConeAngle = 0.0f;
                            float outerConeAngle = 0.0f;
                            if (stream >> innerConeAngle >> outerConeAngle)
                            {
                                light.innerConeAngleDeg = innerConeAngle;
                                light.outerConeAngleDeg = outerConeAngle;
                            }
                        }
                    }
                }
            }

            data.lights.push_back(light);
        }
        else if (token == "ambient")
        {
            PlacedAmbient ambient;
            stream >> ambient.color.x >> ambient.color.y >> ambient.color.z >> ambient.intensity;
            data.ambients.push_back(ambient);
        }
    }

    return data;
}

bool SceneSerializer::Load(Engine& engine, const std::string& sceneName)
{
    const Engine::SceneEntry* entry = engine.FindSceneEntry(sceneName);
    const std::string ownerName = entry ? entry->ownerName : "";
    const std::string sourceAssets = entry ? entry->sourceAssetsPath : "";

    const SceneData data = ReadFile(sceneName, ownerName, sourceAssets);
    if (data.Empty())
        return false;

    auto& world = engine.GetWorld();
    auto* renderEngine = engine.GetRenderEngine();
    const auto& shaders = engine.GetShaders();

    if (!data.lights.empty() || !data.ambients.empty())
    {
        world.GetPool<LightComponent>().Clear();
        world.GetPool<AmbientLightComponent>().Clear();
    }

    struct PendingParent
    {
        std::string groupName;
        std::string parentName;
        std::string attachBone;
        std::vector<Entity> entities;
    };

    std::unordered_map<std::string, Entity> groupRoots;
    std::vector<PendingParent> pendingParents;

    for (const auto& placedLight : data.lights)
    {
        const Entity entity = world.CreateEntity();
        auto& light = world.AddComponent<LightComponent>(entity);
        light.type = static_cast<LightType>(placedLight.type);
        light.color = placedLight.color;
        light.direction = placedLight.direction;
        light.position = placedLight.position;
        light.intensity = placedLight.intensity;
        light.range = placedLight.range;
        light.castShadows = placedLight.castShadows;
        light.shadowOrthoSize = placedLight.shadowOrthoSize;
        light.shadowDistance = placedLight.shadowDistance;
        light.shadowBias = placedLight.shadowBias;
        light.shadowAmbientOcclusion = placedLight.shadowAmbientOcclusion;
        light.shadowFocusDistance = placedLight.shadowFocusDistance;
        light.shadowNormalBias = placedLight.shadowNormalBias;
        light.innerConeAngleDeg = placedLight.innerConeAngleDeg;
        light.outerConeAngleDeg = placedLight.outerConeAngleDeg;
    }

    for (const auto& placedAmbient : data.ambients)
    {
        const Entity entity = world.CreateEntity();
        auto& ambient = world.AddComponent<AmbientLightComponent>(entity);
        ambient.color = placedAmbient.color;
        ambient.intensity = placedAmbient.intensity;
    }

    for (const auto& object : data.objects)
    {
        std::vector<Entity> created;
        const Entity root = EntityFactory::CreateModelEntity(world, renderEngine, object.assetPath, shaders.lit, &created);
        if (root == 0)
            continue;

        const std::string groupName = object.name.empty() ? MakeUniqueGroupName(world, object.assetPath) : object.name;
        std::sort(created.begin(), created.end());
        groupRoots[groupName] = created.front();

        if (!object.parentName.empty())
            pendingParents.push_back({groupName, object.parentName, object.attachBone, created});

        for (size_t partIndex = 0; partIndex < created.size(); partIndex++)
        {
            const Entity entity = created[partIndex];

            if (partIndex < object.visibility.size() && world.HasComponent<MeshComponent>(entity))
                world.GetComponent<MeshComponent>(entity).visible = object.visibility[partIndex] != '0';

            world.AddComponent<PrefabComponent>(entity).assetPath = object.assetPath;
            world.AddComponent<GroupComponent>(entity).name = groupName;
            world.AddComponent<HitFlashComponent>(entity);

            if (world.HasComponent<ShaderComponent>(entity))
                world.GetComponent<ShaderComponent>(entity).shaderType = ShaderRenderType::Lit;

            auto& offset = world.AddComponent<TransformOffsetComponent>(entity);
            offset.rotationDegrees = object.rotationDegrees;

            auto& transform = world.GetComponent<TransformComponent>(entity);
            transform.position = object.position;
            transform.scale = object.scale;
            transform.rotation = offset.GetRotationMatrix();

            if (object.hasCollider && entity == created.front())
            {
                auto& collider = world.AddComponent<ColliderComponent>(entity);
                collider.center = object.colliderCenter;
                collider.size = object.colliderSize;
                collider.isStatic = object.colliderStatic;
                collider.isTrigger = object.colliderTrigger;
                collider.layer = object.colliderLayer;
                collider.collidesWith = object.colliderMask;
                collider.restitution = object.colliderRestitution;
            }

            if (object.hasMaterial)
            {
                auto& material = world.HasComponent<MaterialComponent>(entity) ? world.GetComponent<MaterialComponent>(entity) : world.AddComponent<MaterialComponent>(entity);
                material.diffuseColor = object.diffuseColor;
                material.emissiveColor = object.emissiveColor;
                material.emissiveIntensity = object.emissiveIntensity;
                material.roughness = object.roughness;
                material.metallic = object.metallic;
            }
        }
    }

    for (const auto& pending : pendingParents)
    {
        const auto parentIt = groupRoots.find(pending.parentName);
        if (parentIt == groupRoots.end())
            continue;

        for (const auto entity : pending.entities)
        {
            auto& parentComp = world.HasComponent<ParentComponent>(entity) ? world.GetComponent<ParentComponent>(entity) : world.AddComponent<ParentComponent>(entity);
            parentComp.parent = parentIt->second;
            parentComp.attachBone = pending.attachBone;
        }
    }

    OGL_INFO("SceneSerializer | Loaded " << data.objects.size() << " objects, " << data.lights.size() << " lights from " << GetScenePath(sceneName, ownerName))
    return true;
}
