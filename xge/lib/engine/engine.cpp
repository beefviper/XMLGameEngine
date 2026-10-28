// engine.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "engine/engine.h"

#include "game/random.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
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
		// at all; see Window::init() in window/window.h for why it has to happen
		// here instead.
		this->window->init(game.getCurrentObjects());

		// Sizes are real now: finish any position that depends on one.
		game.resolveSizeDependentPositions();

		game.setCurrentState(0);
		syncedStateChanges = game.stateChangeCount();

		// A player's game put away mid-move stops moving; the keys stay down,
		// and the next state picks up the ones it binds (syncHeldKeysToState).
		game.onTurnPutAway([this] { stopHeldCommands(); });
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
		playingLoops.clear();

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

		// Started again by the next step() if the game still wants them.
		playingLoops.clear();
	}

	void Engine::playRequestedSounds(void)
	{
		for (const std::string& name : game.takeSilenceRequests())
		{
			audio->stop(name);
			playingLoops.erase(name);
		}

		const std::set<std::string> wanted = game.soundsOn();
		for (const std::string& name : playingLoops)
		{
			if (!wanted.count(name)) { audio->stop(name); }
		}
		for (const std::string& name : wanted)
		{
			if (!playingLoops.count(name)) { audio->loop(name); }
		}
		playingLoops = wanted;

		for (const std::string& name : game.takeSoundRequests())
		{
			audio->play(name);
		}

		audio->update();
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

		// A key pressed while the demo plays ends it, and is not played.
		if (demoPlaying && std::any_of(events.begin(), events.end(), [](const auto& event) { return event.second; }))
		{
			stopDemo();
			events.clear();
		}

		if (demoPlaying)
		{
			const std::vector<DemoKey>& keys = game.getDemo().keys;
			for (; demoNext < keys.size() && keys[demoNext].frame <= demoFrame; ++demoNext)
			{
				const KeyCode key = keyCodeFromString(keys[demoNext].button);
				if (keys[demoNext].pressed) { handleKeyPressed(key); }
				else { handleKeyReleased(key); }
			}
		}
		else
		{
			for (const auto& [key, pressed] : events)
			{
				if (pressed) { handleKeyPressed(key); }
				else { handleKeyReleased(key); }
			}

			handleControls(window->controls());
		}

		playComputer();

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

		// A new game: what the last one left to keep is written down (not
		// what a demo did).
		if (!keptFile.empty() && !demoPlaying && game.resetCount() != keptResets)
		{
			keptResets = game.resetCount();
			writeKept();
		}

		if (recording && !demoPlaying)
		{
			++recordedSoFar.frames;
		}
		if (demoPlaying && ++demoFrame >= game.getDemo().frames)
		{
			stopDemo();
		}
		if (game.takeDemoRequest() && !demoPlaying && game.getDemo().recorded)
		{
			startDemo();
		}
	}

	void Engine::record(void)
	{
		recordedSoFar = RawDemo{};
		recordedSoFar.recorded = true;
		recordedSoFar.seed = std::random_device{}();
		setRandomState(std::mt19937{ recordedSoFar.seed });
		game.resetAll();
		recording = true;
	}

	RawDemo Engine::recorded(void) const
	{
		return recordedSoFar;
	}

	void Engine::releaseHeldKeys(void)
	{
		for (std::size_t key = 0; key < static_cast<std::size_t>(KeyCode::Count); ++key)
		{
			if (heldKeys.isDown(static_cast<KeyCode>(key)))
			{
				handleKeyReleased(static_cast<KeyCode>(key));
			}
		}
	}

	void Engine::startDemo(void)
	{
		releaseHeldKeys();
		std::ostringstream kept;
		game.saveKept(kept);
		keptBeforeDemo = kept.str();

		demoPlaying = true;
		demoFrame = 0;
		demoNext = 0;
		setRandomState(std::mt19937{ game.getDemo().seed });
		game.resetAll();
	}

	void Engine::stopDemo(void)
	{
		releaseHeldKeys();
		demoPlaying = false;
		seedRandom();
		game.resetAll();
		std::istringstream kept(keptBeforeDemo);
		game.loadKept(kept);
		keptResets = game.resetCount();
	}

	Engine::~Engine()
	{
		game.onTurnPutAway({});

		// Closed while the demo plays: what was kept before it is what lasts.
		if (demoPlaying)
		{
			std::istringstream kept(keptBeforeDemo);
			game.loadKept(kept);
		}
		writeKept();
	}

	void Engine::keepIn(const std::string& file)
	{
		if (!game.keepsAnything())
		{
			return;
		}
		keptFile = file;
		keptResets = game.resetCount();
		std::ifstream in(file);
		if (in)
		{
			game.loadKept(in);
		}
	}

	void Engine::writeKept(void) const
	{
		if (keptFile.empty())
		{
			return;
		}

		// Written beside it and moved into place, so a program stopped while
		// writing leaves the last good one.
		try
		{
			const std::filesystem::path path{ keptFile };
			if (path.has_parent_path()) { std::filesystem::create_directories(path.parent_path()); }
			std::filesystem::path partial = path;
			partial += ".partial";
			{
				std::ofstream out(partial, std::ios::trunc);
				game.saveKept(out);
				if (!out) { throw std::runtime_error("could not write " + partial.string()); }
			}
			std::filesystem::rename(partial, path);
		}
		catch (const std::exception& error)
		{
			std::cerr << "warning: what the game keeps was not saved: " << error.what() << '\n';
		}
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

		drawFrame(*window, game.getCurrentObjects(), game.getWindowDesc().background,
			[&](const Object& object) { return object.isDrawn && game.isStill(object); },
			[&](const Object& object) { return object.isDrawn && game.isShown(object); });
	}

	void Engine::playComputer(void)
	{
		const std::set<KeyCode> wanted = game.computerButtons();
		computerPressing = true;
		for (const KeyCode key : computerHeld)
		{
			if (!wanted.count(key)) { handleKeyReleased(key); }
		}
		for (const KeyCode key : wanted)
		{
			if (!computerHeld.count(key)) { handleKeyPressed(key); }
		}
		computerPressing = false;
		computerHeld = wanted;
	}

	void Engine::handleKeyPressed(KeyCode key)
	{
		const auto index = static_cast<std::size_t>(key);

		if (recording && !demoPlaying && !computerPressing)
		{
			recordedSoFar.keys.push_back({ keyCodeToString(key), true, recordedSoFar.frames });
		}

		// Held keys repeat as press events on some backends; only the first
		// one counts.
		if (!heldKeys.press(key))
		{
			return;
		}

		// The commands are copied out before any runs: a command run below
		// (<push state="paused" />, <pop />) pushes or pops the state stack.
		const State& state = game.getCurrentState();
		const auto binding = state.input.find(key);
		if (binding == state.input.end())
		{
			return;
		}

		heldCommands[index] = binding->second;
		game.takeTurnStop();
		for (const auto& command : heldCommands[index])
		{
			if (!commandExecutor.executeInput(command, true) || game.takeTurnStop()) { break; }
		}

		syncHeldKeysToState();
	}

	void Engine::handleKeyReleased(KeyCode key)
	{
		const auto index = static_cast<std::size_t>(key);

		if (recording && !demoPlaying && !computerPressing && heldKeys.isDown(key))
		{
			recordedSoFar.keys.push_back({ keyCodeToString(key), false, recordedSoFar.frames });
		}

		heldKeys.release(key);

		// Release exactly what this key's press started, even if the state has
		// changed since (see heldCommands in engine/engine.h). A key that ran nothing
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

	void Engine::handleControls(const Controls& controls)
	{
		// A pad's buttons as keys: its d-pad and its stick pushed past halfway
		// are one set of directions.
		std::array<std::array<bool, padButtonCount>, padCount> padDown{};
		for (std::size_t pad = 0; pad < padCount; ++pad)
		{
			const Pad& now = controls.pads[pad];
			if (!now.connected) { continue; }
			padDown[pad] = now.buttons;
			constexpr float half = 0.5f;
			padDown[pad][static_cast<std::size_t>(PadButton::Up)] = padDown[pad][static_cast<std::size_t>(PadButton::Up)] || now.y < -half;
			padDown[pad][static_cast<std::size_t>(PadButton::Down)] = padDown[pad][static_cast<std::size_t>(PadButton::Down)] || now.y > half;
			padDown[pad][static_cast<std::size_t>(PadButton::Left)] = padDown[pad][static_cast<std::size_t>(PadButton::Left)] || now.x < -half;
			padDown[pad][static_cast<std::size_t>(PadButton::Right)] = padDown[pad][static_cast<std::size_t>(PadButton::Right)] || now.x > half;
		}

		// Where each control is, 0 to 1 across the window; -1 when it is not there.
		const WindowDesc& desc = game.getWindowDesc();
		std::array<float, axisCount> axes{};
		axes.fill(-1.0f);
		if (controls.pointerInside && desc.width > 0.0f && desc.height > 0.0f)
		{
			axes[static_cast<std::size_t>(Axis::MouseX)] = std::clamp(controls.pointerX / desc.width, 0.0f, 1.0f);
			axes[static_cast<std::size_t>(Axis::MouseY)] = std::clamp(controls.pointerY / desc.height, 0.0f, 1.0f);
		}
		for (std::size_t pad = 0; pad < padCount; ++pad)
		{
			const Pad& now = controls.pads[pad];
			if (!now.connected) { continue; }
			axes[static_cast<std::size_t>(pad == 0 ? Axis::Pad1X : Axis::Pad2X)] = std::clamp((now.x + 1.0f) * 0.5f, 0.0f, 1.0f);
			axes[static_cast<std::size_t>(pad == 0 ? Axis::Pad1Y : Axis::Pad2Y)] = std::clamp((now.y + 1.0f) * 0.5f, 0.0f, 1.0f);
		}

		if (!controlsSeen)
		{
			controlsSeen = true;
			lastControls = controls;
			lastPadDown = padDown;
			lastAxes = axes;
			return;
		}

		for (std::size_t button = 0; button < controls.mouse.size(); ++button)
		{
			if (controls.mouse[button] == lastControls.mouse[button]) { continue; }
			const auto key = static_cast<KeyCode>(static_cast<std::size_t>(KeyCode::MouseLeft) + button);
			if (controls.mouse[button]) { handleKeyPressed(key); }
			else { handleKeyReleased(key); }
		}
		for (std::size_t pad = 0; pad < padCount; ++pad)
		{
			for (std::size_t button = 0; button < padButtonCount; ++button)
			{
				if (padDown[pad][button] == lastPadDown[pad][button]) { continue; }
				const KeyCode key = padKey(pad, static_cast<PadButton>(button));
				if (padDown[pad][button]) { handleKeyPressed(key); }
				else { handleKeyReleased(key); }
			}
		}
		lastControls = controls;
		lastPadDown = padDown;

		// A control runs its binding when it has moved (or come back), so a
		// mouse left resting lets the keys move the same object.
		const std::array<float, axisCount> before = lastAxes;
		lastAxes = axes;
		// A copy of the bindings (usually none), not a reference into the
		// state: a command may push or pop it.
		const auto bindings = game.getCurrentState().axes;
		for (const auto& [axis, commands] : bindings)
		{
			const auto index = static_cast<std::size_t>(axis);
			constexpr float still = 0.0005f;
			if (axes[index] < 0.0f || std::abs(axes[index] - before[index]) < still)
			{
				continue;
			}
			for (const auto& command : commands)
			{
				commandExecutor.executeAxis(command, axis, axes[index]);
			}
		}
	}

	void Engine::syncHeldKeysToState()
	{
		if (game.stateChangeCount() == syncedStateChanges)
		{
			return;
		}

		syncedStateChanges = game.stateChangeCount();

		// Everything the old state had running stops first...
		stopHeldCommands();

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

	void Engine::stopHeldCommands(void)
	{
		for (auto& held : heldCommands)
		{
			const std::vector<Command> commands = std::move(held);
			held.clear();

			for (const auto& command : commands)
			{
				commandExecutor.executeInput(command, false);
			}
		}
	}
}
