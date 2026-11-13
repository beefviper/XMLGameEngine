// pong.h
// Pong
// author: beefviper
// date: Nov 13, 2026
//
// The game itself: two paddles, a ball, the scores, who plays the right
// paddle and the four screens. main.cpp draws it and plays its sounds.

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
	constexpr float paddleWidth = 20;
	constexpr float paddleHeight = 120;
	constexpr float paddleSpeed = 5;
	constexpr float ballSize = 16;
	constexpr float ballSpeed = 9;
	constexpr int winningScore = 11;

	enum class Screen { Title, Playing, Paused, Over };

	struct Paddle
	{
		sf::Vector2f position;
		float speed{ 0 };
		int score{ 0 };
	};

	struct Ball
	{
		sf::Vector2f position;
		sf::Vector2f velocity;
		int hits{ 0 };
	};

	class Pong
	{
	public:
		explicit Pong(unsigned seed = std::random_device{}());

		// Who plays the right paddle and how well the computer plays, read
		// from the file and written back to it at every new game.
		void keepChoicesIn(const std::string& file);

		void key(sf::Keyboard::Key key, bool down);
		void frame();

		// The sounds to play since the last time: "paddle", "wall", "score".
		std::vector<std::string> takeSounds();

		Screen screen() const { return screens.back(); }
		bool twoPlayers() const { return players == 2; }
		bool computerIsSmart() const { return smart; }

		Paddle left;
		Paddle right;
		Ball ball;

	private:
		// The screen on top is played; paused and game over sit on top of
		// the game, which shows still beneath them.
		std::vector<Screen> screens{ Screen::Title };

		int players{ 1 };
		bool smart{ false };
		std::string choicesFile;

		bool upHeld{ false };
		bool downHeld{ false };
		bool wHeld{ false };
		bool sHeld{ false };

		std::mt19937 random;
		std::vector<std::string> sounds;

		void serve();
		void newGame();
		void saveChoices() const;

		float computerSpeed() const;
		void movePaddle(Paddle& paddle);
		void moveBall();
		void hitPaddle(const Paddle& paddle, float away);
		void point(Paddle& winner);
	};
}
