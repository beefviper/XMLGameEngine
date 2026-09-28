// engine.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "engine.h"

#include <optional>

namespace xge
{
	Engine::Engine(Game& game) :
		game(game),
		commandExecutor(game)
	{
		WindowDesc windowDesc = game.getWindowDesc();

		std::string name = windowDesc.name;

		const auto width = static_cast<unsigned int>(windowDesc.width);
		const auto height = static_cast<unsigned int>(windowDesc.height);
		const sf::VideoMode videoMode({ width, height });

		const auto windowState = (windowDesc.fullscreen == "true") ? sf::State::Fullscreen : sf::State::Windowed;

		window.create(videoMode, name, sf::Style::Default, windowState);
		window.setFramerateLimit(windowDesc.framerate);

		game.setCurrentState(0);
	}

	void Engine::loop(void)
	{
		while (window.isOpen())
		{
			// TODO: make polling events its own function, return vector<pair<string,bool>> of keypresses
			// pressed = true, released = false, don't need isKeyPressed map anymore?

			while (const std::optional event = window.pollEvent())
			{
				if (event->is<sf::Event::Closed>())
				{
					window.close();
				}
				else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
				{
					handleKeyPressed(keyPressed->code);
				}
				else if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>())
				{
					handleKeyReleased(keyReleased->code);
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
	void Engine::handleKeyPressed(sf::Keyboard::Key code)
	{
		// TODO: fix logic? remove if? just set value in map true?
		if (!isKeyPressed[code])
		{
			isKeyPressed[code] = true;

			for (auto& input : game.getCurrentState().input)
			{
				execute_action(code, input, true);
			}
		}
	}

	// TODO: does game reference need to be passed in? engine has game reference as member
	void Engine::handleKeyReleased(sf::Keyboard::Key code)
	{
		isKeyPressed[code] = false;

		for (auto& input : game.getCurrentState().input)
		{
			execute_action(code, input, false);
		}
	}

	// TODO: does game reference need to be passed in? engine has game reference as member
	void Engine::execute_action(sf::Keyboard::Key code, const KeyBinding& input, bool keyPressed)
	{
		if (input.first != sfmlKeyToString(code))
		{
			return;
		}

		for (const auto& command : input.second)
		{
			commandExecutor.executeInput(command, keyPressed);
		}
	}
}
