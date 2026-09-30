// engine.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "command.h"
#include "command_executor.h"
#include "game.h"
#include "keycode.h"
#include "object.h"
#include "window.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace xge
{
	class Engine
	{
	public:
		// backend picks which Window implementation actually opens (SFML3,
		// Raylib, or SDL2 - see window.h); defaults to SFML3 so existing
		// callers (main.cpp) don't have to name one.
		explicit Engine(Game& game, WindowBackend backend = WindowBackend::SFML3);

		// Same, but with an already-built Window - what lets a test drive
		// Engine's key handling with a fake window instead of opening a real
		// one.
		Engine(Game& game, std::unique_ptr<Window> window);

		void loop(void);

		// TODO: make handleKeyPressed and handleKeyReleased private
		void handleKeyPressed(KeyCode key);
		void handleKeyReleased(KeyCode key);

	private:
		Game& game;
		CommandExecutor commandExecutor;
		std::unique_ptr<Window> window;

		// Engine's own per-frame record of which keys are currently held -
		// built entirely from Window::pollEvents()'s press/release deltas
		// (see window.h), never asked of the window itself, since Engine is
		// what needs this for the whole frame (state input lookups) and
		// there's exactly one of it regardless of which backend is running.
		std::array<bool, static_cast<std::size_t>(KeyCode::Count)> isKeyPressed{};

		// The commands each held key ran when it was pressed, so releasing it
		// sends the release to those same commands whatever state is active by
		// then. Looking the key up in the *current* state's inputs instead
		// lost the release whenever the state had changed while the key was
		// down (hold Left, pause, let go of Left: the paused state has no
		// Left binding, so the player never heard about it).
		std::array<std::vector<Command>, static_cast<std::size_t>(KeyCode::Count)> heldCommands{};
	};
}
