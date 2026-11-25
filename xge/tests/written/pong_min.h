// pong_min.h
// XML Game Engine
// author: beefviper
// date: Nov 13, 2026
//
// What pong_min.xml written out must do, whoever wrote it, run on the one
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
	constexpr float speed = 360; // pixels a second
	constexpr float sixtieth = 1.0f / 60; // a step at 60 frames a second

	const auto served = [&](const auto& ball)
	{
		return ball.position.x == width / 2 - 8 && ball.position.y == height / 2 - 8
			&& std::abs(std::hypot(ball.velocity.x, ball.velocity.y) - speed) < 0.01f
			&& std::abs(ball.velocity.y) <= speed / 2 + 0.01f;
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
		game.ball.velocity = { 180, -300 };
		game.frame(sixtieth);
		CHECK(game.ball.velocity.y == 300);
		game.ball.position = { 600, height - 16 - 3 };
		game.frame(sixtieth);
		CHECK(game.ball.velocity.y == -300);
	}

	// past either side, served again
	{
		Pong game(1);
		game.ball.position = { 3, 100 };
		game.ball.velocity = { -360, 0 };
		game.frame(sixtieth);
		CHECK(served(game.ball));
		game.ball.position = { width - 16 - 3, 100 };
		game.ball.velocity = { 360, 0 };
		game.frame(sixtieth);
		CHECK(served(game.ball));
	}

	// W and S the left paddle, Up and Down the right, 300 a second, held;
	// they stop at the top and the bottom
	{
		Pong game(1);
		game.ball.velocity = {};
		const float middle = game.paddle1.position.y;
		game.key(Key::W, true);
		game.frame(sixtieth);
		CHECK_THAT(game.paddle1.position.y, WithinAbs(middle - 5, 0.001));
		game.key(Key::W, false);
		game.frame(sixtieth);
		CHECK_THAT(game.paddle1.position.y, WithinAbs(middle - 5, 0.001));
		game.key(Key::W, true);
		for (int i = 0; i < 100; ++i) { game.frame(sixtieth); }
		CHECK(game.paddle1.position.y == 0);
		game.key(Key::W, false);
		game.key(Key::Down, true);
		for (int i = 0; i < 100; ++i) { game.frame(sixtieth); }
		CHECK(game.paddle2.position.y == height - 120);
	}

	// a second goes as far at 8 steps as at 60
	{
		Pong slow(1);
		Pong quick(1);
		slow.ball.velocity = {};
		quick.ball.velocity = {};
		slow.key(Key::S, true);
		quick.key(Key::S, true);
		for (int i = 0; i < 8; ++i) { slow.frame(0.125f); }
		for (int i = 0; i < 60; ++i) { quick.frame(sixtieth); }
		CHECK_THAT(slow.paddle1.position.y, WithinAbs(quick.paddle1.position.y, 0.01));
		CHECK_THAT(slow.paddle1.position.y, WithinAbs(720 / 2 - 60 + 300, 0.01));
	}

	// a paddle sends it back, up off its top half, at the same speed
	{
		Pong game(1);
		const auto& paddle = game.paddle1;
		game.ball.position = { paddle.position.x + 20 + 2, paddle.position.y + 10 };
		game.ball.velocity = { -360, 0 };
		game.frame(sixtieth);
		CHECK(game.ball.velocity.x > 0);
		CHECK(game.ball.velocity.y < 0);
		CHECK_THAT(std::hypot(game.ball.velocity.x, game.ball.velocity.y), WithinAbs(speed, 0.01));
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
			game.frame(sixtieth);
			if (served(game.ball)) { ++serves; }
			else if ((before > 0) != (game.ball.velocity.x > 0)) { ++returns; }
			REQUIRE(game.ball.position.x > -speed * sixtieth * 2);
			REQUIRE(game.ball.position.x < width + speed * sixtieth * 2);
			REQUIRE(game.ball.position.y > -speed * sixtieth * 2);
			REQUIRE(game.ball.position.y < height + speed * sixtieth * 2);
		}
		CHECK(returns > 0);
		CHECK(serves > 0);
	}
}
