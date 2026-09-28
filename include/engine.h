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

namespace xge
{
	// One entry from a State's <input> map: which key it's bound to, and the
	// (typed) commands to run when that key is pressed/released.
	using KeyBinding = std::pair<const KeyCode, std::vector<Command>>;

	class Engine
	{
	public:
		// backend picks which Window implementation actually opens (SFML3,
		// Raylib, or SDL2 - see window.h); defaults to SFML3 so existing
		// callers (main.cpp) don't have to name one.
		explicit Engine(Game& game, WindowBackend backend = WindowBackend::SFML3);

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

		void execute_action(KeyCode key, const KeyBinding& input, bool keyPressed = true);
	};
}
