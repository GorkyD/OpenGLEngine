#pragma once
#include <memory>
#include <chrono>

#include "Extension/Extension.h"
#include "Input/InputSystem.h"
#include "Camera/Camera.h"
#include "Camera/CameraMovement.h"

class RenderEngine;
class EntitySystem;
class Window;

class Engine
{
	public:
		Engine();
		virtual ~Engine();

		EntitySystem* GetEntitySystem() const {return entitySystem.get();}

		void Run();
		void Quit();
	private:
		void OnUpdateInternal();

	protected:
		virtual void OnCreate();
		virtual void OnUpdate(float deltaTime) {}
		virtual void OnQuit();

		std::unique_ptr<RenderEngine> renderEngine;
		std::unique_ptr<EntitySystem> entitySystem;
		std::unique_ptr<Window> window;

		std::shared_ptr<InputSystem> inputSystem;

		Camera camera;
		CameraMovement cameraMovement;

		VertexArrayObjectPtr polygonVaoPtr;
		UniformBufferPtr uniformBufferPtr;
		ShaderProgramPtr shaderProgramPtr;

		std::chrono::system_clock::time_point previousTime;

		float speed = 1.5f;
		float scale = -3;

		bool is_Running = true;
};
