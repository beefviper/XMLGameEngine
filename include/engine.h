// engine.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include "command.h"
#include "command_executor.h"
#include "game.h"
#include "object.h"
#include "utils.h"

#include <SFML/Window.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/System/Clock.hpp>

#include <string>
#include <memory>
#include <iomanip>

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
		void handleKeyPressed(sf::Keyboard::Key code);
		void handleKeyReleased(sf::Keyboard::Key code);

	private:
		Game& game;
		CommandExecutor commandExecutor;
		sf::RenderWindow window;
		sf::Clock clock;

		std::map<sf::Keyboard::Key, bool> isKeyPressed;
		void execute_action(sf::Keyboard::Key code, const KeyBinding& input, bool keyPressed = true);
	};
}
