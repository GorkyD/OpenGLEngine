#include "Input/InputSystem.h"
#include <GLFW/glfw3.h>

InputSystem::InputSystem(GLFWwindow* window)
{
	glfwSetWindowUserPointer(window, this);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetCursorPosCallback(window, CursorPosCallback);
	glfwSetWindowFocusCallback(window, FocusCallback);
}

InputSystem::~InputSystem() {}

void InputSystem::KeyCallback(GLFWwindow* w, int key, int scancode, int action, int mods)
{
	if (action == GLFW_REPEAT) return;

	auto* self = static_cast<InputSystem*>(glfwGetWindowUserPointer(w));
	Key k = TranslateKey(key);
	if (k != Key::Unknown)
	{
		const bool isDown = (action == GLFW_PRESS);
		self->keys[static_cast<size_t>(k)] = isDown;
		OGL_INFO("INPUT key=" << static_cast<int>(k) << " glfw=" << key << " " << (isDown ? "DOWN" : "UP"));
	}
}

void InputSystem::MouseButtonCallback(GLFWwindow* w, int button, int action, int mods)
{
	auto* self = static_cast<InputSystem*>(glfwGetWindowUserPointer(w));
	const bool pressed = (action == GLFW_PRESS);

	if (button == GLFW_MOUSE_BUTTON_LEFT)  self->leftMouseDown = pressed;
	if (button == GLFW_MOUSE_BUTTON_RIGHT) self->rightMouseDown = pressed;
}

void InputSystem::CursorPosCallback(GLFWwindow* w, double xPos, double yPos)
{
	auto* self = static_cast<InputSystem*>(glfwGetWindowUserPointer(w));

	if (self->firstMouse)
	{
		self->lastMouseX = xPos;
		self->lastMouseY = yPos;
		self->firstMouse = false;
		return;
	}

	self->mouseDeltaX += static_cast<float>(xPos - self->lastMouseX);
	self->mouseDeltaY += static_cast<float>(yPos - self->lastMouseY);
	self->lastMouseX = xPos;
	self->lastMouseY = yPos;
}

void InputSystem::FocusCallback(GLFWwindow* w, int focused)
{
	if (!focused)
	{
		auto* self = static_cast<InputSystem*>(glfwGetWindowUserPointer(w));
		self->ClearState();
	}
}

void InputSystem::ClearState()
{
	keys.fill(false);
	mouseDeltaX = 0;
	mouseDeltaY = 0;
	leftMouseDown = false;
	rightMouseDown = false;
	firstMouse = true;
}

void InputSystem::Update()
{
	mouseDeltaX = 0;
	mouseDeltaY = 0;
}

bool InputSystem::IsKeyDown(Key key) const
{
	const size_t index = static_cast<size_t>(key);
	if (index >= static_cast<size_t>(Key::Count))
		return false;

	return keys[index];
}

Key InputSystem::TranslateKey(int glfwKey)
{
	switch (glfwKey)
	{
		case GLFW_KEY_A: return Key::A;
		case GLFW_KEY_B: return Key::B;
		case GLFW_KEY_C: return Key::C;
		case GLFW_KEY_D: return Key::D;
		case GLFW_KEY_E: return Key::E;
		case GLFW_KEY_F: return Key::F;
		case GLFW_KEY_G: return Key::G;
		case GLFW_KEY_H: return Key::H;
		case GLFW_KEY_I: return Key::I;
		case GLFW_KEY_J: return Key::J;
		case GLFW_KEY_K: return Key::K;
		case GLFW_KEY_L: return Key::L;
		case GLFW_KEY_M: return Key::M;
		case GLFW_KEY_N: return Key::N;
		case GLFW_KEY_O: return Key::O;
		case GLFW_KEY_P: return Key::P;
		case GLFW_KEY_Q: return Key::Q;
		case GLFW_KEY_R: return Key::R;
		case GLFW_KEY_S: return Key::S;
		case GLFW_KEY_T: return Key::T;
		case GLFW_KEY_U: return Key::U;
		case GLFW_KEY_V: return Key::V;
		case GLFW_KEY_W: return Key::W;
		case GLFW_KEY_X: return Key::X;
		case GLFW_KEY_Y: return Key::Y;
		case GLFW_KEY_Z: return Key::Z;

		case GLFW_KEY_ESCAPE: return Key::Escape;
		case GLFW_KEY_SPACE:  return Key::Space;
		case GLFW_KEY_ENTER:  return Key::Enter;

		case GLFW_KEY_LEFT:  return Key::Left;
		case GLFW_KEY_RIGHT: return Key::Right;
		case GLFW_KEY_UP:    return Key::Up;
		case GLFW_KEY_DOWN:  return Key::Down;

		case GLFW_KEY_LEFT_SHIFT:    return Key::LShift;
		case GLFW_KEY_RIGHT_SHIFT:   return Key::RShift;
		case GLFW_KEY_LEFT_CONTROL:  return Key::LControl;
		case GLFW_KEY_RIGHT_CONTROL: return Key::RControl;
		case GLFW_KEY_LEFT_ALT:      return Key::LAlt;
		case GLFW_KEY_RIGHT_ALT:     return Key::RAlt;

		default: return Key::Unknown;
	}
}

float InputSystem::GetMouseDeltaX() const { return mouseDeltaX; }
float InputSystem::GetMouseDeltaY() const { return mouseDeltaY; }
