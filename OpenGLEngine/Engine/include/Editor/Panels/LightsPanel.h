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
        if (ImGui::Combo("Type", &type, "Directional\0Point\0Spot\0"))
            light.type = static_cast<LightType>(type);

        ImGui::ColorEdit3("Color", &light.color.x);
        ImGui::DragFloat3("Direction", &light.direction.x, 0.01f);
        ImGui::DragFloat3("Position", &light.position.x, 0.01f);
        ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 5.0f);
        ImGui::DragFloat("Range", &light.range, 0.1f, 0.0f, 100.0f);

        if (light.type == LightType::Spot)
        {
            ImGui::DragFloat("Inner Cone Angle", &light.innerConeAngleDeg, 0.5f, 0.0f, 89.0f);
            ImGui::DragFloat("Outer Cone Angle", &light.outerConeAngleDeg, 0.5f, 0.0f, 89.0f);
            if (light.outerConeAngleDeg < light.innerConeAngleDeg)
                light.outerConeAngleDeg = light.innerConeAngleDeg;
        }

        ImGui::Checkbox("Cast Shadows", &light.castShadows);
        if (light.castShadows)
        {
            ImGui::DragFloat("Shadow Ortho Size", &light.shadowOrthoSize, 0.5f, 1.0f, 200.0f);
            ImGui::DragFloat("Shadow Distance", &light.shadowDistance, 0.5f, 1.0f, 500.0f);
            ImGui::DragFloat("Shadow Focus Distance", &light.shadowFocusDistance, 0.5f, 0.0f, 200.0f);
            ImGui::DragFloat("Shadow Bias", &light.shadowBias, 0.0001f, 0.0f, 0.02f, "%.4f");
            ImGui::DragFloat("Shadow Normal Bias", &light.shadowNormalBias, 0.001f, 0.0f, 1.0f, "%.3f");
            ImGui::SliderFloat("Shadow Ambient Occlusion", &light.shadowAmbientOcclusion, 0.0f, 1.0f);
        }

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
