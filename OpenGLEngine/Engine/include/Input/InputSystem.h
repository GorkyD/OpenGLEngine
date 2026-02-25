#pragma once
#include <array>
#include "Extension/Extension.h"

struct GLFWwindow;

class InputSystem
{
	public:
		InputSystem(GLFWwindow* window);
		~InputSystem();

		void Update();

		float GetMouseDeltaX() const;
		float GetMouseDeltaY() const;

		bool IsKeyDown(Key key) const;
		bool IsLeftMouseDown() const { return leftMouseDown; }

	private:
		static void KeyCallback(GLFWwindow* w, int key, int scancode, int action, int mods);
		static void MouseButtonCallback(GLFWwindow* w, int button, int action, int mods);
		static void CursorPosCallback(GLFWwindow* w, double xPos, double yPos);
		static void FocusCallback(GLFWwindow* w, int focused);

		static Key TranslateKey(int glfwKey);
		void ClearState();

		float mouseDeltaX = 0;
		float mouseDeltaY = 0;
		double lastMouseX = 0;
		double lastMouseY = 0;
		bool firstMouse = true;

		std::array<bool, 256> keys = {};

		bool leftMouseDown = false;
		bool rightMouseDown = false;
};
