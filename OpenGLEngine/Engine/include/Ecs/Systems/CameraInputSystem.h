#pragma once
#include "Ecs/Components/CameraComponent.h"
#include "Ecs/Components/FpsControlComponent.h"
#include "Ecs/Components/TransformComponent.h"
#include "Ecs/Core/IEcsSystem.h"
#include "Input/InputSystem.h"
#include "Math/Vector3.h"

class CameraInputSystem : public IEcsSystem
{
	public:
	   CameraInputSystem(InputSystem* input) : input(input) {}

	   void Run(EcsWorld& world, float deltaTime) override
	   {
	      auto& controllers = world.GetPool<FpsControllerComponent>();
	      auto& cameras = world.GetPool<CameraComponent>();
	      auto& transforms = world.GetPool<TransformComponent>();

	      for (auto& pair : controllers)
	      {
				Entity entity = pair.first;
				auto& ctrl = pair.second;

	         if (!cameras.Has(entity) || !transforms.Has(entity)) continue;

	         auto& cam = cameras.Get(entity);
	         auto& transform = transforms.Get(entity);

	         if (input->IsLeftMouseDown())
	         {
	            cam.yaw += input->GetMouseDeltaX() * ctrl.sensitivity;
	            cam.pitch -= input->GetMouseDeltaY() * ctrl.sensitivity;
	            if (cam.pitch < -ctrl.maxPitch) cam.pitch = -ctrl.maxPitch;
	            if (cam.pitch > ctrl.maxPitch) cam.pitch = ctrl.maxPitch;
	         }

	         cam.forward.x = std::cos(cam.pitch) * std::sin(cam.yaw);
	         cam.forward.y = std::sin(cam.pitch);
	         cam.forward.z = std::cos(cam.pitch) * std::cos(cam.yaw);
	         cam.forward = Vector3::Normalize(cam.forward);

	         Vector3 worldUp(0, 1, 0);
	         cam.right = Vector3::Normalize(Vector3::Cross(worldUp, cam.forward));
	         cam.up = Vector3::Cross(cam.forward, cam.right);

	         Vector3 moveDir(0, 0, 0);
	         if (input->IsKeyDown(Key::W)) moveDir += cam.forward;
	         if (input->IsKeyDown(Key::S)) moveDir -= cam.forward;
	         if (input->IsKeyDown(Key::D)) moveDir += cam.right;
	         if (input->IsKeyDown(Key::A)) moveDir -= cam.right;
	         if (input->IsKeyDown(Key::Space))  moveDir += worldUp;
	         if (input->IsKeyDown(Key::LShift)) moveDir -= worldUp;

	         moveDir = Vector3::Normalize(moveDir);
	         transform.position += moveDir * (ctrl.moveSpeed * deltaTime);
	      }
	   }

	private:
		InputSystem* input;
};
