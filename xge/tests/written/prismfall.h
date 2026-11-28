// prismfall.h
// XML Game Engine
// author: beefviper
// date: Nov 28, 2026
//
// What prismfall.xml written out must do, whoever wrote it, run on the one
// windows-cpp writes (test_written_games.cpp): the title, the lens by keys,
// one beam at a time, the lattice stepping and humming and turning at the
// sides, a shard broken, a hole in a pylon, sparks, the lens cracked and back,
// the comet, the speed-up, the endings, a pause and a new game. It shares no
// code with the engine, so this checks what happens, not every frame.

#pragma once

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <SFML/Window/Keyboard.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

template <typename PrismFall>
void prismfall_plays()
{
	using Key = sf::Keyboard::Key;
	using Catch::Matchers::WithinAbs;
	using Screen = decltype(std::declval<const PrismFall&>().screen());
	using Sound = typename decltype(std::declval<PrismFall&>().takeSounds())::value_type;
	using Beam = typename decltype(std::declval<PrismFall&>().beams)::value_type;
	using Spark = typename decltype(std::declval<PrismFall&>().sparks)::value_type;
	using ShardLook = typename decltype(std::declval<PrismFall&>().shards)::value_type::Look;
	using MessageLook = decltype(std::declval<PrismFall&>().message.look);

	constexpr float width = 960;
	constexpr float step = 150;
	constexpr float sixtieth = 1.0f / 60; // a step at 60 frames a second
	constexpr float lensX = width / 2 - 27.5f;
	constexpr float lensY = 720 - 70;

	const auto tap = [](PrismFall& game, Key key)
	{
		game.key(key, true);
		game.key(key, false);
	};
	const auto frames = [](PrismFall& game, int count)
	{
		for (int i = 0; i < count; ++i) { game.frame(sixtieth); }
	};
	const auto heard = [](PrismFall& game, Sound sound)
	{
		const auto sounds = game.takeSounds();
		return std::find(sounds.begin(), sounds.end(), sound) != sounds.end();
	};
	// Playing, with no shard due to shed a spark for a long while.
	const auto playing = [&]()
	{
		PrismFall game;
		tap(game, Key::Num1);
		for (auto& shard : game.shards) { shard.untilFire = 1000; }
		game.takeSounds();
		return game;
	};
	const auto beamAt = [](float x, float y)
	{
		Beam beam;
		beam.position = { x, y };
		beam.velocity = { 0, -780 };
		beam.size = { 3, 20 };
		return beam;
	};
	const auto sparkAt = [](float x, float y)
	{
		Spark spark;
		spark.position = { x, y };
		spark.velocity = { 0, 260 };
		spark.size = { 9, 12 };
		return spark;
	};
	const auto lit = [](const auto& rows)
	{
		std::size_t count = 0;
		for (const auto& row : rows) { count += static_cast<std::size_t>(std::count(row.begin(), row.end(), '*')); }
		return count;
	};

	SECTION("the title waits for 1")
	{
		PrismFall game;
		CHECK(game.screen() == Screen::Title);
		frames(game, 30);
		tap(game, Key::Space);
		CHECK(game.screen() == Screen::Title);
		tap(game, Key::Num1);
		CHECK(game.screen() == Screen::Playing);
	}

	SECTION("a lattice of 36 shards, their looks and worth by row, over three pylons and the lens")
	{
		PrismFall game;
		REQUIRE(game.shards.size() == 36);
		CHECK(game.shards[0].look == ShardLook::Diamond);
		CHECK(game.shards[0].worth == 40);
		CHECK(game.shards[9].look == ShardLook::Ring);
		CHECK(game.shards[9].worth == 25);
		CHECK(game.shards[18].look == ShardLook::Cross);
		CHECK(game.shards[35].worth == 15);
		CHECK_THAT(game.shards[0].position.x, WithinAbs(120, 0.01));
		CHECK_THAT(game.shards[0].position.y, WithinAbs(110, 0.01));
		CHECK_THAT(game.shards[10].position.x, WithinAbs(185, 0.01));
		CHECK_THAT(game.shards[10].position.y, WithinAbs(170, 0.01));
		CHECK(game.pylons.size() == 3);
		CHECK_THAT(game.lens.position.x, WithinAbs(lensX, 0.01));
		CHECK_THAT(game.lens.position.y, WithinAbs(lensY, 0.01));
		CHECK(game.lens.lives == 3);
		CHECK(game.lens.score == 0);
		CHECK_FALSE(game.comet.inPlay);
	}

	SECTION("A and D or the arrows move the lens, and the sides hold it")
	{
		PrismFall game = playing();
		game.key(Key::A, true);
		frames(game, 30);
		CHECK_THAT(game.lens.position.x, WithinAbs(lensX - step / 2, 0.5));
		frames(game, 300);
		CHECK_THAT(game.lens.position.x, WithinAbs(0, 0.01));
		game.key(Key::A, false);
		game.key(Key::Right, true);
		frames(game, 600);
		CHECK_THAT(game.lens.position.x, WithinAbs(width - 55, 0.01));
	}

	SECTION("space shines one beam up from the lens, never two at once")
	{
		PrismFall game = playing();
		tap(game, Key::Space);
		REQUIRE(game.beams.size() == 1);
		CHECK(heard(game, Sound::Beam));
		CHECK_THAT(game.beams[0].position.x, WithinAbs(width / 2 - 1.5f, 0.01));
		CHECK_THAT(game.beams[0].position.y, WithinAbs(lensY, 0.01));
		tap(game, Key::Space);
		CHECK(game.beams.size() == 1);
		CHECK_FALSE(heard(game, Sound::Beam));
		frames(game, 3);
		REQUIRE(game.beams.size() == 1);
		CHECK_THAT(game.beams[0].position.y, WithinAbs(lensY - 39, 0.5));
	}

	SECTION("the lattice steps on together every beat, humming, and at a side turns and drops")
	{
		PrismFall game = playing();
		frames(game, 30);
		CHECK_THAT(game.shards[0].position.x, WithinAbs(120, 0.01));
		frames(game, 7);
		CHECK_THAT(game.shards[0].position.x, WithinAbs(120 + 36 * 0.6f, 0.01));
		CHECK_THAT(game.shards[35].position.x, WithinAbs(640 + 36 * 0.6f, 0.01));
		CHECK(heard(game, Sound::Hum2));
		frames(game, 36);
		CHECK(heard(game, Sound::Hum1));

		// the right-hand column a step from the side
		for (auto& shard : game.shards) { shard.position.x += 270 - 36 * 1.2f; }
		const float top = game.shards[0].position.y;
		frames(game, 36);
		CHECK(game.shards[0].velocity.x < 0);
		CHECK(game.shards[35].velocity.x < 0);
		CHECK_THAT(game.shards[0].position.y, WithinAbs(top + 12, 0.01));
		CHECK(game.shards[8].position.x <= width - 45);
	}

	SECTION("a beam breaks a shard for its worth, with a flash for a moment")
	{
		PrismFall game = playing();
		const auto ring = game.shards[9];
		game.beams.push_back(beamAt(ring.position.x + 20, ring.position.y + 30));
		game.frame(sixtieth);
		CHECK(game.shards.size() == 35);
		CHECK(game.beams.empty());
		CHECK(game.lens.score == 25);
		CHECK(game.flashes.size() == 1);
		CHECK(heard(game, Sound::Shatter));
		frames(game, 15);
		CHECK(game.flashes.empty());
	}

	SECTION("a beam into a pylon cuts a hole in it, and a spark does too")
	{
		PrismFall game = playing();
		const auto pylon = game.pylons[0];
		game.beams.push_back(beamAt(pylon.position.x + 6, pylon.position.y + pylon.size.y - 6));
		game.frame(sixtieth);
		CHECK(game.beams.empty());
		CHECK(lit(game.pylons[0].picture) < lit(game.pylons[1].picture));
		CHECK(game.lens.score == 0);

		const auto other = game.pylons[2];
		game.sparks.push_back(sparkAt(other.position.x + 2, other.position.y - 8));
		game.frame(sixtieth);
		CHECK(game.sparks.empty());
		CHECK(lit(game.pylons[2].picture) < lit(game.pylons[1].picture));
	}

	SECTION("a shard sheds a spark below it when its wait is up, four at most")
	{
		PrismFall game = playing();
		game.shards[30].untilFire = 0.001f;
		game.frame(sixtieth);
		REQUIRE(game.sparks.size() == 1);
		CHECK(game.sparks[0].velocity.y == 260);
		CHECK(game.sparks[0].position.y >= game.shards[30].position.y + game.shards[30].size.y);
		for (std::size_t i = 0; i < 8; ++i) { game.shards[i].untilFire = 0.001f; }
		game.frame(sixtieth);
		CHECK(game.sparks.size() == 4);
	}

	SECTION("a spark on the lens cracks it: a life gone, a second's hold, then back in the middle")
	{
		PrismFall game = playing();
		game.key(Key::A, true);
		frames(game, 20);
		game.key(Key::A, false);
		game.sparks.push_back(sparkAt(game.lens.position.x + 20, game.lens.position.y + 15));
		game.sparks.push_back(sparkAt(100, 300));
		game.frame(sixtieth);
		CHECK(game.lens.lives == 2);
		CHECK_FALSE(game.lens.inPlay);
		CHECK(heard(game, Sound::Crack));
		CHECK(game.screen() == Screen::Cracked);
		const float x = game.shards[0].position.x;
		frames(game, 50);
		CHECK(game.screen() == Screen::Cracked);
		CHECK(game.shards[0].position.x == x);
		frames(game, 12);
		CHECK(game.screen() == Screen::Playing);
		CHECK(game.lens.inPlay);
		CHECK(game.sparks.empty());
		CHECK_THAT(game.lens.position.x, WithinAbs(lensX, 0.01));
	}

	SECTION("the comet comes by now and then, worth 60 to 240 to a beam")
	{
		PrismFall game = playing();
		bool came = false;
		for (int i = 0; i < 26 * 60 && !came; ++i)
		{
			game.frame(sixtieth);
			came = game.comet.inPlay;
		}
		REQUIRE(came);
		CHECK(heard(game, Sound::Comet));
		CHECK(game.comet.velocity.x == 180);
		frames(game, 60);
		game.beams.push_back(beamAt(game.comet.position.x + 20, game.comet.position.y + 10));
		game.frame(sixtieth);
		CHECK_FALSE(game.comet.inPlay);
		CHECK(game.lens.score >= 60);
		CHECK(game.lens.score <= 240);
		CHECK(std::fmod(game.lens.score, 60.0f) == 0);
	}

	SECTION("down to half the shards, the lattice steps faster and further")
	{
		PrismFall game = playing();
		game.shards.resize(19);
		game.frame(sixtieth);
		CHECK(game.shards[0].velocity.x == 36);
		game.shards.pop_back();
		game.frame(sixtieth);
		CHECK_THAT(game.shards[0].velocity.x, WithinAbs(45, 0.01));
		CHECK_THAT(game.shards[17].velocity.x, WithinAbs(45, 0.01));
		CHECK_THAT(game.pace.beat, WithinAbs(0.48, 0.001));
		CHECK(game.pace.next == 9);
		game.frame(sixtieth);
		CHECK_THAT(game.shards[0].velocity.x, WithinAbs(45, 0.01));
	}

	SECTION("every shard broken is a win; the last life, or the lattice on the pylons, a loss")
	{
		PrismFall won = playing();
		won.shards.clear();
		won.frame(sixtieth);
		CHECK(won.screen() == Screen::Over);
		CHECK(won.message.look == MessageLook::Won);

		PrismFall lost = playing();
		lost.lens.lives = 1;
		lost.sparks.push_back(sparkAt(lensX + 20, lensY + 15));
		lost.frame(sixtieth);
		CHECK(lost.screen() == Screen::Over);
		CHECK(lost.message.look == MessageLook::Lost);

		PrismFall landed = playing();
		landed.shards[20].position.y = 460;
		landed.frame(sixtieth);
		CHECK(landed.screen() == Screen::Over);
		CHECK(landed.message.look == MessageLook::Lost);
	}

	SECTION("the best score follows a higher one and outlasts the game")
	{
		PrismFall game = playing();
		game.lens.score = 140;
		game.frame(sixtieth);
		CHECK(game.best.score == 140);
		game.shards.clear();
		game.frame(sixtieth);
		REQUIRE(game.screen() == Screen::Over);
		tap(game, Key::Space);
		CHECK(game.screen() == Screen::Title);
		CHECK(game.best.score == 140);
		CHECK(game.lens.score == 0);
		CHECK(game.lens.lives == 3);
		CHECK(game.shards.size() == 36);
	}

	SECTION("pause holds everything until pressed again")
	{
		PrismFall game = playing();
		tap(game, Key::Pause);
		CHECK(game.screen() == Screen::Paused);
		const float x = game.shards[0].position.x;
		frames(game, 120);
		CHECK(game.shards[0].position.x == x);
		tap(game, Key::Pause);
		CHECK(game.screen() == Screen::Playing);
	}
}
