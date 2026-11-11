#pragma once

#include "Ecs/Core/Entity.h"
#include "Math/Vector3.h"
#include "Systems/ForestBuilder.h"
#include "Systems/GameComponents.h"

class Engine;
class EcsWorld;

class CampfireSystem
{
public:
    void Configure(EcsWorld& world, const CampfireEntities& entities);
    void Update(EcsWorld& world, float deltaTime);
    bool Feed(EcsWorld& world);

    const CampfireComponent* Get(EcsWorld& world) const;
    Vector3 Position() const
    {
        return position;
    }

private:
    Entity entity = 0;
    Vector3 position = {0, 0, 0};
    Vector3 lightBase = {0, 0, 0};

    float baseFogStart = 8.0f;
    float baseFogEnd = 38.0f;
    bool fogCaptured = false;
};
