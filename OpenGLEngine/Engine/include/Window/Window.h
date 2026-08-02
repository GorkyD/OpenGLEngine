#pragma once

#include "Math/Rect.h"

struct GLFWwindow;

class Window
{
public:
    Window(bool windowed = false, const char* title = "OpenGLEngine");
    ~Window();

    Rect GetInnerSize() const;

    void MakeCurrentContext() const;
    void Present(bool vSync) const;
    bool ShouldClose() const;

    void PollEvents();

    void SetCursorLocked(bool locked);
    bool IsFocused() const;

    GLFWwindow* GetGLFWWindow() const
    {
        return window;
    }

private:
    GLFWwindow* window = nullptr;
};
