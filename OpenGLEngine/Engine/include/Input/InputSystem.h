#pragma once
#include <array>
#include <Windows.h>

#include "Extension/Extension.h"

class InputSystem
{
	public:
		InputSystem(HWND windowInstance);
		~InputSystem();

		void ProcessPlatformMessage(const MSG& msg);
		void ClearState();

		void Update();
		void ProcessRawInput(LPARAM lParam);

		float GetMouseDeltaX() const;
		float GetMouseDeltaY() const;

		bool IsKeyDown(Key key) const;
		bool IsLeftMouseDown() const { return leftMouseDown; }

	private:
		Key TranslateKey(const RAWKEYBOARD& keyboard);

		float mouseDeltaX = 0;
		float mouseDeltaY = 0;

		std::array<bool, 256> keys = {};

		bool leftMouseDown = false;
		bool rightMouseDown = false;
};
