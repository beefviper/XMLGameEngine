// utils.cpp
// XML Game Engine
// author: beefviper
// date: Mar 7, 2021

#include "utils.h"

namespace xge
{
	sf::Color sfmlColor(std::string color) noexcept
	{
		auto result = sf::Color::Transparent;

		// TODO: pretty sure I can replace this with a map
		if (color == "color.black") { result = sf::Color::Black; }
		else if (color == "color.white") { result = sf::Color::White; }
		else if (color == "color.red") { result = sf::Color::Red; }
		else if (color == "color.green") { result = sf::Color::Green; }
		else if (color == "color.blue") { result = sf::Color::Blue; }
		else if (color == "color.yellow") { result = sf::Color::Yellow; }
		else if (color == "color.magenta") { result = sf::Color::Magenta; }
		else if (color == "color.cyan") { result = sf::Color::Cyan; }

		return result;
	}

	std::string sfmlKeyToString(sf::Keyboard::Key key)
	{
		switch (key)
		{
		case sf::Keyboard::Key::Unknown:
			return "unknown";
			break;
		case sf::Keyboard::Key::A:
			return "a";
			break;
		case sf::Keyboard::Key::B:
			return "b";
			break;
		case sf::Keyboard::Key::C:
			return "c";
			break;
		case sf::Keyboard::Key::D:
			return "d";
			break;
		case sf::Keyboard::Key::E:
			return "e";
			break;
		case sf::Keyboard::Key::F:
			return "f";
			break;
		case sf::Keyboard::Key::G:
			return "g";
			break;
		case sf::Keyboard::Key::H:
			return "h";
			break;
		case sf::Keyboard::Key::I:
			return "i";
			break;
		case sf::Keyboard::Key::J:
			return "j";
			break;
		case sf::Keyboard::Key::K:
			return "k";
			break;
		case sf::Keyboard::Key::L:
			return "l";
			break;
		case sf::Keyboard::Key::M:
			return "m";
			break;
		case sf::Keyboard::Key::N:
			return "n";
			break;
		case sf::Keyboard::Key::O:
			return "o";
			break;
		case sf::Keyboard::Key::P:
			return "p";
			break;
		case sf::Keyboard::Key::Q:
			return "q";
			break;
		case sf::Keyboard::Key::R:
			return "r";
			break;
		case sf::Keyboard::Key::S:
			return "s";
			break;
		case sf::Keyboard::Key::T:
			return "t";
			break;
		case sf::Keyboard::Key::U:
			return "u";
			break;
		case sf::Keyboard::Key::V:
			return "v";
			break;
		case sf::Keyboard::Key::W:
			return "w";
			break;
		case sf::Keyboard::Key::X:
			return "x";
			break;
		case sf::Keyboard::Key::Y:
			return "y";
			break;
		case sf::Keyboard::Key::Z:
			return "z";
			break;
		case sf::Keyboard::Key::Num0:
			return "num0";
			break;
		case sf::Keyboard::Key::Num1:
			return "num1";
			break;
		case sf::Keyboard::Key::Num2:
			return "num2";
			break;
		case sf::Keyboard::Key::Num3:
			return "num3";
			break;
		case sf::Keyboard::Key::Num4:
			return "num4";
			break;
		case sf::Keyboard::Key::Num5:
			return "num5";
			break;
		case sf::Keyboard::Key::Num6:
			return "num6";
			break;
		case sf::Keyboard::Key::Num7:
			return "num7";
			break;
		case sf::Keyboard::Key::Num8:
			return "num8";
			break;
		case sf::Keyboard::Key::Num9:
			return "num9";
			break;
		case sf::Keyboard::Key::Escape:
			return "escape";
			break;
		case sf::Keyboard::Key::LControl:
			return "lcontrol";
			break;
		case sf::Keyboard::Key::LShift:
			return "lshift";
			break;
		case sf::Keyboard::Key::LAlt:
			return "lalt";
			break;
		case sf::Keyboard::Key::LSystem:
			return "lsystem";
			break;
		case sf::Keyboard::Key::RControl:
			return "rcontrol";
			break;
		case sf::Keyboard::Key::RShift:
			return "rshift";
			break;
		case sf::Keyboard::Key::RAlt:
			return "ralt";
			break;
		case sf::Keyboard::Key::RSystem:
			return "rsystem";
			break;
		case sf::Keyboard::Key::Menu:
			return "menu";
			break;
		case sf::Keyboard::Key::LBracket:
			return "[";
			break;
		case sf::Keyboard::Key::RBracket:
			return "]";
			break;
		case sf::Keyboard::Key::Semicolon:
			return ";";
			break;
		case sf::Keyboard::Key::Comma:
			return ",";
			break;
		case sf::Keyboard::Key::Period:
			return ".";
			break;
		case sf::Keyboard::Key::Apostrophe:
			return "'";
			break;
		case sf::Keyboard::Key::Slash:
			return "/";
			break;
		case sf::Keyboard::Key::Backslash:
			return "\\";
			break;
		case sf::Keyboard::Key::Grave:
			return "~";
			break;
		case sf::Keyboard::Key::Equal:
			return "=";
			break;
		case sf::Keyboard::Key::Hyphen:
			return "-";
			break;
		case sf::Keyboard::Key::Space:
			return "space";
			break;
		case sf::Keyboard::Key::Enter:
			return "enter";
			break;
		case sf::Keyboard::Key::Backspace:
			return "backspace";
			break;
		case sf::Keyboard::Key::Tab:
			return "tab";
			break;
		case sf::Keyboard::Key::PageUp:
			return "pageup";
			break;
		case sf::Keyboard::Key::PageDown:
			return "pagedown";
			break;
		case sf::Keyboard::Key::End:
			return "end";
			break;
		case sf::Keyboard::Key::Home:
			return "home";
			break;
		case sf::Keyboard::Key::Insert:
			return "insert";
			break;
		case sf::Keyboard::Key::Delete:
			return "delete";
			break;
		case sf::Keyboard::Key::Add:
			return "add";
			break;
		case sf::Keyboard::Key::Subtract:
			return "subtract";
			break;
		case sf::Keyboard::Key::Multiply:
			return "multiply";
			break;
		case sf::Keyboard::Key::Divide:
			return "divide";
			break;
		case sf::Keyboard::Key::Left:
			return "left";
			break;
		case sf::Keyboard::Key::Right:
			return "right";
			break;
		case sf::Keyboard::Key::Up:
			return "up";
			break;
		case sf::Keyboard::Key::Down:
			return "down";
			break;
		case sf::Keyboard::Key::Numpad0:
			return "numpad0";
			break;
		case sf::Keyboard::Key::Numpad1:
			return "numpad1";
			break;
		case sf::Keyboard::Key::Numpad2:
			return "numpad2";
			break;
		case sf::Keyboard::Key::Numpad3:
			return "numpad3";
			break;
		case sf::Keyboard::Key::Numpad4:
			return "numpad4";
			break;
		case sf::Keyboard::Key::Numpad5:
			return "numpad5";
			break;
		case sf::Keyboard::Key::Numpad6:
			return "numpad6";
			break;
		case sf::Keyboard::Key::Numpad7:
			return "numpad7";
			break;
		case sf::Keyboard::Key::Numpad8:
			return "numpad8";
			break;
		case sf::Keyboard::Key::Numpad9:
			return "numpad9";
			break;
		case sf::Keyboard::Key::F1:
			return "f1";
			break;
		case sf::Keyboard::Key::F2:
			return "f2";
			break;
		case sf::Keyboard::Key::F3:
			return "f3";
			break;
		case sf::Keyboard::Key::F4:
			return "f4";
			break;
		case sf::Keyboard::Key::F5:
			return "f5";
			break;
		case sf::Keyboard::Key::F6:
			return "f6";
			break;
		case sf::Keyboard::Key::F7:
			return "f7";
			break;
		case sf::Keyboard::Key::F8:
			return "f8";
			break;
		case sf::Keyboard::Key::F9:
			return "f9";
			break;
		case sf::Keyboard::Key::F10:
			return "f10";
			break;
		case sf::Keyboard::Key::F11:
			return "f11";
			break;
		case sf::Keyboard::Key::F12:
			return "f12";
			break;
		case sf::Keyboard::Key::F13:
			return "f13";
			break;
		case sf::Keyboard::Key::F14:
			return "f14";
			break;
		case sf::Keyboard::Key::F15:
			return "f15";
			break;
		case sf::Keyboard::Key::Pause:
			return "pause";
			break;
		default:
			return "";
			break;
		}
	}
}
