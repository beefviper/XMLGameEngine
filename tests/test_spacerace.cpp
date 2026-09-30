// test_spacerace.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026
//
// Catch2 tests for games/spacerace.xml, played frame by frame by a real
// xge::Game with no window. Space Race is described with verbs the language
// already had (held move.*, wrap(), reset(), inc(), a variable condition), so
// these tests are about the game working, not about anything new.
//
// Window::init() normally measures each object's size once a backend exists;
// measure() gives every shape the size its sprite implies instead.

#include "command_executor.h"
#include "game.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <string>
#include <vector>

using namespace xge;

namespace
{
	constexpr float kWindowWidth = 800.0f;

	struct Table
	{
		Game game{ "games/spacerace.xml" };
		CommandExecutor executor{ game };

		Table()
		{
			for (auto& object : game.getCurrentObjects())
			{
				object.size = measureShapeSize(object.spriteParams, object.shapeKind);
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

		bool atStart(int player)
		{
			return rocket(player).position.x == rocket(player).positionOriginal.x
				&& rocket(player).position.y == rocket(player).positionOriginal.y;
		}
	};
}

TEST_CASE("spacerace.xml loads two rockets and a field of debris", "[spacerace]")
{
	Table table;

	int debris = 0;
	for (const auto& object : table.game.getCurrentObjects())
	{
		if (object.objClass == "debris") { ++debris; }
	}
	CHECK(debris == 27);
	CHECK(table.score(1) == 0.0f);
	CHECK(table.score(2) == 0.0f);

	// One in each half, side by side at the bottom.
	CHECK(table.rocket(1).position.x < kWindowWidth / 2);
	CHECK(table.rocket(2).position.x > kWindowWidth / 2);
	CHECK(table.rocket(1).position.y == table.rocket(2).position.y);
}

TEST_CASE("a held key flies a rocket up and stops at the bottom of the screen", "[spacerace]")
{
	Table table;
	const float startY = table.rocket(1).position.y;

	table.key(1, "up", true);
	table.frames(10);
	CHECK(table.rocket(1).position.y == startY - 30.0f);
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
	const Object& debris = table.game.getObject("debris5b");

	table.rocket(2).position = { debris.position.x, debris.position.y };
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
	table.frames(6);

	CHECK(table.score(2) == 1.0f);
	CHECK(table.score(1) == 0.0f);
	CHECK(table.rocket(2).position.y > 400.0f);
	CHECK(table.game.getObject("score2").spriteParams.at(1) == "1");
	CHECK(table.game.getCurrentState().name == "playing");
}

TEST_CASE("first to two points wins", "[spacerace]")
{
	SECTION("player 1")
	{
		Table table;
		table.rocket(1).variable["score"] = 1.0f;
		table.game.getObject("rocket1").position.y = 10.0f;
		table.key(1, "up", true);
		table.frames(6);

		CHECK(table.score(1) == 2.0f);
		CHECK(table.game.getCurrentState().name == "player1wins");
	}

	SECTION("player 2, and one point is not enough")
	{
		Table table;
		table.rocket(2).position.y = 10.0f;
		table.key(2, "up", true);
		table.frames(6);
		CHECK(table.game.getCurrentState().name == "playing");

		table.key(2, "up", false);
		table.rocket(2).position.y = 10.0f;
		table.key(2, "up", true);
		table.frames(6);
		CHECK(table.score(2) == 2.0f);
		CHECK(table.game.getCurrentState().name == "player2wins");

		// Space starts a new game from the menu with the scores back at 0.
		table.executor.executeInput(Command{ CmdReset{} }, true);
		CHECK(table.game.getCurrentState().name == "mainmenu");
		CHECK(table.score(2) == 0.0f);
		CHECK(table.game.getObject("score2").spriteParams.at(1) == "0");
	}
}

TEST_CASE("the debris loops round without losing its spacing", "[spacerace][wrap]")
{
	Table table;

	std::vector<std::string> lanes;
	for (int lane = 1; lane <= 9; ++lane) { lanes.push_back("debris" + std::to_string(lane)); }

	const auto spacing = [&](const std::string& lane)
	{
		const Object& first = table.game.getObject(lane + "a");
		const float loop = kWindowWidth + first.size.x;
		std::vector<float> gaps;
		for (const char member : { 'b', 'c' })
		{
			const Object& other = table.game.getObject(lane + member);
			gaps.push_back(std::fmod(other.position.x - first.position.x + 2 * loop, loop));
		}
		return gaps;
	};

	std::vector<std::vector<float>> before;
	for (const auto& lane : lanes) { before.push_back(spacing(lane)); }

	for (int frame = 0; frame < 2000; ++frame)
	{
		table.frames(1);
		for (const auto& lane : lanes)
		{
			for (const char member : { 'a', 'b', 'c' })
			{
				const Object& debris = table.game.getObject(lane + member);
				REQUIRE(debris.position.x > -debris.size.x - 4.0f);
				REQUIRE(debris.position.x < kWindowWidth + 4.0f);
			}
		}
	}

	for (std::size_t i = 0; i < lanes.size(); ++i)
	{
		const auto after = spacing(lanes[i]);
		for (std::size_t g = 0; g < after.size(); ++g)
		{
			INFO(lanes[i]);
			CHECK(std::abs(after[g] - before[i][g]) < 0.5f);
		}
	}
}
