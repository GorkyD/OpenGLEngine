#pragma once

class Engine;

class IScene
{
public:
    virtual ~IScene() = default;

    virtual void OnLoad(Engine& engine) = 0;
    virtual void OnUnload(Engine& engine) {}
    virtual void OnUpdate(Engine& engine, float deltaTime) {}
};
