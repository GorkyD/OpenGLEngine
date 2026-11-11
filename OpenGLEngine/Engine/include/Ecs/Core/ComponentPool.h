#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <stdexcept>
#include <utility>
#include <vector>
#include "Entity.h"

class IComponentPool
{
public:
    virtual ~IComponentPool() = default;
    virtual void Remove(Entity entity) = 0;
    virtual bool Has(Entity entity) const = 0;
    virtual void Clear() = 0;
};

template <typename T> class ComponentPool : public IComponentPool
{
    static constexpr uint32_t NoIndex = UINT32_MAX;

    std::deque<std::pair<Entity, T>> dense;
    std::vector<uint32_t> sparse;

    uint32_t IndexOf(Entity entity) const
    {
        return entity < sparse.size() ? sparse[entity] : NoIndex;
    }

public:
    class Iterator
    {
        ComponentPool* pool;
        size_t index;

    public:
        Iterator(ComponentPool* pool, size_t index) : pool(pool), index(index) {}

        std::pair<Entity, T>& operator*() const
        {
            return pool->dense[index];
        }
        std::pair<Entity, T>* operator->() const
        {
            return &pool->dense[index];
        }
        Iterator& operator++()
        {
            ++index;
            return *this;
        }
        bool operator!=(const Iterator&) const
        {
            return index < pool->dense.size();
        }
        bool operator==(const Iterator& other) const
        {
            return !(*this != other);
        }
    };

    T& Add(Entity entity)
    {
        const uint32_t existing = IndexOf(entity);
        if (existing != NoIndex)
            return dense[existing].second;

        if (entity >= sparse.size())
            sparse.resize(static_cast<size_t>(entity) + 1, NoIndex);

        sparse[entity] = static_cast<uint32_t>(dense.size());
        dense.emplace_back(entity, T{});
        return dense.back().second;
    }

    T& Get(Entity entity)
    {
        const uint32_t index = IndexOf(entity);
        if (index == NoIndex)
            throw std::out_of_range("ComponentPool::Get: entity has no such component");
        return dense[index].second;
    }

    bool Has(Entity entity) const override
    {
        return IndexOf(entity) != NoIndex;
    }

    void Remove(Entity entity) override
    {
        const uint32_t index = IndexOf(entity);
        if (index == NoIndex)
            return;

        const uint32_t last = static_cast<uint32_t>(dense.size() - 1);
        if (index != last)
        {
            dense[index] = std::move(dense[last]);
            sparse[dense[index].first] = index;
        }

        dense.pop_back();
        sparse[entity] = NoIndex;
    }

    void Clear() override
    {
        dense.clear();
        sparse.clear();
    }

    size_t Size() const
    {
        return dense.size();
    }

    Iterator begin()
    {
        return Iterator(this, 0);
    }
    Iterator end()
    {
        return Iterator(this, dense.size());
    }
};
