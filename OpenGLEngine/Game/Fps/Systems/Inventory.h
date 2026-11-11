#pragma once

#include <array>
#include <string>
#include "Systems/GameComponents.h"

class Inventory
{
public:
    struct Slot
    {
        ItemId item = ItemId::None;
        int count = 0;
    };

    static constexpr int Capacity = 8;

    int Add(ItemId item, int count);
    int Remove(ItemId item, int count);
    int Count(ItemId item) const;
    int UsedSlots() const;
    void Clear();

    const std::array<Slot, Capacity>& Slots() const
    {
        return slots;
    }

private:
    std::array<Slot, Capacity> slots = {};
};
