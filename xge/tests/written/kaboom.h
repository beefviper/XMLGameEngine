// kaboom.h
// XML Game Engine
// author: beefviper
// date: Nov 25, 2026
//
// What kaboom.xml written out must do, whoever wrote it, run on the one
// windows-cpp writes (test_written_games.cpp): the title, the bucket by keys
// and mouse, the bomber pacing and turning, his bombs every so often from
// under him, a catch, a bomb on the ground taking a bucket and every bomb,
// the waves one after another, losing, winning, a pause and a new game. It
// shares no code with the engine, so this checks what happens, not every
// frame.

#pragma once

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <SFML/Window/Keyboard.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

template <typename Kaboom>
void kaboom_plays()
{
	using Key = sf::Keyboard::Key;
	using Catch::Matchers::WithinAbs;
	using Screen = decltype(std::declval<const Kaboom&>().screen());
	using Sound = typename decltype(std::declval<Kaboom&>().takeSounds())::value_type;
	using Bomb = typename decltype(std::declval<Kaboom&>().bombs)::value_type;
	using BucketLook = decltype(std::declval<Kaboom&>().bucket.look);
	using MessageLook = decltype(std::declval<Kaboom&>().message.look);

	constexpr float width = 800;
	constexpr float height = 600;
	constexpr float step = 540;
	constexpr float roof = 104;
	constexpr float sixtieth = 1.0f / 60; // a step at 60 frames a second

	const auto tap = [](Kaboom& game, Key key)
	{
		game.key(key, true);
		game.key(key, false);
	};
	const auto frames = [](Kaboom& game, int count)
	{
		for (int i = 0; i < count; ++i) { game.frame(sixtieth); }
	};
	const auto heard = [](Kaboom& game, Sound sound)
	{
		const auto sounds = game.takeSounds();
		return std::find(sounds.begin(), sounds.end(), sound) != sounds.end();
	};
	// Playing, with no bomb falling and none due for a long while.
	const auto playing = [&]()
	{
		Kaboom game;
		tap(game, Key::Num1);
		game.bombs.clear();
		game.bomber.untilFire = 100;
		game.takeSounds();
		return game;
	};
	const auto bombAt = [](float x, float y)
	{
		Bomb bomb;
		bomb.position = { x, y };
		bomb.velocity = { 0, 150 };
		bomb.size = { 24, 24 };
		return bomb;
	};

	SECTION("the title waits for 1, and the first wave starts with its sound")
	{
		Kaboom game;
		CHECK(game.screen() == Screen::Title);
		frames(game, 30);
		tap(game, Key::Space);
		CHECK(game.screen() == Screen::Title);
		tap(game, Key::Num1);
		CHECK(game.screen() == Screen::Playing);
		CHECK(game.wave().wave == 1);
		CHECK(heard(game, Sound::Wave));
	}

	SECTION("a stack of three buckets at the bottom, the bomber on the roof")
	{
		Kaboom game;
		CHECK(game.bucket.size.x == 110);
		CHECK(game.bucket.size.y == 55);
		CHECK_THAT(game.bucket.position.x, WithinAbs(width / 2 - 55, 0.01));
		CHECK_THAT(game.bucket.position.y, WithinAbs(420, 0.01));
		CHECK(game.bucket.look == BucketLook::Three);
		CHECK(game.bucket.buckets == 3);
		CHECK(game.bucket.score == 0);
		CHECK(game.bomber.size.x == 48);
		CHECK(game.bomber.size.y == 56);
		CHECK_THAT(game.bomber.position.x, WithinAbs(width / 2 - 24, 0.01));
		CHECK_THAT(game.bomber.position.y, WithinAbs(roof - 56, 0.01));
		CHECK(std::abs(game.bomber.velocity.x) == 150);
		CHECK(game.bombs.empty());
	}

	SECTION("A and D or the arrows move the buckets, and the sides hold them")
	{
		Kaboom game = playing();
		game.key(Key::A, true);
		frames(game, 30);
		CHECK_THAT(game.bucket.position.x, WithinAbs(width / 2 - 55 - step / 2, 0.5));
		frames(game, 60);
		CHECK_THAT(game.bucket.position.x, WithinAbs(0, 0.01));
		game.key(Key::A, false);
		game.key(Key::Right, true);
		frames(game, 120);
		CHECK_THAT(game.bucket.position.x, WithinAbs(width - 110, 0.01));
	}

	SECTION("the mouse puts the buckets' middle under it, inside the window")
	{
		Kaboom game = playing();
		game.mouse(300);
		CHECK_THAT(game.bucket.position.x, WithinAbs(300 - 55, 0.01));
		game.mouse(10);
		CHECK_THAT(game.bucket.position.x, WithinAbs(0, 0.01));
		game.mouse(width);
		CHECK_THAT(game.bucket.position.x, WithinAbs(width - 110, 0.01));
	}

	SECTION("the bomber paces the roof, turning when he likes and at the walls")
	{
		Kaboom game = playing();
		int turns = 0;
		float way = game.bomber.velocity.x;
		for (int i = 0; i < 600; ++i)
		{
			game.frame(sixtieth);
			CHECK(std::abs(game.bomber.velocity.x) == 150);
			// a step or two past a wall at most, as he turns there
			CHECK(game.bomber.position.x >= -6);
			CHECK(game.bomber.position.x <= width - 48 + 6);
			CHECK_THAT(game.bomber.position.y, WithinAbs(roof - 56, 0.01));
			if (game.bomber.velocity.x != way) { ++turns; }
			way = game.bomber.velocity.x;
		}
		// ten seconds of a turn at most every 1.4
		CHECK(turns >= 7);
	}

	SECTION("a bomb every drop seconds from under him, falling, ten at most")
	{
		Kaboom game;
		tap(game, Key::Num1);
		int count = 0;
		while (game.bombs.empty() && count < 100)
		{
			game.frame(sixtieth);
			++count;
		}
		CHECK(std::abs(count - 42) <= 1);
		REQUIRE(game.bombs.size() == 1);
		const auto& bomb = game.bombs.front();
		CHECK(bomb.size.x == 24);
		CHECK(bomb.size.y == 24);
		CHECK(bomb.velocity.x == 0);
		CHECK(bomb.velocity.y == 150);
		// from his middle, below him, moved on by the step
		CHECK(std::abs(bomb.position.x + 12 - (game.bomber.position.x + 24)) <= 3);
		CHECK_THAT(bomb.position.y, WithinAbs(roof + 150 * sixtieth, 0.5));

		// held up high, they pile to ten and no more
		for (int i = 0; i < 1200; ++i)
		{
			game.frame(sixtieth);
			for (auto& each : game.bombs) { each.position.y = 200; }
		}
		CHECK(game.bombs.size() == 10);
	}

	SECTION("a bomb in the buckets is the wave's points, and gone")
	{
		Kaboom game = playing();
		game.bombs.push_back(bombAt(game.bucket.position.x + 40, game.bucket.position.y - 20));
		frames(game, 1);
		CHECK(game.bucket.score == 1);
		CHECK(game.bombs.empty());
		CHECK(heard(game, Sound::Catch));
	}

	SECTION("a bomb on the ground sets them all off, and takes a bucket")
	{
		Kaboom game = playing();
		game.bucket.position.x = 0;
		game.bombs.push_back(bombAt(400, 300));
		game.bombs.push_back(bombAt(600, height - 25));
		game.bombs.push_back(bombAt(500, 200));
		frames(game, 1);
		CHECK(game.bombs.empty());
		CHECK(heard(game, Sound::Boom));
		CHECK(game.bucket.buckets == 2);
		CHECK(game.bucket.look == BucketLook::Two);
		CHECK(game.bucket.size.y == 35);
		CHECK_THAT(game.bucket.position.y, WithinAbs(420, 0.01));

		game.bombs.push_back(bombAt(600, height - 25));
		frames(game, 1);
		CHECK(game.bucket.look == BucketLook::One);
		CHECK(game.bucket.size.y == 15);
		CHECK(game.screen() == Screen::Playing);

		// the last one gone: game over
		game.bombs.push_back(bombAt(600, height - 25));
		frames(game, 1);
		CHECK(game.bucket.buckets == 0);
		CHECK(game.screen() == Screen::Over);
		CHECK(game.message.look == MessageLook::Lost);
		CHECK(heard(game, Sound::Gameover));
		// and nothing moves once it is over
		const float x = game.bomber.position.x;
		frames(game, 30);
		CHECK(game.bomber.position.x == x);
	}

	SECTION("the wave's score is the next wave: a quicker bomber, worth more")
	{
		Kaboom game = playing();
		game.bucket.score = 9;
		game.bombs.push_back(bombAt(600, 300));
		game.bombs.push_back(bombAt(game.bucket.position.x + 40, game.bucket.position.y - 20));
		frames(game, 1);
		CHECK(game.bucket.score == 10);
		CHECK(game.wave().wave == 2);
		CHECK(game.bombs.empty());
		CHECK(heard(game, Sound::Wave));
		CHECK(std::abs(game.bomber.velocity.x) == 180);
		CHECK_THAT(game.bomber.position.x, WithinAbs(width / 2 - 24, 0.01));
		game.bomber.untilFire = 100;
		game.bombs.push_back(bombAt(game.bucket.position.x + 40, game.bucket.position.y - 20));
		frames(game, 1);
		CHECK(game.bucket.score == 12);
	}

	SECTION("through the eighth wave is a win")
	{
		Kaboom game = playing();
		const int paces[] = { 150, 180, 210, 240, 270, 300, 330, 360 };
		for (int wave = 1; wave < 8; ++wave)
		{
			CHECK(game.wave().wave == wave);
			CHECK(std::abs(game.bomber.velocity.x) == static_cast<float>(paces[wave - 1]));
			game.bucket.score = game.wave().upto;
			frames(game, 1);
		}
		CHECK(game.wave().wave == 8);
		CHECK(game.wave().upto == 2900);
		CHECK(game.screen() == Screen::Playing);
		game.takeSounds();
		game.bucket.score = 2900;
		frames(game, 1);
		CHECK(game.screen() == Screen::Over);
		CHECK(game.message.look == MessageLook::Won);
		CHECK(heard(game, Sound::Wave));
	}

	SECTION("Pause stops the game until Pause again")
	{
		Kaboom game = playing();
		game.key(Key::D, true);
		frames(game, 5);
		tap(game, Key::Pause);
		CHECK(game.screen() == Screen::Paused);
		const float bucketX = game.bucket.position.x;
		const float bomberX = game.bomber.position.x;
		frames(game, 30);
		game.mouse(10);
		CHECK(game.bucket.position.x == bucketX);
		CHECK(game.bomber.position.x == bomberX);
		tap(game, Key::Pause);
		CHECK(game.screen() == Screen::Playing);
		frames(game, 1);
		CHECK(game.bucket.position.x > bucketX);
	}

	SECTION("Space after the end is a new game, from the title")
	{
		Kaboom game = playing();
		game.bucket.score = 30;
		game.bucket.buckets = 1;
		game.bucket.look = BucketLook::One;
		game.bombs.push_back(bombAt(600, height - 25));
		frames(game, 1);
		REQUIRE(game.screen() == Screen::Over);
		tap(game, Key::Num1);
		CHECK(game.screen() == Screen::Over);
		tap(game, Key::Space);
		CHECK(game.screen() == Screen::Title);
		CHECK(game.bucket.score == 0);
		CHECK(game.bucket.buckets == 3);
		CHECK(game.bucket.look == BucketLook::Three);
		CHECK(game.bucket.size.y == 55);
		CHECK(game.wave().wave == 1);
		CHECK(game.bombs.empty());
	}
}
