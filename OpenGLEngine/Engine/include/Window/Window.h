#pragma once

#include <vector>
#include "Math/Rect.h"

struct GLFWwindow;
struct GLFWcursor;

class Window
{
public:
    Window(bool windowed = false, const char* title = "OpenGLEngine");
    ~Window();

    Rect GetInnerSize() const;

    void MakeCurrentContext() const;
    void SetVSync(bool vSync);
    void Present() const;
    bool ShouldClose() const;

    void PollEvents();

    void SetCursorLocked(bool locked);
    int CreateCursorImage(const unsigned char* rgba, int width, int height, int hotX, int hotY);
    void UseCursor(int cursor);
    bool IsFocused() const;

    GLFWwindow* GetGLFWWindow() const
    {
        return window;
    }

private:
    GLFWwindow* window = nullptr;
    std::vector<GLFWcursor*> cursors;
    int activeCursor = -1;
};
