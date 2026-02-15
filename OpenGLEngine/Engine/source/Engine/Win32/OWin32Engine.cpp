#include <Windows.h>
#include "Engine/OEngine.h"

void OEngine::Run()
{
	OnCreate();
	while (is_Running)
	{
		MSG msg;
		if (PeekMessage(&msg, HWND(), NULL, NULL, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				is_Running = false;
				continue;
			}
			else
			{
				inputSystem->ProcessPlatformMessage(msg);
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		OnUpdateInternal();
	}

	OnQuit();
}
