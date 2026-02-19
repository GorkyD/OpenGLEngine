#pragma once
#include <memory>
#include <chrono>

#include "Extension/Extension.h"
#include "Input/InputSystem.h"
#include "Ecs/Core/EcsSystems.h"

class RenderEngine;
class Window;

class Engine
{
	public:
		Engine();
		virtual ~Engine();

		void Run();
		void Quit();
	private:
		void OnUpdateInternal();

	protected:
		virtual void OnCreate();
		virtual void OnUpdate(float deltaTime) {}
		virtual void OnQuit();


		EcsWorld world;
		std::unique_ptr<EcsSystems> systems;

		std::unique_ptr<RenderEngine> renderEngine;
		std::unique_ptr<Window> window;

		std::shared_ptr<InputSystem> inputSystem;

		std::chrono::system_clock::time_point previousTime;

		bool is_Running = true;
};
