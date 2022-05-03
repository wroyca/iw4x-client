#include <STDInclude.hpp>

namespace Components
{
	Dvar::Var RawMouse::M_RawInput;
	int RawMouse::MouseRawX = 0;
	int RawMouse::MouseRawY = 0;

	void RawMouse::IN_ClampMouseMove()
	{
		tagRECT rc;
		tagPOINT curPos;

		GetCursorPos(&curPos);
		GetWindowRect(Game::g_wv->hWnd, &rc);
		auto isClamped = false;
		if (curPos.x >= rc.left)
		{
			if (curPos.x >= rc.right)
			{
				curPos.x = rc.right - 1;
				isClamped = true;
			}
		}
		else
		{
			curPos.x = rc.left;
			isClamped = true;
		}
		if (curPos.y >= rc.top)
		{
			if (curPos.y >= rc.bottom)
			{
				curPos.y = rc.bottom - 1;
				isClamped = true;
			}
		}
		else
		{
			curPos.y = rc.top;
			isClamped = true;
		}

		if (isClamped)
		{
			SetCursorPos(curPos.x, curPos.y);
		}
	}

	BOOL RawMouse::OnRawInput(LPARAM lParam, WPARAM)
	{
		auto dwSize = sizeof(RAWINPUT);
		static BYTE lpb[sizeof(RAWINPUT)];

		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));

		auto* raw = reinterpret_cast<RAWINPUT*>(lpb);
		if (raw->header.dwType == RIM_TYPEMOUSE)
		{
			// Is there's really absolute mouse on earth?
			if (raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
			{
				MouseRawX = raw->data.mouse.lLastX;
				MouseRawY = raw->data.mouse.lLastY;
			}
			else
			{
				MouseRawX += raw->data.mouse.lLastX;
				MouseRawY += raw->data.mouse.lLastY;
			}
		}

		return TRUE;
	}

	void RawMouse::IN_RawMouseMove()
	{
		static auto r_fullscreen = Dvar::Var("r_fullscreen");

		if (GetForegroundWindow() == Game::g_wv->hWnd)
		{
			if (r_fullscreen.get<bool>())
				IN_ClampMouseMove();

			static auto oldX = 0, oldY = 0;

			auto dx = MouseRawX - oldX;
			auto dy = MouseRawY - oldY;

			oldX = MouseRawX;
			oldY = MouseRawY;

			// Don't use raw input for menu?
			// Because it needs to call the ScreenToClient
			tagPOINT curPos;
			GetCursorPos(&curPos);
			Game::s_wmv->oldPos = curPos;
			ScreenToClient(Game::g_wv->hWnd, &curPos);

			Game::g_wv->recenterMouse = Game::CL_MouseEvent(curPos.x, curPos.y, dx, dy);

			if (Game::g_wv->recenterMouse)
			{
				Game::IN_RecenterMouse();
			}
		}
	}

	void RawMouse::IN_RawMouse_Init()
	{
		if (Game::g_wv->hWnd && RawMouse::M_RawInput.get<bool>())
		{
#ifdef DEBUG
			Logger::Print("Raw Mouse Init.\n");
#endif

			RAWINPUTDEVICE Rid[1];
			Rid[0].usUsagePage = 0x01; // HID_USAGE_PAGE_GENERIC
			Rid[0].usUsage = 0x02; // HID_USAGE_GENERIC_MOUSE
			Rid[0].dwFlags = RIDEV_INPUTSINK;
			Rid[0].hwndTarget = Game::g_wv->hWnd;

			RegisterRawInputDevices(Rid, ARRAYSIZE(Rid), sizeof(Rid[0]));
		}
	}

	void RawMouse::IN_Init()
	{
		Game::IN_Init();
		IN_RawMouse_Init();
	}

	void RawMouse::IN_MouseMove()
	{
		if (RawMouse::M_RawInput.get<bool>())
		{
			IN_RawMouseMove();
		}
		else
		{
			Game::IN_MouseMove();
		}
	}

	RawMouse::RawMouse()
	{
		Utils::Hook(0x475E65, RawMouse::IN_MouseMove, HOOK_JUMP).install()->quick();
		Utils::Hook(0x475E8D, RawMouse::IN_MouseMove, HOOK_JUMP).install()->quick();
		Utils::Hook(0x475E9E, RawMouse::IN_MouseMove, HOOK_JUMP).install()->quick();

		Utils::Hook(0x467C03, RawMouse::IN_Init, HOOK_CALL).install()->quick();
		Utils::Hook(0x64D095, RawMouse::IN_Init, HOOK_JUMP).install()->quick();

		Dvar::OnInit([]()
		{
			RawMouse::M_RawInput = Dvar::Register<bool>("m_rawinput", true, Game::dvar_flag::DVAR_ARCHIVE, "Use raw mouse input, Improves accuracy & has better support for higher polling rates. Use in_restart to take effect if not enabled.");
		});

		Window::OnWndMessage(WM_INPUT, RawMouse::OnRawInput);
		Window::OnCreate(RawMouse::IN_RawMouse_Init);
	}
}
