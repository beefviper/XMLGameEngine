// keycode.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include <array>
#include <cstddef>
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

		// The mouse's buttons, and two gamepads' buttons (an Xbox layout:
		// A B X Y, the shoulders, Back and Start), each pad's directions
		// from its d-pad or its left stick pushed past halfway. The window
		// reports what is down (Controls, below); Engine turns the changes
		// into presses and releases like a key's.
		MouseLeft, MouseRight, MouseMiddle,
		Pad1A, Pad1B, Pad1X, Pad1Y, Pad1LB, Pad1RB, Pad1Back, Pad1Start, Pad1Up, Pad1Down, Pad1Left, Pad1Right,
		Pad2A, Pad2B, Pad2X, Pad2Y, Pad2LB, Pad2RB, Pad2Back, Pad2Start, Pad2Up, Pad2Down, Pad2Left, Pad2Right,

		Count
	};

	// The buttons of one gamepad, in KeyCode's order from PadNA.
	enum class PadButton { A, B, X, Y, LB, RB, Back, Start, Up, Down, Left, Right, Count };

	constexpr std::size_t padButtonCount = static_cast<std::size_t>(PadButton::Count);
	constexpr std::size_t padCount = 2;

	// The key a pad's button is: pad 0 or 1.
	constexpr KeyCode padKey(std::size_t pad, PadButton button) noexcept
	{
		return static_cast<KeyCode>(static_cast<std::size_t>(pad == 0 ? KeyCode::Pad1A : KeyCode::Pad2A) + static_cast<std::size_t>(button));
	}

	// The controls a <input axis="..."> follows: where the mouse is across
	// the window, and where each pad's left stick is pushed.
	enum class Axis { MouseX, MouseY, Pad1X, Pad1Y, Pad2X, Pad2Y, Count };

	constexpr std::size_t axisCount = static_cast<std::size_t>(Axis::Count);

	// What the window says the mouse and the pads are doing now, asked for
	// once a frame (Window::controls).
	struct Pad
	{
		bool connected{};
		std::array<bool, padButtonCount> buttons{};  // the d-pad's directions included, not the stick's
		float x{};  // the left stick, -1 (left) to 1 (right)
		float y{};  // -1 (up) to 1 (down)
	};

	struct Controls
	{
		bool pointerInside{};  // the mouse is over the game
		float pointerX{};      // where, in the game's own pixels
		float pointerY{};
		std::array<bool, 3> mouse{};  // left, right, middle
		std::array<Pad, padCount> pads{};
	};

	// <input axis="mouse.x">: the names are mouse.x, mouse.y, pad1.x,
	// pad1.y, pad2.x and pad2.y. Axis::Count for anything else.
	Axis axisFromString(const std::string& name) noexcept;
	std::string axisToString(Axis axis) noexcept;
	std::vector<std::string> axisNames();

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

	// Which keys are down. A held key repeats as press events on some window
	// libraries; only the first counts, so a game hears each key go down
	// once and come up once. The engine and a generated game both listen
	// through one.
	class HeldKeys
	{
	public:
		// The key went down: true when it was up, so this press is a real one.
		bool press(KeyCode key) noexcept
		{
			bool& down = keys[static_cast<std::size_t>(key)];
			if (down) { return false; }
			down = true;
			return true;
		}

		void release(KeyCode key) noexcept { keys[static_cast<std::size_t>(key)] = false; }

		bool isDown(KeyCode key) const noexcept { return keys[static_cast<std::size_t>(key)]; }

	private:
		std::array<bool, static_cast<std::size_t>(KeyCode::Count)> keys{};
	};
}
