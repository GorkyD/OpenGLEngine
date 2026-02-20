#pragma once

#include "Ecs/Components/CameraComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Extension/Extension.h"
#include "Render/RenderEngine.h"
#include "Render/UniformBuffer.h"
#include "Render/UniformData.h"
#include "Window/Window.h"

class CameraMatrixSystem : public IEcsSystem
{
	public:
	   CameraMatrixSystem(RenderEngine* re, UniformBufferPtr ub, Window* w) : renderEngine(re), uniformBuffer(ub), window(w) {}

	   void Run(EcsWorld& world, float deltaTime) override
	   {
	      auto& cameras = world.GetPool<CameraComponent>();
	      auto& transforms = world.GetPool<TransformComponent>();

	      for (auto& pair : cameras)
	      {
				Entity entity = pair.first;
				auto& cam = pair.second;

	         if (!cam.isActive || !transforms.Has(entity)) continue;

				const auto& pos = transforms.Get(entity).position;

	         Matrix4 view;
	         view.SetLookAtLeftHanded(pos, pos + cam.forward, Vector3(0, 1, 0));

				const auto displaySize = window->GetInnerSize();
				const float aspect = static_cast<float>(displaySize.width) / static_cast<float>(displaySize.height);

	         Matrix4 projection;
	         projection.SetPerspectiveLeftHanded(
	            cam.fovY, aspect, cam.nearPlane, cam.farPlane);

				UniformData data = { Matrix4(), view, projection };
	         uniformBuffer->SetData(&data);
	         renderEngine->SetUniformBuffer(uniformBuffer, 0);

	         break;
	      }
	   }

	private:
	RenderEngine* renderEngine;
	UniformBufferPtr uniformBuffer;
	Window* window;
};
