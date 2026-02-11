#include <cassert>
#include <glad/glad_wgl.h>
#include <stdexcept>
#include <Windows.h>
#include "Render/ORenderEngine.h"

ORenderEngine::ORenderEngine()
{
	WNDCLASSEX windowClassEx = {};
	windowClassEx.style = CS_OWNDC;
	windowClassEx.lpfnWndProc = DefWindowProc;
	windowClassEx.lpszClassName = L"OpenGLEngineDummy";
	windowClassEx.cbSize = sizeof(WNDCLASSEX);

	const auto classId = RegisterClassEx(&windowClassEx);

	const HWND dummyWindow = CreateWindowEx(
		0,
		MAKEINTATOM(classId),
		L"OGL3DDummyWindow",
		0,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		nullptr,
		nullptr,
		windowClassEx.hInstance,
		nullptr);

	assert(dummyWindow);

	const auto dummyDC = GetDC(dummyWindow);

	PIXELFORMATDESCRIPTOR pixelformatdescriptor = {};
	pixelformatdescriptor.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pixelformatdescriptor.nVersion = 1;
	pixelformatdescriptor.iPixelType = PFD_TYPE_RGBA;
	pixelformatdescriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pixelformatdescriptor.cColorBits = 32;
	pixelformatdescriptor.cAlphaBits = 8;
	pixelformatdescriptor.cDepthBits = 24;
	pixelformatdescriptor.cStencilBits = 8;
	pixelformatdescriptor.iLayerType = PFD_MAIN_PLANE;


	const auto pixelFormat = ChoosePixelFormat(dummyDC, &pixelformatdescriptor);
	SetPixelFormat(dummyDC, pixelFormat, &pixelformatdescriptor);

	const auto dummyContext = wglCreateContext(dummyDC);
	assert(dummyContext);

	const bool res = wglMakeCurrent(dummyDC, dummyContext);
	assert(res);

	if (!gladLoadWGL(dummyDC))
		OGL_ERROR("OGraphicsEngine - gladLoadWGL failed");

	if (!gladLoadGL())
		OGL_ERROR("OGraphicsEngine - gladLoadGL failed");

	wglMakeCurrent(dummyDC, dummyContext);


	wglMakeCurrent(dummyDC, nullptr);
	wglDeleteContext(dummyContext);
	ReleaseDC(dummyWindow, dummyDC);
	DestroyWindow(dummyWindow);
}

ORenderEngine::~ORenderEngine()
{

}
