// snake.h
// XML Game Engine
// author: beefviper
// date: Nov 28, 2026
//
// What snake.xml written out must do, whoever wrote it, run on the one
// windows-cpp writes (test_written_games.cpp): the title, the field as its
// map draws it, off to the right at once, a cell a step, a press turning it
// but never straight back, food eaten (longer, faster, ten points, the next
// in a free cell), the wall and its own body ending the game while its tail
// moves out of the way, a pause and a new game. It shares no code with the
// engine, so this checks what happens, not every frame.

#pragma once

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <SFML/Window/Keyboard.hpp>

#include <algorithm>
#include <cstddef>
#include <utility>

template <typename Snake>
void snake_plays()
{
	using Key = sf::Keyboard::Key;
	using Catch::Matchers::WithinAbs;
	using Screen = decltype(std::declval<const Snake&>().screen());
	using Sound = typename decltype(std::declval<Snake&>().takeSounds())::value_type;
	using Spot = std::remove_cvref_t<decltype(std::declval<Snake&>().head.spot)>;

	constexpr float sixtieth = 1.0f / 60;

	const auto tap = [](Snake& game, Key key)
	{
		game.key(key, true);
		game.key(key, false);
	};
	const auto heard = [](Snake& game, Sound sound)
	{
		const auto sounds = game.takeSounds();
		return std::find(sounds.begin(), sounds.end(), sound) != sounds.end();
	};
	// One step's time: the head a cell on.
	const auto step = [](Snake& game, int count = 1)
	{
		for (int i = 0; i < count; ++i) { game.frame(game.head.every); }
	};
	const auto has = [](const auto& spots, Spot spot) { return std::find(spots.begin(), spots.end(), spot) != spots.end(); };
	// Playing, the head set off right on the first frame.
	const auto playing = [&]()
	{
		Snake game(7);
		tap(game, Key::Num1);
		game.frame(sixtieth);
		game.takeSounds();
		return game;
	};

	SECTION("the title waits for 1")
	{
		Snake game;
		CHECK(game.screen() == Screen::Title);
		game.frame(1);
		tap(game, Key::Space);
		CHECK(game.screen() == Screen::Title);
		tap(game, Key::Num1);
		CHECK(game.screen() == Screen::Playing);
	}

	SECTION("the field as its map draws it: a wall round it, three links behind the head, one food")
	{
		Snake game;
		CHECK(game.walls.size() == 32 * 2 + 20 * 2);
		CHECK(has(game.walls, Spot{ 0, 0 }));
		CHECK(has(game.walls, Spot{ 31, 21 }));
		CHECK(game.head.spot == Spot{ 9, 11 });
		REQUIRE(game.body.size() == 3);
		CHECK(game.body[0] == Spot{ 8, 11 });
		CHECK(game.body[2] == Spot{ 6, 11 });
		REQUIRE(game.food.size() == 1);
		CHECK(game.food[0] == Spot{ 22, 11 });
		CHECK(game.status.score == 0);
	}

	SECTION("off to the right at once, a cell every eighth of a second, the body after it")
	{
		Snake game = playing();
		CHECK(game.head.going);
		CHECK_THAT(game.head.every, WithinAbs(0.125, 0.0001));
		const int column = game.head.spot.column;
		step(game, 3);
		CHECK(game.head.spot == Spot{ column + 3, 11 });
		CHECK(game.body[0] == Spot{ column + 2, 11 });
		CHECK(game.body.size() == 3);

		// at 60 frames a second, 8 steps a second
		const Spot before = game.head.spot;
		for (int i = 0; i < 60; ++i) { game.frame(sixtieth); }
		CHECK(game.head.spot.column - before.column >= 7);
		CHECK(game.head.spot.column - before.column <= 8);
	}

	SECTION("a press turns it, but never straight back")
	{
		Snake game = playing();
		tap(game, Key::A);
		step(game);
		CHECK(game.head.way == Spot{ 1, 0 });
		const Spot at = game.head.spot;
		tap(game, Key::W);
		step(game);
		CHECK(game.head.spot == Spot{ at.column, at.row - 1 });
		tap(game, Key::Down);
		step(game);
		CHECK(game.head.spot == Spot{ at.column, at.row - 2 });
		tap(game, Key::Left);
		step(game);
		CHECK(game.head.spot == Spot{ at.column - 1, at.row - 2 });
	}

	SECTION("food eaten: a link longer, faster, ten points, and the next in a free cell")
	{
		Snake game = playing();
		while (game.head.spot.column < 21) { step(game); }
		game.takeSounds();
		step(game);
		CHECK(game.head.spot == Spot{ 22, 11 });
		CHECK(game.status.score == 10);
		CHECK(game.body.size() == 4);
		CHECK_THAT(game.status.speed, WithinAbs(8.25, 0.0001));
		CHECK(heard(game, Sound::Eat));
		REQUIRE(game.food.size() == 1);
		const Spot food = game.food[0];
		CHECK(food != game.head.spot);
		CHECK_FALSE(has(game.walls, food));
		CHECK_FALSE(has(game.body, food));

		// the next turn goes at the new speed
		tap(game, Key::S);
		CHECK_THAT(game.head.every, WithinAbs(1 / 8.25, 0.0001));
	}

	SECTION("the wall ends the game")
	{
		Snake game = playing();
		tap(game, Key::W);
		int steps = 0;
		while (game.screen() == Screen::Playing && steps < 40)
		{
			step(game);
			++steps;
		}
		CHECK(game.screen() == Screen::Over);
		CHECK(game.head.spot.row == 1);
		CHECK(game.status.over == 1);
		CHECK(heard(game, Sound::Over));
	}

	SECTION("its own body ends the game, but its tail moves out of the way")
	{
		// three links: up, left, down comes round onto the tail as it leaves
		Snake game = playing();
		tap(game, Key::W);
		step(game);
		tap(game, Key::A);
		step(game);
		tap(game, Key::S);
		step(game);
		CHECK(game.screen() == Screen::Playing);
		CHECK(game.status.over == 0);

		// four links: the same is a bite
		Snake longer = playing();
		while (longer.head.spot.column < 22) { step(longer); }
		REQUIRE(longer.body.size() == 4);
		longer.food.clear();
		tap(longer, Key::W);
		step(longer);
		tap(longer, Key::A);
		step(longer);
		tap(longer, Key::S);
		step(longer);
		CHECK(longer.screen() == Screen::Over);
	}

	SECTION("Pause stops it, and again goes on")
	{
		Snake game = playing();
		tap(game, Key::Pause);
		CHECK(game.screen() == Screen::Paused);
		const Spot at = game.head.spot;
		for (int i = 0; i < 60; ++i) { game.frame(sixtieth); }
		CHECK(game.head.spot == at);
		tap(game, Key::Pause);
		CHECK(game.screen() == Screen::Playing);
		step(game);
		CHECK(game.head.spot == Spot{ at.column + 1, at.row });
	}

	SECTION("Space after the end is a new game, from the title")
	{
		Snake game = playing();
		game.status.score = 30;
		tap(game, Key::W);
		while (game.screen() == Screen::Playing) { step(game); }
		tap(game, Key::Space);
		CHECK(game.screen() == Screen::Title);
		CHECK(game.status.score == 0);
		CHECK(game.head.spot == Spot{ 9, 11 });
		CHECK_FALSE(game.head.going);
		CHECK(game.body.size() == 3);
		tap(game, Key::Num1);
		game.frame(sixtieth);
		CHECK(game.head.going);
	}
}
