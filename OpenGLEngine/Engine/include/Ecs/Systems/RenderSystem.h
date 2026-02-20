#pragma once
#include "Ecs/Components/MeshComponent.h"
#include "Ecs/Components/ShaderComponent.h"
#include "Ecs/Components/MaterialComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Render/RenderEngine.h"
#include "Render/UniformBuffer.h"
#include "Render/Texture.h"
#include "Math/Matrix4.h"
#include "Math/Vector4.h"
#include <glad/glad.h>

class RenderSystem : public IEcsSystem
{
public:
	RenderSystem(RenderEngine* re, UniformBufferPtr ub) : renderEngine(re), uniformBuffer(ub) {}

	void Run(EcsWorld& world, float deltaTime) override
	{
		auto& meshes = world.GetPool<MeshComponent>();
		auto& shaders = world.GetPool<ShaderComponent>();
		auto& materials = world.GetPool<MaterialComponent>();
		auto& transforms = world.GetPool<TransformComponent>();

		for (auto& pair : meshes)
		{
			Entity entity = pair.first;
			auto& mesh = pair.second;

			if (!shaders.Has(entity)) continue;

			Matrix4 worldMatrix;
			if (transforms.Has(entity))
				worldMatrix = transforms.Get(entity).GetModelMatrix();

			uniformBuffer->SetSubData(&worldMatrix, 0, sizeof(Matrix4));

			auto& shaderComp = shaders.Get(entity);
			renderEngine->SetShaderProgram(shaderComp.shader);

			Vector4 color = { 1, 1, 1, 1 };
			int hasTexture = 0;
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, 0);

			if (materials.Has(entity))
			{
				auto& mat = materials.Get(entity);
				color = mat.diffuseColor;
				if (mat.diffuseTexture)
				{
					glBindTexture(GL_TEXTURE_2D, mat.diffuseTexture->GetId());
					hasTexture = 1;
				}
			}

			int colorLoc = glGetUniformLocation(shaderComp.shader->GetId(), "diffuseColor");
			glUniform4f(colorLoc, color.x, color.y, color.z, color.w);

			int hasTexLoc = glGetUniformLocation(shaderComp.shader->GetId(), "hasTexture");
			glUniform1i(hasTexLoc, hasTexture);

			renderEngine->SetVertexArrayObject(mesh.vao);
			renderEngine->DrawIndexedTriangles(List, mesh.indexCount);
		}
	}

private:
	RenderEngine* renderEngine;
	UniformBufferPtr uniformBuffer;
};
