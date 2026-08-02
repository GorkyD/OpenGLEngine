#pragma once

#include <string>
#include "Ecs/Core/Entity.h"

struct ParentComponent
{
    Entity parent = INVALID_ENTITY;
    std::string attachBone;
};
