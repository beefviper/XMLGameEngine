// win32.h
// XML Game Engine
// author: beefviper
// date: Nov 14, 2026

#pragma once

#include "engine/keys.h"
#include "game/object.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <string>
#include <utility>
#include <vector>

namespace xge
{
	// The window the Direct3D backends draw in, made by Win32 itself: its
	// keyboard and mouse from Win32, its pads from XInput, and where in it the
	// game is shown. Only window/direct3d*.cpp include this (Windows only).
	//
	// It can be resized: the game is fitted to it with its proportions,
	// centred on whole pixels (shown()), as in the SFML window.
	class Win32Window
	{
	public:
		// The game's size times its scale inside the frame, or the whole
		// screen. Throws std::runtime_error, naming `library`, if Win32 cannot
		// make it. It is shown by show(), once the library is ready to draw.
		Win32Window(const WindowDesc& windowDesc, const std::string& library);
		~Win32Window();

		Win32Window(const Win32Window&) = delete;
		Win32Window& operator=(const Win32Window&) = delete;

		HWND handle() const noexcept { return window; }
		void show();
		void destroy();

		// Open until it is closed by close() or by the person (the close box,
		// Alt+F4): the frame that read that still draws, so nothing is torn
		// down then.
		bool isOpen() const noexcept { return window != nullptr && !closeRequested; }

		// Hands every message waiting to the window; gives the keys that went
		// down or up since the last call.
		std::vector<std::pair<KeyCode, bool>> pollEvents();

		// True once after the window changed size, when the game has been
		// fitted to it again; the library then resizes what it draws into.
		bool takeResized();

		// Where the game is in the window, in its pixels, and how many of
		// them make one of the game's.
		struct Shown
		{
			float left{};
			float top{};
			float width{};
			float height{};
		};
		const Shown& shown() const noexcept { return place; }
		float fit() const noexcept { return scale; }
		float gameWidth() const noexcept { return width; }
		float gameHeight() const noexcept { return height; }

		Controls controls() const;
		std::pair<int, int> position() const;
		void setPosition(int x, int y);
		void setTitle(const std::string& title);

	private:
		HWND window{ nullptr };
		float width{ 1.0f };
		float height{ 1.0f };
		float scale{ 1.0f };
		Shown place{};
		bool closeRequested{ false };
		bool resized{ false };
		std::vector<std::pair<KeyCode, bool>> keys;

		void fitGame();
		static LRESULT CALLBACK receive(HWND from, UINT message, WPARAM wParam, LPARAM lParam);
		LRESULT handle(HWND from, UINT message, WPARAM wParam, LPARAM lParam);
	};

	// UTF-8 to the wide text Win32 takes.
	std::wstring wideText(const std::string& text);
}
