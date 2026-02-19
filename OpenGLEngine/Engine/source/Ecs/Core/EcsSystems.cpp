#include "Ecs/Core/EcsSystems.h"

EcsSystems& EcsSystems::Add(std::unique_ptr<IEcsSystem> system)
{
	systems.push_back(std::move(system));
	return *this;
}

void EcsSystems::Init() const
{
	for (auto& s : systems) 
		s->Init(world);
}


void EcsSystems::Update(float deltaTime) const
{
	for (auto& s : systems) 
		s->Run(world, deltaTime);

	world.ProcessDeferred();
}
