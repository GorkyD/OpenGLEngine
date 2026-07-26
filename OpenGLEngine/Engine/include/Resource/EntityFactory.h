#pragma once

#include <string>
#include "Ecs/Core/Entity.h"
#include "Extension/Extension.h"

class EcsWorld;
class RenderEngine;

class EntityFactory
{
public:
    static Entity CreateModelEntity(EcsWorld& world, RenderEngine* renderEngine, const std::string& path, ShaderProgramPtr shader);
};
