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

		const int width = static_cast<int>(windowDesc.width);
		const int height = static_cast<int>(windowDesc.height);
		const sf::VideoMode videoMode(width, height);

		const auto windowMode = (windowDesc.fullscreen == "true") ? sf::Style::Fullscreen : sf::Style::Default;

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

			for (auto& object : game.getCurrentObjects())
			{
				if (game.isShown(object))
				{
					window.draw(*object.sprite);
				}
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

	void Engine::move(Object& object, Engine::Direction direction, float step) noexcept
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
		constexpr auto sStart = 0;
		const auto sEnd = input.first.find(".");

		return input.first.substr(sStart, sEnd);
	}

	std::string Engine::getCommandFromInput(PairStringString input)
	{
		const auto sStart = input.first.rfind('.') + 1;
		const auto sEnd = input.first.length();
		const auto sLength = sEnd - sStart;

		return input.first.substr(sStart, sLength);
	}

	float Engine::getValueFromAction(std::string action)
	{
		const auto sStart = action.find("(") + 1;
		const auto sEnd = action.find(")");
		const auto sLength = sEnd - sStart;
		auto sVar = action.substr(sStart, sLength);

		return game.getVariable(sVar);
	}
}
