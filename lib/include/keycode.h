// keycode.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include <string>
#include <vector>

namespace xge
{
	// Every key a State's <input button="..."> can name, expressed as a
	// backend-agnostic enum instead of the ad-hoc strings Engine used to
	// compare directly (see utils.h's old sfmlKeyToString, whose vocabulary
	// this mirrors exactly, one enumerator per string it could produce) -
	// so pollEvents() and Engine's own per-frame key state can work the same
	// way no matter which windowing backend is behind Window (see window.h).
	// Each backend maps its own native key type to this set once, in its own
	// pollEvents() (see window_sfml.cpp / window_raylib.cpp / window_sdl2.cpp).
	//
	// Count is a sentinel, not a real key - only ever used to size Engine's
	// per-frame key-state array (see engine.h). Keep it last.
	enum class KeyCode
	{
		Unknown,

		A, B, C, D, E, F, G, H, I, J, K, L, M,
		N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

		Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

		Escape,
		LControl, LShift, LAlt, LSystem,
		RControl, RShift, RAlt, RSystem,
		Menu,

		LBracket, RBracket, Semicolon, Comma, Period,
		Apostrophe, Slash, Backslash, Grave, Equal, Hyphen,

		Space, Enter, Backspace, Tab,
		PageUp, PageDown, End, Home, Insert, Delete,

		Add, Subtract, Multiply, Divide,

		Left, Right, Up, Down,

		Numpad0, Numpad1, Numpad2, Numpad3, Numpad4,
		Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,

		F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15,

		Pause,

		Count
	};

	// Parses a State's <input button="..."> attribute (e.g. "space", "a",
	// "f1") into a KeyCode. Returns KeyCode::Unknown for anything not
	// recognized - same string vocabulary keyCodeToString() below produces,
	// so a game's own XML round-trips through printGame() unchanged.
	KeyCode keyCodeFromString(const std::string& name) noexcept;

	// Inverse of keyCodeFromString() - used for debug output (State's
	// operator<<) so a KeyCode-keyed input map still prints the same button
	// names the XML used.
	std::string keyCodeToString(KeyCode key) noexcept;

	// Every name keyCodeFromString() knows, for the loader to say what there
	// is when a game names another.
	std::vector<std::string> keyNames();
}
