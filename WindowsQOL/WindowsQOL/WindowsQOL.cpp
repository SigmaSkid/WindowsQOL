#ifdef _DEBUG
#include <iostream>
using namespace std;
#endif

#include <Windows.h>

// area used for hot corner
constexpr int affected_area = 5;

// how many ticks/second (hz)
constexpr int update_rate_per_second = 40;

// delay between ticks
constexpr int sleep_delay = 1000 / update_rate_per_second;

// delay between switching virtual desktops in taskview
constexpr int timer_delay = 80 / sleep_delay;

enum FocusedWindow {
	FocusedWindowed,
	FocusedFullscreen,
	FocusedTaskview,
	FocusedDesktop,
	FocusedDragging
};

namespace globals {
	static bool taskbar_autohide = false;
	static int current_window_status = FocusedWindowed;
	static HWND focused_hwnd{};
	static HWND taskview_hwnd{};
	static RECT focused_rect{};
	static int screenx = 1920, screeny = 1080;
};

bool operator==(const RECT& lhs, const RECT& rhs)
{
	return lhs.left == rhs.left && lhs.top == rhs.top && lhs.right == rhs.right && lhs.bottom == rhs.bottom;
}

bool operator!=(const RECT& lhs, const RECT& rhs)
{
	return !(lhs == rhs);
}

void PressWindowsKey()
{
	// press the Windows key
	INPUT inputs[2];
	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wVk = VK_LWIN;
	inputs[0].ki.dwFlags = 0;
	inputs[0].ki.time = 0;
	inputs[0].ki.dwExtraInfo = 0;

	inputs[1].type = INPUT_KEYBOARD;
	inputs[1].ki.wVk = VK_TAB;
	inputs[1].ki.dwFlags = 0;
	inputs[1].ki.time = 0;
	inputs[1].ki.dwExtraInfo = 0;

	SendInput(2, inputs, sizeof(INPUT));

	inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
	inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(2, inputs, sizeof(INPUT));
}

void MoveVirtualDesktopLeft()
{
	// press the Windows key
	INPUT inputs[3];
	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wVk = VK_LWIN;
	inputs[0].ki.dwFlags = 0;
	inputs[0].ki.time = 0;
	inputs[0].ki.dwExtraInfo = 0;

	inputs[1].type = INPUT_KEYBOARD;
	inputs[1].ki.wVk = VK_LCONTROL;
	inputs[1].ki.dwFlags = 0;
	inputs[1].ki.time = 0;
	inputs[1].ki.dwExtraInfo = 0;

	inputs[2].type = INPUT_KEYBOARD;
	inputs[2].ki.wVk = VK_LEFT;
	inputs[2].ki.dwFlags = 0;
	inputs[2].ki.time = 0;
	inputs[2].ki.dwExtraInfo = 0;

	SendInput(3, inputs, sizeof(INPUT));

	inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
	inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
	inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

	SendInput(3, inputs, sizeof(INPUT));
}

void MoveVirtualDesktopRight()
{
	// press the Windows key
	INPUT inputs[3];
	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wVk = VK_LWIN;
	inputs[0].ki.dwFlags = 0;
	inputs[0].ki.time = 0;
	inputs[0].ki.dwExtraInfo = 0;

	inputs[1].type = INPUT_KEYBOARD;
	inputs[1].ki.wVk = VK_LCONTROL;
	inputs[1].ki.dwFlags = 0;
	inputs[1].ki.time = 0;
	inputs[1].ki.dwExtraInfo = 0;

	inputs[2].type = INPUT_KEYBOARD;
	inputs[2].ki.wVk = VK_RIGHT;
	inputs[2].ki.dwFlags = 0;
	inputs[2].ki.time = 0;
	inputs[2].ki.dwExtraInfo = 0;

	SendInput(3, inputs, sizeof(INPUT));

	inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
	inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
	inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

	SendInput(3, inputs, sizeof(INPUT));
}

void GetScreenSize(int& x, int& y)
{
	x = GetSystemMetrics(SM_CXSCREEN);
	y = GetSystemMetrics(SM_CYSCREEN);
}

// 0/false == normal
// 1/true == autohide
bool IsTaskbarAutoHideEnabled()
{
	APPBARDATA abd = {};
	abd.cbSize = sizeof(APPBARDATA);
	abd.hWnd = FindWindow(L"Shell_TrayWnd", NULL);
	abd.uCallbackMessage = WM_USER;

	UINT state = (UINT)SHAppBarMessage(ABM_GETSTATE, &abd);
	return ((state & ABS_AUTOHIDE) == ABS_AUTOHIDE);
}

void UpdateGlobalData()
{
	globals::taskbar_autohide = IsTaskbarAutoHideEnabled();

	globals::focused_hwnd = GetForegroundWindow();
	globals::taskview_hwnd = FindWindow(NULL, L"Task View");

	GetScreenSize(globals::screenx, globals::screeny);

	GetWindowRect(globals::focused_hwnd, &globals::focused_rect);

	// Check if the foreground window is the Windows 11 desktop
	wchar_t className[256];
	GetClassNameW(globals::focused_hwnd, className, sizeof(className));
	if (wcscmp(className, L"WorkerW") == 0) {
		// Windows 11 desktop is in focus
		globals::current_window_status = FocusedDesktop;
		return;
	}

	if (globals::focused_hwnd == globals::taskview_hwnd)
	{
		globals::current_window_status = FocusedTaskview;
		return;
	}

	// Check if the foreground window covers the entire screen
	if ((globals::focused_rect.right - globals::focused_rect.left) == globals::screenx &&
		(globals::focused_rect.bottom - globals::focused_rect.top) == globals::screeny) {
		// Fullscreen app is in focus
		globals::current_window_status = FocusedFullscreen;
		return;
	}

	// Check if we're dragging a window
	static RECT last_rect;
	// btw this will trigger if we changed focus too.
	if (last_rect != globals::focused_rect)
	{
		last_rect = globals::focused_rect;
		globals::current_window_status = FocusedDragging;
		return;
	}

	globals::current_window_status = FocusedWindowed;
	return;
}

// opens taskview when hovering in the top left corner
void HandleHotCorner()
{
	if (globals::current_window_status == FocusedFullscreen)
		return;

	static bool prevent = false;
	static bool pressed = false;

	POINT cursorPos;
	GetCursorPos(&cursorPos);

	if (cursorPos.x < affected_area && cursorPos.y < affected_area)
	{
		if (prevent)
			return;

		// if holding mouse button, like dragging/resizing
		if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
		{
			prevent = true;
			return;
		}

		if (!pressed)
		{
			PressWindowsKey();
			pressed = true;
		}
	}
	else {
		// reset when cursor outside of corner
		prevent = false;

		if (pressed) {
			pressed = false;
		}
	}
}

// switches virtual desktop by hovering mouse on the edge of the screen
void HandleBetterTaskview()
{
	if (globals::current_window_status != FocusedTaskview)
		return;

	POINT cursorPos;
	GetCursorPos(&cursorPos);

	int screenWidth, screenHeight;
	GetScreenSize(screenWidth, screenHeight);

	static int timer = 0;
	if (timer < timer_delay)
	{
		timer++;
	}
	else
	{

		if (cursorPos.y > max(affected_area, screenHeight / 8)
			&& cursorPos.y < min(screenHeight - affected_area, screenHeight * 0.875f))
		{
			if (cursorPos.x < affected_area) {
				MoveVirtualDesktopLeft();
				timer = 0;
			}
			else if (cursorPos.x + affected_area > screenWidth)
			{
				MoveVirtualDesktopRight();
				timer = 0;
			}
		}
	}
}

// prevents windows from being dragged outside of the screen
void HandleBetterWindows()
{
	if (globals::current_window_status != FocusedDragging)
		return;

	static RECT last_rect;

	if (last_rect == globals::focused_rect)
		return;

	int screenWidth, screenHeight;
	GetScreenSize(screenWidth, screenHeight);

	// Why windows why.
	constexpr int horizontal_padding = 8;
	const int taskbar_padding = globals::taskbar_autohide ? -8 : 40;

	// if the window is bigger than our screen, return instead of panic mode
	if (globals::focused_rect.right - globals::focused_rect.left + horizontal_padding * 2 >= screenWidth
		|| globals::focused_rect.bottom - globals::focused_rect.top + taskbar_padding >= screenHeight)
	{
		return;
	}

#ifdef _DEBUG
	cout << "User tried to move window to: " << endl <<
		"     " << globals::focused_rect.top << endl <<
		globals::focused_rect.left << "     " << globals::focused_rect.right << endl <<
		"     " << globals::focused_rect.bottom << endl;
#endif

	int x = globals::focused_rect.left;
	int y = globals::focused_rect.top;

	// check if window is too far right
	if (globals::focused_rect.right > screenWidth + horizontal_padding)
	{
		int offset = globals::focused_rect.right - (screenWidth + horizontal_padding);
		x -= offset;
	}

	// if window is too far down
	if (globals::focused_rect.bottom > screenHeight - taskbar_padding)
	{
		int offset = globals::focused_rect.bottom - (screenHeight - taskbar_padding);
		y -= offset;
	}

	// if window is too far up -- doesn't seem possible, but just in case
	if (globals::focused_rect.top < 0)
	{
		y = 0;
	}

	// if window is too far left
	if (globals::focused_rect.left < -horizontal_padding)
	{
		x = -horizontal_padding;
	}

	// move the window
	if (x != globals::focused_rect.left || y != globals::focused_rect.top)
	{
		SetWindowPos(globals::focused_hwnd, NULL, x, y, 0, 0, SWP_NOSIZE);
#ifdef _DEBUG
		cout << "Fixed to: x:" << x << " y:" << y << endl << endl << endl;
#endif
	}

}

void MyMainFunction() {
	UpdateGlobalData();
	HandleHotCorner();
	HandleBetterTaskview();
	HandleBetterWindows();
}

int main()
{
	while (true) {
		MyMainFunction();
		Sleep(sleep_delay);
	}
}