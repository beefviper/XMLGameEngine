// pong.cpp
// Pong
// author: beefviper
// date: Nov 24, 2026

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

		float sine(float degrees) { return std::sin(degrees * pi / 180); }
		float cosine(float degrees) { return std::cos(degrees * pi / 180); }

		float sign(float number) { return number > 0 ? 1.0f : number < 0 ? -1.0f : 0.0f; }

		bool touching(const Thing& a, const Thing& b)
		{
			return a.position.x < b.position.x + b.size.x && b.position.x < a.position.x + a.size.x
				&& a.position.y < b.position.y + b.size.y && b.position.y < a.position.y + a.size.y;
		}

		// Off the side of `other` it hit, at the same speed, up to `degrees` off
		// straight out: steeper the further from that side's middle, towards
		// the end it hit nearer to.
		void deflect(Thing& thing, const Thing& other, float degrees)
		{
			const sf::Vector2f middle = thing.position + thing.size / 2.0f;
			const sf::Vector2f otherMiddle = other.position + other.size / 2.0f;
			const sf::Vector2f apart = middle - otherMiddle;
			const sf::Vector2f reach = (thing.size + other.size) / 2.0f;
			const float speed = std::hypot(thing.velocity.x, thing.velocity.y);

			if (reach.x - std::abs(apart.x) < reach.y - std::abs(apart.y))
			{
				// its left or right side
				const float away = apart.x < 0 ? -1.0f : 1.0f;
				const float angle = degrees * std::clamp(apart.y / (other.size.y / 2), -1.0f, 1.0f);
				thing.velocity = { away * speed * cosine(angle), speed * sine(angle) };
				thing.position.x = otherMiddle.x + away * reach.x - thing.size.x / 2;
			}
			else
			{
				// its top or bottom
				const float away = apart.y < 0 ? -1.0f : 1.0f;
				const float angle = degrees * std::clamp(apart.x / (other.size.x / 2), -1.0f, 1.0f);
				thing.velocity = { speed * sine(angle), away * speed * cosine(angle) };
				thing.position.y = otherMiddle.y + away * reach.y - thing.size.y / 2;
			}
		}
	}

	Pong::Pong(unsigned seed) : random(seed)
	{
		newGame();
	}

	float Pong::between(float min, float max)
	{
		return std::uniform_real_distribution<float>(min, max)(random);
	}

	// A new game: everything as it starts but what is kept, from the first screen.
	void Pong::newGame()
	{
		startPaddle1();
		startPaddle2();
		startBall();
		screens = { Screen::Title };
	}

	void Pong::startPaddle1()
	{
		paddle1.size = { width, height };
		paddle1.position = { margin, windowHeight / 2 - height / 2 };
		paddle1.velocity = {};
		paddle1.score = 0;
	}

	void Pong::startPaddle2()
	{
		paddle2.size = { width, height };
		paddle2.position = { windowWidth - margin - width, windowHeight / 2 - height / 2 };
		paddle2.velocity = {};
		paddle2.score = 0;
	}

	// Where the ball starts, and starts again.
	void Pong::startBall()
	{
		const float angle = between(-30, 30);
		const float side = between(-1, 1);
		ball.size = { ballSize, ballSize };
		ball.position = { windowWidth / 2 - ballSize / 2, windowHeight / 2 - ballSize / 2 };
		ball.velocity = { sign(side) * ballSpeed * cosine(angle), ballSpeed * sine(angle) };
		ball.hits = 0;
	}

	// One line each: the computer's look, then its level.
	void Pong::keepIn(const std::string& file)
	{
		keptFile = file;
		std::ifstream in(file);
		int computerLook = 0;
		int computerLevel = 0;
		if (in >> computerLook >> computerLevel && computerLook >= 0 && computerLook < 2)
		{
			computer.look = static_cast<Computer::Look>(computerLook);
			computer.level = computerLevel;
		}
	}

	void Pong::keep() const
	{
		if (keptFile.empty()) { return; }
		std::ofstream out(keptFile);
		out << static_cast<int>(computer.look) << '\n' << computer.level << '\n';
	}

	std::vector<Sound> Pong::takeSounds()
	{
		return std::exchange(sounds, {});
	}

	void Pong::key(sf::Keyboard::Key key, bool down)
	{
		using Key = sf::Keyboard::Key;
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
			if (key == Key::Num1 || key == Key::Numpad1) { people = 1; screens.push_back(Screen::Playing); }
			if (key == Key::Num2 || key == Key::Numpad2) { people = 2; screens.push_back(Screen::Playing); }
			if (key == Key::A) { computer.level = 1; computer.look = Computer::Look::Easy; }
			if (key == Key::D) { computer.level = 2; computer.look = Computer::Look::Smart; }
			break;
		case Screen::Playing:
			if (key == Key::Pause) { screens.push_back(Screen::Paused); }
			break;
		case Screen::Paused:
			if (key == Key::Pause) { screens.pop_back(); }
			break;
		case Screen::Over:
			if (key == Key::Space) { keep(); newGame(); }
			break;
		}
	}

	void Pong::pad(unsigned pad, PadButton button, bool down)
	{
		if (pad == 1 && button == PadButton::Up) { pad1UpHeld = down; }
		if (pad == 1 && button == PadButton::Down) { pad1DownHeld = down; }
		if (pad == 2 && button == PadButton::Up) { pad2UpHeld = down; }
		if (pad == 2 && button == PadButton::Down) { pad2DownHeld = down; }
		if (!down) { return; }

		switch (screen())
		{
		case Screen::Playing:
			if (pad == 1 && button == PadButton::Start) { screens.push_back(Screen::Paused); }
			break;
		case Screen::Paused:
			if (pad == 1 && button == PadButton::Start) { screens.pop_back(); }
			break;
		default: break;
		}
	}

	void Pong::frame(float seconds)
	{
		if (screen() != Screen::Playing) { return; }

		// the keys held, seat 2's by the computer while nobody has it
		bool seat2Up = upHeld || pad2UpHeld;
		bool seat2Down = downHeld || pad2DownHeld;
		if (people < 2) { computerHolds(seat2Up, seat2Down); }
		paddle1.velocity = {};
		if (wHeld || pad1UpHeld) { paddle1.velocity.y -= step; }
		if (sHeld || pad1DownHeld) { paddle1.velocity.y += step; }
		paddle2.velocity = {};
		if (seat2Up) { paddle2.velocity.y -= step; }
		if (seat2Down) { paddle2.velocity.y += step; }

		// everything moves
		paddle1.position += paddle1.velocity * seconds;
		paddle2.position += paddle2.velocity * seconds;
		ball.position += ball.velocity * seconds;

		// at the window's edges
		for (Paddle* paddle : { &paddle1, &paddle2 })
		{
			if (paddle->position.y < 0) { paddle->position.y = 0; }
			if (paddle->position.y > windowHeight - paddle->size.y) { paddle->position.y = windowHeight - paddle->size.y; }
		}
		if (ball.position.y < 0)
		{
			ball.velocity.y = std::abs(ball.velocity.y);
			sounds.push_back(Sound::Wall);
		}
		if (ball.position.y > windowHeight - ball.size.y)
		{
			ball.velocity.y = -std::abs(ball.velocity.y);
			sounds.push_back(Sound::Wall);
		}
		if (ball.position.x < 0)
		{
			paddle2.score += 1;
			sounds.push_back(Sound::Score);
			startBall();
		}
		if (ball.position.x > windowWidth - ball.size.x)
		{
			paddle1.score += 1;
			sounds.push_back(Sound::Score);
			startBall();
		}

		// what meets what
		for (const Paddle* paddle : { &paddle1, &paddle2 })
		{
			if (touching(ball, *paddle))
			{
				deflect(ball, *paddle, 45);
				ball.hits += 1;
				sounds.push_back(Sound::Paddle);
				if (ball.hits == 4 || ball.hits == 12)
				{
					ball.velocity.x *= 1.3f;
					ball.velocity.y *= 1.3f;
				}
			}
		}

		// the game's own tests, the first that holds
		if (paddle1.score >= 11 || paddle2.score >= 11) { screens.push_back(Screen::Over); }
	}

	// The computer in seat 2: the keys it holds.
	void Pong::computerHolds(bool& seat2Up, bool& seat2Down) const
	{
		seat2Up = false;
		seat2Down = false;
		if (computer.level == 1)
		{
			if (paddle2.position.y < ball.position.y + ballSize / 2 - height / 2 - 8) { seat2Down = true; }
			if (paddle2.position.y > ball.position.y + ballSize / 2 - height / 2 + 8) { seat2Up = true; }
		}
		if (computer.level == 2)
		{
			if (ball.velocity.x > 0 && ball.position.x > windowWidth / 2)
			{
				if (paddle2.position.y < ball.position.y + ballSize / 2 - height / 2 - 8) { seat2Down = true; }
				if (paddle2.position.y > ball.position.y + ballSize / 2 - height / 2 + 8) { seat2Up = true; }
			}
			if (ball.velocity.x <= 0 || ball.position.x <= windowWidth / 2)
			{
				if (paddle2.position.y < windowHeight / 2 - height / 2 - 8) { seat2Down = true; }
				if (paddle2.position.y > windowHeight / 2 - height / 2 + 8) { seat2Up = true; }
			}
		}
	}
}
