// freeway.h
// XML Game Engine
// author: beefviper
// date: Nov 27, 2026
//
// What freeway.xml written out must do, whoever wrote it, run on the one
// windows-cpp writes (test_written_games.cpp): the title, the ten lanes and
// their wrap, a hop a press, the window refusing a hop, a hit knocking a
// chicken back a lane, a crossing scoring and starting again, the clock,
// the winner or a draw, a pause and a new game. It shares no code with the
// engine, so this checks what happens, not every frame.

#pragma once

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <SFML/Window/Keyboard.hpp>

#include <algorithm>
#include <cstddef>
#include <utility>

template <typename Freeway>
void freeway_plays()
{
	using Key = sf::Keyboard::Key;
	using Catch::Matchers::WithinAbs;
	using Screen = decltype(std::declval<const Freeway&>().screen());
	using Sound = typename decltype(std::declval<Freeway&>().takeSounds())::value_type;
	using WinnerLook = decltype(std::declval<Freeway&>().winner.look);

	constexpr float width = 800;
	constexpr float cell = 50;
	constexpr float chickenSize = 32; // 8 pixels drawn 4 to a pixel
	constexpr float sixtieth = 1.0f / 60; // a step at 60 frames a second
	// where a chicken stands in a row: 9 down from the row's top
	const auto rowY = [](int row) { return static_cast<float>(row) * cell + 9; };

	const auto tap = [](Freeway& game, Key key)
	{
		game.key(key, true);
		game.key(key, false);
	};
	const auto frames = [](Freeway& game, int count)
	{
		for (int i = 0; i < count; ++i) { game.frame(sixtieth); }
	};
	const auto heard = [](Freeway& game, Sound sound)
	{
		const auto sounds = game.takeSounds();
		return std::find(sounds.begin(), sounds.end(), sound) != sounds.end();
	};
	// Crossing, with no traffic unless a check puts some there.
	const auto crossing = [&]()
	{
		Freeway game;
		tap(game, Key::Num1);
		game.traffic.clear();
		game.takeSounds();
		return game;
	};
	// Until the clock runs out: the frames it took.
	const auto toTheEnd = [&](Freeway& game)
	{
		int count = 0;
		while (game.screen() == Screen::Playing && count < 9000)
		{
			game.frame(sixtieth);
			++count;
		}
		return count;
	};

	SECTION("the title waits for 1, and the crossing is for two")
	{
		Freeway game;
		CHECK(game.screen() == Screen::Title);
		frames(game, 30);
		tap(game, Key::Space);
		CHECK(game.screen() == Screen::Title);
		tap(game, Key::Num1);
		CHECK(game.screen() == Screen::Playing);
		CHECK(game.people == 2);
		CHECK(heard(game, Sound::Start));
	}

	SECTION("ten lanes, a row each, the top five driving left and the rest right")
	{
		Freeway game;
		REQUIRE(game.traffic.size() == 28);
		const float speeds[] = { -60, -90, -120, -150, -210, 210, 150, 120, 90, 60 };
		int trucks = 0;
		for (const auto& car : game.traffic)
		{
			const int row = static_cast<int>((car.position.y - 11) / cell);
			REQUIRE(row >= 1);
			REQUIRE(row <= 10);
			CHECK_THAT(car.position.y, WithinAbs(static_cast<float>(row) * cell + 11, 0.01));
			CHECK(car.velocity.x == speeds[row - 1]);
			CHECK(car.velocity.y == 0);
			CHECK(car.size.y == 28);
			if (car.size.x == 88) { ++trucks; }
			else { CHECK(car.size.x == 48); }
		}
		CHECK(trucks == 4);
	}

	SECTION("a lane's cars are evenly spaced round the loop")
	{
		Freeway game;
		std::size_t inLane1 = 0;
		for (const auto& car : game.traffic)
		{
			if (car.position.y != cell + 11) { continue; }
			CHECK_THAT(car.position.x, WithinAbs(40 + static_cast<float>(inLane1) * (width / 3 + 16), 0.01));
			++inLane1;
		}
		CHECK(inLane1 == 3);
	}

	SECTION("a car gone off one side comes back in at the other")
	{
		Freeway game;
		tap(game, Key::Num1);
		auto& left = game.traffic.front(); // lane 1, -60 across
		left.position.x = -left.size.x + 0.5f;
		auto& right = game.traffic.back(); // lane 10, 60 across
		right.position.x = width - 0.5f;
		frames(game, 1);
		CHECK_THAT(left.position.x, WithinAbs(-left.size.x + 0.5f - 1 + width + left.size.x, 0.01));
		CHECK_THAT(right.position.x, WithinAbs(width - 0.5f + 1 - width - right.size.x, 0.01));

		// and however long the drive, every car stays within a car of the window
		frames(game, 600);
		for (const auto& car : game.traffic)
		{
			CHECK(car.position.x >= -car.size.x);
			CHECK(car.position.x <= width);
		}
	}

	SECTION("the chickens start at the bottom, a quarter in from each side")
	{
		Freeway game;
		CHECK(game.chicken1.size.x == chickenSize);
		CHECK(game.chicken1.size.y == chickenSize);
		CHECK_THAT(game.chicken1.position.x + chickenSize / 2, WithinAbs(width / 4, 0.01));
		CHECK_THAT(game.chicken2.position.x + chickenSize / 2, WithinAbs(width * 3 / 4, 0.01));
		CHECK_THAT(game.chicken1.position.y, WithinAbs(rowY(11), 0.01));
		CHECK_THAT(game.chicken2.position.y, WithinAbs(rowY(11), 0.01));
		CHECK(game.chicken1.score == 0);
		CHECK(game.chicken2.score == 0);
	}

	SECTION("a press is one hop, W and S for chicken 1, the arrows for chicken 2")
	{
		Freeway game = crossing();
		tap(game, Key::W);
		CHECK(heard(game, Sound::Hop));
		frames(game, 30);
		CHECK_THAT(game.chicken1.position.y, WithinAbs(rowY(10), 0.01)); // nothing keeps it going
		CHECK_THAT(game.chicken2.position.y, WithinAbs(rowY(11), 0.01));

		game.key(Key::Up, true);
		frames(game, 30); // held is still one hop
		game.key(Key::Up, false);
		tap(game, Key::Up);
		CHECK_THAT(game.chicken2.position.y, WithinAbs(rowY(9), 0.01));

		tap(game, Key::S);
		CHECK_THAT(game.chicken1.position.y, WithinAbs(rowY(11), 0.01));
		tap(game, Key::Down);
		CHECK_THAT(game.chicken2.position.y, WithinAbs(rowY(10), 0.01));
	}

	SECTION("a hop that would leave the window is refused, and silent")
	{
		Freeway game = crossing();
		tap(game, Key::S);
		CHECK_THAT(game.chicken1.position.y, WithinAbs(rowY(11), 0.01));
		CHECK_FALSE(heard(game, Sound::Hop));
	}

	SECTION("a hit knocks a chicken back a lane and costs nothing else")
	{
		Freeway game = crossing();
		game.chicken2.score = 3;
		game.chicken2.position.y = rowY(5);
		game.traffic.push_back({});
		auto& car = game.traffic.back();
		car.position = { game.chicken2.position.x, 5 * cell + 11 };
		car.size = { 48, 28 };
		frames(game, 1);
		CHECK_THAT(game.chicken2.position.y, WithinAbs(rowY(6), 0.01));
		CHECK(game.chicken2.score == 3);
		CHECK(heard(game, Sound::Hit));
		CHECK(game.screen() == Screen::Playing);
	}

	SECTION("a hop onto a car is a hit, straight back")
	{
		Freeway game = crossing();
		game.traffic.push_back({});
		auto& car = game.traffic.back();
		car.position = { game.chicken1.position.x - 5, 10 * cell + 11 };
		car.size = { 48, 28 };
		tap(game, Key::W);
		frames(game, 1);
		CHECK_THAT(game.chicken1.position.y, WithinAbs(rowY(11), 0.01));
	}

	SECTION("the two chickens do not get in each other's way")
	{
		Freeway game = crossing();
		game.chicken2.position = game.chicken1.position;
		frames(game, 5);
		CHECK_THAT(game.chicken1.position.y, WithinAbs(rowY(11), 0.01));
		CHECK_THAT(game.chicken2.position.y, WithinAbs(rowY(11), 0.01));
	}

	SECTION("the far side is a point, and back to the start")
	{
		Freeway game = crossing();
		game.chicken1.position.y = rowY(1);
		tap(game, Key::W);
		frames(game, 1);
		CHECK(game.chicken1.score == 1);
		CHECK(game.chicken2.score == 0);
		CHECK(heard(game, Sound::Cross));
		CHECK_THAT(game.chicken1.position.y, WithinAbs(rowY(11), 0.01));
		CHECK_THAT(game.chicken1.position.x + chickenSize / 2, WithinAbs(width / 4, 0.01));
		CHECK(game.screen() == Screen::Playing);
	}

	SECTION("the clock counts down from 136 a second, and runs out")
	{
		Freeway game = crossing();
		CHECK(game.clock.seconds == 136);
		frames(game, 61);
		CHECK(game.clock.seconds == 135);
		const int more = toTheEnd(game);
		CHECK(more >= 135 * 60 - 2);
		CHECK(more <= 135 * 60 + 2);
		CHECK(game.screen() == Screen::Over);
		CHECK(heard(game, Sound::End));
	}

	SECTION("the most crossings win, and the same is a draw")
	{
		Freeway game = crossing();
		toTheEnd(game);
		frames(game, 2);
		CHECK(game.winner.look == WinnerLook::Draw);

		game = crossing();
		game.chicken1.score = 4;
		game.chicken2.score = 2;
		toTheEnd(game);
		frames(game, 1);
		CHECK(game.winner.look == WinnerLook::Player1);

		game = crossing();
		game.chicken1.score = 1;
		game.chicken2.score = 3;
		toTheEnd(game);
		frames(game, 1);
		CHECK(game.winner.look == WinnerLook::Player2);

		// and nothing moves once it is over
		const float y = game.chicken1.position.y;
		tap(game, Key::W);
		frames(game, 60);
		CHECK(game.chicken1.position.y == y);
	}

	SECTION("Pause stops the clock and the traffic until Pause again")
	{
		Freeway game;
		tap(game, Key::Num1);
		frames(game, 10);
		tap(game, Key::Pause);
		CHECK(game.screen() == Screen::Paused);
		const float carX = game.traffic.front().position.x;
		const auto seconds = game.clock.seconds;
		frames(game, 120);
		CHECK(game.traffic.front().position.x == carX);
		CHECK(game.clock.seconds == seconds);
		tap(game, Key::W);
		CHECK_THAT(game.chicken1.position.y, WithinAbs(rowY(11), 0.01));
		tap(game, Key::Pause);
		CHECK(game.screen() == Screen::Playing);
		frames(game, 1);
		CHECK(game.traffic.front().position.x < carX);
	}

	SECTION("Space after the crossing is a new game, from the title")
	{
		Freeway game = crossing();
		game.chicken1.score = 2;
		toTheEnd(game);
		frames(game, 1);
		REQUIRE(game.winner.look == WinnerLook::Player1);
		tap(game, Key::Num1);
		CHECK(game.screen() == Screen::Over);
		tap(game, Key::Space);
		CHECK(game.screen() == Screen::Title);
		CHECK(game.chicken1.score == 0);
		CHECK(game.winner.look == WinnerLook::Draw);
		CHECK(game.clock.seconds == 136);
		CHECK(game.traffic.size() == 28);
	}
}
