// breakout.h
// XML Game Engine
// author: beefviper
// date: Nov 24, 2026
//
// What breakout.xml written out must do, whoever wrote it, run on the one
// windows-cpp writes (test_written_games.cpp): the title, the wall, the
// serve, the bounces, the paddle by keys and mouse, the bricks' worth and
// sounds, the speed-ups, the half paddle, lost balls, the second wall, a
// pause, winning and losing. It shares no code with the engine, so this
// checks what happens, not every frame.

#pragma once

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <SFML/Window/Keyboard.hpp>

#include <algorithm>
#include <cmath>
#include <set>
#include <utility>

template <typename Breakout>
void breakout_plays()
{
	using Key = sf::Keyboard::Key;
	using Catch::Matchers::WithinAbs;
	using Screen = decltype(std::declval<const Breakout&>().screen());
	using Sound = typename decltype(std::declval<Breakout&>().takeSounds())::value_type;
	using BrickLook = decltype(std::declval<Breakout&>().wall.front().look);
	using PlayerLook = decltype(std::declval<Breakout&>().player.look);
	using MessageLook = decltype(std::declval<Breakout&>().message.look);

	constexpr float width = 640;
	constexpr float height = 720;
	constexpr float left = 28; // (640 - 14 * 38 - 13 * 4) / 2
	constexpr float top = 110;
	constexpr float across = 42; // a brick and the gap
	constexpr float down = 18;
	constexpr float paddle = 64;
	constexpr float paddleY = height - 60;
	constexpr float step = 480;
	constexpr float ballSize = 10;
	constexpr float serve = 240;
	constexpr float sixtieth = 1.0f / 60; // a step at 60 frames a second

	const auto tap = [](Breakout& game, Key key)
	{
		game.key(key, true);
		game.key(key, false);
	};
	const auto frames = [](Breakout& game, int count)
	{
		for (int i = 0; i < count; ++i) { game.frame(sixtieth); }
	};
	const auto speedOf = [](const auto& ball) { return std::hypot(ball.velocity.x, ball.velocity.y); };
	const auto heard = [](Breakout& game, Sound sound)
	{
		const auto sounds = game.takeSounds();
		return std::find(sounds.begin(), sounds.end(), sound) != sounds.end();
	};
	const auto playing = [&](unsigned seed)
	{
		Breakout game(seed);
		tap(game, Key::Num1);
		game.takeSounds();
		return game;
	};
	// Only the bricks at these rows and columns are left.
	const auto keepOnly = [&](Breakout& game, std::set<std::pair<int, int>> kept)
	{
		std::erase_if(game.wall, [&](const auto& brick)
		{
			const int row = static_cast<int>(std::lround((brick.position.y - top) / down));
			const int column = static_cast<int>(std::lround((brick.position.x - left) / across));
			return !kept.contains({ row, column });
		});
	};
	// The ball just below the brick at this row and column, going up into it.
	const auto under = [&](Breakout& game, int row, int column)
	{
		game.ball.position = { left + across * static_cast<float>(column) + 14, top + down * static_cast<float>(row) + 14 + 1 };
		game.ball.velocity = { 0, -serve };
	};

	SECTION("1 starts the game from the title")
	{
		Breakout game(1);
		CHECK(game.screen() == Screen::Title);
		tap(game, Key::Num1);
		CHECK(game.screen() == Screen::Playing);

		Breakout other(1);
		tap(other, Key::Numpad1);
		CHECK(other.screen() == Screen::Playing);
	}

	SECTION("the wall is eight rows of fourteen, two of each colour, worth 7, 5, 3 and 1")
	{
		const Breakout game(1);
		REQUIRE(game.wall.size() == 112);
		const BrickLook looks[] = { BrickLook::Red, BrickLook::Orange, BrickLook::Green, BrickLook::Yellow };
		const int worths[] = { 7, 5, 3, 1 };
		std::set<std::pair<int, int>> places;
		for (const auto& brick : game.wall)
		{
			const float row = (brick.position.y - top) / down;
			const float column = (brick.position.x - left) / across;
			REQUIRE_THAT(row, WithinAbs(std::round(row), 0.001));
			REQUIRE_THAT(column, WithinAbs(std::round(column), 0.001));
			const int r = static_cast<int>(std::lround(row));
			const int c = static_cast<int>(std::lround(column));
			REQUIRE(r >= 0);
			REQUIRE(r < 8);
			REQUIRE(c >= 0);
			REQUIRE(c < 14);
			CHECK(brick.look == looks[r / 2]);
			CHECK(brick.worth == worths[r / 2]);
			CHECK(brick.size.x == 38);
			CHECK(brick.size.y == 14);
			places.insert({ r, c });
		}
		CHECK(places.size() == 112);
	}

	SECTION("the ball is served from the middle, downwards, up to 180 sideways")
	{
		for (unsigned seed = 0; seed < 50; ++seed)
		{
			const Breakout game(seed);
			CHECK(game.ball.position.x == width / 2 - ballSize / 2);
			CHECK(game.ball.position.y == height / 2 - ballSize / 2);
			CHECK(game.ball.velocity.y == serve);
			CHECK(std::abs(game.ball.velocity.x) <= 180);
		}
	}

	SECTION("the ball bounces off the sides and the top, and the top halves the paddle")
	{
		Breakout game = playing(1);
		game.ball.position = { 2, 400 };
		game.ball.velocity = { -180, serve };
		game.frame(sixtieth);
		CHECK(game.ball.velocity.x == 180);
		CHECK(heard(game, Sound::Wall));

		game.ball.position = { width - ballSize - 2, 400 };
		game.ball.velocity = { 180, serve };
		game.frame(sixtieth);
		CHECK(game.ball.velocity.x == -180);

		CHECK(game.player.look == PlayerLook::Full);
		game.ball.position = { 300, 2 };
		game.ball.velocity = { 0, -serve };
		game.frame(sixtieth);
		CHECK(game.ball.velocity.y == serve);
		CHECK(heard(game, Sound::Wall));
		CHECK(game.player.look == PlayerLook::Half);
		CHECK(game.player.size.x == paddle / 2);
	}

	SECTION("a brick hit scores its worth, sounds its colour, goes, and sends the ball back")
	{
		Breakout game = playing(1);
		under(game, 7, 5);
		game.frame(sixtieth);
		CHECK(game.status.score == 1);
		CHECK(game.wall.size() == 111);
		CHECK(game.ball.velocity.y == serve);
		CHECK(heard(game, Sound::Yellow));

		under(game, 0, 3);
		keepOnly(game, { { 0, 3 }, { 0, 4 } });
		game.frame(sixtieth);
		CHECK(game.status.score == 8);
		CHECK(heard(game, Sound::Red));
	}

	SECTION("the fourth and the twelfth paddle hits make the ball faster")
	{
		Breakout game = playing(1);
		game.ball.velocity = { 0, serve };
		const auto hit = [&]
		{
			const float speed = speedOf(game.ball);
			game.ball.position = { game.player.position.x + paddle / 2 - ballSize / 2, paddleY - ballSize + 2 };
			game.ball.velocity = { 0, speed };
			game.frame(sixtieth);
		};
		for (int i = 1; i <= 3; ++i) { hit(); }
		CHECK(game.ball.hits == 3);
		CHECK(game.ball.velocity.y < 0);
		CHECK(heard(game, Sound::Paddle));
		CHECK_THAT(speedOf(game.ball), WithinAbs(serve, 0.01));
		hit();
		CHECK_THAT(speedOf(game.ball), WithinAbs(serve * 1.25f, 0.01));
		for (int i = 5; i <= 11; ++i) { hit(); }
		CHECK_THAT(speedOf(game.ball), WithinAbs(serve * 1.25f, 0.01));
		hit();
		CHECK(game.ball.hits == 12);
		CHECK_THAT(speedOf(game.ball), WithinAbs(serve * 1.25f * 1.25f, 0.01));
	}

	SECTION("a paddle hit off its end sends the ball out at an angle")
	{
		Breakout game = playing(1);
		game.ball.position = { game.player.position.x + paddle - ballSize, paddleY - ballSize + 2 };
		game.ball.velocity = { 0, serve };
		game.frame(sixtieth);
		CHECK(game.ball.velocity.y < 0);
		CHECK(game.ball.velocity.x > 100);
		CHECK_THAT(speedOf(game.ball), WithinAbs(serve, 0.01));
	}

	SECTION("the first brick reached in the back four rows makes the ball faster, once")
	{
		Breakout game = playing(1);
		keepOnly(game, { { 7, 0 }, { 3, 5 }, { 3, 7 }, { 0, 13 } });
		under(game, 7, 0);
		game.frame(sixtieth);
		CHECK_THAT(speedOf(game.ball), WithinAbs(serve, 0.01));

		under(game, 3, 5);
		game.frame(sixtieth);
		CHECK(game.ball.deep == 1);
		CHECK_THAT(speedOf(game.ball), WithinAbs(serve * 1.25f, 0.01));

		const float speed = speedOf(game.ball);
		under(game, 3, 7);
		game.ball.velocity = { 0, -speed };
		game.frame(sixtieth);
		CHECK_THAT(speedOf(game.ball), WithinAbs(speed, 0.01));
	}

	SECTION("the keys move the paddle, which stops at the sides")
	{
		Breakout game = playing(1);
		game.ball.velocity = {};
		game.key(Key::A, true);
		game.frame(sixtieth);
		CHECK_THAT(game.player.position.x, WithinAbs(width / 2 - paddle / 2 - step * sixtieth, 0.001));
		frames(game, 100);
		CHECK(game.player.position.x == 0);
		game.key(Key::A, false);

		game.key(Key::Right, true);
		frames(game, 100);
		CHECK(game.player.position.x == width - paddle);
	}

	SECTION("the mouse steers the paddle's middle, kept inside the window, only in play")
	{
		Breakout title(1);
		title.mouse(100);
		CHECK(title.player.position.x == width / 2 - paddle / 2);

		Breakout game = playing(1);
		game.mouse(100);
		CHECK(game.player.position.x == 100 - paddle / 2);
		game.mouse(-50);
		CHECK(game.player.position.x == 0);
		game.mouse(1000);
		CHECK(game.player.position.x == width - paddle);
	}

	SECTION("a lost ball costs one of three and serves again; the last ends the game")
	{
		Breakout game = playing(1);
		game.ball.position = { 300, height - ballSize - 2 };
		game.ball.velocity = { 0, serve };
		game.frame(sixtieth);
		CHECK(game.status.balls == 2);
		CHECK(heard(game, Sound::Lost));
		CHECK(game.ball.position.y == height / 2 - ballSize / 2);

		game.status.balls = 1;
		game.ball.position = { 300, height - ballSize - 2 };
		game.ball.velocity = { 0, serve };
		game.frame(sixtieth);
		CHECK(game.screen() == Screen::Over);
		CHECK(game.message.look == MessageLook::Lost);

		const auto stopped = game.ball.position;
		frames(game, 10);
		CHECK(game.ball.position == stopped);

		tap(game, Key::Space);
		CHECK(game.screen() == Screen::Title);
		CHECK(game.status.balls == 3);
		CHECK(game.status.score == 0);
		CHECK(game.wall.size() == 112);
	}

	SECTION("clearing the wall builds a second one; clearing that wins")
	{
		Breakout game = playing(1);
		game.status.score = 448;
		game.player.look = PlayerLook::Half;
		game.wall.clear();
		game.frame(sixtieth);
		CHECK(game.screen() == Screen::Playing);
		CHECK(game.status.walls == 1);
		CHECK(game.wall.size() == 112);
		CHECK(game.ball.position.y == height / 2 - ballSize / 2);
		CHECK(game.status.score == 448);

		game.wall.clear();
		game.frame(sixtieth);
		CHECK(game.screen() == Screen::Over);
		CHECK(game.message.look == MessageLook::Won);
	}

	SECTION("a pause stops play")
	{
		Breakout game = playing(1);
		frames(game, 2);
		tap(game, Key::Pause);
		CHECK(game.screen() == Screen::Paused);
		const auto ball = game.ball.position;
		frames(game, 10);
		CHECK(game.ball.position == ball);
		tap(game, Key::Pause);
		CHECK(game.screen() == Screen::Playing);
		game.frame(sixtieth);
		CHECK(game.ball.position != ball);
	}

	SECTION("long play keeps the ball in the window and the score within the walls")
	{
		for (unsigned seed = 0; seed < 5; ++seed)
		{
			Breakout game = playing(seed);
			int games = 0;
			for (int frame = 0; frame < 9000; ++frame)
			{
				// follow the ball, a little late
				if (frame % 4 == 0) { game.mouse(game.ball.position.x + ballSize / 2 + static_cast<float>((frame / 40) % 30 - 15)); }
				if (game.screen() != Screen::Playing) { ++games; tap(game, Key::Space); tap(game, Key::Num1); }
				game.frame(sixtieth);

				REQUIRE(game.ball.position.x >= -serve * 4 * sixtieth * 2);
				REQUIRE(game.ball.position.x <= width + serve * 4 * sixtieth * 2);
				REQUIRE(game.ball.position.y >= -serve * 4 * sixtieth * 2);
				REQUIRE(game.ball.position.y <= height + serve * 4 * sixtieth * 2);
				REQUIRE(game.status.score <= 2 * 448);
				REQUIRE(game.status.balls >= 0);
			}
			CHECK((games > 0 || game.status.score > 0));
		}
	}
}
