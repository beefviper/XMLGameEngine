// key_queue.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "key_queue.h"

#include <QKeyEvent>

namespace xge
{
	namespace
	{
		KeyCode keyCodeFromQt(int key, Qt::KeyboardModifiers modifiers) noexcept
		{
			if (key >= Qt::Key_A && key <= Qt::Key_Z)
			{
				return static_cast<KeyCode>(static_cast<int>(KeyCode::A) + (key - Qt::Key_A));
			}

			if (key >= Qt::Key_0 && key <= Qt::Key_9)
			{
				const int digit = key - Qt::Key_0;
				const KeyCode first = (modifiers & Qt::KeypadModifier) ? KeyCode::Numpad0 : KeyCode::Num0;
				return static_cast<KeyCode>(static_cast<int>(first) + digit);
			}

			if (key >= Qt::Key_F1 && key <= Qt::Key_F15)
			{
				return static_cast<KeyCode>(static_cast<int>(KeyCode::F1) + (key - Qt::Key_F1));
			}

			switch (key)
			{
			case Qt::Key_Escape: return KeyCode::Escape;
			case Qt::Key_Control: return KeyCode::LControl;
			case Qt::Key_Shift: return KeyCode::LShift;
			case Qt::Key_Alt: return KeyCode::LAlt;
			case Qt::Key_Meta: return KeyCode::LSystem;
			case Qt::Key_Menu: return KeyCode::Menu;
			case Qt::Key_BracketLeft: return KeyCode::LBracket;
			case Qt::Key_BracketRight: return KeyCode::RBracket;
			case Qt::Key_Semicolon: return KeyCode::Semicolon;
			case Qt::Key_Comma: return KeyCode::Comma;
			case Qt::Key_Period: return KeyCode::Period;
			case Qt::Key_Apostrophe: return KeyCode::Apostrophe;
			case Qt::Key_Slash: return (modifiers & Qt::KeypadModifier) ? KeyCode::Divide : KeyCode::Slash;
			case Qt::Key_Backslash: return KeyCode::Backslash;
			case Qt::Key_QuoteLeft: return KeyCode::Grave;
			case Qt::Key_Equal: return KeyCode::Equal;
			case Qt::Key_Minus: return (modifiers & Qt::KeypadModifier) ? KeyCode::Subtract : KeyCode::Hyphen;
			case Qt::Key_Plus: return KeyCode::Add;
			case Qt::Key_Asterisk: return KeyCode::Multiply;
			case Qt::Key_Space: return KeyCode::Space;
			case Qt::Key_Return:
			case Qt::Key_Enter: return KeyCode::Enter;
			case Qt::Key_Backspace: return KeyCode::Backspace;
			case Qt::Key_Tab: return KeyCode::Tab;
			case Qt::Key_PageUp: return KeyCode::PageUp;
			case Qt::Key_PageDown: return KeyCode::PageDown;
			case Qt::Key_End: return KeyCode::End;
			case Qt::Key_Home: return KeyCode::Home;
			case Qt::Key_Insert: return KeyCode::Insert;
			case Qt::Key_Delete: return KeyCode::Delete;
			case Qt::Key_Left: return KeyCode::Left;
			case Qt::Key_Right: return KeyCode::Right;
			case Qt::Key_Up: return KeyCode::Up;
			case Qt::Key_Down: return KeyCode::Down;
			case Qt::Key_Pause: return KeyCode::Pause;
			default: return KeyCode::Unknown;
			}
		}
	}

	bool KeyQueue::press(const QKeyEvent& event)
	{
		const KeyCode key = keyCodeFromQt(event.key(), event.modifiers());
		if (key == KeyCode::Unknown)
		{
			return false;
		}

		// Held keys repeat as press events; the engine only wants the first.
		if (!event.isAutoRepeat())
		{
			queue(key, true);
		}

		return true;
	}

	bool KeyQueue::release(const QKeyEvent& event)
	{
		const KeyCode key = keyCodeFromQt(event.key(), event.modifiers());
		if (key == KeyCode::Unknown)
		{
			return false;
		}

		if (!event.isAutoRepeat())
		{
			queue(key, false);
		}

		return true;
	}

	void KeyQueue::releaseAll()
	{
		for (std::size_t index = 0; index < down.size(); ++index)
		{
			if (down[index])
			{
				queue(static_cast<KeyCode>(index), false);
			}
		}
	}

	void KeyQueue::clear()
	{
		pending.clear();
		down.fill(false);
	}

	std::vector<std::pair<KeyCode, bool>> KeyQueue::take()
	{
		std::vector<std::pair<KeyCode, bool>> events;
		events.swap(pending);
		return events;
	}

	void KeyQueue::queue(KeyCode key, bool pressed)
	{
		down[static_cast<std::size_t>(key)] = pressed;
		pending.emplace_back(key, pressed);
	}
}
