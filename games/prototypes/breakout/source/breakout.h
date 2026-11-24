// breakout.h
// Breakout
// author: beefviper
// date: Nov 24, 2026
//
// Breakout: its numbers, its things and its rules. main.cpp draws it and plays its sounds.

#pragma once

#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <random>
#include <vector>

namespace breakout
{
	constexpr float windowWidth = 640;
	constexpr float windowHeight = 720;

	constexpr float brick = 38;
	constexpr float gap = 4;
	constexpr float line = 18;
	constexpr float top = 110;
	constexpr float paddle = 64;
	constexpr float step = 480;

	enum class Screen { Title, Playing, Paused, Over };
	enum class Sound { Wall, Paddle, Red, Orange, Green, Yellow, Lost };

	// Something on the screen: where it is, how it moves, how big it is.
	struct Thing
	{
		sf::Vector2f position;
		sf::Vector2f velocity;
		sf::Vector2f size;
	};

	struct Status
	{
		int score{ 0 };
		int balls{ 3 };
		int walls{ 0 };
	};

	struct Brick : Thing
	{
		enum class Look { Red, Orange, Green, Yellow };
		Look look{ Look::Red };
		int worth{ 0 };
		Sound sound{ Sound::Red };
	};

	struct Player : Thing
	{
		enum class Look { Full, Half };
		Look look{ Look::Full };
	};

	struct Ball : Thing
	{
		int hits{ 0 };
		int deep{ 0 };
	};

	struct Message
	{
		enum class Look { Won, Lost };
		Look look{ Look::Won };
	};

	// A game pad's buttons, named for an Xbox pad's.
	enum class PadButton { A, B, X, Y, LB, RB, Back, Start, Up, Down, Left, Right };

	class Breakout
	{
	public:
		explicit Breakout(unsigned seed = std::random_device{}());

		void key(sf::Keyboard::Key key, bool down);
		void pad(unsigned pad, PadButton button, bool down);
		void mouse(float x);
		void frame(float seconds);

		Screen screen() const { return screens.back(); }
		// The sounds asked for since the last time.
		std::vector<Sound> takeSounds();

		Status status;
		std::vector<Brick> wall;
		Player player;
		Ball ball;
		Message message;

	private:
		std::vector<Screen> screens{ Screen::Title };
		std::vector<Sound> sounds;

		std::mt19937 random;
		float between(float min, float max);

		bool aHeld{ false };
		bool leftHeld{ false };
		bool dHeld{ false };
		bool rightHeld{ false };
		bool pad1LeftHeld{ false };
		bool pad1RightHeld{ false };

		void newGame();
		void startWall();
		void addBricks(Brick::Look look, float y, int worth, Sound sound);
		void startPlayer();
		void startBall();
		void faster();
	};
}
