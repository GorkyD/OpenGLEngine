#pragma once

#include "Math/Vector4.h"

class Engine;

class IScene
{
public:
    virtual ~IScene() = default;

    virtual void OnLoad(Engine& engine) = 0;
    virtual void OnUnload(Engine& engine) {}
    virtual void OnUpdate(Engine& engine, float deltaTime) {}
    virtual void OnEditorUpdate(Engine& engine, float deltaTime) {}
    virtual void OnDebugUI(Engine& engine) {}

    virtual bool WantsCursorLocked() const
    {
        return false;
    }

    virtual Vector4 GetClearColor() const
    {
        return {0.0f, 0.0f, 0.0f, 1.0f};
    }
};
