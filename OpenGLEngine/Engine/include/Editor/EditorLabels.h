#pragma once

#include <string>
#include "Ecs/Components/GroupComponent.h"
#include "Ecs/Components/MeshNameComponent.h"
#include "Ecs/Components/NameComponent.h"
#include "Ecs/Core/EcsWorld.h"

class EditorLabels
{
public:
    static std::string EntityLabel(EcsWorld& world, Entity entity)
    {
        std::string name;

        if (world.HasComponent<NameComponent>(entity))
            name = world.GetComponent<NameComponent>(entity).name;
        if (name.empty() && world.HasComponent<MeshNameComponent>(entity))
            name = world.GetComponent<MeshNameComponent>(entity).name;
        if (name.empty())
            name = "Entity";

        return name + " (" + std::to_string(entity) + ")";
    }

    static std::string GroupKey(EcsWorld& world, Entity entity)
    {
        if (world.HasComponent<GroupComponent>(entity))
            return world.GetComponent<GroupComponent>(entity).name;

        return EntityLabel(world, entity);
    }

    static std::string Trim(const std::string& value)
    {
        const auto first = value.find_first_not_of(" \t\n\r");
        if (first == std::string::npos)
            return "";

        const auto last = value.find_last_not_of(" \t\n\r");
        return value.substr(first, last - first + 1);
    }

    static bool IsGroupNameTaken(EcsWorld& world, const std::string& name, Entity ignoreEntity)
    {
        const std::string ignoreName = world.HasComponent<GroupComponent>(ignoreEntity) ? world.GetComponent<GroupComponent>(ignoreEntity).name : "";

        for (auto& [entity, group] : world.GetPool<GroupComponent>())
        {
            if (group.name != ignoreName && group.name == name)
                return true;
        }

        return false;
    }
};
