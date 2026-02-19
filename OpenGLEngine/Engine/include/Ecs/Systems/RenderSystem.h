#pragma once
#include "Ecs/Components/MeshComponent.h"
#include "Ecs/Components/ShaderComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Render/RenderEngine.h"

class RenderSystem : public IEcsSystem
{
public:
	RenderSystem(RenderEngine* re) : renderEngine(re) {}

	void Run(EcsWorld& world, float deltaTime) override
	{
		auto& meshes = world.GetPool<MeshComponent>();
		auto& shaders = world.GetPool<ShaderComponent>();

		for (auto& pair : meshes)
		{
			Entity entity = pair.first;
			auto& mesh = pair.second;

			if (!shaders.Has(entity)) continue;

			renderEngine->SetVertexArrayObject(mesh.vao);
			renderEngine->SetShaderProgram(shaders.Get(entity).shader);
			renderEngine->DrawIndexedTriangles(List, mesh.indexCount);
		}
	}

private:
	RenderEngine* renderEngine;
};
