// pong_min.h
// XML Game Engine
// author: beefviper
// date: Nov 13, 2026
//
// What pong_min.xml written out must do, whoever wrote it: the hand-written
// one in games/prototypes/pong_min (test_prototype_pong.cpp) and the one
// windows-cpp writes (test_written_games.cpp). The written game shares no
// code with the engine, so this checks what happens, not every frame.

#pragma once

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <SFML/Window/Keyboard.hpp>

#include <cmath>

template <typename Pong>
void pong_min_plays()
{
	using Key = sf::Keyboard::Key;
	using Catch::Matchers::WithinAbs;
	constexpr float width = 1280;
	constexpr float height = 720;
	constexpr float speed = 6;

	const auto served = [&](const auto& ball)
	{
		return ball.position.x == width / 2 - 8 && ball.position.y == height / 2 - 8
			&& std::abs(std::hypot(ball.velocity.x, ball.velocity.y) - speed) < 0.001f
			&& std::abs(ball.velocity.y) <= speed / 2 + 0.001f;
	};

	// served from the middle, within 30 degrees of straight left or right
	for (unsigned seed = 0; seed < 30; ++seed)
	{
		const Pong game(seed);
		CHECK(served(game.ball));
	}

	// off the top and the bottom
	{
		Pong game(1);
		game.ball.position = { 600, 3 };
		game.ball.velocity = { 3, -5 };
		game.frame();
		CHECK(game.ball.velocity.y == 5);
		game.ball.position = { 600, height - 16 - 3 };
		game.frame();
		CHECK(game.ball.velocity.y == -5);
	}

	// past either side, served again
	{
		Pong game(1);
		game.ball.position = { 3, 100 };
		game.ball.velocity = { -6, 0 };
		game.frame();
		CHECK(served(game.ball));
		game.ball.position = { width - 16 - 3, 100 };
		game.ball.velocity = { 6, 0 };
		game.frame();
		CHECK(served(game.ball));
	}

	// W and S the left paddle, Up and Down the right, five a frame, held;
	// they stop at the top and the bottom
	{
		Pong game(1);
		game.ball.velocity = {};
		const float middle = game.paddle1.position.y;
		game.key(Key::W, true);
		game.frame();
		CHECK(game.paddle1.position.y == middle - 5);
		game.key(Key::W, false);
		game.frame();
		CHECK(game.paddle1.position.y == middle - 5);
		game.key(Key::W, true);
		for (int i = 0; i < 100; ++i) { game.frame(); }
		CHECK(game.paddle1.position.y == 0);
		game.key(Key::W, false);
		game.key(Key::Down, true);
		for (int i = 0; i < 100; ++i) { game.frame(); }
		CHECK(game.paddle2.position.y == height - 120);
	}

	// a paddle sends it back, up off its top half, at the same speed
	{
		Pong game(1);
		const auto& paddle = game.paddle1;
		game.ball.position = { paddle.position.x + 20 + 2, paddle.position.y + 10 };
		game.ball.velocity = { -6, 0 };
		game.frame();
		CHECK(game.ball.velocity.x > 0);
		CHECK(game.ball.velocity.y < 0);
		CHECK_THAT(std::hypot(game.ball.velocity.x, game.ball.velocity.y), WithinAbs(speed, 0.001));
		CHECK(game.ball.position.x >= paddle.position.x + 20);
	}

	// played a while: the ball stays about the window, is returned, and is
	// served again
	{
		Pong game(7);
		const Key keys[] = { Key::W, Key::Down, Key::S, Key::Up };
		int returns = 0;
		int serves = 0;
		for (int frame = 0; frame < 3600; ++frame)
		{
			const Key key = keys[(frame / 30) % 4];
			if (frame % 30 == 0) { game.key(key, true); }
			if (frame % 30 == 20) { game.key(key, false); }
			const float before = game.ball.velocity.x;
			game.frame();
			if (served(game.ball)) { ++serves; }
			else if ((before > 0) != (game.ball.velocity.x > 0)) { ++returns; }
			REQUIRE(game.ball.position.x > -speed * 2);
			REQUIRE(game.ball.position.x < width + speed * 2);
			REQUIRE(game.ball.position.y > -speed * 2);
			REQUIRE(game.ball.position.y < height + speed * 2);
		}
		CHECK(returns > 0);
		CHECK(serves > 0);
	}
}
