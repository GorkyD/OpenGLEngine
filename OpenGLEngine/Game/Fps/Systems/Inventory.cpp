#include "Systems/Inventory.h"
#include <algorithm>

int Inventory::Add(ItemId item, int count)
{
    if (item == ItemId::None || count <= 0)
        return 0;

    const int maxStack = ItemMaxStack(item);
    int remaining = count;

    for (auto& slot : slots)
    {
        if (remaining <= 0)
            break;
        if (slot.item != item || slot.count >= maxStack)
            continue;
        const int moved = (std::min)(remaining, maxStack - slot.count);
        slot.count += moved;
        remaining -= moved;
    }

    for (auto& slot : slots)
    {
        if (remaining <= 0)
            break;
        if (slot.item != ItemId::None)
            continue;
        const int moved = (std::min)(remaining, maxStack);
        slot.item = item;
        slot.count = moved;
        remaining -= moved;
    }

    return count - remaining;
}

int Inventory::Remove(ItemId item, int count)
{
    int removed = 0;

    for (auto it = slots.rbegin(); it != slots.rend() && removed < count; ++it)
    {
        if (it->item != item)
            continue;
        const int taken = (std::min)(count - removed, it->count);
        it->count -= taken;
        removed += taken;
        if (it->count == 0)
            it->item = ItemId::None;
    }

    return removed;
}

int Inventory::Count(ItemId item) const
{
    int total = 0;
    for (const auto& slot : slots)
    {
        if (slot.item == item)
            total += slot.count;
    }
    return total;
}

int Inventory::UsedSlots() const
{
    int used = 0;
    for (const auto& slot : slots)
    {
        if (slot.item != ItemId::None)
            used++;
    }
    return used;
}

void Inventory::Clear()
{
    slots = {};
}
