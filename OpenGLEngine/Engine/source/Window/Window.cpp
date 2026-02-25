#include "Window/Window.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cassert>

Window::Window()
{
	int ok = glfwInit();
	assert(ok);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	window = glfwCreateWindow(1024, 768, "OpenGLEngine", nullptr, nullptr);
	assert(window);

	glfwMakeContextCurrent(window);

	const int gladOk = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	assert(gladOk);
}

Window::~Window()
{
	if (window)
		glfwDestroyWindow(window);
	glfwTerminate();
}

Rect Window::GetInnerSize() const
{
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	return { w, h };
}

void Window::MakeCurrentContext() const
{
	glfwMakeContextCurrent(window);
}

void Window::Present(bool vSync) const
{
	glfwSwapInterval(vSync ? 1 : 0);
	glfwSwapBuffers(window);
}

bool Window::ShouldClose() const
{
	return glfwWindowShouldClose(window);
}

void Window::PollEvents()
{
	glfwPollEvents();
}
