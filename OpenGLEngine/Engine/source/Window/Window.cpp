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

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = monitor ? glfwGetVideoMode(monitor) : nullptr;

    if (mode)
    {
        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
        window = glfwCreateWindow(mode->width, mode->height, "OpenGLEngine", monitor, nullptr);
        glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
        glfwSetWindowPos(window, 0, 0);
        glfwSetWindowSize(window, mode->width, mode->height);
    }
    else
    {
        window = glfwCreateWindow(1024, 768, "OpenGLEngine", nullptr, nullptr);
    }
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
    return {w, h};
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
