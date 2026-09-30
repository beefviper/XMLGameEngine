// engine.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "engine.h"

#include <utility>

namespace xge
{
	Engine::Engine(Game& game, WindowBackend backend) :
		Engine(game, WindowFactory::create(game.getWindowDesc(), backend))
	{
	}

	Engine::Engine(Game& game, std::unique_ptr<Window> window) :
		game(game),
		commandExecutor(game),
		window(std::move(window))
	{
		// Builds and measures every object's initial visual now that a
		// window (and therefore a real backend to build against) exists -
		// this used to happen inside Game's own constructor, via the
		// standalone game_sfml class, before Engine or any Window existed
		// at all; see Window::init() in window.h for why it has to happen
		// here instead.
		this->window->init(game.getCurrentObjects());

		game.setCurrentState(0);
		syncedStateChanges = game.stateChangeCount();
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

			// A <condition> can change the state during updateObjects.
			syncHeldKeysToState();

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

	void Engine::handleKeyPressed(KeyCode key)
	{
		const auto index = static_cast<std::size_t>(key);

		// Held keys repeat as press events on some backends; only the first
		// one counts.
		if (isKeyPressed[index])
		{
			return;
		}

		isKeyPressed[index] = true;

		// A copy of the State, not a reference into it: a command run below
		// (state('paused'), state()) pushes or pops the state stack.
		const State state = game.getCurrentState();
		const auto binding = state.input.find(key);
		if (binding == state.input.end())
		{
			return;
		}

		heldCommands[index] = binding->second;
		for (const auto& command : heldCommands[index])
		{
			commandExecutor.executeInput(command, true);
		}

		syncHeldKeysToState();
	}

	void Engine::handleKeyReleased(KeyCode key)
	{
		const auto index = static_cast<std::size_t>(key);

		isKeyPressed[index] = false;

		// Release exactly what this key's press started, even if the state has
		// changed since (see heldCommands in engine.h). A key that ran nothing
		// when pressed has nothing to release.
		const std::vector<Command> commands = std::move(heldCommands[index]);
		heldCommands[index].clear();

		for (const auto& command : commands)
		{
			commandExecutor.executeInput(command, false);
		}

		syncHeldKeysToState();
	}

	void Engine::syncHeldKeysToState()
	{
		if (game.stateChangeCount() == syncedStateChanges)
		{
			return;
		}

		syncedStateChanges = game.stateChangeCount();

		// Everything the old state had running stops first...
		for (auto& held : heldCommands)
		{
			const std::vector<Command> commands = std::move(held);
			held.clear();

			for (const auto& command : commands)
			{
				commandExecutor.executeInput(command, false);
			}
		}

		// ...then the new state's continuous bindings start for the keys that
		// are still down.
		const State state = game.getCurrentState();
		for (const auto& [key, commands] : state.input)
		{
			const auto index = static_cast<std::size_t>(key);
			if (!isKeyPressed[index])
			{
				continue;
			}

			for (const auto& command : commands)
			{
				if (commandExecutor.executeHeldInput(command))
				{
					heldCommands[index].push_back(command);
				}
			}
		}
	}
}
