// pong.cpp
// Pong
// author: beefviper
// date: Nov 13, 2026

#include "pong.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <utility>

namespace pong
{
	namespace
	{
		constexpr float pi = 3.14159265358979f;

		float radians(float degrees) { return degrees * pi / 180; }

		// Up held, down held: which way a paddle goes, at its speed.
		float upOrDown(bool up, bool down)
		{
			if (up == down) { return 0; }
			return up ? -paddleSpeed : paddleSpeed;
		}

		bool overlaps(sf::Vector2f a, sf::Vector2f aSize, sf::Vector2f b, sf::Vector2f bSize)
		{
			return a.x < b.x + bSize.x && b.x < a.x + aSize.x && a.y < b.y + bSize.y && b.y < a.y + aSize.y;
		}
	}

	Pong::Pong(unsigned seed) : random(seed)
	{
		newGame();
	}

	// The ball from the middle at its first speed, within 30 degrees of
	// straight left or right, to either side.
	void Pong::serve()
	{
		const float angle = std::uniform_real_distribution<float>(-30, 30)(random);
		const float side = std::uniform_real_distribution<float>(-1, 1)(random) < 0 ? -1.0f : 1.0f;
		ball.position = { windowWidth / 2 - ballSize / 2, windowHeight / 2 - ballSize / 2 };
		ball.velocity = { side * ballSpeed * std::cos(radians(angle)), ballSpeed * std::sin(radians(angle)) };
		ball.hits = 0;
	}

	void Pong::newGame()
	{
		left = { { margin, windowHeight / 2 - paddleHeight / 2 } };
		right = { { windowWidth - margin - paddleWidth, windowHeight / 2 - paddleHeight / 2 } };
		serve();
		screens = { Screen::Title };
	}

	// Two numbers, one a line: players (1 or 2), then 1 for easy or 2 for
	// smart.
	void Pong::keepChoicesIn(const std::string& file)
	{
		choicesFile = file;
		std::ifstream in(file);
		int keptPlayers = 0;
		int keptLevel = 0;
		if (in >> keptPlayers >> keptLevel && (keptPlayers == 1 || keptPlayers == 2) && (keptLevel == 1 || keptLevel == 2))
		{
			players = keptPlayers;
			smart = keptLevel == 2;
		}
	}

	void Pong::saveChoices() const
	{
		if (choicesFile.empty()) { return; }
		std::ofstream out(choicesFile);
		out << players << '\n' << (smart ? 2 : 1) << '\n';
	}

	void Pong::key(sf::Keyboard::Key key, bool down)
	{
		using Key = sf::Keyboard::Key;

		// The paddle keys move a paddle while they are down, whatever the
		// screen, so one still held after a pause goes on moving.
		switch (key)
		{
		case Key::W: wHeld = down; break;
		case Key::S: sHeld = down; break;
		case Key::Up: upHeld = down; break;
		case Key::Down: downHeld = down; break;
		default: break;
		}
		if (!down) { return; }

		switch (screen())
		{
		case Screen::Title:
			if (key == Key::Space) { players = 1; screens.push_back(Screen::Playing); }
			if (key == Key::Num2 || key == Key::Numpad2) { players = 2; screens.push_back(Screen::Playing); }
			if (key == Key::A) { smart = false; }
			if (key == Key::D) { smart = true; }
			break;
		case Screen::Playing:
			if (key == Key::Pause) { screens.push_back(Screen::Paused); }
			break;
		case Screen::Paused:
			if (key == Key::Pause) { screens.pop_back(); }
			break;
		case Screen::Over:
			if (key == Key::Space) { saveChoices(); newGame(); }
			break;
		}
	}

	void Pong::frame()
	{
		if (screen() != Screen::Playing) { return; }

		left.speed = upOrDown(wHeld, sHeld);
		right.speed = twoPlayers() ? upOrDown(upHeld, downHeld) : computerSpeed();
		movePaddle(left);
		movePaddle(right);
		moveBall();

		if (left.score >= winningScore || right.score >= winningScore) { screens.push_back(Screen::Over); }
	}

	// The computer keeps the right paddle's middle within 8 of where it
	// wants it. Easy wants it level with the ball, always; smart only once
	// the ball comes over the net towards it, and in the middle until then.
	float Pong::computerSpeed() const
	{
		const bool coming = ball.velocity.x > 0 && ball.position.x > windowWidth / 2;
		const float wanted = !smart || coming ? ball.position.y + ballSize / 2 : windowHeight / 2;
		const float aim = wanted - paddleHeight / 2;
		if (right.position.y < aim - 8) { return paddleSpeed; }
		if (right.position.y > aim + 8) { return -paddleSpeed; }
		return 0;
	}

	// A paddle stops at the top and the bottom.
	void Pong::movePaddle(Paddle& paddle)
	{
		paddle.position.y = std::clamp(paddle.position.y + paddle.speed, 0.0f, windowHeight - paddleHeight);
	}

	void Pong::moveBall()
	{
		ball.position += ball.velocity;

		// off the top and the bottom
		if (ball.position.y < 0 && ball.velocity.y < 0)
		{
			ball.velocity.y = -ball.velocity.y;
			sounds.push_back("wall");
		}
		if (ball.position.y > windowHeight - ballSize && ball.velocity.y > 0)
		{
			ball.velocity.y = -ball.velocity.y;
			sounds.push_back("wall");
		}

		// off a paddle
		const sf::Vector2f paddleSize{ paddleWidth, paddleHeight };
		const sf::Vector2f size{ ballSize, ballSize };
		if (ball.velocity.x < 0 && overlaps(ball.position, size, left.position, paddleSize))
		{
			ball.position.x = left.position.x + paddleWidth;
			hitPaddle(left, 1);
		}
		if (ball.velocity.x > 0 && overlaps(ball.position, size, right.position, paddleSize))
		{
			ball.position.x = right.position.x - ballSize;
			hitPaddle(right, -1);
		}

		// past a paddle: a point to the other side
		if (ball.position.x < 0) { point(right); }
		if (ball.position.x > windowWidth - ballSize) { point(left); }
	}

	// Back at the same speed, up to 45 degrees off straight, steeper the
	// further from the paddle's middle it hits: off the top half it goes up,
	// off the bottom half down. The fourth hit and the twelfth make it
	// faster, as the arcade's did.
	void Pong::hitPaddle(const Paddle& paddle, float away)
	{
		const float offset = (ball.position.y + ballSize / 2) - (paddle.position.y + paddleHeight / 2);
		const float angle = 45 * std::clamp(offset / (paddleHeight / 2), -1.0f, 1.0f);
		const float speed = std::hypot(ball.velocity.x, ball.velocity.y);
		ball.velocity = { away * speed * std::cos(radians(angle)), speed * std::sin(radians(angle)) };

		ball.hits += 1;
		if (ball.hits == 4 || ball.hits == 12) { ball.velocity *= 1.3f; }
		sounds.push_back("paddle");
	}

	void Pong::point(Paddle& winner)
	{
		winner.score += 1;
		sounds.push_back("score");
		serve();
	}

	std::vector<std::string> Pong::takeSounds()
	{
		return std::exchange(sounds, {});
	}
}
