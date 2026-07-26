#pragma once

#include "Ecs/Components/CameraComponent.h"
#include "Ecs/Components/OutlineComponent.h"
#include "Ecs/Components/PickableComponent.h"
#include "Ecs/Components/TextComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Input/InputSystem.h"
#include "Math/Vector3.h"
#include "Render/Font.h"
#include "Window/Window.h"
#include <cmath>
#include <limits>

class HoverSystem : public IEcsSystem
{
public:
    HoverSystem(InputSystem* input, Window* window) : input(input), window(window) {}

    void Init(EcsWorld& world) override
    {
        const auto font = Font::LoadFromFile("Assets/Fonts/JetBrainsMono-Regular.ttf", 28.0f);
        if (!font)
            return;

        tooltipEntity = world.CreateEntity();
        hasTooltip = true;
        auto& tooltip = world.AddComponent<TextComponent>(tooltipEntity);
        tooltip.font = font;
        tooltip.text = "";
        tooltip.color = {1.0f, 0.92f, 0.6f, 1.0f};
        tooltip.scale = 0.7f;
        tooltip.visible = false;
    }

    void Run(EcsWorld& world, float deltaTime) override
    {
        auto& pickables = world.GetPool<PickableComponent>();
        auto& transforms = world.GetPool<TransformComponent>();
        auto& cameras = world.GetPool<CameraComponent>();

        Vector3 camPos, camForward, camRight, camUp;
        float fovY = 0.0f;
        bool haveCamera = false;

        for (auto& pair : cameras)
        {
            if (!pair.second.isActive || !transforms.Has(pair.first))
                continue;
            camPos = transforms.Get(pair.first).position;
            camForward = pair.second.forward;
            camRight = pair.second.right;
            camUp = pair.second.up;
            fovY = pair.second.fovY;
            haveCamera = true;
            break;
        }

        Entity hitEntity = 0;
        bool hasHit = false;

        if (haveCamera)
        {
            const Rect screen = window->GetInnerSize();
            if (screen.width > 0 && screen.height > 0)
            {
                const float aspect = static_cast<float>(screen.width) / static_cast<float>(screen.height);
                const float ndcX = (2.0f * input->GetMouseX() / static_cast<float>(screen.width)) - 1.0f;
                const float ndcY = 1.0f - (2.0f * input->GetMouseY() / static_cast<float>(screen.height));

                const float tanFovY = std::tan(fovY * 0.5f);
                const float tanFovX = tanFovY * aspect;

                Vector3 rayDir = camForward + camRight * (ndcX * tanFovX) + camUp * (ndcY * tanFovY);
                rayDir = Vector3::Normalize(rayDir);

                float closestDist = (std::numeric_limits<float>::max)();

                for (auto& pair : pickables)
                {
                    const Entity entity = pair.first;
                    if (!transforms.Has(entity))
                        continue;

                    const auto& transform = transforms.Get(entity);
                    const float radius = pair.second.radius * transform.scale.x;

                    const Vector3 toCenter = transform.position - camPos;
                    const float tCenter = Vector3::Dot(toCenter, rayDir);
                    if (tCenter < 0.0f)
                        continue;

                    const Vector3 closestPoint = camPos + rayDir * tCenter;
                    const Vector3 diff = transform.position - closestPoint;
                    const float distSq = Vector3::Dot(diff, diff);
                    if (distSq > radius * radius)
                        continue;

                    if (tCenter < closestDist)
                    {
                        closestDist = tCenter;
                        hitEntity = entity;
                        hasHit = true;
                    }
                }
            }
        }

        if (hasHit && (!hasHovered || hitEntity != hoveredEntity))
        {
            if (hasHovered)
                world.GetPool<OutlineComponent>().Remove(hoveredEntity);
            world.AddComponent<OutlineComponent>(hitEntity);
            hoveredEntity = hitEntity;
            hasHovered = true;
        }
        else if (!hasHit && hasHovered)
        {
            world.GetPool<OutlineComponent>().Remove(hoveredEntity);
            hasHovered = false;
        }

        if (hasTooltip && world.HasComponent<TextComponent>(tooltipEntity))
        {
            auto& tooltip = world.GetComponent<TextComponent>(tooltipEntity);
            if (hasHit)
            {
                tooltip.visible = true;
                tooltip.text = pickables.Get(hitEntity).name;
                tooltip.position = {input->GetMouseX() + 18.0f, input->GetMouseY() + 18.0f};
            }
            else
            {
                tooltip.visible = false;
            }
        }
    }

private:
    InputSystem* input;
    Window* window;

    Entity tooltipEntity = 0;
    Entity hoveredEntity = 0;

    bool hasTooltip = false;
    bool hasHovered = false;
};
