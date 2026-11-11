#include "Systems/GameHud.h"
#include <cstdio>
#include "Ecs/Components/TextComponent.h"
#include "Ecs/Core/EcsWorld.h"
#include "Engine/Engine.h"
#include "Render/Font.h"
#include "Window/Window.h"

namespace
{
const Vector4 HudColor = {0.92f, 0.86f, 0.72f, 1.0f};
const Vector4 DimColor = {0.6f, 0.56f, 0.48f, 1.0f};
const Vector4 AlertColor = {0.85f, 0.12f, 0.08f, 1.0f};
}

Entity GameHud::CreateText(EcsWorld& world, float pixelHeight, bool large)
{
    const auto font = Font::LoadFromFile("Assets/Fonts/JetBrainsMono-Regular.ttf", pixelHeight);
    if (!font)
        return 0;

    const Entity entity = world.CreateEntity();
    auto& text = world.AddComponent<TextComponent>(entity);
    text.font = font;
    text.color = large ? AlertColor : HudColor;
    text.visible = false;
    return entity;
}

void GameHud::Init(Engine& engine)
{
    auto& world = engine.GetWorld();
    status = CreateText(world, 22.0f, false);
    hint = CreateText(world, 18.0f, false);
    prompt = CreateText(world, 22.0f, false);
    message = CreateText(world, 56.0f, true);
    subMessage = CreateText(world, 22.0f, false);
    for (auto& slot : slots)
        slot = CreateText(world, 18.0f, false);

    if (world.HasComponent<TextComponent>(hint))
        world.GetComponent<TextComponent>(hint).color = DimColor;
}

void GameHud::SetText(EcsWorld& world, Entity entity, const std::string& text, float x, float y, bool centered)
{
    if (entity == 0 || !world.HasComponent<TextComponent>(entity))
        return;

    auto& component = world.GetComponent<TextComponent>(entity);
    component.text = text;
    component.visible = !text.empty();
    const float width = component.font ? component.font->MeasureWidth(text, component.scale) : 0.0f;
    component.position = {centered ? x - width * 0.5f : x, y};
}

void GameHud::Update(Engine& engine, float health, int magazine, int reserve, float fuelRatio, const Inventory& inventory, const std::string& promptValue,
                     const std::string& messageValue, const std::string& subMessageValue)
{
    auto& world = engine.GetWorld();
    const Rect screen = engine.GetWindow()->GetInnerSize();
    const float width = static_cast<float>(screen.width);
    const float height = static_cast<float>(screen.height);

    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "HP %3.0f    AMMO %d / %d    FIRE %3.0f%%", health, magazine, reserve, fuelRatio * 100.0f);
    statusText = buffer;
    SetText(world, status, statusText, 28.0f, height - 70.0f, false);

    if (world.HasComponent<TextComponent>(status))
        world.GetComponent<TextComponent>(status).color = health < 30.0f ? AlertColor : HudColor;

    SetText(world, hint, "[E] use  [H] bandage  [R] reload  [V] axe  [L] light", 28.0f, height - 36.0f, false);

    float slotY = 60.0f;
    for (int i = 0; i < Inventory::Capacity; i++)
    {
        const auto& slot = inventory.Slots()[i];
        std::string line;
        if (slot.item != ItemId::None)
        {
            std::snprintf(buffer, sizeof(buffer), "%d  %s x%d", i + 1, ItemName(slot.item), slot.count);
            line = buffer;
        }
        else
        {
            std::snprintf(buffer, sizeof(buffer), "%d  -", i + 1);
            line = buffer;
        }
        SetText(world, slots[i], line, 28.0f, slotY, false);
        if (world.HasComponent<TextComponent>(slots[i]))
            world.GetComponent<TextComponent>(slots[i]).color = slot.item != ItemId::None ? HudColor : DimColor;
        slotY += 24.0f;
    }

    promptText = promptValue;
    SetText(world, prompt, promptText, width * 0.5f, height * 0.62f, true);

    messageText = messageValue;
    SetText(world, message, messageText, width * 0.5f, height * 0.45f, true);
    SetText(world, subMessage, subMessageValue, width * 0.5f, height * 0.45f + 48.0f, true);
}
