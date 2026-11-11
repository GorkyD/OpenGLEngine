#pragma once

#include <array>
#include <string>
#include "Ecs/Core/Entity.h"
#include "Systems/Inventory.h"

class Engine;
class EcsWorld;

class GameHud
{
public:
    void Init(Engine& engine);
    void Update(Engine& engine, float health, int magazine, int reserve, float fuelRatio, const Inventory& inventory, const std::string& prompt, const std::string& message,
                const std::string& subMessage);

    const std::string& StatusText() const
    {
        return statusText;
    }
    const std::string& PromptText() const
    {
        return promptText;
    }
    const std::string& MessageText() const
    {
        return messageText;
    }

private:
    Entity CreateText(EcsWorld& world, float pixelHeight, bool large);
    void SetText(EcsWorld& world, Entity entity, const std::string& text, float x, float y, bool centered);

    Entity status = 0;
    Entity hint = 0;
    Entity prompt = 0;
    Entity message = 0;
    Entity subMessage = 0;
    std::array<Entity, Inventory::Capacity> slots = {};

    std::string statusText;
    std::string promptText;
    std::string messageText;
};
