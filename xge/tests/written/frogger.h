// frogger.h
// XML Game Engine
// author: beefviper
// date: Nov 27, 2026
//
// What frogger.xml written out must do, whoever wrote it, run on the one
// windows-cpp writes (test_written_games.cpp): the title, the lanes of the
// river and the road and their wrap, a hop a press, being run over, falling
// in, riding a log and being ridden off the side, the turtles diving, a frog
// home and a bay filled, the hedge, the clock, the end, a pause and a new
// game. It shares no code with the engine, so this checks what happens, not
// every frame.

#pragma once

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <SFML/Window/Keyboard.hpp>

#include <algorithm>
#include <cstddef>
#include <map>
#include <utility>

template <typename Frogger>
void frogger_plays()
{
	using Key = sf::Keyboard::Key;
	using Catch::Matchers::WithinAbs;
	using Screen = decltype(std::declval<const Frogger&>().screen());
	using Sound = typename decltype(std::declval<Frogger&>().takeSounds())::value_type;
	using Log = typename decltype(std::declval<Frogger&>().river)::value_type;
	using Car = typename decltype(std::declval<Frogger&>().traffic)::value_type;
	using PadLook = typename decltype(std::declval<Frogger&>().pads)::value_type::Look;
	using MessageLook = decltype(std::declval<Frogger&>().message.look);

	constexpr float width = 624;
	constexpr float cell = 48;
	constexpr float sixtieth = 1.0f / 60; // a step at 60 frames a second
	// where the frog stands in a row: 8 down from the row's top
	const auto rowY = [](int row) { return static_cast<float>(row) * cell + 8; };
	constexpr float startX = 6 * cell + 4;

	const auto tap = [](Frogger& game, Key key)
	{
		game.key(key, true);
		game.key(key, false);
	};
	const auto frames = [](Frogger& game, int count)
	{
		for (int i = 0; i < count; ++i) { game.frame(sixtieth); }
	};
	const auto heard = [](Frogger& game, Sound sound)
	{
		const auto sounds = game.takeSounds();
		return std::find(sounds.begin(), sounds.end(), sound) != sounds.end();
	};
	// Playing, with nothing on the river or the road unless a check puts it there.
	const auto crossing = [&]()
	{
		Frogger game;
		tap(game, Key::Num1);
		game.river.clear();
		game.traffic.clear();
		game.takeSounds();
		return game;
	};
	// Something under the frog, as long as it, going `speed` across.
	const auto under = [](const Frogger& game, auto one, float speed)
	{
		one.position = game.frog.position;
		one.size = { 48, 36 };
		one.velocity = { speed, 0 };
		return one;
	};
	// The frog lost: what is left of it played out, the next one in.
	const auto nextFrog = [&](Frogger& game)
	{
		frames(game, 100);
	};

	SECTION("the title waits for 1")
	{
		Frogger game;
		CHECK(game.screen() == Screen::Title);
		tap(game, Key::Space);
		CHECK(game.screen() == Screen::Title);
		tap(game, Key::Num1);
		CHECK(game.screen() == Screen::Playing);
		CHECK(heard(game, Sound::Start));
	}

	SECTION("five lanes of logs and turtles and five of traffic, each its own way")
	{
		Frogger game;
		const std::map<int, float> speeds{ { 2, 84 }, { 3, -96 }, { 4, 120 }, { 5, 60 }, { 6, -72 }, { 8, -78 }, { 9, 192 }, { 10, -96 }, { 11, 72 }, { 12, -60 } };
		std::map<int, int> counts;
		const auto lane = [&](const auto& one)
		{
			const int row = static_cast<int>(one.position.y / cell);
			REQUIRE(speeds.count(row) == 1);
			CHECK(one.velocity.x == speeds.at(row));
			CHECK(one.velocity.y == 0);
			++counts[row];
		};
		for (const Log& one : game.river) { lane(one); }
		for (const Car& one : game.traffic) { lane(one); }
		CHECK(game.river.size() == 15);
		CHECK(game.traffic.size() == 14);
		CHECK(counts == std::map<int, int>{ { 2, 3 }, { 3, 4 }, { 4, 2 }, { 5, 3 }, { 6, 3 }, { 8, 2 }, { 9, 2 }, { 10, 4 }, { 11, 3 }, { 12, 3 } });

		// however long they go, each stays within its length of the window
		tap(game, Key::Num1);
		game.frog.position.y = rowY(7); // on the bank, out of the way
		frames(game, 1200);
		for (const Log& one : game.river)
		{
			CHECK(one.position.x >= -one.size.x);
			CHECK(one.position.x <= width);
		}
		for (const Car& one : game.traffic)
		{
			CHECK(one.position.x >= -one.size.x);
			CHECK(one.position.x <= width);
		}
	}

	SECTION("the frog starts at the bottom in the middle, three lives")
	{
		Frogger game;
		CHECK(game.frog.position.x == startX);
		CHECK(game.frog.position.y == rowY(13));
		CHECK(game.frog.lives == 3);
		CHECK(game.frog.homes == 0);
		CHECK(game.frog.score == 0);
		CHECK(game.frog.inPlay);
		CHECK(game.clock.time == 40);
		for (const auto& pad : game.pads) { CHECK(pad.look == PadLook::Empty); }
		CHECK(game.pads.size() == 5);
	}

	SECTION("a press is one hop, by W A S D or the arrows, and not out of the window")
	{
		Frogger game = crossing();
		tap(game, Key::S);
		CHECK(game.frog.position.y == rowY(13));
		CHECK_FALSE(heard(game, Sound::Hop));
		game.key(Key::W, true);
		frames(game, 30);
		game.key(Key::W, false);
		CHECK(game.frog.position.y == rowY(12));
		CHECK(heard(game, Sound::Hop));
		tap(game, Key::Up);
		CHECK(game.frog.position.y == rowY(11));
		tap(game, Key::Down);
		CHECK(game.frog.position.y == rowY(12));
		tap(game, Key::A);
		CHECK(game.frog.position.x == startX - cell);
		tap(game, Key::Right);
		tap(game, Key::D);
		CHECK(game.frog.position.x == startX + cell);
	}

	SECTION("run over: what is left of it, then the next frog and a life less")
	{
		Frogger game = crossing();
		game.frog.position.y = rowY(10);
		game.traffic.push_back(under(game, Car{}, 0));
		frames(game, 1);
		CHECK_FALSE(game.frog.inPlay);
		CHECK(game.splats.size() == 1);
		CHECK(heard(game, Sound::Squash));
		game.traffic.clear();

		// out of play, a press does nothing
		const float y = game.frog.position.y;
		tap(game, Key::W);
		CHECK(game.frog.position.y == y);
		CHECK(game.frog.lives == 3);

		frames(game, 60);
		CHECK_FALSE(game.frog.inPlay);
		nextFrog(game);
		CHECK(game.frog.inPlay);
		CHECK(game.frog.lives == 2);
		CHECK(game.splats.empty());
		CHECK(game.frog.position.x == startX);
		CHECK(game.frog.position.y == rowY(13));
		CHECK(game.clock.time >= 39);
	}

	SECTION("a car that only grazes its legs misses it")
	{
		Frogger game = crossing();
		game.frog.position.y = rowY(10);
		Car car = under(game, Car{}, 0);
		car.size = { 40, 32 };
		car.position.x = game.frog.position.x + 40 - 4; // over the tips of its legs
		game.traffic.push_back(car);
		frames(game, 1);
		CHECK(game.frog.inPlay);
	}

	SECTION("in the water with nothing under it: a splash, and a life less")
	{
		Frogger game = crossing();
		game.frog.position.y = rowY(5);
		frames(game, 1);
		CHECK_FALSE(game.frog.inPlay);
		CHECK(game.splashes.size() == 1);
		CHECK(heard(game, Sound::Splash));
		nextFrog(game);
		CHECK(game.frog.inPlay);
		CHECK(game.frog.lives == 2);
		CHECK(game.splashes.empty());
	}

	SECTION("on a log it is carried along, and safe")
	{
		Frogger game = crossing();
		game.frog.position.y = rowY(4);
		game.river.push_back(under(game, Log{}, 60));
		frames(game, 60);
		CHECK(game.frog.inPlay);
		CHECK_THAT(game.frog.position.x, WithinAbs(startX + 60, 1.5));
		CHECK_THAT(game.frog.position.x - game.river.back().position.x, WithinAbs(0, 1.5));

		// and ridden off the side, it is lost
		frames(game, 240);
		CHECK_FALSE(game.frog.inPlay);
		CHECK(game.splats.size() == 1);
	}

	SECTION("the turtles of the third row dive for a while, and drown a frog on them")
	{
		Frogger game;
		tap(game, Key::Num1);
		game.frog.position.y = rowY(7);
		const auto row3 = [&]()
		{
			for (const Log& one : game.river)
			{
				if (static_cast<int>(one.position.y / cell) == 3) { return one.look; }
			}
			return game.river.front().look;
		};
		const auto up = row3();
		frames(game, 200);
		CHECK(row3() == up);
		frames(game, 20);
		const auto down = row3();
		CHECK(down != up);
		frames(game, 92);
		CHECK(row3() == up);

		// a frog on one that is under goes in
		Frogger other = crossing();
		other.frog.position.y = rowY(3);
		Log turtle = under(other, Log{}, 0);
		turtle.look = down;
		other.river.push_back(turtle);
		frames(other, 1);
		CHECK_FALSE(other.frog.inPlay);
		CHECK(other.splashes.size() == 1);

		// and on one that is up, it is safe
		Frogger safe = crossing();
		safe.frog.position.y = rowY(3);
		turtle = under(safe, Log{}, 0);
		turtle.look = up;
		safe.river.push_back(turtle);
		frames(safe, 10);
		CHECK(safe.frog.inPlay);
	}

	SECTION("a frog in an empty bay is home: points, the bay filled, the next frog")
	{
		Frogger game = crossing();
		frames(game, 1);
		game.frog.position.y = rowY(2);
		game.river.push_back(under(game, Log{}, 0));
		tap(game, Key::W);
		frames(game, 1);
		CHECK(game.frog.homes == 1);
		CHECK(game.frog.score == 50 + 10 * 40);
		CHECK(game.pads[2].look == PadLook::Home);
		CHECK(game.frog.inPlay);
		CHECK(game.frog.position.y == rowY(13));
		CHECK(game.frog.lives == 3);
		CHECK(heard(game, Sound::Home));
	}

	SECTION("a bay already filled, or the hedge, loses the frog")
	{
		Frogger game = crossing();
		game.pads[2].look = PadLook::Home;
		game.frog.position.y = rowY(1);
		frames(game, 1);
		CHECK_FALSE(game.frog.inPlay);
		CHECK(game.splats.size() == 1);

		Frogger hedge = crossing();
		hedge.frog.position = { 30, rowY(1) };
		frames(hedge, 1);
		CHECK_FALSE(hedge.frog.inPlay);
		CHECK(hedge.frog.homes == 0);
	}

	SECTION("the clock counts down a second at a time, and at none the frog is lost")
	{
		Frogger game = crossing();
		CHECK(game.clock.time == 40);
		frames(game, 61);
		CHECK(game.clock.time == 39);
		frames(game, 38 * 60);
		CHECK(game.frog.inPlay);
		frames(game, 62);
		CHECK_FALSE(game.frog.inPlay);
		nextFrog(game);
		CHECK(game.frog.lives == 2);
		CHECK(game.clock.time >= 39);
	}

	SECTION("the last life lost is game over, five home a win")
	{
		Frogger game = crossing();
		game.frog.lives = 1;
		game.frog.position.y = rowY(5);
		frames(game, 1);
		nextFrog(game);
		CHECK(game.screen() == Screen::Over);
		CHECK(game.message.look == MessageLook::Lost);
		CHECK(heard(game, Sound::Gameover));

		Frogger won = crossing();
		won.frog.homes = 4;
		won.frog.position.y = rowY(1);
		frames(won, 1);
		CHECK(won.frog.homes == 5);
		frames(won, 1);
		CHECK(won.screen() == Screen::Over);
		CHECK(won.message.look == MessageLook::Won);
		CHECK(heard(won, Sound::Win));
	}

	SECTION("Pause stops the river, the road and the clock until Pause again")
	{
		Frogger game;
		tap(game, Key::Num1);
		frames(game, 10);
		tap(game, Key::Pause);
		CHECK(game.screen() == Screen::Paused);
		const float carX = game.traffic.front().position.x;
		const float time = game.clock.time;
		frames(game, 120);
		CHECK(game.traffic.front().position.x == carX);
		CHECK(game.clock.time == time);
		tap(game, Key::W);
		CHECK(game.frog.position.y == rowY(13));
		tap(game, Key::Pause);
		CHECK(game.screen() == Screen::Playing);
		frames(game, 1);
		CHECK(game.traffic.front().position.x != carX);
	}

	SECTION("Space after the game is a new one, from the title")
	{
		Frogger game = crossing();
		game.pads[0].look = PadLook::Home;
		game.frog.lives = 1;
		game.frog.score = 300;
		game.frog.position.y = rowY(5);
		frames(game, 1);
		nextFrog(game);
		REQUIRE(game.screen() == Screen::Over);
		tap(game, Key::Num1);
		CHECK(game.screen() == Screen::Over);
		tap(game, Key::Space);
		CHECK(game.screen() == Screen::Title);
		CHECK(game.frog.lives == 3);
		CHECK(game.frog.score == 0);
		CHECK(game.pads[0].look == PadLook::Empty);
		CHECK(game.river.size() == 15);
		CHECK(game.traffic.size() == 14);
	}
}
