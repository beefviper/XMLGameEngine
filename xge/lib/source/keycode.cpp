// keycode.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "keycode.h"

#include <array>
#include <utility>

namespace xge
{
	namespace
	{
		// Single source of truth for the KeyCode <-> string mapping - both
		// keyCodeFromString and keyCodeToString are built from this one table
		// instead of keeping two switch statements in sync by hand.
		constexpr std::pair<KeyCode, const char*> table[] = {
			{ KeyCode::A, "a" }, { KeyCode::B, "b" }, { KeyCode::C, "c" }, { KeyCode::D, "d" },
			{ KeyCode::E, "e" }, { KeyCode::F, "f" }, { KeyCode::G, "g" }, { KeyCode::H, "h" },
			{ KeyCode::I, "i" }, { KeyCode::J, "j" }, { KeyCode::K, "k" }, { KeyCode::L, "l" },
			{ KeyCode::M, "m" }, { KeyCode::N, "n" }, { KeyCode::O, "o" }, { KeyCode::P, "p" },
			{ KeyCode::Q, "q" }, { KeyCode::R, "r" }, { KeyCode::S, "s" }, { KeyCode::T, "t" },
			{ KeyCode::U, "u" }, { KeyCode::V, "v" }, { KeyCode::W, "w" }, { KeyCode::X, "x" },
			{ KeyCode::Y, "y" }, { KeyCode::Z, "z" },

			{ KeyCode::Num0, "num0" }, { KeyCode::Num1, "num1" }, { KeyCode::Num2, "num2" },
			{ KeyCode::Num3, "num3" }, { KeyCode::Num4, "num4" }, { KeyCode::Num5, "num5" },
			{ KeyCode::Num6, "num6" }, { KeyCode::Num7, "num7" }, { KeyCode::Num8, "num8" },
			{ KeyCode::Num9, "num9" },

			{ KeyCode::Escape, "escape" },
			{ KeyCode::LControl, "lcontrol" }, { KeyCode::LShift, "lshift" },
			{ KeyCode::LAlt, "lalt" }, { KeyCode::LSystem, "lsystem" },
			{ KeyCode::RControl, "rcontrol" }, { KeyCode::RShift, "rshift" },
			{ KeyCode::RAlt, "ralt" }, { KeyCode::RSystem, "rsystem" },
			{ KeyCode::Menu, "menu" },

			{ KeyCode::LBracket, "[" }, { KeyCode::RBracket, "]" },
			{ KeyCode::Semicolon, ";" }, { KeyCode::Comma, "," }, { KeyCode::Period, "." },
			{ KeyCode::Apostrophe, "'" }, { KeyCode::Slash, "/" }, { KeyCode::Backslash, "\\" },
			{ KeyCode::Grave, "~" }, { KeyCode::Equal, "=" }, { KeyCode::Hyphen, "-" },

			{ KeyCode::Space, "space" }, { KeyCode::Enter, "enter" },
			{ KeyCode::Backspace, "backspace" }, { KeyCode::Tab, "tab" },
			{ KeyCode::PageUp, "pageup" }, { KeyCode::PageDown, "pagedown" },
			{ KeyCode::End, "end" }, { KeyCode::Home, "home" },
			{ KeyCode::Insert, "insert" }, { KeyCode::Delete, "delete" },

			{ KeyCode::Add, "add" }, { KeyCode::Subtract, "subtract" },
			{ KeyCode::Multiply, "multiply" }, { KeyCode::Divide, "divide" },

			{ KeyCode::Left, "left" }, { KeyCode::Right, "right" },
			{ KeyCode::Up, "up" }, { KeyCode::Down, "down" },

			{ KeyCode::Numpad0, "numpad0" }, { KeyCode::Numpad1, "numpad1" },
			{ KeyCode::Numpad2, "numpad2" }, { KeyCode::Numpad3, "numpad3" },
			{ KeyCode::Numpad4, "numpad4" }, { KeyCode::Numpad5, "numpad5" },
			{ KeyCode::Numpad6, "numpad6" }, { KeyCode::Numpad7, "numpad7" },
			{ KeyCode::Numpad8, "numpad8" }, { KeyCode::Numpad9, "numpad9" },

			{ KeyCode::F1, "f1" }, { KeyCode::F2, "f2" }, { KeyCode::F3, "f3" },
			{ KeyCode::F4, "f4" }, { KeyCode::F5, "f5" }, { KeyCode::F6, "f6" },
			{ KeyCode::F7, "f7" }, { KeyCode::F8, "f8" }, { KeyCode::F9, "f9" },
			{ KeyCode::F10, "f10" }, { KeyCode::F11, "f11" }, { KeyCode::F12, "f12" },
			{ KeyCode::F13, "f13" }, { KeyCode::F14, "f14" }, { KeyCode::F15, "f15" },

			{ KeyCode::Pause, "pause" },

			{ KeyCode::MouseLeft, "mouse.left" }, { KeyCode::MouseRight, "mouse.right" },
			{ KeyCode::MouseMiddle, "mouse.middle" },
			{ KeyCode::Pad1A, "pad1.a" }, { KeyCode::Pad1B, "pad1.b" }, { KeyCode::Pad1X, "pad1.x" }, { KeyCode::Pad1Y, "pad1.y" },
			{ KeyCode::Pad1LB, "pad1.lb" }, { KeyCode::Pad1RB, "pad1.rb" }, { KeyCode::Pad1Back, "pad1.back" }, { KeyCode::Pad1Start, "pad1.start" },
			{ KeyCode::Pad1Up, "pad1.up" }, { KeyCode::Pad1Down, "pad1.down" }, { KeyCode::Pad1Left, "pad1.left" }, { KeyCode::Pad1Right, "pad1.right" },
			{ KeyCode::Pad2A, "pad2.a" }, { KeyCode::Pad2B, "pad2.b" }, { KeyCode::Pad2X, "pad2.x" }, { KeyCode::Pad2Y, "pad2.y" },
			{ KeyCode::Pad2LB, "pad2.lb" }, { KeyCode::Pad2RB, "pad2.rb" }, { KeyCode::Pad2Back, "pad2.back" }, { KeyCode::Pad2Start, "pad2.start" },
			{ KeyCode::Pad2Up, "pad2.up" }, { KeyCode::Pad2Down, "pad2.down" }, { KeyCode::Pad2Left, "pad2.left" }, { KeyCode::Pad2Right, "pad2.right" },
			{ KeyCode::Unknown, "unknown" },
		};

		constexpr std::pair<Axis, const char*> axes[] = {
			{ Axis::MouseX, "mouse.x" }, { Axis::MouseY, "mouse.y" },
			{ Axis::Pad1X, "pad1.x" }, { Axis::Pad1Y, "pad1.y" },
			{ Axis::Pad2X, "pad2.x" }, { Axis::Pad2Y, "pad2.y" },
		};
	}

	KeyCode keyCodeFromString(const std::string& name) noexcept
	{
		for (const auto& [key, text] : table)
		{
			if (name == text) { return key; }
		}

		return KeyCode::Unknown;
	}

	std::vector<std::string> keyNames()
	{
		std::vector<std::string> names;
		for (const auto& entry : table) { names.emplace_back(entry.second); }
		return names;
	}

	std::string keyCodeToString(KeyCode key) noexcept
	{
		for (const auto& [candidate, text] : table)
		{
			if (candidate == key) { return text; }
		}

		return "unknown";
	}

	Axis axisFromString(const std::string& name) noexcept
	{
		for (const auto& [axis, text] : axes)
		{
			if (name == text) { return axis; }
		}

		return Axis::Count;
	}

	std::string axisToString(Axis axis) noexcept
	{
		for (const auto& [candidate, text] : axes)
		{
			if (candidate == axis) { return text; }
		}

		return "unknown";
	}

	std::vector<std::string> axisNames()
	{
		std::vector<std::string> names;
		for (const auto& entry : axes) { names.emplace_back(entry.second); }
		return names;
	}
}
