// breakout.cpp
// Breakout
// author: beefviper
// date: Nov 24, 2026

#include "breakout.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace breakout
{
	namespace
	{
		constexpr float pi = 3.14159265358979f;

		float sine(float degrees) { return std::sin(degrees * pi / 180); }
		float cosine(float degrees) { return std::cos(degrees * pi / 180); }

		bool touching(const Thing& a, const Thing& b)
		{
			return a.position.x < b.position.x + b.size.x && b.position.x < a.position.x + a.size.x
				&& a.position.y < b.position.y + b.size.y && b.position.y < a.position.y + a.size.y;
		}

		// Off the side of `other` it hit: its speed into that side turned round.
		void bounce(Thing& thing, const Thing& other)
		{
			const sf::Vector2f apart = (thing.position + thing.size / 2.0f) - (other.position + other.size / 2.0f);
			const sf::Vector2f reach = (thing.size + other.size) / 2.0f;
			if (reach.x - std::abs(apart.x) < reach.y - std::abs(apart.y))
			{
				thing.velocity.x = apart.x < 0 ? -std::abs(thing.velocity.x) : std::abs(thing.velocity.x);
			}
			else
			{
				thing.velocity.y = apart.y < 0 ? -std::abs(thing.velocity.y) : std::abs(thing.velocity.y);
			}
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

	Breakout::Breakout(unsigned seed) : random(seed)
	{
		newGame();
	}

	float Breakout::between(float min, float max)
	{
		return std::uniform_real_distribution<float>(min, max)(random);
	}

	// A new game: everything as it starts but what is kept, from the first screen.
	void Breakout::newGame()
	{
		status = {};
		startWall();
		startPlayer();
		startBall();
		message = {};
		screens = { Screen::Title };
	}

	// Where the wall starts, and starts again.
	void Breakout::startWall()
	{
		wall.clear();
		addBricks(Brick::Look::Red, top, 7, Sound::Red);
		addBricks(Brick::Look::Orange, top + 2 * line, 5, Sound::Orange);
		addBricks(Brick::Look::Green, top + 4 * line, 3, Sound::Green);
		addBricks(Brick::Look::Yellow, top + 6 * line, 1, Sound::Yellow);
	}

	// 2 rows of 14 bricks of one look, the first row's top at y.
	void Breakout::addBricks(Brick::Look look, float y, int worth, Sound sound)
	{
		const sf::Vector2f size{ brick, line - gap };
		for (int row = 0; row < 2; ++row)
		{
			for (int column = 0; column < 14; ++column)
			{
				const sf::Vector2f position{ (windowWidth - 14 * brick - 13 * gap) / 2 + static_cast<float>(column) * (size.x + gap), y + static_cast<float>(row) * (size.y + gap) };
				wall.push_back({ { position, {}, size }, look, worth, sound });
			}
		}
	}

	void Breakout::startPlayer()
	{
		player.size = { paddle, 12 };
		player.position = { windowWidth / 2 - paddle / 2, windowHeight - 60 };
		player.velocity = {};
		player.look = Player::Look::Full;
	}

	// Where the ball starts, and starts again.
	void Breakout::startBall()
	{
		ball.size = { 10, 10 };
		ball.position = { windowWidth / 2 - 5, windowHeight / 2 - 5 };
		ball.velocity = { between(-180, 180), 240 };
		ball.hits = 0;
		ball.deep = 0;
	}

	std::vector<Sound> Breakout::takeSounds()
	{
		return std::exchange(sounds, {});
	}

	void Breakout::key(sf::Keyboard::Key key, bool down)
	{
		using Key = sf::Keyboard::Key;
		switch (key)
		{
		case Key::A: aHeld = down; break;
		case Key::Left: leftHeld = down; break;
		case Key::D: dHeld = down; break;
		case Key::Right: rightHeld = down; break;
		default: break;
		}
		if (!down) { return; }

		switch (screen())
		{
		case Screen::Title:
			if (key == Key::Num1 || key == Key::Numpad1) { screens.push_back(Screen::Playing); }
			break;
		case Screen::Playing:
			if (key == Key::Pause) { screens.push_back(Screen::Paused); }
			break;
		case Screen::Paused:
			if (key == Key::Pause) { screens.pop_back(); }
			break;
		case Screen::Over:
			if (key == Key::Space) { newGame(); }
			break;
		}
	}

	void Breakout::pad(unsigned pad, PadButton button, bool down)
	{
		if (pad == 1 && button == PadButton::Left) { pad1LeftHeld = down; }
		if (pad == 1 && button == PadButton::Right) { pad1RightHeld = down; }
		if (!down) { return; }

		switch (screen())
		{
		case Screen::Title:
			if (pad == 1 && button == PadButton::Start) { screens.push_back(Screen::Playing); }
			break;
		case Screen::Playing:
			if (pad == 1 && button == PadButton::Start) { screens.push_back(Screen::Paused); }
			break;
		case Screen::Paused:
			if (pad == 1 && button == PadButton::Start) { screens.pop_back(); }
			break;
		case Screen::Over:
			if (pad == 1 && button == PadButton::Start) { newGame(); }
			break;
		}
	}

	// The mouse across the window: the player's middle follows it, kept inside the window.
	void Breakout::mouse(float x)
	{
		if (screen() != Screen::Playing) { return; }
		player.position.x = std::clamp(x - player.size.x / 2, 0.0f, windowWidth - player.size.x);
	}

	void Breakout::frame(float seconds)
	{
		if (screen() != Screen::Playing) { return; }

		// the keys held
		player.velocity = {};
		if (aHeld || leftHeld || pad1LeftHeld) { player.velocity.x -= step; }
		if (dHeld || rightHeld || pad1RightHeld) { player.velocity.x += step; }

		// everything moves
		player.position += player.velocity * seconds;
		ball.position += ball.velocity * seconds;

		// at the window's edges
		if (player.position.x < 0) { player.position.x = 0; }
		if (player.position.x > windowWidth - player.size.x) { player.position.x = windowWidth - player.size.x; }
		if (ball.position.x < 0)
		{
			ball.velocity.x = std::abs(ball.velocity.x);
			sounds.push_back(Sound::Wall);
		}
		if (ball.position.x > windowWidth - ball.size.x)
		{
			ball.velocity.x = -std::abs(ball.velocity.x);
			sounds.push_back(Sound::Wall);
		}
		if (ball.position.y < 0)
		{
			ball.velocity.y = std::abs(ball.velocity.y);
			sounds.push_back(Sound::Wall);
			player.look = Player::Look::Half;
			player.size.x = paddle / 2;
		}
		if (ball.position.y > windowHeight - ball.size.y)
		{
			status.balls -= 1;
			sounds.push_back(Sound::Lost);
			startBall();
		}

		// what meets what
		if (touching(ball, player))
		{
			deflect(ball, player, 60);
			ball.hits += 1;
			sounds.push_back(Sound::Paddle);
			if (ball.hits == 4 || ball.hits == 12) { faster(); }
		}
		for (auto cell = wall.begin(); cell != wall.end();)
		{
			if (touching(ball, *cell))
			{
				bounce(ball, *cell);
				if (ball.position.y < top + 4 * line && ball.deep == 0)
				{
					ball.deep += 1;
					faster();
				}
				status.score += cell->worth;
				sounds.push_back(cell->sound);
				cell = wall.erase(cell);
			}
			else { ++cell; }
		}

		// the game's own tests, the first that holds
		if (wall.empty() && status.walls >= 1)
		{
			message.look = Message::Look::Won;
			screens.push_back(Screen::Over);
		}
		else if (wall.empty())
		{
			status.walls += 1;
			startWall();
			startBall();
		}
		else if (status.balls <= 0)
		{
			message.look = Message::Look::Lost;
			screens.push_back(Screen::Over);
		}
	}

	void Breakout::faster()
	{
		ball.velocity.x *= 1.25f;
		ball.velocity.y *= 1.25f;
	}
}
