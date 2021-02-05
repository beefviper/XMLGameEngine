// engine.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include <memory>

#include <SFML/Window.hpp>

#include "utils.h"
#include "game.h"
#include "object.h"

using PairStringString = std::pair<const std::string, std::string>;

namespace xge
{
	class Engine
	{
	public:
		Engine(xge::Game& game);

		void loop(void);

		void handleKeyPressed(xge::Game& game, sf::Event& event);
		void handleKeyReleased(xge::Game& game, sf::Event& event);

	private:
		xge::Game& game;
		sf::RenderWindow window;
		enum class Direction { None, Up, Down, Left, Right };

		std::map<sf::Keyboard::Key, bool> isKeyPressed;
		void execute_action(xge::Game& game, sf::Event& event, PairStringString& input, bool keyPressed = true);
		void move(xge::Object& object, Engine::Direction direction, float step);
		void move(xge::Object& object, std::string direction, float step);

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
