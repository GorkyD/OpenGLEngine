#pragma once

#include <string>
#include <vector>
#include <imgui.h>
#include "Ecs/Components/AmbientLightComponent.h"
#include "Ecs/Components/LightComponent.h"
#include "Ecs/Core/EcsWorld.h"
#include "Editor/EditorUndoTracker.h"

class LightsPanel
{
public:
    void DrawLights(Engine& engine, EcsWorld& world)
    {
        for (auto& [entity, light] : world.GetPool<LightComponent>())
            DrawLightNode(engine, world, entity, light);
    }

    void DrawAmbient(Engine& engine, EcsWorld& world)
    {
        for (auto& [entity, ambient] : world.GetPool<AmbientLightComponent>())
            DrawAmbientNode(engine, world, entity, ambient);
    }

private:
    void DrawLightNode(Engine& engine, EcsWorld& world, Entity entity, LightComponent& light)
    {
        const std::string label = "Light " + std::to_string(entity);
        if (!ImGui::TreeNode(label.c_str()))
            return;

        const std::vector<Entity> target = {entity};
        auto edit = [&](const char* name, auto&& widget) { return undo.Edit(engine, world, target, StateLight, name, widget); };

        int type = static_cast<int>(light.type);
        edit("Light Type",
             [&]
             {
                 if (!ImGui::Combo("Type", &type, "Directional\0Point\0Spot\0"))
                     return false;
                 light.type = static_cast<LightType>(type);
                 return true;
             });

        edit("Light Color", [&] { return ImGui::ColorEdit3("Color", &light.color.x); });
        edit("Light Direction", [&] { return ImGui::DragFloat3("Direction", &light.direction.x, 0.01f); });
        edit("Light Position", [&] { return ImGui::DragFloat3("Position", &light.position.x, 0.01f); });
        edit("Light Intensity", [&] { return ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 5.0f); });
        edit("Light Range", [&] { return ImGui::DragFloat("Range", &light.range, 0.1f, 0.0f, 100.0f); });

        if (light.type == LightType::Spot)
        {
            edit("Spot Inner Cone", [&] { return ImGui::DragFloat("Inner Cone Angle", &light.innerConeAngleDeg, 0.5f, 0.0f, 89.0f); });
            edit("Spot Outer Cone", [&] { return ImGui::DragFloat("Outer Cone Angle", &light.outerConeAngleDeg, 0.5f, 0.0f, 89.0f); });
            if (light.outerConeAngleDeg < light.innerConeAngleDeg)
                light.outerConeAngleDeg = light.innerConeAngleDeg;
        }

        edit("Cast Shadows", [&] { return ImGui::Checkbox("Cast Shadows", &light.castShadows); });
        if (light.castShadows)
        {
            edit("Shadow Ortho Size", [&] { return ImGui::DragFloat("Shadow Ortho Size", &light.shadowOrthoSize, 0.5f, 1.0f, 200.0f); });
            edit("Shadow Distance", [&] { return ImGui::DragFloat("Shadow Distance", &light.shadowDistance, 0.5f, 1.0f, 500.0f); });
            edit("Shadow Focus Distance", [&] { return ImGui::DragFloat("Shadow Focus Distance", &light.shadowFocusDistance, 0.5f, 0.0f, 200.0f); });
            edit("Shadow Bias", [&] { return ImGui::DragFloat("Shadow Bias", &light.shadowBias, 0.0001f, 0.0f, 0.02f, "%.4f"); });
            edit("Shadow Normal Bias", [&] { return ImGui::DragFloat("Shadow Normal Bias", &light.shadowNormalBias, 0.001f, 0.0f, 1.0f, "%.3f"); });
            edit("Shadow Ambient Occlusion", [&] { return ImGui::SliderFloat("Shadow Ambient Occlusion", &light.shadowAmbientOcclusion, 0.0f, 1.0f); });
        }

        ImGui::TreePop();
    }

    void DrawAmbientNode(Engine& engine, EcsWorld& world, Entity entity, AmbientLightComponent& ambient)
    {
        const std::string label = "Ambient " + std::to_string(entity);
        if (!ImGui::TreeNode(label.c_str()))
            return;

        const std::vector<Entity> target = {entity};
        undo.Edit(engine, world, target, StateAmbient, "Ambient Color", [&] { return ImGui::ColorEdit3("Color", &ambient.color.x); });
        undo.Edit(engine, world, target, StateAmbient, "Ambient Intensity", [&] { return ImGui::SliderFloat("Intensity", &ambient.intensity, 0.0f, 5.0f); });

        ImGui::TreePop();
    }

    EditorUndoTracker undo;
};
