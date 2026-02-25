#pragma once
#include "Math/Rect.h"

struct GLFWwindow;

class Window
{
	public:
		Window();
		~Window();

		Rect GetInnerSize() const;

		void MakeCurrentContext() const;
		void Present(bool vSync) const;
		bool ShouldClose() const;

		void PollEvents();

		GLFWwindow* GetGLFWWindow() const { return window; }
	private:
		GLFWwindow* window = nullptr;
};
