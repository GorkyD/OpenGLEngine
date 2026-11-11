#include "Editor/EditorState.h"
#include "Ecs/Components/EditorIdComponent.h"
#include "Ecs/Components/MeshComponent.h"
#include "Ecs/Components/ParentComponent.h"
#include "Ecs/Core/EcsWorld.h"

uint64_t EditorState::nextId = 1;

uint64_t EditorState::NewId()
{
    return nextId++;
}

uint64_t EditorState::IdOf(EcsWorld& world, Entity entity)
{
    if (world.HasComponent<EditorIdComponent>(entity))
        return world.GetComponent<EditorIdComponent>(entity).id;

    const uint64_t id = NewId();
    world.AddComponent<EditorIdComponent>(entity).id = id;
    return id;
}

void EditorState::AssignId(EcsWorld& world, Entity entity, uint64_t id)
{
    world.AddComponent<EditorIdComponent>(entity).id = id;
}

Entity EditorState::Resolve(EcsWorld& world, uint64_t id)
{
    if (id == 0)
        return INVALID_ENTITY;

    for (auto& [entity, editorId] : world.GetPool<EditorIdComponent>())
    {
        if (editorId.id == id)
            return entity;
    }

    return INVALID_ENTITY;
}

template <typename T> static void CaptureComponent(EcsWorld& world, Entity entity, CapturedComponent<T>& out)
{
    out.present = world.HasComponent<T>(entity);
    if (out.present)
        out.value = world.GetComponent<T>(entity);
}

template <typename T> static void RestoreComponent(EcsWorld& world, Entity entity, const CapturedComponent<T>& in)
{
    if (in.present)
        world.AddComponent<T>(entity) = in.value;
    else
        world.GetPool<T>().Remove(entity);
}

EntityState EditorState::Capture(EcsWorld& world, Entity entity, unsigned int mask)
{
    EntityState state;
    state.id = IdOf(world, entity);
    state.mask = mask;

    if (mask & StateTransform)
    {
        CaptureComponent(world, entity, state.transform);
        CaptureComponent(world, entity, state.offset);
    }
    if (mask & StateMaterial)
        CaptureComponent(world, entity, state.material);
    if (mask & StateCollider)
        CaptureComponent(world, entity, state.collider);
    if (mask & StateLight)
        CaptureComponent(world, entity, state.light);
    if (mask & StateAmbient)
        CaptureComponent(world, entity, state.ambient);
    if (mask & StateGroup)
        CaptureComponent(world, entity, state.group);
    if (mask & StateName)
        CaptureComponent(world, entity, state.name);

    if (mask & StateParent)
    {
        state.hasParent = world.HasComponent<ParentComponent>(entity);
        if (state.hasParent)
        {
            const auto& parent = world.GetComponent<ParentComponent>(entity);
            state.parentId = parent.parent != INVALID_ENTITY ? IdOf(world, parent.parent) : 0;
            state.attachBone = parent.attachBone;
        }
    }

    if (mask & StateVisibility)
    {
        state.hasMesh = world.HasComponent<MeshComponent>(entity);
        if (state.hasMesh)
            state.meshVisible = world.GetComponent<MeshComponent>(entity).visible;
    }

    return state;
}

std::vector<EntityState> EditorState::Capture(EcsWorld& world, const std::vector<Entity>& entities, unsigned int mask)
{
    std::vector<EntityState> states;
    states.reserve(entities.size());
    for (const auto entity : entities)
        states.push_back(Capture(world, entity, mask));
    return states;
}

void EditorState::Restore(EcsWorld& world, Entity target, const EntityState& state)
{
    if (target == INVALID_ENTITY)
        return;

    if (state.mask & StateTransform)
    {
        RestoreComponent(world, target, state.transform);
        RestoreComponent(world, target, state.offset);
    }
    if (state.mask & StateMaterial)
        RestoreComponent(world, target, state.material);
    if (state.mask & StateCollider)
        RestoreComponent(world, target, state.collider);
    if (state.mask & StateLight)
        RestoreComponent(world, target, state.light);
    if (state.mask & StateAmbient)
        RestoreComponent(world, target, state.ambient);
    if (state.mask & StateGroup)
        RestoreComponent(world, target, state.group);
    if (state.mask & StateName)
        RestoreComponent(world, target, state.name);

    if (state.mask & StateParent)
    {
        if (state.hasParent)
        {
            auto& parent = world.AddComponent<ParentComponent>(target);
            parent.parent = Resolve(world, state.parentId);
            parent.attachBone = state.attachBone;
        }
        else
        {
            world.GetPool<ParentComponent>().Remove(target);
        }
    }

    if ((state.mask & StateVisibility) && state.hasMesh && world.HasComponent<MeshComponent>(target))
        world.GetComponent<MeshComponent>(target).visible = state.meshVisible;
}

void EditorState::Restore(EcsWorld& world, const std::vector<EntityState>& states)
{
    for (const auto& state : states)
        Restore(world, Resolve(world, state.id), state);
}
