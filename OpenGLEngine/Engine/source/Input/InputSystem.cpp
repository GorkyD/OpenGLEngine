#include "Input/InputSystem.h"

#include <cassert>
#include <vector>

#include "Extension/OExtension.h"

InputSystem::InputSystem(HWND windowInstance)
{
   RAWINPUTDEVICE rid[2];

   rid[0].usUsagePage = 0x01;
   rid[0].usUsage = 0x06;
   rid[0].dwFlags = RIDEV_INPUTSINK;
   rid[0].hwndTarget = static_cast<HWND>(windowInstance);

   rid[1].usUsagePage = 0x01;
   rid[1].usUsage = 0x02;
   rid[1].dwFlags = RIDEV_INPUTSINK;
   rid[1].hwndTarget = static_cast<HWND>(windowInstance);

   if (!RegisterRawInputDevices(rid, 2, sizeof(rid[0])))
   {
      OGL_ERROR("Failed to register input device")
   }
}

InputSystem::~InputSystem(){}

void InputSystem::ProcessPlatformMessage(const MSG& msg)
{
   switch (msg.message)
   {
   case WM_INPUT:
      ProcessRawInput(msg.lParam);
      break;

   case WM_KILLFOCUS:
      ClearState();
      break;
   }
}

void InputSystem::ClearState()
{
   keys.fill(false);

   mouseDeltaX = 0.0f;
   mouseDeltaY = 0.0f;

   leftMouseDown = false;
   rightMouseDown = false;
}

void InputSystem::ProcessRawInput(LPARAM lParam)
{
   UINT dataSize = 0;
   GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &dataSize, sizeof(RAWINPUTHEADER));
   if (dataSize == 0) return;

   std::vector<BYTE> buffer(dataSize);
   if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, buffer.data(), &dataSize, sizeof(RAWINPUTHEADER)) != dataSize)
      return;

   auto* raw = reinterpret_cast<RAWINPUT*>(buffer.data());

   if (raw->header.dwType == RIM_TYPEMOUSE)
   {
      mouseDeltaX += static_cast<float>(raw->data.mouse.lLastX);
      mouseDeltaY += static_cast<float>(raw->data.mouse.lLastY);

      USHORT flags = raw->data.mouse.usButtonFlags;
      if (flags & RI_MOUSE_LEFT_BUTTON_DOWN)  leftMouseDown = true;
      if (flags & RI_MOUSE_LEFT_BUTTON_UP)    leftMouseDown = false;
      if (flags & RI_MOUSE_RIGHT_BUTTON_DOWN) rightMouseDown = true;
      if (flags & RI_MOUSE_RIGHT_BUTTON_UP)   rightMouseDown = false;
   }
   else if (raw->header.dwType == RIM_TYPEKEYBOARD)
   {
      const bool isDown = !(raw->data.keyboard.Flags & RI_KEY_BREAK);
      const RAWKEYBOARD& keyboard = raw->data.keyboard;

      Key key = TranslateKey(keyboard);
      if (key != Key::Unknown)
      {
         keys[static_cast<size_t>(key)] = isDown;
         OGL_INFO(L"WM_INPUT vkey=" + std::to_wstring(keyboard.VKey) +L" key=" + std::to_wstring(static_cast<int>(key)) +L" " + (isDown ? L"DOWN" : L"UP"));
      }
   }
}

void InputSystem::Update()
{
   mouseDeltaX = 0;
   mouseDeltaY = 0;
}

bool InputSystem::IsKeyDown(Key key) const
{
   size_t index = static_cast<size_t>(key);
   if (index >= static_cast<size_t>(Key::Count))
      return false;

   return keys[index];
}

Key InputSystem::TranslateKey(const RAWKEYBOARD& keyboard)
{
   USHORT vkey = keyboard.VKey;
   USHORT makeCode = keyboard.MakeCode;
   USHORT flags = keyboard.Flags;

   if (vkey == VK_SHIFT)
      vkey = static_cast<USHORT>(MapVirtualKey(keyboard.MakeCode, MAPVK_VSC_TO_VK_EX));

   switch (vkey)
   {
	   case 'A': return Key::A;
	   case 'B': return Key::B;
	   case 'C': return Key::C;
	   case 'D': return Key::D;
	   case 'E': return Key::E;
	   case 'F': return Key::F;
	   case 'G': return Key::G;
	   case 'H': return Key::H;
	   case 'I': return Key::I;
	   case 'J': return Key::J;
	   case 'K': return Key::K;
	   case 'L': return Key::L;
	   case 'M': return Key::M;
	   case 'N': return Key::N;
	   case 'O': return Key::O;
	   case 'P': return Key::P;
	   case 'Q': return Key::Q;
	   case 'R': return Key::R;
	   case 'S': return Key::S;
	   case 'T': return Key::T;
	   case 'U': return Key::U;
	   case 'V': return Key::V;
	   case 'W': return Key::W;
	   case 'X': return Key::X;
	   case 'Y': return Key::Y;
	   case 'Z': return Key::Z;

	   case VK_ESCAPE: return Key::Escape;
	   case VK_SPACE: return Key::Space;
	   case VK_RETURN: return Key::Enter;

	   case VK_LEFT: return Key::Left;
	   case VK_RIGHT: return Key::Right;
	   case VK_UP: return Key::Up;
	   case VK_DOWN: return Key::Down;

      case VK_SHIFT:
         if (makeCode == 0x2A) return Key::LShift;
         if (makeCode == 0x36) return Key::RShift;
         return Key::Unknown;

      case VK_CONTROL:
         return (flags & RI_KEY_E0) ? Key::RControl : Key::LControl;

      case VK_MENU:
         return (flags & RI_KEY_E0) ? Key::RAlt : Key::LAlt;

      //TODO: SHIFT/CONTROL/ALT FIX

	   default:
	      return Key::Unknown;
   }
}

float InputSystem::GetMouseDeltaX() const { return mouseDeltaX; }
float InputSystem::GetMouseDeltaY() const { return mouseDeltaY; }
