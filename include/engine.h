// engine.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "command.h"
#include "command_executor.h"
#include "game.h"
#include "object.h"
#include "window.h"

#include <map>
#include <memory>
#include <string>

namespace xge
{
	// One entry from a State's <input> map: which key it's bound to, and the
	// (typed) commands to run when that key is pressed/released.
	using KeyBinding = std::pair<const std::string, std::vector<Command>>;

	class Engine
	{
	public:
		Engine(Game& game);

		void loop(void);

		// TODO: make handleKeyPressed and handleKeyRelease private
		void handleKeyPressed(const std::string& keyName);
		void handleKeyReleased(const std::string& keyName);

	private:
		Game& game;
		CommandExecutor commandExecutor;
		std::unique_ptr<Window> window;

		std::map<std::string, bool> isKeyPressed;
		void execute_action(const std::string& keyName, const KeyBinding& input, bool keyPressed = true);
	};
}
