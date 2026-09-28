// engine.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "engine.h"

namespace xge
{
	Engine::Engine(Game& game, WindowBackend backend) :
		game(game),
		commandExecutor(game),
		window(WindowFactory::create(game.getWindowDesc(), backend))
	{
		// Builds and measures every object's initial visual now that a
		// window (and therefore a real backend to build against) exists -
		// this used to happen inside Game's own constructor, via the
		// standalone game_sfml class, before Engine or any Window existed
		// at all; see Window::init() in window.h for why it has to happen
		// here instead.
		window->init(game.getCurrentObjects());

		game.setCurrentState(0);
	}

	void Engine::loop(void)
	{
		while (window->isOpen())
		{
			for (auto& [key, pressed] : window->pollEvents())
			{
				if (pressed) { handleKeyPressed(key); }
				else { handleKeyReleased(key); }
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
	void Engine::handleKeyPressed(KeyCode key)
	{
		// TODO: fix logic? remove if? just set value in map true?
		if (!isKeyPressed[static_cast<std::size_t>(key)])
		{
			isKeyPressed[static_cast<std::size_t>(key)] = true;

			for (auto& input : game.getCurrentState().input)
			{
				execute_action(key, input, true);
			}
		}
	}

	// TODO: does game reference need to be passed in? engine has game reference as member
	void Engine::handleKeyReleased(KeyCode key)
	{
		isKeyPressed[static_cast<std::size_t>(key)] = false;

		for (auto& input : game.getCurrentState().input)
		{
			execute_action(key, input, false);
		}
	}

	// TODO: does game reference need to be passed in? engine has game reference as member
	void Engine::execute_action(KeyCode key, const KeyBinding& input, bool keyPressed)
	{
		if (input.first != key)
		{
			return;
		}

		for (const auto& command : input.second)
		{
			commandExecutor.executeInput(command, keyPressed);
		}
	}
}
