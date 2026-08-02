#pragma once

#include "ComponentPool.h"
#include "Entity.h"
#include <cstddef>
#include <memory>
#include <vector>

class EcsWorld
{
public:
    Entity CreateEntity();

    void DestroyEntity(Entity entity);
    void ProcessDeferred();
    void Clear();

    template <typename T> ComponentPool<T>& GetPool()
    {
        const size_t typeId = ComponentTypeId<T>();

        if (typeId >= pools.size())
            pools.resize(typeId + 1);

        if (!pools[typeId])
            pools[typeId] = std::make_unique<ComponentPool<T>>();

        return *static_cast<ComponentPool<T>*>(pools[typeId].get());
    }

    template <typename T> T& AddComponent(Entity entity)
    {
        return GetPool<T>().Add(entity);
    }

    template <typename T> T& GetComponent(Entity entity)
    {
        return GetPool<T>().Get(entity);
    }

    template <typename T> bool HasComponent(Entity entity)
    {
        return GetPool<T>().Has(entity);
    }

private:
    static size_t NextComponentTypeId()
    {
        static size_t counter = 0;
        return counter++;
    }

    template <typename T> static size_t ComponentTypeId()
    {
        static const size_t id = NextComponentTypeId();
        return id;
    }

    Entity nextEntity = 0;
    std::vector<std::unique_ptr<IComponentPool>> pools;
    std::vector<Entity> alive;
    std::vector<Entity> toDestroy;
};
