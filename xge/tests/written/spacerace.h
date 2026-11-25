// spacerace.h
// XML Game Engine
// author: beefviper
// date: Nov 25, 2026
//
// What spacerace.xml written out must do, whoever wrote it, run on the one
// windows-cpp writes (test_written_games.cpp): the title, the debris and its
// wrap, the rockets by keys, a point off the top, a crash by the picture's
// lit pixels, the clock ending the race, the winner or a draw, a pause and
// a new game. It shares no code with the engine, so this checks what
// happens, not every frame.

#pragma once

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <SFML/Window/Keyboard.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

template <typename SpaceRace>
void spacerace_plays()
{
	using Key = sf::Keyboard::Key;
	using Catch::Matchers::WithinAbs;
	using Screen = decltype(std::declval<const SpaceRace&>().screen());
	using Sound = typename decltype(std::declval<SpaceRace&>().takeSounds())::value_type;
	using WinnerLook = decltype(std::declval<SpaceRace&>().winner.look);

	constexpr float width = 800;
	constexpr float height = 600;
	constexpr float step = 150;
	constexpr float pixel = 3; // the rocket's picture is drawn 3 to a pixel
	constexpr float rocketWidth = 5 * pixel;
	constexpr float rocketHeight = 7 * pixel;
	constexpr float startY = height - rocketHeight - 10;
	constexpr float across = 101; // a piece and its padding, (800 + 8) / 8
	constexpr float sixtieth = 1.0f / 60; // a step at 60 frames a second

	const auto tap = [](SpaceRace& game, Key key)
	{
		game.key(key, true);
		game.key(key, false);
	};
	const auto frames = [](SpaceRace& game, int count)
	{
		for (int i = 0; i < count; ++i) { game.frame(sixtieth); }
	};
	const auto heard = [](SpaceRace& game, Sound sound)
	{
		const auto sounds = game.takeSounds();
		return std::find(sounds.begin(), sounds.end(), sound) != sounds.end();
	};
	// Racing, with no debris in the way unless a check puts some there.
	const auto racing = [&]()
	{
		SpaceRace game;
		tap(game, Key::Num1);
		game.debris.clear();
		game.takeSounds();
		return game;
	};
	const auto atStart = [&](const auto& rocket, float x)
	{
		CHECK_THAT(rocket.position.x, WithinAbs(x, 0.01));
		CHECK_THAT(rocket.position.y, WithinAbs(startY, 0.01));
	};
	// Until the clock has sunk out of sight: the frames it took.
	const auto toTheEnd = [&](SpaceRace& game)
	{
		int count = 0;
		while (game.screen() == Screen::Playing && count < 4000)
		{
			game.frame(sixtieth);
			++count;
		}
		return count;
	};

	SECTION("the title waits for 1, and the race is for two")
	{
		SpaceRace game;
		CHECK(game.screen() == Screen::Title);
		frames(game, 30);
		tap(game, Key::Space);
		CHECK(game.screen() == Screen::Title);
		tap(game, Key::Num1);
		CHECK(game.screen() == Screen::Playing);
		CHECK(game.people == 2);
	}

	SECTION("twelve rows of eight pieces, each row at its own speed")
	{
		SpaceRace game;
		REQUIRE(game.debris.size() == 96);
		const float speeds[] = { 90, -150, 120, -60 };
		for (int row = 0; row < 12; ++row)
		{
			for (int column = 0; column < 8; ++column)
			{
				const auto& piece = game.debris[static_cast<std::size_t>(row * 8 + column)];
				CHECK_THAT(piece.position.x, WithinAbs(static_cast<float>(column) * across, 0.01));
				CHECK_THAT(piece.position.y, WithinAbs(50 + static_cast<float>(row) * 32, 0.01));
				CHECK(piece.size.x == 8);
				CHECK(piece.size.y == 4);
				CHECK(piece.velocity.x == speeds[row % 4]);
				CHECK(piece.velocity.y == 0);
			}
		}
	}

	SECTION("a piece gone off one side comes back in at the other")
	{
		SpaceRace game;
		tap(game, Key::Num1);
		auto& right = game.debris[0]; // 90 across
		right.position.x = width - 0.5f;
		auto& left = game.debris[8]; // -150 across
		left.position.x = -7.5f;
		frames(game, 1);
		CHECK_THAT(right.position.x, WithinAbs(width - 0.5f + 1.5f - width - 8, 0.01));
		CHECK_THAT(left.position.x, WithinAbs(-7.5f - 2.5f + width + 8, 0.01));

		// and, however long the race, every piece stays within a piece of the window
		frames(game, 600);
		for (const auto& piece : game.debris)
		{
			CHECK(piece.position.x >= -8);
			CHECK(piece.position.x <= width);
		}
	}

	SECTION("the rockets start at the bottom, a quarter in from each side")
	{
		SpaceRace game;
		CHECK(game.rocket1.size.x == rocketWidth);
		CHECK(game.rocket1.size.y == rocketHeight);
		atStart(game.rocket1, width / 4 - rocketWidth / 2);
		atStart(game.rocket2, width * 3 / 4 - rocketWidth / 2);
		CHECK(game.rocket1.score == 0);
		CHECK(game.rocket2.score == 0);
	}

	SECTION("W and S fly rocket 1, the arrows rocket 2, and the bottom holds them")
	{
		SpaceRace game = racing();
		game.key(Key::W, true);
		game.key(Key::Down, true);
		frames(game, 60);
		CHECK_THAT(game.rocket1.position.y, WithinAbs(startY - step, 0.5));
		CHECK_THAT(game.rocket2.position.y, WithinAbs(height - rocketHeight, 0.01));
		CHECK_THAT(game.rocket1.position.x, WithinAbs(width / 4 - rocketWidth / 2, 0.01));
		game.key(Key::W, false);
		game.key(Key::S, true);
		frames(game, 120);
		CHECK_THAT(game.rocket1.position.y, WithinAbs(height - rocketHeight, 0.01));
		game.key(Key::S, false);
		game.key(Key::Down, false);
		game.key(Key::Up, true);
		frames(game, 30);
		CHECK_THAT(game.rocket2.position.y, WithinAbs(height - rocketHeight - step / 2, 0.5));
	}

	SECTION("off the top is a point, and back to the start")
	{
		SpaceRace game = racing();
		game.rocket2.position.y = 2;
		game.key(Key::Up, true);
		frames(game, 1);
		game.key(Key::Up, false);
		CHECK(game.rocket2.score == 1);
		CHECK(game.rocket1.score == 0);
		CHECK(heard(game, Sound::Score));
		atStart(game.rocket2, width * 3 / 4 - rocketWidth / 2);
	}

	SECTION("debris on a lit pixel sends a rocket back, its points kept")
	{
		SpaceRace game = racing();
		game.rocket1.score = 3;
		game.rocket1.position.y = 300;

		// the picture's top left pixel is dark: a piece there misses
		game.debris.push_back({ { game.rocket1.position.x - 6, 300 - 2 }, {}, { 8, 4 } });
		frames(game, 1);
		CHECK_THAT(game.rocket1.position.y, WithinAbs(300, 0.01));
		CHECK_FALSE(heard(game, Sound::Crash));

		// its nose is lit
		game.debris.back().position = { game.rocket1.position.x + 2 * pixel, 300 };
		frames(game, 1);
		CHECK(heard(game, Sound::Crash));
		atStart(game.rocket1, width / 4 - rocketWidth / 2);
		CHECK(game.rocket1.score == 3);
		atStart(game.rocket2, width * 3 / 4 - rocketWidth / 2);
	}

	SECTION("the clock sinks out of sight in a minute, and the race is over")
	{
		SpaceRace game = racing();
		CHECK_THAT(game.clock.position.y, WithinAbs(0, 0.01));
		CHECK_THAT(game.clock.size.y, WithinAbs(height, 0.01));
		frames(game, 1800);
		CHECK_THAT(game.clock.position.y, WithinAbs(height / 2, 0.5));
		game.takeSounds();
		const int more = toTheEnd(game);
		CHECK(std::abs(more - 1800) <= 2);
		CHECK(game.screen() == Screen::Over);
		CHECK(heard(game, Sound::End));
	}

	SECTION("the most points win, and the same is a draw")
	{
		SpaceRace game = racing();
		toTheEnd(game);
		frames(game, 2);
		CHECK(game.winner.look == WinnerLook::Draw);

		game = racing();
		game.rocket1.score = 4;
		game.rocket2.score = 2;
		toTheEnd(game);
		frames(game, 1);
		CHECK(game.winner.look == WinnerLook::Player1);

		game = racing();
		game.rocket1.score = 1;
		game.rocket2.score = 5;
		toTheEnd(game);
		frames(game, 1);
		CHECK(game.winner.look == WinnerLook::Player2);

		// and nothing moves once it is over
		const float clockY = game.clock.position.y;
		frames(game, 60);
		CHECK(game.clock.position.y == clockY);
	}

	SECTION("Pause stops the race until Pause again")
	{
		SpaceRace game = racing();
		game.key(Key::W, true);
		frames(game, 10);
		tap(game, Key::Pause);
		CHECK(game.screen() == Screen::Paused);
		const float clockY = game.clock.position.y;
		const float rocketY = game.rocket1.position.y;
		frames(game, 60);
		CHECK(game.clock.position.y == clockY);
		CHECK(game.rocket1.position.y == rocketY);
		tap(game, Key::Pause);
		CHECK(game.screen() == Screen::Playing);
		frames(game, 1);
		CHECK(game.rocket1.position.y < rocketY);
	}

	SECTION("Space after the race is a new game, from the title")
	{
		SpaceRace game = racing();
		game.rocket1.score = 2;
		toTheEnd(game);
		frames(game, 1);
		REQUIRE(game.winner.look == WinnerLook::Player1);
		tap(game, Key::Num1);
		CHECK(game.screen() == Screen::Over);
		tap(game, Key::Space);
		CHECK(game.screen() == Screen::Title);
		CHECK(game.rocket1.score == 0);
		CHECK(game.winner.look == WinnerLook::Draw);
		CHECK_THAT(game.clock.position.y, WithinAbs(0, 0.01));
		CHECK(game.debris.size() == 96);
	}
}
