// engine.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "engine.h"

namespace xge
{
	Engine::Engine(Game& game) :
		game(game)
	{
		WindowDesc windowDesc = game.getWindowDesc();

		std::string name = windowDesc.name;

		int width = static_cast<int>(windowDesc.width);
		int height = static_cast<int>(windowDesc.height);
		sf::VideoMode videoMode(width, height);

		auto windowMode = (windowDesc.fullscreen == "true") ? sf::Style::Fullscreen : sf::Style::Default;

		window.create(videoMode, name, windowMode);
		window.setFramerateLimit(windowDesc.framerate);
	}

	void Engine::loop(void)
	{
		while (window.isOpen())
		{
			sf::Event event;
			while (window.pollEvent(event))
			{
				switch (event.type)
				{
				case sf::Event::Closed:
					window.close();
					break;

				case sf::Event::KeyPressed:
					handleKeyPressed(game, event);
					break;

				case sf::Event::KeyReleased:
					handleKeyReleased(game, event);
					break;

				default:
					break;
				}
			}

			game.updateObjects();

			window.clear(sfmlColor(game.getWindowDesc().background));

			for (auto& name : game.getCurrentState().show)
			{
				window.draw(*game.getObject(name).sprite);
			}

			window.display();
		}
	}

	void Engine::handleKeyPressed(Game& game, sf::Event& event)
	{
		if (!isKeyPressed[event.key.code])
		{
			isKeyPressed[event.key.code] = true;

			for (auto& input : game.getCurrentState().input)
			{
				execute_action(game, event, input, true);
			}
		}
	}

	void Engine::handleKeyReleased(Game& game, sf::Event& event)
	{

		isKeyPressed[event.key.code] = false;

		for (auto& input : game.getCurrentState().input)
		{
			execute_action(game, event, input, false);
		}
	}

	void Engine::execute_action(Game& game, sf::Event& event, PairStringString& input, bool keyPressed)
	{
		if (input.second == sfmlKeyToString(event.key.code))
		{
			std::string aObject = getObjectFromInput(input);
			std::string aCommand = getCommandFromInput(input);

			if (aObject == "state")
			{
				if (keyPressed)
				{
					game.setCurrentState(aCommand);
				}
			}
			else if (&game.getObject(aObject) != nullptr)
			{
				std::string aAction = game.getObject(aObject).action[aCommand];
				auto aStep = getValueFromAction(aAction);

				if (!keyPressed)
				{
					aStep = 0;
				}

				move(game.getObject(aObject), aCommand, aStep);
			}
		}
	}

	void Engine::move(Object& object, Engine::Direction direction, float step)
	{
		switch (direction)
		{
		case Direction::Up:
			object.velocity.y = -step;
			break;
		case Direction::Down:
			object.velocity.y = step;
			break;
		case Direction::Left:
			object.velocity.x = -step;
			break;
		case Direction::Right:
			object.velocity.x = step;
			break;
		}
	}

	void Engine::move(Object& object, std::string direction, float step)
	{
		move(object, mapDirection[direction], step);
	}

	std::string Engine::getObjectFromInput(PairStringString input)
	{
		auto sStart = 0;
		auto sEnd = input.first.find(".");

		return input.first.substr(sStart, sEnd);
	}

	std::string Engine::getCommandFromInput(PairStringString input)
	{
		auto sStart = input.first.rfind('.') + 1;
		auto sEnd = input.first.length();
		auto sLength = sEnd - sStart;

		return input.first.substr(sStart, sLength);
	}

	float Engine::getValueFromAction(std::string action)
	{
		auto sStart = action.find("(") + 1;
		auto sEnd = action.find(")");
		auto sLength = sEnd - sStart;
		auto sVar = action.substr(sStart, sLength);

		return game.getVariable(sVar);
	}

	sf::Color sfmlColor(std::string color)
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
		case sf::Keyboard::Unknown:
			return "unknown";
			break;
		case sf::Keyboard::A:
			return "a";
			break;
		case sf::Keyboard::B:
			return "b";
			break;
		case sf::Keyboard::C:
			return "c";
			break;
		case sf::Keyboard::D:
			return "d";
			break;
		case sf::Keyboard::E:
			return "e";
			break;
		case sf::Keyboard::F:
			return "f";
			break;
		case sf::Keyboard::G:
			return "g";
			break;
		case sf::Keyboard::H:
			return "h";
			break;
		case sf::Keyboard::I:
			return "i";
			break;
		case sf::Keyboard::J:
			return "j";
			break;
		case sf::Keyboard::K:
			return "k";
			break;
		case sf::Keyboard::L:
			return "l";
			break;
		case sf::Keyboard::M:
			return "m";
			break;
		case sf::Keyboard::N:
			return "n";
			break;
		case sf::Keyboard::O:
			return "o";
			break;
		case sf::Keyboard::P:
			return "p";
			break;
		case sf::Keyboard::Q:
			return "q";
			break;
		case sf::Keyboard::R:
			return "r";
			break;
		case sf::Keyboard::S:
			return "s";
			break;
		case sf::Keyboard::T:
			return "t";
			break;
		case sf::Keyboard::U:
			return "u";
			break;
		case sf::Keyboard::V:
			return "v";
			break;
		case sf::Keyboard::W:
			return "w";
			break;
		case sf::Keyboard::X:
			return "x";
			break;
		case sf::Keyboard::Y:
			return "y";
			break;
		case sf::Keyboard::Z:
			return "z";
			break;
		case sf::Keyboard::Num0:
			return "num0";
			break;
		case sf::Keyboard::Num1:
			return "num1";
			break;
		case sf::Keyboard::Num2:
			return "num2";
			break;
		case sf::Keyboard::Num3:
			return "num3";
			break;
		case sf::Keyboard::Num4:
			return "num4";
			break;
		case sf::Keyboard::Num5:
			return "num5";
			break;
		case sf::Keyboard::Num6:
			return "num6";
			break;
		case sf::Keyboard::Num7:
			return "num7";
			break;
		case sf::Keyboard::Num8:
			return "num8";
			break;
		case sf::Keyboard::Num9:
			return "num9";
			break;
		case sf::Keyboard::Escape:
			return "escape";
			break;
		case sf::Keyboard::LControl:
			return "lcontrol";
			break;
		case sf::Keyboard::LShift:
			return "lshift";
			break;
		case sf::Keyboard::LAlt:
			return "lalt";
			break;
		case sf::Keyboard::LSystem:
			return "lsystem";
			break;
		case sf::Keyboard::RControl:
			return "rcontrol";
			break;
		case sf::Keyboard::RShift:
			return "rshift";
			break;
		case sf::Keyboard::RAlt:
			return "ralt";
			break;
		case sf::Keyboard::RSystem:
			return "rsystem";
			break;
		case sf::Keyboard::Menu:
			return "menu";
			break;
		case sf::Keyboard::LBracket:
			return "[";
			break;
		case sf::Keyboard::RBracket:
			return "]";
			break;
		case sf::Keyboard::Semicolon:
			return ";";
			break;
		case sf::Keyboard::Comma:
			return ",";
			break;
		case sf::Keyboard::Period:
			return ".";
			break;
		case sf::Keyboard::Quote:
			return "'";
			break;
		case sf::Keyboard::Slash:
			return "/";
			break;
		case sf::Keyboard::Backslash:
			return "\\";
			break;
		case sf::Keyboard::Tilde:
			return "~";
			break;
		case sf::Keyboard::Equal:
			return "=";
			break;
		case sf::Keyboard::Hyphen:
			return "-";
			break;
		case sf::Keyboard::Space:
			return "space";
			break;
		case sf::Keyboard::Enter:
			return "enter";
			break;
		case sf::Keyboard::Backspace:
			return "backspace";
			break;
		case sf::Keyboard::Tab:
			return "tab";
			break;
		case sf::Keyboard::PageUp:
			return "pageup";
			break;
		case sf::Keyboard::PageDown:
			return "pagedown";
			break;
		case sf::Keyboard::End:
			return "end";
			break;
		case sf::Keyboard::Home:
			return "home";
			break;
		case sf::Keyboard::Insert:
			return "insert";
			break;
		case sf::Keyboard::Delete:
			return "delete";
			break;
		case sf::Keyboard::Add:
			return "add";
			break;
		case sf::Keyboard::Subtract:
			return "subtract";
			break;
		case sf::Keyboard::Multiply:
			return "multiply";
			break;
		case sf::Keyboard::Divide:
			return "divide";
			break;
		case sf::Keyboard::Left:
			return "left";
			break;
		case sf::Keyboard::Right:
			return "right";
			break;
		case sf::Keyboard::Up:
			return "up";
			break;
		case sf::Keyboard::Down:
			return "down";
			break;
		case sf::Keyboard::Numpad0:
			return "numpad0";
			break;
		case sf::Keyboard::Numpad1:
			return "numpad1";
			break;
		case sf::Keyboard::Numpad2:
			return "numpad2";
			break;
		case sf::Keyboard::Numpad3:
			return "numpad3";
			break;
		case sf::Keyboard::Numpad4:
			return "numpad4";
			break;
		case sf::Keyboard::Numpad5:
			return "numpad5";
			break;
		case sf::Keyboard::Numpad6:
			return "numpad6";
			break;
		case sf::Keyboard::Numpad7:
			return "numpad7";
			break;
		case sf::Keyboard::Numpad8:
			return "numpad8";
			break;
		case sf::Keyboard::Numpad9:
			return "numpad9";
			break;
		case sf::Keyboard::F1:
			return "f1";
			break;
		case sf::Keyboard::F2:
			return "f2";
			break;
		case sf::Keyboard::F3:
			return "f3";
			break;
		case sf::Keyboard::F4:
			return "f4";
			break;
		case sf::Keyboard::F5:
			return "f5";
			break;
		case sf::Keyboard::F6:
			return "f6";
			break;
		case sf::Keyboard::F7:
			return "f7";
			break;
		case sf::Keyboard::F8:
			return "f8";
			break;
		case sf::Keyboard::F9:
			return "f9";
			break;
		case sf::Keyboard::F10:
			return "f10";
			break;
		case sf::Keyboard::F11:
			return "f11";
			break;
		case sf::Keyboard::F12:
			return "f12";
			break;
		case sf::Keyboard::F13:
			return "f13";
			break;
		case sf::Keyboard::F14:
			return "f14";
			break;
		case sf::Keyboard::F15:
			return "f15";
			break;
		case sf::Keyboard::Pause:
			return "pause";
			break;
		default:
			return "";
			break;
		}
	}
}