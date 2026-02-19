#pragma once
#include "EcsWorld.h"

class IEcsSystem
{
	public:
	   virtual ~IEcsSystem() = default;
	   virtual void Init(EcsWorld& world) {}
	   virtual void Run(EcsWorld& world, float deltaTime) = 0;
};
