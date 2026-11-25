// pong.h
// XML Game Engine
// author: beefviper
// date: Nov 24, 2026
//
// What pong.xml written out must do, whoever wrote it, run on the one
// windows-cpp writes (test_written_games.cpp): the title's choices, the
// serve, the bounces, the paddles, the points, a pause, game over, the
// computer and the kept choices. It shares no code with the engine, so
// this checks what happens, not every frame.

#pragma once

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <SFML/Window/Keyboard.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>
#include <utility>

template <typename Pong>
void pong_plays()
{
	using Key = sf::Keyboard::Key;
	using Catch::Matchers::WithinAbs;
	using Screen = decltype(std::declval<const Pong&>().screen());
	using Sound = typename decltype(std::declval<Pong&>().takeSounds())::value_type;
	using Look = decltype(std::declval<Pong&>().computer.look);

	constexpr float width = 1280;
	constexpr float height = 720;
	constexpr float margin = 30;
	constexpr float paddleWidth = 20;
	constexpr float paddleHeight = 120;
	constexpr float paddleSpeed = 300;
	constexpr float ballSize = 16;
	constexpr float ballSpeed = 540;
	constexpr float sixtieth = 1.0f / 60; // a step at 60 frames a second

	const auto tap = [](Pong& game, Key key)
	{
		game.key(key, true);
		game.key(key, false);
	};
	const auto frames = [](Pong& game, int count)
	{
		for (int i = 0; i < count; ++i) { game.frame(sixtieth); }
	};
	const auto speedOf = [](const auto& ball) { return std::hypot(ball.velocity.x, ball.velocity.y); };
	const auto heard = [](Pong& game, Sound sound)
	{
		const auto sounds = game.takeSounds();
		return std::find(sounds.begin(), sounds.end(), sound) != sounds.end();
	};
	// Playing against the computer.
	const auto playing = [&](unsigned seed)
	{
		Pong game(seed);
		tap(game, Key::Num1);
		return game;
	};

	SECTION("the title chooses who plays and how well")
	{
		Pong game(1);
		CHECK(game.screen() == Screen::Title);
		CHECK(game.computer.look == Look::Easy);
		tap(game, Key::D);
		CHECK(game.computer.look == Look::Smart);
		CHECK(game.computer.level == 2);
		tap(game, Key::A);
		CHECK(game.computer.look == Look::Easy);
		CHECK(game.computer.level == 1);

		tap(game, Key::Num2);
		CHECK(game.screen() == Screen::Playing);
		CHECK(game.people == 2);

		Pong other(1);
		tap(other, Key::Numpad1);
		CHECK(other.screen() == Screen::Playing);
		CHECK(other.people == 1);
	}

	SECTION("the ball is served from the middle within 30 degrees")
	{
		for (unsigned seed = 0; seed < 50; ++seed)
		{
			const Pong game(seed);
			CHECK(game.ball.position.x == width / 2 - ballSize / 2);
			CHECK(game.ball.position.y == height / 2 - ballSize / 2);
			CHECK_THAT(speedOf(game.ball), WithinAbs(ballSpeed, 0.01));
			CHECK(std::abs(game.ball.velocity.y) <= ballSpeed * 0.5f + 0.01f);
		}
	}

	SECTION("the ball bounces off the top and the bottom")
	{
		Pong game = playing(1);
		game.takeSounds();
		game.ball.position = { 600, 4 };
		game.ball.velocity = { 180, -360 };
		game.frame(sixtieth);
		CHECK(game.ball.velocity.y == 360);
		CHECK(heard(game, Sound::Wall));

		game.ball.position = { 600, height - ballSize - 2 };
		game.ball.velocity = { 180, 360 };
		game.frame(sixtieth);
		CHECK(game.ball.velocity.y == -360);
	}

	SECTION("a paddle sends the ball back, steeper off its ends, faster on the fourth hit")
	{
		Pong game = playing(1);
		game.takeSounds();
		const auto hitLeft = [&](float offset)
		{
			game.ball.position = { margin + paddleWidth - 2, game.paddle1.position.y + paddleHeight / 2 - ballSize / 2 + offset };
			game.ball.velocity = { -ballSpeed, 0 };
			game.frame(sixtieth);
		};

		hitLeft(0);
		CHECK(game.ball.velocity.x > 0);
		CHECK_THAT(game.ball.velocity.y, WithinAbs(0, 0.01));
		CHECK(heard(game, Sound::Paddle));
		CHECK(game.ball.position.x >= margin + paddleWidth);

		hitLeft(-50);
		CHECK(game.ball.velocity.x > 0);
		CHECK(game.ball.velocity.y < -180);

		hitLeft(60);
		CHECK(game.ball.velocity.y > 0);

		CHECK_THAT(speedOf(game.ball), WithinAbs(ballSpeed, 0.5));
		hitLeft(0);
		CHECK(game.ball.hits == 4);
		CHECK_THAT(speedOf(game.ball), WithinAbs(ballSpeed * 1.3f, 0.5));
	}

	SECTION("a paddle stops at the top and the bottom")
	{
		Pong game = playing(1);
		game.ball.velocity = {};
		game.key(Key::W, true);
		frames(game, 200);
		CHECK(game.paddle1.position.y == 0);
		game.key(Key::W, false);
		frames(game, 5);
		CHECK(game.paddle1.position.y == 0);

		game.key(Key::S, true);
		frames(game, 200);
		CHECK(game.paddle1.position.y == height - paddleHeight);
	}

	SECTION("past a paddle is a point to the other side and a new serve")
	{
		Pong game = playing(1);
		game.ball.position = { 3, 100 };
		game.ball.velocity = { -540, 0 };
		game.ball.hits = 5;
		game.frame(sixtieth);
		CHECK(game.paddle2.score == 1);
		CHECK(game.paddle1.score == 0);
		CHECK(game.ball.hits == 0);
		CHECK(game.ball.position.x == width / 2 - ballSize / 2);
		CHECK(heard(game, Sound::Score));

		game.ball.position = { width - ballSize - 3, 100 };
		game.ball.velocity = { 540, 0 };
		game.frame(sixtieth);
		CHECK(game.paddle1.score == 1);
	}

	SECTION("eleven ends the game, and Space starts a new one from the title")
	{
		Pong game = playing(1);
		game.paddle1.score = 10;
		game.ball.position = { width - ballSize - 3, 100 };
		game.ball.velocity = { 540, 0 };
		game.frame(sixtieth);
		CHECK(game.screen() == Screen::Over);

		const auto stopped = game.ball.position;
		frames(game, 10);
		CHECK(game.ball.position == stopped);

		tap(game, Key::Space);
		CHECK(game.screen() == Screen::Title);
		CHECK(game.paddle1.score == 0);
		CHECK(game.paddle2.score == 0);
	}

	SECTION("a pause stops play, and a paddle key held through it moves again")
	{
		Pong game = playing(1);
		game.ball.velocity = {};
		game.key(Key::S, true);
		frames(game, 2);
		tap(game, Key::Pause);
		CHECK(game.screen() == Screen::Paused);
		const float y = game.paddle1.position.y;
		frames(game, 10);
		CHECK(game.paddle1.position.y == y);

		tap(game, Key::Pause);
		CHECK(game.screen() == Screen::Playing);
		game.frame(sixtieth);
		CHECK_THAT(game.paddle1.position.y, WithinAbs(y + paddleSpeed * sixtieth, 0.001));
	}

	SECTION("Up and Down move the right paddle only for two players")
	{
		Pong one = playing(1);
		one.ball.position = { 100, one.paddle2.position.y + paddleHeight / 2 - ballSize / 2 };
		one.ball.velocity = {};
		one.key(Key::Up, true);
		frames(one, 10);
		CHECK(one.paddle2.position.y == height / 2 - paddleHeight / 2);

		Pong two(1);
		tap(two, Key::Num2);
		two.ball.velocity = {};
		two.key(Key::Up, true);
		frames(two, 10);
		CHECK_THAT(two.paddle2.position.y, WithinAbs(height / 2 - paddleHeight / 2 - 10 * paddleSpeed * sixtieth, 0.01));
		two.key(Key::Up, false);
		frames(two, 10);
		CHECK_THAT(two.paddle2.position.y, WithinAbs(height / 2 - paddleHeight / 2 - 10 * paddleSpeed * sixtieth, 0.01));
	}

	SECTION("the computer follows the ball; smart waits in the middle until it comes")
	{
		Pong easy = playing(1);
		easy.ball.position = { 300, 50 };
		easy.ball.velocity = {};
		frames(easy, 100);
		CHECK(easy.paddle2.position.y < 50);

		Pong smart(1);
		tap(smart, Key::D);
		tap(smart, Key::Num1);
		smart.ball.position = { 300, 50 };
		smart.ball.velocity = { -0.06f, 0 };
		frames(smart, 100);
		CHECK(std::abs(smart.paddle2.position.y - (height / 2 - paddleHeight / 2)) <= 8);

		smart.ball.position = { 800, 50 };
		smart.ball.velocity = { 0.06f, 0 };
		frames(smart, 100);
		CHECK(smart.paddle2.position.y < 50);
	}

	SECTION("how well the computer plays is kept from one game to the next")
	{
		const std::filesystem::path file = std::filesystem::temp_directory_path() / "xge_test_pong_kept.txt";
		std::filesystem::remove(file);

		Pong game(1);
		game.keepIn(file.string());
		tap(game, Key::D);
		tap(game, Key::Num1);
		game.paddle1.score = 11;
		game.frame(sixtieth);
		tap(game, Key::Space);
		CHECK(game.computer.look == Look::Smart);

		Pong next(1);
		next.keepIn(file.string());
		CHECK(next.computer.look == Look::Smart);
		CHECK(next.computer.level == 2);
		std::filesystem::remove(file);
	}

	SECTION("long play keeps everything in the window")
	{
		for (unsigned seed = 0; seed < 5; ++seed)
		{
			Pong game = playing(seed);
			int games = 0;
			for (int frame = 0; frame < 9000; ++frame)
			{
				const Key key = (frame / 30) % 2 ? Key::W : Key::S;
				if (frame % 30 == 0) { game.key(key, true); }
				if (frame % 30 == 20) { game.key(key, false); }
				if (game.screen() != Screen::Playing) { ++games; tap(game, Key::Space); tap(game, Key::Num1); }
				game.frame(sixtieth);

				REQUIRE(game.ball.position.x >= -ballSpeed * 2 * sixtieth * 2);
				REQUIRE(game.ball.position.x <= width + ballSpeed * 2 * sixtieth * 2);
				REQUIRE(game.ball.position.y >= -ballSpeed * 2 * sixtieth * 2);
				REQUIRE(game.ball.position.y <= height + ballSpeed * 2 * sixtieth * 2);
				REQUIRE(game.paddle1.score <= 11);
				REQUIRE(game.paddle2.score <= 11);
			}
			CHECK(games > 0);
		}
	}
}
