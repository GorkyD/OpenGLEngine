#pragma once

#include "IEcsSystem.h"

class EcsSystems
{
	public:
	   EcsSystems(EcsWorld& w) : world(w) {}

		EcsSystems& Add(std::unique_ptr<IEcsSystem> system);

		void Init() const;
		void Update(float deltaTime) const;

	private:
		EcsWorld& world;
		std::vector<std::unique_ptr<IEcsSystem>> systems;
};

