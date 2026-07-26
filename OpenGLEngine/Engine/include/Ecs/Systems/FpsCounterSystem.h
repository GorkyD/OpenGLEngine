#pragma once

#include "Ecs/Components/TextComponent.h"
#include "Ecs/Core/Entity.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Render/Font.h"
#include "Window/Window.h"
#include <cstdio>

class FpsCounterSystem : public IEcsSystem
{
public:
    explicit FpsCounterSystem(Window* window) : window(window) {}

    void Init(EcsWorld& world) override
    {
        auto font = Font::LoadFromFile("Assets/Fonts/JetBrainsMono-Regular.ttf", 22.0f);
        if (!font)
            return;

        entity = world.CreateEntity();

        auto& text = world.AddComponent<TextComponent>(entity);
        text.font = font;
        text.text = "FPS: --";
        text.color = {0.05f, 0.05f, 0.05f, 1.0f};
        text.scale = 1.0f;

        UpdatePosition(text);
    }

    void Run(EcsWorld& world, float deltaTime) override
    {
        if (entity == 0 || !world.HasComponent<TextComponent>(entity))
            return;

        auto& text = world.GetComponent<TextComponent>(entity);

        accumulatedTime += deltaTime;
        frameCount++;

        constexpr float updateInterval = 0.25f;
        if (accumulatedTime < updateInterval)
            return;

        const float fps = accumulatedTime > 0.0f ? static_cast<float>(frameCount) / accumulatedTime : 0.0f;
        accumulatedTime = 0.0f;
        frameCount = 0;

        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "FPS: %.0f", fps);
        text.text = buffer;

        UpdatePosition(text);
    }

private:
    void UpdatePosition(TextComponent& text)
    {
        const Rect screen = window->GetInnerSize();
        const float textWidth = text.font->MeasureWidth(text.text, text.scale);
        constexpr float margin = 10.0f;
        text.position = {static_cast<float>(screen.width) - textWidth - margin, margin};
    }

    Window* window;

    Entity entity = 0;
    float accumulatedTime = 0.0f;
    int frameCount = 0;
};
