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
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace xge
{
	class Engine
	{
	public:
		// backend picks which Window implementation actually opens (SFML3,
		// Raylib, SDL2 or OpenGL - see window.h); defaults to SFML3 so existing
		// callers don't have to name one.
		explicit Engine(Game& game, WindowBackend backend = WindowBackend::SFML3);

		// Same, but with an already-built Window - what lets a test drive
		// Engine's key handling with a fake window instead of opening a real
		// one.
		Engine(Game& game, std::unique_ptr<Window> window);

		void loop(void);

		// Swaps the Window for another while the game is running: the old one
		// is destroyed first (some libraries can only have one window at a
		// time), then `create` is called for the new one, and every object's
		// picture is built again for it. Nothing about the game changes - not
		// the objects, the values, or the state the game is in - and the keys
		// held stay held. If `create` throws, the engine has no window and can
		// only be given one by calling this again (or destroyed); step() and
		// render() throw std::logic_error until then.
		void replaceWindow(const std::function<std::unique_ptr<Window>()>& create);

		// What loop() does each frame, split in two so a front end that owns
		// the event loop (the Qt application, XGEGUI) can run the engine from
		// its own timer instead of handing control to loop(). step() is the
		// simulation: it reads the keys, moves everything, and checks the
		// conditions. render() draws the current picture without moving
		// anything, so a paused game can be redrawn after its values are edited.
		void step(void);
		void render(void);

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

		// The commands each held key is currently driving: what its press ran,
		// or, after a state change, what the new state's binding for it
		// resumed. Releasing a key sends the release to exactly these, so it
		// always reaches whatever it started, even across a state change.
		std::array<std::vector<Command>, static_cast<std::size_t>(KeyCode::Count)> heldCommands{};

		// Game::stateChangeCount() as of the last syncHeldKeysToState().
		unsigned long syncedStateChanges = 0;

		// The current state decides what a held key means: when the state has
		// changed, releases what the held keys were driving in the old one and
		// resumes the new state's continuous bindings (an action's move.*) for
		// the keys still down - so a paddle stops while a state that does not
		// bind its key is up and moves again on return, with no re-press. A
		// key's state-changing or one-shot commands are never resumed, only
		// run on a real press (otherwise holding Space through a menu would
		// pause the game the moment it started).
		void syncHeldKeysToState();

		// Throws std::logic_error when there is no window (see replaceWindow).
		void requireWindow(void) const;
	};
}
