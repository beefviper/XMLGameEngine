// win32.cpp
// XML Game Engine
// author: beefviper
// date: Nov 14, 2026

#include "window/win32.h"

#include <xinput.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace xge
{
	namespace
	{
		constexpr wchar_t kClassName[] = L"xge-win32";

		// A key as Win32 reports it. Shift, Control and Alt say which side
		// from the scan code and the extended-key bit.
		KeyCode keyFromWin32(WPARAM key, LPARAM detail) noexcept
		{
			const bool extended = (detail & (1 << 24)) != 0;
			if (key >= 'A' && key <= 'Z') { return static_cast<KeyCode>(static_cast<int>(KeyCode::A) + static_cast<int>(key - 'A')); }
			if (key >= '0' && key <= '9') { return static_cast<KeyCode>(static_cast<int>(KeyCode::Num0) + static_cast<int>(key - '0')); }
			if (key >= VK_NUMPAD0 && key <= VK_NUMPAD9) { return static_cast<KeyCode>(static_cast<int>(KeyCode::Numpad0) + static_cast<int>(key - VK_NUMPAD0)); }
			if (key >= VK_F1 && key <= VK_F15) { return static_cast<KeyCode>(static_cast<int>(KeyCode::F1) + static_cast<int>(key - VK_F1)); }
			switch (key)
			{
			case VK_SHIFT:
			{
				const UINT scan = static_cast<UINT>((detail >> 16) & 0xFF);
				return MapVirtualKeyW(scan, MAPVK_VSC_TO_VK_EX) == static_cast<UINT>(VK_RSHIFT) ? KeyCode::RShift : KeyCode::LShift;
			}
			case VK_CONTROL: return extended ? KeyCode::RControl : KeyCode::LControl;
			case VK_MENU: return extended ? KeyCode::RAlt : KeyCode::LAlt;
			case VK_LWIN: return KeyCode::LSystem;
			case VK_RWIN: return KeyCode::RSystem;
			case VK_APPS: return KeyCode::Menu;
			case VK_ESCAPE: return KeyCode::Escape;
			case VK_OEM_4: return KeyCode::LBracket;
			case VK_OEM_6: return KeyCode::RBracket;
			case VK_OEM_1: return KeyCode::Semicolon;
			case VK_OEM_COMMA: return KeyCode::Comma;
			case VK_OEM_PERIOD: return KeyCode::Period;
			case VK_OEM_7: return KeyCode::Apostrophe;
			case VK_OEM_2: return KeyCode::Slash;
			case VK_OEM_5: return KeyCode::Backslash;
			case VK_OEM_3: return KeyCode::Grave;
			case VK_OEM_PLUS: return KeyCode::Equal;
			case VK_OEM_MINUS: return KeyCode::Hyphen;
			case VK_SPACE: return KeyCode::Space;
			case VK_RETURN: return KeyCode::Enter;
			case VK_BACK: return KeyCode::Backspace;
			case VK_TAB: return KeyCode::Tab;
			case VK_PRIOR: return KeyCode::PageUp;
			case VK_NEXT: return KeyCode::PageDown;
			case VK_END: return KeyCode::End;
			case VK_HOME: return KeyCode::Home;
			case VK_INSERT: return KeyCode::Insert;
			case VK_DELETE: return KeyCode::Delete;
			case VK_ADD: return KeyCode::Add;
			case VK_SUBTRACT: return KeyCode::Subtract;
			case VK_MULTIPLY: return KeyCode::Multiply;
			case VK_DIVIDE: return KeyCode::Divide;
			case VK_LEFT: return KeyCode::Left;
			case VK_RIGHT: return KeyCode::Right;
			case VK_UP: return KeyCode::Up;
			case VK_DOWN: return KeyCode::Down;
			case VK_PAUSE: return KeyCode::Pause;
			default: return KeyCode::Unknown;
			}
		}
	}

	std::wstring wideText(const std::string& text)
	{
		if (text.empty()) { return {}; }
		const int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
		std::wstring result(static_cast<std::size_t>(length), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), length);
		return result;
	}

	Win32Window::Win32Window(const WindowDesc& windowDesc, const std::string& library) :
		width(std::max(windowDesc.width, 1.0f)),
		height(std::max(windowDesc.height, 1.0f))
	{
		const int times = std::max(1, windowDesc.scale);

		const HINSTANCE instance = GetModuleHandleW(nullptr);
		WNDCLASSEXW kind{};
		kind.cbSize = sizeof(kind);
		kind.style = CS_HREDRAW | CS_VREDRAW;
		kind.lpfnWndProc = &Win32Window::receive;
		kind.hInstance = instance;
		kind.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));   // the arrow (IDC_ARROW)
		kind.lpszClassName = kClassName;
		RegisterClassExW(&kind);   // already registered by an earlier window: fine

		const bool fullscreen = windowDesc.fullscreen == "true";
		const DWORD style = fullscreen ? WS_POPUP : WS_OVERLAPPEDWINDOW;
		RECT frame{ 0, 0, static_cast<LONG>(width) * times, static_cast<LONG>(height) * times };
		if (fullscreen) { frame = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) }; }
		else { AdjustWindowRect(&frame, style, FALSE); }

		window = CreateWindowExW(0, kClassName, wideText(windowDesc.name).c_str(), style,
			fullscreen ? 0 : CW_USEDEFAULT, fullscreen ? 0 : CW_USEDEFAULT, frame.right - frame.left, frame.bottom - frame.top,
			nullptr, nullptr, instance, this);
		if (!window) { throw std::runtime_error(library + " could not make a window"); }
		fitGame();
	}

	Win32Window::~Win32Window()
	{
		destroy();
	}

	void Win32Window::show()
	{
		ShowWindow(window, SW_SHOW);
		UpdateWindow(window);
	}

	void Win32Window::destroy()
	{
		if (!window) { return; }
		DestroyWindow(window);
		window = nullptr;
	}

	LRESULT CALLBACK Win32Window::receive(HWND from, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (message == WM_NCCREATE)
		{
			const auto* made = reinterpret_cast<const CREATESTRUCTW*>(lParam);
			SetWindowLongPtrW(from, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(made->lpCreateParams));
		}
		auto* self = reinterpret_cast<Win32Window*>(GetWindowLongPtrW(from, GWLP_USERDATA));
		if (self) { return self->handle(from, message, wParam, lParam); }
		return DefWindowProcW(from, message, wParam, lParam);
	}

	LRESULT Win32Window::handle(HWND from, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_CLOSE:
			// Not torn down here: the frame that reads this still draws.
			closeRequested = true;
			return 0;
		case WM_SIZE:
			resized = true;
			return 0;
		case WM_SYSCHAR:
			return 0;   // Alt with a letter: no beep for a menu there is not
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			const bool down = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
			const bool repeat = down && (lParam & (1 << 30)) != 0;
			const KeyCode key = keyFromWin32(wParam, lParam);
			if (!repeat && key != KeyCode::Unknown) { keys.emplace_back(key, down); }
			// Alt and F10 would otherwise open the window's menu and pause
			// the game; Alt+F4 still closes it.
			if (message == WM_SYSKEYDOWN && wParam == VK_F4) { break; }
			return 0;
		}
		default:
			break;
		}
		return DefWindowProcW(from, message, wParam, lParam);
	}

	std::vector<std::pair<KeyCode, bool>> Win32Window::pollEvents()
	{
		MSG message{};
		while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessageW(&message);
		}
		std::vector<std::pair<KeyCode, bool>> events;
		events.swap(keys);
		return events;
	}

	bool Win32Window::takeResized()
	{
		if (!resized || !window) { return false; }
		resized = false;
		RECT client{};
		GetClientRect(window, &client);
		if (client.right <= client.left || client.bottom <= client.top) { return false; }   // minimised
		fitGame();
		return true;
	}

	void Win32Window::fitGame()
	{
		RECT client{};
		GetClientRect(window, &client);
		const float wide = static_cast<float>(std::max<LONG>(client.right - client.left, 1));
		const float high = static_cast<float>(std::max<LONG>(client.bottom - client.top, 1));
		scale = std::min(wide / width, high / height);
		place.width = std::floor(width * scale);
		place.height = std::floor(height * scale);
		place.left = std::floor((wide - place.width) / 2.0f);
		place.top = std::floor((high - place.height) / 2.0f);
	}

	Controls Win32Window::controls() const
	{
		Controls controls;
		if (!window || GetForegroundWindow() != window) { return controls; }

		POINT at{};
		GetCursorPos(&at);
		ScreenToClient(window, &at);
		RECT client{};
		GetClientRect(window, &client);
		controls.pointerInside = at.x >= 0 && at.y >= 0 && at.x < client.right && at.y < client.bottom;
		controls.pointerX = (static_cast<float>(at.x) - place.left) / scale;
		controls.pointerY = (static_cast<float>(at.y) - place.top) / scale;
		controls.mouse = { (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0, (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0,
			(GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0 };

		// XInput numbers its pads 0 to 3, each an Xbox layout.
		constexpr WORD buttons[padButtonCount] = {
			XINPUT_GAMEPAD_A, XINPUT_GAMEPAD_B, XINPUT_GAMEPAD_X, XINPUT_GAMEPAD_Y,
			XINPUT_GAMEPAD_LEFT_SHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER, XINPUT_GAMEPAD_BACK, XINPUT_GAMEPAD_START,
			XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_RIGHT,
		};
		std::size_t pad = 0;
		for (DWORD user = 0; user < XUSER_MAX_COUNT && pad < padCount; ++user)
		{
			XINPUT_STATE state{};
			if (XInputGetState(user, &state) != ERROR_SUCCESS) { continue; }
			Pad& now = controls.pads[pad++];
			now.connected = true;
			for (std::size_t button = 0; button < padButtonCount; ++button)
			{
				now.buttons[button] = (state.Gamepad.wButtons & buttons[button]) != 0;
			}
			now.x = std::clamp(static_cast<float>(state.Gamepad.sThumbLX) / 32767.0f, -1.0f, 1.0f);
			now.y = std::clamp(-static_cast<float>(state.Gamepad.sThumbLY) / 32767.0f, -1.0f, 1.0f);
		}
		return controls;
	}

	std::pair<int, int> Win32Window::position() const
	{
		RECT where{};
		if (!window || !GetWindowRect(window, &where)) { return { 0, 0 }; }
		return { static_cast<int>(where.left), static_cast<int>(where.top) };
	}

	void Win32Window::setPosition(int x, int y)
	{
		if (window) { SetWindowPos(window, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE); }
	}

	void Win32Window::setTitle(const std::string& title)
	{
		if (window) { SetWindowTextW(window, wideText(title).c_str()); }
	}
}
