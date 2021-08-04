// engine.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include "game.h"
#include "object.h"
#include "utils.h"

#include <SFML/Window.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <string>
#include <memory>

namespace xge
{
	using PairStringString = std::pair<const std::string, std::string>;

	class Engine
	{
	public:
		Engine(Game& game);

		void loop(void);

		void handleKeyPressed(Game& game, sf::Event& event);
		void handleKeyReleased(Game& game, sf::Event& event);

	private:
		Game& game;
		sf::RenderWindow window;
		enum class Direction { None, Up, Down, Left, Right };

		std::map<sf::Keyboard::Key, bool> isKeyPressed;
		void execute_action(Game& game, sf::Event& event, PairStringString& input, bool keyPressed = true);
		void move(Object& object, Engine::Direction direction, float step) noexcept;
		void move(Object& object, std::string direction, float step);

		std::string getObjectFromInput(PairStringString input);
		std::string getCommandFromInput(PairStringString input);
		float getValueFromAction(std::string action);

		std::map<std::string, Direction> mapDirection
		{
			{"up", Direction::Up},
			{"down", Direction::Down},
			{"left", Direction::Left},
			{"right", Direction::Right	},
		};
	};
}