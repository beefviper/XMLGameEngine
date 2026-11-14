// glfw.cpp
// XML Game Engine
// author: beefviper
// date: Nov 14, 2026

#include "window/glfw.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstddef>

namespace xge
{
	KeyCode keyFromGlfw(int key) noexcept
	{
		if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z)
		{
			return static_cast<KeyCode>(static_cast<int>(KeyCode::A) + (key - GLFW_KEY_A));
		}

		if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
		{
			return static_cast<KeyCode>(static_cast<int>(KeyCode::Num0) + (key - GLFW_KEY_0));
		}

		if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9)
		{
			return static_cast<KeyCode>(static_cast<int>(KeyCode::Numpad0) + (key - GLFW_KEY_KP_0));
		}

		if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F15)
		{
			return static_cast<KeyCode>(static_cast<int>(KeyCode::F1) + (key - GLFW_KEY_F1));
		}

		switch (key)
		{
		case GLFW_KEY_ESCAPE: return KeyCode::Escape;
		case GLFW_KEY_LEFT_CONTROL: return KeyCode::LControl;
		case GLFW_KEY_LEFT_SHIFT: return KeyCode::LShift;
		case GLFW_KEY_LEFT_ALT: return KeyCode::LAlt;
		case GLFW_KEY_LEFT_SUPER: return KeyCode::LSystem;
		case GLFW_KEY_RIGHT_CONTROL: return KeyCode::RControl;
		case GLFW_KEY_RIGHT_SHIFT: return KeyCode::RShift;
		case GLFW_KEY_RIGHT_ALT: return KeyCode::RAlt;
		case GLFW_KEY_RIGHT_SUPER: return KeyCode::RSystem;
		case GLFW_KEY_MENU: return KeyCode::Menu;
		case GLFW_KEY_LEFT_BRACKET: return KeyCode::LBracket;
		case GLFW_KEY_RIGHT_BRACKET: return KeyCode::RBracket;
		case GLFW_KEY_SEMICOLON: return KeyCode::Semicolon;
		case GLFW_KEY_COMMA: return KeyCode::Comma;
		case GLFW_KEY_PERIOD: return KeyCode::Period;
		case GLFW_KEY_APOSTROPHE: return KeyCode::Apostrophe;
		case GLFW_KEY_SLASH: return KeyCode::Slash;
		case GLFW_KEY_BACKSLASH: return KeyCode::Backslash;
		case GLFW_KEY_GRAVE_ACCENT: return KeyCode::Grave;
		case GLFW_KEY_EQUAL: return KeyCode::Equal;
		case GLFW_KEY_MINUS: return KeyCode::Hyphen;
		case GLFW_KEY_SPACE: return KeyCode::Space;
		case GLFW_KEY_ENTER:
		case GLFW_KEY_KP_ENTER: return KeyCode::Enter;
		case GLFW_KEY_BACKSPACE: return KeyCode::Backspace;
		case GLFW_KEY_TAB: return KeyCode::Tab;
		case GLFW_KEY_PAGE_UP: return KeyCode::PageUp;
		case GLFW_KEY_PAGE_DOWN: return KeyCode::PageDown;
		case GLFW_KEY_END: return KeyCode::End;
		case GLFW_KEY_HOME: return KeyCode::Home;
		case GLFW_KEY_INSERT: return KeyCode::Insert;
		case GLFW_KEY_DELETE: return KeyCode::Delete;
		case GLFW_KEY_KP_ADD: return KeyCode::Add;
		case GLFW_KEY_KP_SUBTRACT: return KeyCode::Subtract;
		case GLFW_KEY_KP_MULTIPLY: return KeyCode::Multiply;
		case GLFW_KEY_KP_DIVIDE: return KeyCode::Divide;
		case GLFW_KEY_LEFT: return KeyCode::Left;
		case GLFW_KEY_RIGHT: return KeyCode::Right;
		case GLFW_KEY_UP: return KeyCode::Up;
		case GLFW_KEY_DOWN: return KeyCode::Down;
		case GLFW_KEY_PAUSE: return KeyCode::Pause;
		default: return KeyCode::Unknown;
		}
	}

	void readGlfwPads(Controls& controls)
	{
		constexpr int buttons[padButtonCount] = {
			GLFW_GAMEPAD_BUTTON_A, GLFW_GAMEPAD_BUTTON_B, GLFW_GAMEPAD_BUTTON_X, GLFW_GAMEPAD_BUTTON_Y,
			GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, GLFW_GAMEPAD_BUTTON_BACK, GLFW_GAMEPAD_BUTTON_START,
			GLFW_GAMEPAD_BUTTON_DPAD_UP, GLFW_GAMEPAD_BUTTON_DPAD_DOWN, GLFW_GAMEPAD_BUTTON_DPAD_LEFT, GLFW_GAMEPAD_BUTTON_DPAD_RIGHT,
		};
		std::size_t pad = 0;
		for (int joystick = GLFW_JOYSTICK_1; joystick <= GLFW_JOYSTICK_LAST && pad < padCount; ++joystick)
		{
			GLFWgamepadstate state;
			if (!glfwJoystickIsGamepad(joystick) || !glfwGetGamepadState(joystick, &state)) { continue; }
			Pad& now = controls.pads[pad++];
			now.connected = true;
			for (std::size_t button = 0; button < padButtonCount; ++button)
			{
				now.buttons[button] = state.buttons[buttons[button]] == GLFW_PRESS;
			}
			now.x = state.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
			now.y = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
		}
	}
}
