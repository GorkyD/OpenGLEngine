#pragma once
#include "Math/ORect.h"

class OWindow
{
	public:
		OWindow();
		~OWindow();

		ORect GetInnerSize() const;

		void MakeCurrentContext() const;
		void Present(bool vSync) const;
		
		void* GetWindowInstance() const { return windowInstance; }
	private:
		void* windowInstance = nullptr;
		void* windowContext = nullptr;
};
