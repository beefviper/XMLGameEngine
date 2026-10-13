// engine.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "engine.h"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace xge
{
	Engine::Engine(Game& gameToRun, std::unique_ptr<Window> newWindow, std::unique_ptr<Audio> newAudio) :
		game(gameToRun),
		commandExecutor(gameToRun),
		window(std::move(newWindow)),
		audio(newAudio ? std::move(newAudio) : std::make_unique<NullAudio>())
	{
		this->audio->load(game.getSounds());

		// Builds and measures every object's initial visual now that a
		// window (and therefore a real backend to build against) exists -
		// this used to happen inside Game's own constructor, via the
		// standalone game_sfml class, before Engine or any Window existed
		// at all; see Window::init() in window.h for why it has to happen
		// here instead.
		this->window->init(game.getCurrentObjects());

		// Sizes are real now: finish any position that depends on one.
		game.resolveSizeDependentPositions();

		game.setCurrentState(0);
		syncedStateChanges = game.stateChangeCount();
	}

	void Engine::replaceWindow(const std::function<std::unique_ptr<Window>()>& create)
	{
		window.reset();
		window = create();

		window->init(game.getCurrentObjects());

		// Sizes may differ a little between libraries (text above all).
		game.resolveSizeDependentPositions();
	}

	void Engine::replaceAudio(const std::function<std::unique_ptr<Audio>()>& create)
	{
		audio.reset();

		try
		{
			audio = create();
		}
		catch (...)
		{
			audio = std::make_unique<NullAudio>();
			throw;
		}

		if (!audio)
		{
			audio = std::make_unique<NullAudio>();
		}

		audio->load(game.getSounds());
	}

	void Engine::silence(void)
	{
		audio->stopAll();
	}

	void Engine::playRequestedSounds(void)
	{
		for (const std::string& name : game.takeSoundRequests())
		{
			audio->play(name);
		}
	}

	void Engine::requireWindow(void) const
	{
		// Only after a replaceWindow() whose `create` threw.
		if (!window)
		{
			throw std::logic_error("the engine has no window: the last one asked for could not be made");
		}
	}

	bool Engine::isWindowOpen(void) const
	{
		return window && window->isOpen();
	}

	void Engine::loop(void)
	{
		while (isWindowOpen())
		{
			step();
			render();
		}
	}

	void Engine::step(void)
	{
		requireWindow();

		auto events = std::move(pumpedKeys);
		pumpedKeys.clear();

		for (const auto& event : window->pollEvents())
		{
			events.push_back(event);
		}

		for (const auto& [key, pressed] : events)
		{
			if (pressed) { handleKeyPressed(key); }
			else { handleKeyReleased(key); }
		}

		game.updateObjects();

		// A <condition> can change the state during updateObjects.
		syncHeldKeysToState();

		// A text that grew or shrank (a score) needs its position worked out
		// again for the new size, measured now rather than at the next draw.
		for (auto& object : game.getCurrentObjects())
		{
			if (object.visualDirty && game_expr::sizeNeedsBackend(object.shapeKind))
			{
				window->prepare(object);
			}
		}
		game.resolveSizeDependentPositions();

		// Keys, collisions and conditions have all had their say.
		playRequestedSounds();
	}

	void Engine::pump(void)
	{
		requireWindow();

		for (const auto& event : window->pollEvents())
		{
			pumpedKeys.push_back(event);
		}
	}

	void Engine::render(void)
	{
		requireWindow();

		drawFrame(*window, game.getCurrentObjects(), game.getWindowDesc().background, [&](const Object& object) { return game.isShown(object); });
	}

	void Engine::handleKeyPressed(KeyCode key)
	{
		const auto index = static_cast<std::size_t>(key);

		// Held keys repeat as press events on some backends; only the first
		// one counts.
		if (!heldKeys.press(key))
		{
			return;
		}

		// A copy of the State, not a reference into it: a command run below
		// (<push state="paused" />, <pop />) pushes or pops the state stack.
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

		heldKeys.release(key);

		// Release exactly what this key's press started, even if the state has
		// changed since (see heldCommands in engine.h). A key that ran nothing
		// when pressed has nothing to release.
		const std::vector<Command> commands = std::move(heldCommands[index]);
		heldCommands[index].clear();

		for (const auto& command : commands)
		{
			commandExecutor.executeInput(command, false);
		}

		// An action is held for as long as any one of its keys is down:
		// letting go of A while Left is still held (both move left) must not
		// stop it.
		for (std::size_t other = 0; other < heldCommands.size(); ++other)
		{
			if (other == index || !heldKeys.isDown(static_cast<KeyCode>(other)))
			{
				continue;
			}

			for (const auto& command : heldCommands[other])
			{
				commandExecutor.executeHeldInput(command);
			}
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
			if (!heldKeys.isDown(key))
			{
				continue;
			}

			const auto index = static_cast<std::size_t>(key);
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
