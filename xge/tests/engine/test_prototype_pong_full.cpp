// test_prototype_pong_full.cpp
// XML Game Engine
// author: beefviper
// date: Nov 13, 2026
//
// Catch2 tests that the stand-alone Pong (games/prototypes/pong, pong.xml
// written by hand on plain SFML) does what pong.xml says: the title's
// choices, the serve, the bounces, the paddles, the points, a pause, game
// over, the computer and the kept choices. It shares no code with the
// engine, so these check what happens, not every frame.

#include "pong.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace pong;
using Key = sf::Keyboard::Key;
using Catch::Matchers::WithinAbs;

namespace
{
	void tap(Pong& game, Key key)
	{
		game.key(key, true);
		game.key(key, false);
	}

	void frames(Pong& game, int count)
	{
		for (int i = 0; i < count; ++i) { game.frame(); }
	}

	// Playing against the computer, the ball held still in the middle so
	// nothing else happens.
	Pong playing(unsigned seed = 1)
	{
		Pong game(seed);
		tap(game, Key::Space);
		return game;
	}

	float speedOf(const Ball& ball) { return std::hypot(ball.velocity.x, ball.velocity.y); }

	bool has(const std::vector<std::string>& sounds, const std::string& sound)
	{
		for (const std::string& each : sounds) { if (each == sound) { return true; } }
		return false;
	}
}

TEST_CASE("stand-alone pong: the title chooses who plays and how well", "[prototype][pong]")
{
	Pong game;
	CHECK(game.screen() == Screen::Title);
	CHECK_FALSE(game.computerIsSmart());

	tap(game, Key::D);
	CHECK(game.computerIsSmart());
	tap(game, Key::A);
	CHECK_FALSE(game.computerIsSmart());

	tap(game, Key::Num2);
	CHECK(game.screen() == Screen::Playing);
	CHECK(game.twoPlayers());

	Pong other;
	tap(other, Key::Space);
	CHECK(other.screen() == Screen::Playing);
	CHECK_FALSE(other.twoPlayers());
}

TEST_CASE("stand-alone pong: the ball is served from the middle within 30 degrees", "[prototype][pong]")
{
	for (unsigned seed = 0; seed < 50; ++seed)
	{
		const Pong game(seed);
		CHECK(game.ball.position.x == windowWidth / 2 - ballSize / 2);
		CHECK(game.ball.position.y == windowHeight / 2 - ballSize / 2);
		CHECK_THAT(speedOf(game.ball), WithinAbs(ballSpeed, 0.001));
		CHECK(std::abs(game.ball.velocity.y) <= ballSpeed * 0.5f + 0.001f);
	}
}

TEST_CASE("stand-alone pong: the ball bounces off the top and the bottom", "[prototype][pong]")
{
	Pong game = playing();
	game.takeSounds();
	game.ball.position = { 600, 4 };
	game.ball.velocity = { 3, -6 };
	game.frame();
	CHECK(game.ball.velocity.y == 6);
	CHECK(has(game.takeSounds(), "wall"));

	game.ball.position = { 600, windowHeight - ballSize - 2 };
	game.ball.velocity = { 3, 6 };
	game.frame();
	CHECK(game.ball.velocity.y == -6);
}

TEST_CASE("stand-alone pong: a paddle sends the ball back, steeper off its ends, faster on the fourth hit", "[prototype][pong]")
{
	Pong game = playing();
	game.takeSounds();
	const auto hitLeft = [&](float offset)
	{
		game.ball.position = { margin + paddleWidth + 2, game.left.position.y + paddleHeight / 2 - ballSize / 2 + offset };
		game.ball.velocity = { -ballSpeed, 0 };
		game.frame();
	};

	hitLeft(0);
	CHECK(game.ball.velocity.x > 0);
	CHECK_THAT(game.ball.velocity.y, WithinAbs(0, 0.01));
	CHECK(has(game.takeSounds(), "paddle"));

	hitLeft(-50);
	CHECK(game.ball.velocity.x > 0);
	CHECK(game.ball.velocity.y < -3);

	hitLeft(60);
	CHECK(game.ball.velocity.y > 0);
	CHECK_THAT(game.ball.velocity.x, WithinAbs(game.ball.velocity.y, 0.01));

	CHECK_THAT(speedOf(game.ball), WithinAbs(ballSpeed, 0.01));
	hitLeft(0);
	CHECK(game.ball.hits == 4);
	CHECK_THAT(speedOf(game.ball), WithinAbs(ballSpeed * 1.3f, 0.01));
}

TEST_CASE("stand-alone pong: a paddle stops at the top and the bottom", "[prototype][pong]")
{
	Pong game = playing();
	game.ball.velocity = {};
	game.key(Key::W, true);
	frames(game, 200);
	CHECK(game.left.position.y == 0);
	game.key(Key::W, false);
	frames(game, 5);
	CHECK(game.left.position.y == 0);

	game.key(Key::S, true);
	frames(game, 200);
	CHECK(game.left.position.y == windowHeight - paddleHeight);
}

TEST_CASE("stand-alone pong: past a paddle is a point to the other side and a new serve", "[prototype][pong]")
{
	Pong game = playing();
	game.ball.position = { 3, 100 };
	game.ball.velocity = { -9, 0 };
	game.ball.hits = 5;
	game.frame();
	CHECK(game.right.score == 1);
	CHECK(game.left.score == 0);
	CHECK(game.ball.hits == 0);
	CHECK(game.ball.position.x == windowWidth / 2 - ballSize / 2);
	CHECK(has(game.takeSounds(), "score"));

	game.ball.position = { windowWidth - ballSize - 3, 100 };
	game.ball.velocity = { 9, 0 };
	game.frame();
	CHECK(game.left.score == 1);
}

TEST_CASE("stand-alone pong: eleven ends the game, Space goes back to the title", "[prototype][pong]")
{
	Pong game = playing();
	game.left.score = 10;
	game.ball.position = { windowWidth - ballSize - 3, 100 };
	game.ball.velocity = { 9, 0 };
	game.frame();
	CHECK(game.screen() == Screen::Over);

	const sf::Vector2f stopped = game.ball.position;
	frames(game, 10);
	CHECK(game.ball.position == stopped);

	tap(game, Key::Space);
	CHECK(game.screen() == Screen::Title);
	CHECK(game.left.score == 0);
	CHECK(game.right.score == 0);
}

TEST_CASE("stand-alone pong: a pause stops play, and a paddle key held through it moves again", "[prototype][pong]")
{
	Pong game = playing();
	game.ball.velocity = {};
	game.key(Key::S, true);
	frames(game, 2);
	tap(game, Key::Pause);
	CHECK(game.screen() == Screen::Paused);
	const float y = game.left.position.y;
	frames(game, 10);
	CHECK(game.left.position.y == y);

	tap(game, Key::Pause);
	CHECK(game.screen() == Screen::Playing);
	game.frame();
	CHECK(game.left.position.y == y + paddleSpeed);
}

TEST_CASE("stand-alone pong: Up and Down move the right paddle only for two players", "[prototype][pong]")
{
	Pong one = playing();
	one.ball.position = { 100, one.right.position.y + paddleHeight / 2 - ballSize / 2 };
	one.ball.velocity = {};
	one.key(Key::Up, true);
	frames(one, 10);
	CHECK(one.right.position.y == windowHeight / 2 - paddleHeight / 2);

	Pong two(1);
	tap(two, Key::Num2);
	two.ball.velocity = {};
	two.key(Key::Up, true);
	frames(two, 10);
	CHECK(two.right.position.y == windowHeight / 2 - paddleHeight / 2 - 10 * paddleSpeed);
	two.key(Key::Up, false);
	frames(two, 10);
	CHECK(two.right.position.y == windowHeight / 2 - paddleHeight / 2 - 10 * paddleSpeed);
}

TEST_CASE("stand-alone pong: the computer follows the ball; smart waits in the middle until it comes", "[prototype][pong]")
{
	Pong easy = playing();
	easy.ball.position = { 300, 50 };
	easy.ball.velocity = {};
	frames(easy, 100);
	CHECK(easy.right.position.y < 50);

	Pong smart(1);
	tap(smart, Key::D);
	tap(smart, Key::Space);
	smart.ball.position = { 300, 50 };
	smart.ball.velocity = { -0.001f, 0 };
	frames(smart, 100);
	CHECK(std::abs(smart.right.position.y - (windowHeight / 2 - paddleHeight / 2)) <= 8);

	smart.ball.position = { 800, 50 };
	smart.ball.velocity = { 0.001f, 0 };
	frames(smart, 100);
	CHECK(smart.right.position.y < 50);
}

TEST_CASE("stand-alone pong: who plays and how well are kept from one game to the next", "[prototype][pong]")
{
	const std::filesystem::path file = std::filesystem::temp_directory_path() / "xge_test_pong_kept.txt";
	std::filesystem::remove(file);

	Pong game;
	game.keepChoicesIn(file.string());
	tap(game, Key::D);
	tap(game, Key::Num2);
	game.left.score = 11;
	game.frame();
	tap(game, Key::Space);

	Pong next;
	next.keepChoicesIn(file.string());
	CHECK(next.computerIsSmart());
	CHECK(next.twoPlayers());
	std::filesystem::remove(file);
}

TEST_CASE("stand-alone pong: long play keeps everything in the window", "[prototype][pong]")
{
	for (unsigned seed = 0; seed < 5; ++seed)
	{
		Pong game(seed);
		tap(game, Key::Space);
		int games = 0;
		for (int frame = 0; frame < 9000; ++frame)
		{
			const Key key = (frame / 30) % 2 ? Key::W : Key::S;
			if (frame % 30 == 0) { game.key(key, true); }
			if (frame % 30 == 20) { game.key(key, false); }
			if (game.screen() != Screen::Playing) { ++games; tap(game, Key::Space); tap(game, Key::Space); }
			game.frame();

			REQUIRE(game.ball.position.x >= -ballSpeed * 2);
			REQUIRE(game.ball.position.x <= windowWidth + ballSpeed * 2);
			REQUIRE(game.ball.position.y >= -ballSpeed * 2);
			REQUIRE(game.ball.position.y <= windowHeight + ballSpeed * 2);
			REQUIRE(game.left.score <= winningScore);
			REQUIRE(game.right.score <= winningScore);
		}
		CHECK(games > 0);
	}
}
