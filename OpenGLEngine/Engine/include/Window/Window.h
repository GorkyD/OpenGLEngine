#pragma once
#include "Math/Rect.h"

class Window
{
	public:
		Window();
		~Window();

		Rect GetInnerSize() const;

		void MakeCurrentContext() const;
		void Present(bool vSync) const;
		
		void* GetWindowInstance() const { return windowInstance; }
	private:
		void* windowInstance = nullptr;
		void* windowContext = nullptr;
};
