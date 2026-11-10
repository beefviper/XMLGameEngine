// test_spacerace.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026
//
// Catch2 tests for games/descriptions/spacerace.xml, played frame by frame by a real
// xge::Game with no window: two rockets, a group of twelve rows of debris, and a
// clock bar whose y ends the race. These tests are about the game working, not
// about anything new.
//
// Window::init() normally measures each object's size once a backend exists;
// measure() gives every shape the size its sprite implies instead.

#include "play/executor.h"
#include "game/game.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <string>
#include <vector>

using namespace xge;
using Catch::Approx;

namespace
{
	constexpr float kWindowWidth = 800.0f;

	struct Table
	{
		Game game{ "descriptions/spacerace.xml" };
		CommandExecutor executor{ game };

		Table()
		{
			for (auto& object : game.getCurrentObjects())
			{
				object.size = measureShapeSize(object.picture);
			}
			game.setCurrentState("playing");
		}

		Object& rocket(int player) { return game.getObject("rocket" + std::to_string(player)); }
		float score(int player) { return rocket(player).variable["score"]; }

		void key(int player, const char* direction, bool down)
		{
			executor.executeInput(Command{ CmdTriggerAction{ "rocket" + std::to_string(player), direction } }, down);
		}

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { game.updateObjects(); }
		}

		// Lets the clock run out: the bar is put at the bottom, and the race ends.
		void timeUp()
		{
			game.getObject("clock").position.y = 600.0f;
			frames(1);
		}

		bool atStart(int player)
		{
			return rocket(player).position.x == rocket(player).start.position.x
				&& rocket(player).position.y == rocket(player).start.position.y;
		}
	};
}

TEST_CASE("spacerace.xml loads two rockets, a field of debris and the clock", "[spacerace]")
{
	Table table;

	int debris = 0;
	for (const auto& object : table.game.getCurrentObjects())
	{
		if (object.objClass == "debris") { ++debris; }
	}
	CHECK(debris == 96);
	CHECK(table.score(1) == 0.0f);
	CHECK(table.score(2) == 0.0f);

	// One in each half, side by side at the bottom, as tall as the picture.
	CHECK(table.rocket(1).position.x < kWindowWidth / 2);
	CHECK(table.rocket(2).position.x > kWindowWidth / 2);
	CHECK(table.rocket(1).position.y == table.rocket(2).position.y);
	CHECK(table.rocket(1).size.x == 15.0f);
	CHECK(table.rocket(1).size.y == 21.0f);
	CHECK(table.rocket(1).position.y == 600.0f - 21.0f - 10.0f);

	// The clock sinks a window's height in a minute.
	CHECK(table.game.getObject("clock").velocity.y * 60.0f * 60.0f == Catch::Approx(600.0f));
}

TEST_CASE("a held key flies a rocket up and stops at the bottom of the screen", "[spacerace]")
{
	Table table;
	const float startY = table.rocket(1).position.y;

	table.key(1, "up", true);
	table.frames(10);
	CHECK(table.rocket(1).position.y == startY - 25.0f);
	CHECK(table.rocket(2).position.y == startY); // the other rocket is not touched

	table.key(1, "up", false);
	table.key(1, "down", true);
	table.frames(60);
	CHECK(table.rocket(1).position.y == 600.0f - table.rocket(1).size.y); // back down, stuck to the bottom
	table.frames(60);
	// ...and no further, though the key is still down.
	CHECK(table.rocket(1).position.y + table.rocket(1).size.y <= 600.0f);
}

TEST_CASE("a rocket hit by debris goes back to the start", "[spacerace]")
{
	Table table;
	const Object& debris = table.game.getObject("debris.4.5");

	table.rocket(2).position = { debris.position.x - 4.0f, debris.position.y - 10.0f };
	table.frames(1);

	CHECK(table.atStart(2));
	CHECK(table.score(2) == 0.0f);
}

TEST_CASE("the rockets do not hurt each other", "[spacerace]")
{
	Table table;

	table.rocket(1).position = { 300.0f, 500.0f };
	table.rocket(2).position = { 300.0f, 500.0f };
	table.key(1, "up", true);
	table.frames(1);

	CHECK(table.rocket(2).position.x == 300.0f);
	CHECK(table.rocket(2).position.y == 500.0f);
}

TEST_CASE("getting off the top scores a point and sends the rocket back", "[spacerace]")
{
	Table table;

	// Above the debris, a few frames from the edge.
	table.rocket(2).position.y = 10.0f;
	table.key(2, "up", true);
	for (int frame = 0; frame < 10 && table.score(2) == 0.0f; ++frame) { table.frames(1); }
	table.key(2, "up", false);

	CHECK(table.score(2) == 1.0f);
	CHECK(table.score(1) == 0.0f);
	CHECK(table.rocket(2).position.y > 560.0f); // back at the bottom (and one step up, the key was down)
	CHECK(table.game.getObject("score2").picture.text == "1");
	CHECK(table.game.getCurrentState().name == "playing");
}

TEST_CASE("when the clock runs out, the most crossings win", "[spacerace]")
{
	SECTION("the race runs until the bar is gone")
	{
		Table table;
		table.frames(60 * 60 - 2);
		CHECK(table.game.getCurrentState().name == "playing");
		table.frames(3);
		CHECK(table.game.getCurrentState().name == "wins");
	}

	SECTION("player 1")
	{
		Table table;
		table.game.setVariable("rocket1", "score", 3.0f);
		table.game.setVariable("rocket2", "score", 2.0f);
		table.timeUp();
		REQUIRE(table.game.getCurrentState().name == "wins");
		table.frames(1);
		CHECK(table.game.getObject("wins.winner").lookName() == "player1");
	}

	SECTION("player 2, then a new game from the menu")
	{
		Table table;
		table.game.setVariable("rocket2", "score", 1.0f);
		table.timeUp();
		table.frames(1);
		CHECK(table.game.getObject("wins.winner").lookName() == "player2");

		// Space starts a new game from the menu with the scores back at 0.
		table.executor.executeInput(Command{ CmdReset{} }, true);
		CHECK(table.game.getCurrentState().name == "mainmenu");
		CHECK(table.score(2) == 0.0f);
		CHECK(table.game.getObject("wins.winner").lookName() == "draw");
		CHECK(table.game.getObject("score2").picture.text == "0");
	}

	SECTION("a draw")
	{
		Table table;
		table.game.setVariable("rocket1", "score", 2.0f);
		table.game.setVariable("rocket2", "score", 2.0f);
		table.timeUp();
		table.frames(1);
		CHECK(table.game.getObject("wins.winner").lookName() == "draw");
	}
}

TEST_CASE("the debris loops round without losing its spacing", "[spacerace][wrap]")
{
	Table table;

	// Eight to a row, a window width plus their own apart: 808 / 8 = 101.
	const auto spacing = [&](int row)
	{
		const Object& first = table.game.getObject("debris.1." + std::to_string(row));
		const float loop = kWindowWidth + first.size.x;
		std::vector<float> gaps;
		for (int column = 2; column <= 8; ++column)
		{
			const Object& other = table.game.getObject("debris." + std::to_string(column) + "." + std::to_string(row));
			gaps.push_back(std::fmod(other.position.x - first.position.x + 2 * loop, loop));
		}
		return gaps;
	};

	std::vector<std::vector<float>> before;
	for (int row = 1; row <= 12; ++row) { before.push_back(spacing(row)); }
	CHECK(before[0][0] == Catch::Approx(101.0f));

	for (int frame = 0; frame < 2000; ++frame)
	{
		table.frames(1);
		for (const auto& debris : table.game.getCurrentObjects())
		{
			if (debris.objClass != "debris") { continue; }
			REQUIRE(debris.position.x > -debris.size.x - 4.0f);
			REQUIRE(debris.position.x < kWindowWidth + 4.0f);
		}
	}

	for (int row = 1; row <= 12; ++row)
	{
		const auto after = spacing(row);
		for (std::size_t g = 0; g < after.size(); ++g)
		{
			INFO(row);
			CHECK(std::abs(after[g] - before[static_cast<std::size_t>(row - 1)][g]) < 0.5f);
		}
	}
}

TEST_CASE("each state has its own objects: the title, the race, the signs", "[spacerace][states]")
{
	Game game{ "descriptions/spacerace.xml" };
	game.setCurrentState("mainmenu");
	CHECK(game.isShown(game.getObject("title")));
	CHECK_FALSE(game.isShown(game.getObject("rocket1")));

	game.pushState("playing");
	CHECK(game.getObject("rocket1").name == "playing.rocket1");
	CHECK(game.getObject("debris.4.5").name == "playing.debris.4.5");
	CHECK(game.isShown(game.getObject("debris.4.5")));
	CHECK(game.isShown(game.getObject("score2")));
	CHECK_FALSE(game.isShown(game.getObject("title")));
}

TEST_CASE("paused and the win screen draw the race still underneath", "[spacerace][states]")
{
	Table table;
	Object& debris = table.game.getObject("debris.4.5"); // row 5: +1.5 a frame
	Object& clock = table.game.getObject("clock");
	const float x = debris.position.x;
	const float y = clock.position.y;

	table.game.pushState("paused");
	table.frames(30);
	CHECK(debris.position.x == x);
	CHECK(clock.position.y == y);
	CHECK(table.game.isStill(debris));
	CHECK(table.game.isStill(table.rocket(1)));
	CHECK(table.game.isShown(table.game.getObject("paused.paused")));
	table.game.popState();
	table.frames(2);
	CHECK(debris.position.x == Catch::Approx(x + 3.0f));
	CHECK(table.game.isShown(debris));

	table.game.setVariable("rocket2", "score", 2.0f);
	table.timeUp();
	REQUIRE(table.game.getCurrentState().name == "wins");
	table.frames(1);
	CHECK(table.game.isStill(table.rocket(2)));
	CHECK(table.game.isStill(table.game.getObject("score2")));
	CHECK(table.game.isShown(table.game.getObject("wins.winner")));
	CHECK(table.game.getObject("wins.winner").lookName() == "player2");
	CHECK(table.game.isShown(table.game.getObject("continue")));
}
