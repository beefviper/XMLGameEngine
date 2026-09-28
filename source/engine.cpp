// engine.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "engine.h"

namespace xge
{
	Engine::Engine(Game& game) :
		game(game),
		commandExecutor(game),
		window(WindowFactory::create(game.getWindowDesc()))
	{
		game.setCurrentState(0);
	}

	void Engine::loop(void)
	{
		while (window->isOpen())
		{
			for (auto& [keyName, pressed] : window->pollEvents())
			{
				if (pressed) { handleKeyPressed(keyName); }
				else { handleKeyReleased(keyName); }
			}

			game.updateObjects();

			window->clear(game.getWindowDesc().background);

			for (auto& object : game.getCurrentObjects())
			{
				if (game.isShown(object))
				{
					window->draw(object);
				}
			}

			window->display();
		}
	}

	// TODO: does game reference need to be passed in? engine has game reference as member
	void Engine::handleKeyPressed(const std::string& keyName)
	{
		// TODO: fix logic? remove if? just set value in map true?
		if (!isKeyPressed[keyName])
		{
			isKeyPressed[keyName] = true;

			for (auto& input : game.getCurrentState().input)
			{
				execute_action(keyName, input, true);
			}
		}
	}

	// TODO: does game reference need to be passed in? engine has game reference as member
	void Engine::handleKeyReleased(const std::string& keyName)
	{
		isKeyPressed[keyName] = false;

		for (auto& input : game.getCurrentState().input)
		{
			execute_action(keyName, input, false);
		}
	}

	// TODO: does game reference need to be passed in? engine has game reference as member
	void Engine::execute_action(const std::string& keyName, const KeyBinding& input, bool keyPressed)
	{
		if (input.first != keyName)
		{
			return;
		}

		for (const auto& command : input.second)
		{
			commandExecutor.executeInput(command, keyPressed);
		}
	}
}
