#pragma once

#include <string>
#include <imgui.h>
#include "Ecs/Components/AmbientLightComponent.h"
#include "Ecs/Components/LightComponent.h"
#include "Ecs/Core/EcsWorld.h"

class LightsPanel
{
public:
    void DrawLights(EcsWorld& world)
    {
        for (auto& [entity, light] : world.GetPool<LightComponent>())
            DrawLightNode(entity, light);
    }

    void DrawAmbient(EcsWorld& world)
    {
        for (auto& [entity, ambient] : world.GetPool<AmbientLightComponent>())
            DrawAmbientNode(entity, ambient);
    }

private:
    void DrawLightNode(Entity entity, LightComponent& light)
    {
        const std::string label = "Light " + std::to_string(entity);
        if (!ImGui::TreeNode(label.c_str()))
            return;

        int type = static_cast<int>(light.type);
        if (ImGui::Combo("Type", &type, "Directional\0Point\0"))
            light.type = static_cast<LightType>(type);

        ImGui::ColorEdit3("Color", &light.color.x);
        ImGui::DragFloat3("Direction", &light.direction.x, 0.01f);
        ImGui::DragFloat3("Position", &light.position.x, 0.01f);
        ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 5.0f);
        ImGui::DragFloat("Range", &light.range, 0.1f, 0.0f, 100.0f);

        ImGui::TreePop();
    }

    void DrawAmbientNode(Entity entity, AmbientLightComponent& ambient)
    {
        const std::string label = "Ambient " + std::to_string(entity);
        if (!ImGui::TreeNode(label.c_str()))
            return;

        ImGui::ColorEdit3("Color", &ambient.color.x);
        ImGui::SliderFloat("Intensity", &ambient.intensity, 0.0f, 5.0f);

        ImGui::TreePop();
    }
};
