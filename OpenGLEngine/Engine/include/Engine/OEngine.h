#pragma once
#include <memory>
#include <chrono>

#include "Extension/OExtension.h"
#include "Input/InputSystem.h"

class ORenderEngine;
class OEntitySystem;
class OWindow;

class OEngine
{
	public:
		OEngine();
		virtual ~OEngine();

		OEntitySystem* GetEntitySystem() const {return entitySystem.get();}

		void Run();
		void Quit();
	private:
		void OnUpdateInternal();

	protected:
		virtual void OnCreate();
		virtual void OnUpdate(float deltaTime) {}
		virtual void OnQuit();

		std::unique_ptr<ORenderEngine> renderEngine;
		std::unique_ptr<OEntitySystem> entitySystem;
		std::unique_ptr<OWindow> window;

		std::shared_ptr<InputSystem> inputSystem;

		OVertexArrayObjectPtr polygonVaoPtr;
		OUniformBufferPtr uniformBufferPtr;
		OShaderProgramPtr shaderProgramPtr;

		std::chrono::system_clock::time_point previousTime;

		float speed = 1.5f;
		float scale = -3;

		bool is_Running = true;
};

															