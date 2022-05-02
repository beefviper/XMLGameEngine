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

	void Engine::execute_action(Game& game, sf::Event& event, PairStringVectorString& input, bool keyPressed)
	{
		if (input.first == sfmlKeyToString(event.key.code))
		{
			std::string aObject = input.second.at(0);
			std::string aCommand = input.second.at(1);

			if (aObject == "state")
			{
				if (keyPressed)
				{
					game.setCurrentState(aCommand);
				}
			}
			else if (&game.getObject(aObject) != nullptr)
			{
				std::string aAction = game.getObject(aObject).action[aCommand].at(0);
				
				if (aAction == "moveleft" || aAction == "moveright" || aAction == "moveup" || aAction == "movedown")
				{
					auto aStep = std::stof(game.getObject(aObject).action[aCommand].at(1));

					if (!keyPressed)
					{
						aStep = 0;
					}

					move(game.getObject(aObject), aCommand, aStep);
				}
				else if ( aAction == "fire")
				{
					auto currentX = game.getObject(aObject).position.x;
					auto& currentObjName = game.getObject(aObject).action[aCommand].at(1);
					auto& currentObj = game.getObject(currentObjName);

					currentObj.position.x = currentX + game.getObject(aObject).sprite.get()->getLocalBounds().width / 2;
					currentObj.position.y = game.getObject(aObject).sprite.get()->getGlobalBounds().top;
					currentObj.velocity.y = -2;
					currentObj.isVisible = true;
				}
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
}
