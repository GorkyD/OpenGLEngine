#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "Ecs/Components/AmbientLightComponent.h"
#include "Ecs/Components/ColliderComponent.h"
#include "Ecs/Components/GroupComponent.h"
#include "Ecs/Components/LightComponent.h"
#include "Ecs/Components/MaterialComponent.h"
#include "Ecs/Components/NameComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Components/TransformOffsetComponent.h"
#include "Ecs/Core/Entity.h"

class EcsWorld;

enum EditorStateMask : unsigned int
{
    StateTransform = 1u << 0,
    StateMaterial = 1u << 1,
    StateParent = 1u << 2,
    StateCollider = 1u << 3,
    StateLight = 1u << 4,
    StateAmbient = 1u << 5,
    StateGroup = 1u << 6,
    StateName = 1u << 7,
    StateVisibility = 1u << 8,
    StateAll = 0x1FFu
};

template <typename T> struct CapturedComponent
{
    bool present = false;
    T value{};
};

struct EntityState
{
    uint64_t id = 0;
    unsigned int mask = 0;

    CapturedComponent<TransformComponent> transform;
    CapturedComponent<TransformOffsetComponent> offset;
    CapturedComponent<MaterialComponent> material;
    CapturedComponent<ColliderComponent> collider;
    CapturedComponent<LightComponent> light;
    CapturedComponent<AmbientLightComponent> ambient;
    CapturedComponent<GroupComponent> group;
    CapturedComponent<NameComponent> name;

    bool hasParent = false;
    uint64_t parentId = 0;
    std::string attachBone;

    bool hasMesh = false;
    bool meshVisible = true;
};

class EditorState
{
public:
    static uint64_t IdOf(EcsWorld& world, Entity entity);
    static uint64_t NewId();
    static void AssignId(EcsWorld& world, Entity entity, uint64_t id);
    static Entity Resolve(EcsWorld& world, uint64_t id);

    static EntityState Capture(EcsWorld& world, Entity entity, unsigned int mask);
    static std::vector<EntityState> Capture(EcsWorld& world, const std::vector<Entity>& entities, unsigned int mask);

    static void Restore(EcsWorld& world, Entity target, const EntityState& state);
    static void Restore(EcsWorld& world, const std::vector<EntityState>& states);

private:
    static uint64_t nextId;
};
