// main.cpp
// Pong
// author: beefviper
// date: Nov 24, 2026
//
// The window: keys in, a step of the time since the last frame, the picture out.

#include "pong_min.h"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <optional>

namespace
{
	using namespace pong_min;

	void box(sf::RenderWindow& window, sf::Vector2f position, sf::Vector2f extent, sf::Color color = sf::Color::White)
	{
		sf::RectangleShape shape(extent);
		shape.setPosition(position);
		shape.setFillColor(color);
		window.draw(shape);
	}
}

int main()
{
	sf::RenderWindow window(sf::VideoMode({ static_cast<unsigned>(windowWidth), static_cast<unsigned>(windowHeight) }), "Pong");
	window.setFramerateLimit(60);
	window.setKeyRepeatEnabled(false);

	Pong game;
	sf::Clock clock;
	while (window.isOpen())
	{
		while (const std::optional event = window.pollEvent())
		{
			if (event->is<sf::Event::Closed>()) { window.close(); }
			if (const auto* key = event->getIf<sf::Event::KeyPressed>()) { game.key(key->code, true); }
			if (const auto* key = event->getIf<sf::Event::KeyReleased>()) { game.key(key->code, false); }
		}

		// the time since the last frame, at most a quarter of a second
		game.frame(std::min(clock.restart().asSeconds(), 0.25f));

		window.clear(sf::Color::Black);
		box(window, game.ball.position, game.ball.size);
		box(window, game.paddle1.position, game.paddle1.size);
		box(window, game.paddle2.position, game.paddle2.size);
		window.display();
	}
}
