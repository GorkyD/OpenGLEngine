#pragma once
#include "Ecs/Components/MeshComponent.h"
#include "Ecs/Components/ShaderComponent.h"
#include "Ecs/Components/MaterialComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Render/RenderEngine.h"
#include "Render/Texture.h"
#include <glad/glad.h>

class RenderSystem : public IEcsSystem
{
public:
	RenderSystem(RenderEngine* re) : renderEngine(re) {}

	void Run(EcsWorld& world, float deltaTime) override
	{
		auto& meshes = world.GetPool<MeshComponent>();
		auto& shaders = world.GetPool<ShaderComponent>();
		auto& materials = world.GetPool<MaterialComponent>();

		for (auto& pair : meshes)
		{
			Entity entity = pair.first;
			auto& mesh = pair.second;

			if (!shaders.Has(entity)) continue;

			renderEngine->SetShaderProgram(shaders.Get(entity).shader);

			if (materials.Has(entity))
			{
				auto& mat = materials.Get(entity);
				if (mat.diffuseTexture)
				{
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, mat.diffuseTexture->GetId());
				}
			}

			renderEngine->SetVertexArrayObject(mesh.vao);
			renderEngine->DrawIndexedTriangles(List, mesh.indexCount);
		}
	}

private:
	RenderEngine* renderEngine;
};
