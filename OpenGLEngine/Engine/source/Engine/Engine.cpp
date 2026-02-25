#include <chrono>
#include "Window/Window.h"
#include "Engine/Engine.h"
#include "Math/Vector4.h"
#include "Render/RenderEngine.h"

Engine::Engine()
{
	window = std::make_unique<Window>();
	renderEngine = std::make_unique<RenderEngine>();
	inputSystem = std::make_shared<InputSystem>(window->GetGLFWWindow());
	renderEngine->SetViewPort(window->GetInnerSize());
}

Engine::~Engine(){}

void Engine::OnCreate(){}

void Engine::Run()
{
	OnCreate();
	while (is_Running && !window->ShouldClose())
	{
		window->PollEvents();
		OnUpdateInternal();
	}
	OnQuit();
}

void Engine::OnUpdateInternal()
{
	const auto currentTime = std::chrono::system_clock::now();
	auto elapsedSeconds = std::chrono::duration<double>();
	if (previousTime.time_since_epoch().count())
		elapsedSeconds = currentTime - previousTime;
	previousTime = currentTime;

	const auto deltaTime = static_cast<float>(elapsedSeconds.count());

	renderEngine->Clear(Vector4(0, 0, 0, 1));
	renderEngine->SetFaceCulling(CullingType::BackFace);
	renderEngine->SetWindingOrder(ClockWise);

	systems->Update(deltaTime);
	OnUpdate(deltaTime);
	inputSystem->Update();

	window->Present(false);
}

void Engine::OnQuit() {}

void Engine::Quit() { is_Running = false;}
