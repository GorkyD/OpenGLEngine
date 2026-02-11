#include "Window/OWindow.h"
#include <Windows.h>
#include <glad_wgl.c>
#include <glad/glad.h>
#include <cassert>

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_DESTROY:
		{
			OWindow* o_window = reinterpret_cast<OWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
			break;
		}
		case WM_CLOSE:
		{
			PostQuitMessage(0);
			break;
		}
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return NULL;
}

OWindow::OWindow()
{
	WNDCLASSEX windowClassEx = {};
	windowClassEx.cbSize = sizeof(WNDCLASSEX);
	windowClassEx.lpszClassName = L"OpenGLEngine";
	windowClassEx.lpfnWndProc = &WndProc;
	windowClassEx.style = CS_OWNDC;

	const auto classId = RegisterClassEx(&windowClassEx);
	assert(classId);

	RECT rect = { 0,0,1024,768 };
	AdjustWindowRect(&rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,false);

	windowInstance = CreateWindowEx(NULL, 
		MAKEINTATOM(classId), 
		windowClassEx.lpszClassName, 
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 
		CW_USEDEFAULT, 
		CW_USEDEFAULT,
		rect.right- rect.left, 
		rect.bottom - rect.top,
		nullptr, nullptr, nullptr, nullptr);

	assert(windowInstance);

	SetWindowLongPtr(static_cast<HWND>(windowInstance), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

	ShowWindow(static_cast<HWND>(windowInstance), SW_SHOW);
	UpdateWindow(static_cast<HWND>(windowInstance));

	auto hdc = GetDC(static_cast<HWND>(windowInstance));

	int pixelFormatAttributes[] = {
		 WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		 WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		 WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		 WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
		 WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		 WGL_COLOR_BITS_ARB, 32,
		 WGL_DEPTH_BITS_ARB, 24,
		 WGL_STENCIL_BITS_ARB, 8,
		 0
	};

	int pixelFormat = 0;
	UINT numFormats = 0;
	wglChoosePixelFormatARB(hdc,pixelFormatAttributes,nullptr,1,&pixelFormat,&numFormats);
	assert(numFormats);

	PIXELFORMATDESCRIPTOR pixelformatdescriptor = {};
	DescribePixelFormat(hdc, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pixelformatdescriptor);
	SetPixelFormat(hdc, pixelFormat, &pixelformatdescriptor);

	const int openGLAttributes[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		WGL_CONTEXT_MINOR_VERSION_ARB, 6,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};

	windowContext = wglCreateContextAttribsARB(hdc, nullptr, openGLAttributes);
	assert(windowContext);
}

OWindow::~OWindow()
{
	wglDeleteContext(static_cast<HGLRC>(windowContext));
	DestroyWindow(static_cast<HWND>(windowInstance));
}

ORect OWindow::GetInnerSize() const
{
	RECT rc = {};
	GetClientRect(static_cast<HWND>(windowInstance), &rc);
	return {rc.right - rc.left, rc.bottom - rc.top};
}

void OWindow::MakeCurrentContext() const
{
	wglMakeCurrent(GetDC(static_cast<HWND>(windowInstance)), static_cast<HGLRC>(windowContext));
}

void OWindow::Present(bool vSync) const
{
	wglSwapIntervalEXT(vSync);
	wglSwapLayerBuffers(GetDC(static_cast<HWND>(windowInstance)),WGL_SWAP_MAIN_PLANE);
}

