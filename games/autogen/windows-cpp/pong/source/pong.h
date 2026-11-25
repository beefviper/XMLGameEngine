// pong.h
// Pong
// author: beefviper
// date: Nov 24, 2026
//
// Pong: its numbers, its things and its rules. main.cpp draws it and plays its sounds.

#pragma once

#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <random>
#include <string>
#include <vector>

namespace pong
{
	constexpr float windowWidth = 1280;
	constexpr float windowHeight = 720;

	constexpr float margin = 30;
	constexpr float width = 20;
	constexpr float height = 120;
	constexpr float step = 300;
	constexpr float ballSize = 16;
	constexpr float ballSpeed = 540;

	enum class Screen { Title, Playing, Paused, Over };
	enum class Sound { Paddle, Wall, Score };

	// Something on the screen: where it is, how it moves, how big it is.
	struct Thing
	{
		sf::Vector2f position;
		sf::Vector2f velocity;
		sf::Vector2f size;
	};

	struct Computer
	{
		enum class Look { Easy, Smart };
		Look look{ Look::Easy };
		int level{ 1 };
	};

	struct Paddle : Thing
	{
		int score{ 0 };
	};

	struct Ball : Thing
	{
		int hits{ 0 };
	};

	// A game pad's buttons, named for an Xbox pad's.
	enum class PadButton { A, B, X, Y, LB, RB, Back, Start, Up, Down, Left, Right };

	class Pong
	{
	public:
		explicit Pong(unsigned seed = std::random_device{}());

		// What is kept from one game to the next: read from this file, and
		// written to it by keep().
		void keepIn(const std::string& file);
		void keep() const;

		void key(sf::Keyboard::Key key, bool down);
		void pad(unsigned pad, PadButton button, bool down);
		void frame(float seconds);

		Screen screen() const { return screens.back(); }
		// The sounds asked for since the last time.
		std::vector<Sound> takeSounds();

		int people{ 1 };
		Computer computer;
		Paddle paddle1;
		Paddle paddle2;
		Ball ball;

	private:
		std::vector<Screen> screens{ Screen::Title };
		std::vector<Sound> sounds;
		std::string keptFile;

		std::mt19937 random;
		float between(float min, float max);

		bool wHeld{ false };
		bool sHeld{ false };
		bool upHeld{ false };
		bool downHeld{ false };
		bool pad1UpHeld{ false };
		bool pad1DownHeld{ false };
		bool pad2UpHeld{ false };
		bool pad2DownHeld{ false };

		void newGame();
		void startPaddle1();
		void startPaddle2();
		void startBall();
		void computerHolds(bool& seat2Up, bool& seat2Down) const;
	};
}
