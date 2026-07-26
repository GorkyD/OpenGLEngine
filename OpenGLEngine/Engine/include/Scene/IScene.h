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

    virtual Vector4 GetClearColor() const
    {
        return {0.0f, 0.0f, 0.0f, 1.0f};
    }
};
