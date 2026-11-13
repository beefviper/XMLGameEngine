// pong_min.cpp
// Pong
// author: beefviper
// date: Nov 13, 2026

#include "pong_min.h"

#include <algorithm>
#include <cmath>

namespace pong_min
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
		startBall();
		paddle1 = { { margin, windowHeight / 2 - height / 2 }, {}, { width, height } };
		paddle2 = { { windowWidth - margin - width, windowHeight / 2 - height / 2 }, {}, { width, height } };
	}

	float Pong::between(float min, float max)
	{
		return std::uniform_real_distribution<float>(min, max)(random);
	}

	// Where the ball starts, and starts again.
	void Pong::startBall()
	{
		const float speed = 6;
		const float angle = between(-30, 30);
		const float side = between(-1, 1);
		ball.size = { size, size };
		ball.position = { windowWidth / 2 - size / 2, windowHeight / 2 - size / 2 };
		ball.velocity = { sign(side) * speed * cosine(angle), speed * sine(angle) };
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
	}

	void Pong::frame()
	{
		// the keys held
		paddle1.velocity = {};
		if (wHeld) { paddle1.velocity.y -= step; }
		if (sHeld) { paddle1.velocity.y += step; }
		paddle2.velocity = {};
		if (upHeld) { paddle2.velocity.y -= step; }
		if (downHeld) { paddle2.velocity.y += step; }

		// everything moves
		ball.position += ball.velocity;
		paddle1.position += paddle1.velocity;
		paddle2.position += paddle2.velocity;

		// at the window's edges
		if (ball.position.y < 0) { ball.velocity.y = std::abs(ball.velocity.y); }
		if (ball.position.y > windowHeight - ball.size.y) { ball.velocity.y = -std::abs(ball.velocity.y); }
		if (ball.position.x < 0 || ball.position.x > windowWidth - ball.size.x) { startBall(); }
		for (Thing* paddle : { &paddle1, &paddle2 })
		{
			if (paddle->position.y < 0) { paddle->position.y = 0; }
			if (paddle->position.y > windowHeight - paddle->size.y) { paddle->position.y = windowHeight - paddle->size.y; }
		}

		// what meets what
		for (const Thing* other : { &paddle1, &paddle2 })
		{
			if (touching(ball, *other)) { deflect(ball, *other, 45); }
		}
	}
}
