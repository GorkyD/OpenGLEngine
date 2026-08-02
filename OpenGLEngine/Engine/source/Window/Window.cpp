#include "Window/Window.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cassert>

Window::Window(bool windowed, const char* title)
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

    if (windowed)
    {
        glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        const int width = mode ? static_cast<int>(mode->width * 0.75f) : 1280;
        const int height = mode ? static_cast<int>(mode->height * 0.75f) : 720;

        window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (window && mode)
            glfwSetWindowPos(window, (mode->width - width) / 2, (mode->height - height) / 2);
    }
    else if (mode)
    {
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        window = glfwCreateWindow(mode->width, mode->height, title, nullptr, nullptr);
        glfwSetWindowPos(window, 0, 0);
    }
    else
    {
        window = glfwCreateWindow(1024, 768, title, nullptr, nullptr);
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

bool Window::IsFocused() const
{
    return glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;
}

void Window::SetCursorLocked(bool locked)
{
    glfwSetInputMode(window, GLFW_CURSOR, locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}
