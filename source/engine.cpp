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

		game.setCurrentState(0);
	}

	void Engine::loop(void)
	{
		while (window.isOpen())
		{
			// TODO: make polling events its own function, return vector<pair<string,bool>> of keypresses
			// pressed = true, released = false, don't need isKeyPressed map anymore?

			sf::Event event;
			while (window.pollEvent(event))
			{
				switch (event.type)
				{
				case sf::Event::Closed:
					window.close();
					break;

				case sf::Event::KeyPressed:
					handleKeyPressed(event);
					break;

				case sf::Event::KeyReleased:
					handleKeyReleased(event);
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

	// TODO: does game reference need to be passed in? engine has game reference as member
	void Engine::handleKeyPressed(const sf::Event& event)
	{
		// TODO: fix logic? remove if? just set value in map true?
		if (!isKeyPressed[event.key.code])
		{
			isKeyPressed[event.key.code] = true;

			for (auto& input : game.getCurrentState().input)
			{
				execute_action(event, input, true);
			}
		}
	}

	// TODO: does game reference need to be passed in? engine has game reference as member
	void Engine::handleKeyReleased(const sf::Event& event)
	{
		isKeyPressed[event.key.code] = false;

		for (auto& input : game.getCurrentState().input)
		{
			execute_action(event, input, false);
		}
	}

	// TODO: does game reference need to be passed in? engine has game reference as member
	void Engine::execute_action(const sf::Event& event, PairStringVectorString& input, bool keyPressed)
	{
		if (input.first == sfmlKeyToString(event.key.code))
		{
			std::string aObject = input.second.at(0);
			std::string aCommand = input.second.at(1);

			if (aObject == "state")
			{
				if (keyPressed)
				{
					if (aCommand == "pop")
					{
						game.popState();
					}
					else
					{
						game.pushState(aCommand);
					}
				}
			}
			else
			{
				std::string aAction = game.getObject(aObject).action[aCommand].at(0);

				// TODO: move move() code out of Engine, put in Game
				// Engine can pass along a vector<string> of commands

				if (aAction == "moveleft" || aAction == "moveright" || aAction == "moveup" || aAction == "movedown")
				{
					auto aStep = std::stof(game.getObject(aObject).action[aCommand].at(1));

					if (!keyPressed)
					{
						aStep = 0;
					}

					move(game.getObject(aObject), aCommand, aStep);
				}
				else if (aAction == "fire")
				{
					if (keyPressed)
					{
						auto currentX = game.getObject(aObject).position.x;
						auto& currentObjName = game.getObject(aObject).action[aCommand].at(1);
						auto& currentObj = game.getObject(currentObjName);

						if (currentObj.collisionData.enabled == false)
						{
							currentObj.position.x = currentX + game.getObject(aObject).sprite.get()->getLocalBounds().width / 2;
							currentObj.position.y = game.getObject(aObject).sprite.get()->getGlobalBounds().top;
							currentObj.velocity.y = currentObj.variable["speed"];
							currentObj.isVisible = true;
							currentObj.collisionData.enabled = true;
						}
					}
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
		default:
			break;
		}
	}

	void Engine::move(Object& object, std::string direction, float step)
	{
		move(object, mapDirection[direction], step);
	}
}
