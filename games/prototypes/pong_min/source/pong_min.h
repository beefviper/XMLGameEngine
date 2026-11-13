// pong_min.h
// Pong
// author: beefviper
// date: Nov 13, 2026
//
// Pong: its numbers, its things and its rules. main.cpp draws it.

#pragma once

#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <random>

namespace pong_min
{
	constexpr float windowWidth = 1280;
	constexpr float windowHeight = 720;

	constexpr float margin = 30;
	constexpr float size = 16;
	constexpr float width = 20;
	constexpr float height = 120;
	constexpr float step = 5;

	// Something on the screen: where it is, how it moves, how big it is.
	struct Thing
	{
		sf::Vector2f position;
		sf::Vector2f velocity;
		sf::Vector2f size;
	};

	class Pong
	{
	public:
		explicit Pong(unsigned seed = std::random_device{}());

		void key(sf::Keyboard::Key key, bool down);
		void frame();

		Thing ball;
		Thing paddle1;
		Thing paddle2;

	private:
		std::mt19937 random;
		float between(float min, float max);

		bool wHeld{ false };
		bool sHeld{ false };
		bool upHeld{ false };
		bool downHeld{ false };

		void startBall();
	};
}
