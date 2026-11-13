// main.cpp
// Pong
// author: beefviper
// date: Nov 13, 2026
//
// The window: keys in, 60 frames a second, the picture out.

#include "pong_min.h"

#include <SFML/Graphics.hpp>

#include <optional>

namespace
{
	void draw(sf::RenderWindow& window, const pong_min::Thing& thing)
	{
		sf::RectangleShape shape(thing.size);
		shape.setPosition(thing.position);
		shape.setFillColor(sf::Color::White);
		window.draw(shape);
	}
}

int main()
{
	using namespace pong_min;

	sf::RenderWindow window(sf::VideoMode({ static_cast<unsigned>(windowWidth), static_cast<unsigned>(windowHeight) }), "Pong");
	window.setFramerateLimit(60);
	window.setKeyRepeatEnabled(false);

	Pong game;
	while (window.isOpen())
	{
		while (const std::optional event = window.pollEvent())
		{
			if (event->is<sf::Event::Closed>()) { window.close(); }
			if (const auto* key = event->getIf<sf::Event::KeyPressed>()) { game.key(key->code, true); }
			if (const auto* key = event->getIf<sf::Event::KeyReleased>()) { game.key(key->code, false); }
		}

		game.frame();

		window.clear(sf::Color::Black);
		draw(window, game.ball);
		draw(window, game.paddle1);
		draw(window, game.paddle2);
		window.display();
	}
}
