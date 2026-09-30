// test_gemcatcher.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026
//
// Catch2 tests for games/gemcatcher.xml, played frame by frame by a real
// xge::Game with no window. Gem Catcher is described with verbs the language
// already had (held <move>, <stick />, <inc />, <dec />, <reset />, a variable
// condition on each end of the game), and it draws its fall speeds and starting
// heights from <random> once at load, so these tests are about the game working
// whatever those draws were.
//
// Window::init() normally measures each object's size once a backend exists;
// measure() gives every shape the size its sprite implies instead.

#include "command_executor.h"
#include "game.h"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace xge;

namespace
{
	constexpr float kWindowWidth = 800.0f;
	constexpr float kWindowHeight = 600.0f;

	struct Table
	{
		Game game{ "games/gemcatcher.xml" };
		CommandExecutor executor{ game };

		Table()
		{
			for (auto& object : game.getCurrentObjects())
			{
				object.size = measureShapeSize(object.spriteParams, object.shapeKind);
			}
			game.setCurrentState("playing");
		}

		Object& basket() { return game.getObject("basket"); }

		// For tests that slide the basket about while the pieces keep falling: enough
		// lives that no run of bad luck ends the game, and a score no run of good
		// luck can win with.
		void keepPlaying()
		{
			basket().variable["lives"] = 1000.0f;
			basket().variable["score"] = -1000.0f;
		}
		float score() { return basket().variable["score"]; }
		float lives() { return basket().variable["lives"]; }

		void key(const char* direction, bool down)
		{
			executor.executeInput(Command{ CmdTriggerAction{ "basket", direction } }, down);
		}

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { game.updateObjects(); }
		}

		// Puts a gem or a bomb inside the basket, ready to be caught on the next frame.
		void dropOnto(const std::string& name)
		{
			Object& piece = game.getObject(name);
			piece.position = { basket().position.x + 10.0f, basket().position.y - piece.size.y / 2.0f };
		}

		// A reset puts a piece back at the start of its fall, and the rest of that
		// frame's move still happens, so "back at the start" is the start plus at
		// most one frame of falling.
		bool atStart(const std::string& name)
		{
			const Object& piece = game.getObject(name);
			return piece.position.x == piece.positionOriginal.x
				&& piece.position.y >= piece.positionOriginal.y
				&& piece.position.y <= piece.positionOriginal.y + piece.velocity.y;
		}
	};
}

TEST_CASE("gemcatcher.xml loads a basket, six gems and three bombs", "[gemcatcher]")
{
	Table table;

	int gems = 0;
	int bombs = 0;
	for (const auto& object : table.game.getCurrentObjects())
	{
		if (object.objClass == "gems") { ++gems; }
		if (object.objClass == "bombs") { ++bombs; }
	}
	CHECK(gems == 6);
	CHECK(bombs == 3);
	CHECK(table.score() == 0.0f);
	CHECK(table.lives() == 3.0f);

	// Centered along the bottom.
	CHECK(table.basket().position.x + table.basket().size.x / 2.0f == kWindowWidth / 2.0f);
	CHECK(table.basket().position.y + table.basket().size.y < kWindowHeight);
}

TEST_CASE("every gem and bomb starts above the screen and falls at a speed drawn from its range", "[gemcatcher][random]")
{
	// The draws are made when the game loads, so look at a good many loads.
	for (int load = 0; load < 20; ++load)
	{
		Table table;
		for (const std::string name : { "gems", "bombs" })
		{
			const int count = (name == "gems") ? 6 : 3;
			const float lowest = (name == "gems") ? 2.0f : 3.0f;
			const float highest = (name == "gems") ? 4.0f : 5.0f;
			for (int i = 1; i <= count; ++i)
			{
				const Object& piece = table.game.getObject(name + "." + std::to_string(i));
				INFO(name << "." << i);
				CHECK(piece.position.y < 0.0f);
				CHECK(piece.position.y + piece.size.y <= 0.0f); // all of it is off the top
				CHECK(piece.velocity.x == 0.0f);
				CHECK(piece.velocity.y >= lowest);
				CHECK(piece.velocity.y <= highest);
			}
		}
	}
}

TEST_CASE("the pieces do not all draw the same fall", "[gemcatcher][random]")
{
	Table table;

	bool speedsDiffer = false;
	bool heightsDiffer = false;
	const Object& first = table.game.getObject("gems.1");
	for (int i = 2; i <= 6; ++i)
	{
		const Object& other = table.game.getObject("gems." + std::to_string(i));
		speedsDiffer = speedsDiffer || other.velocity.y != first.velocity.y;
		heightsDiffer = heightsDiffer || other.position.y != first.position.y;
	}
	CHECK(speedsDiffer);
	CHECK(heightsDiffer);
}

TEST_CASE("held keys slide the basket and it stops at the sides", "[gemcatcher]")
{
	Table table;
	table.keepPlaying(); // the basket crosses under falling bombs; the game must not end on the way
	const float startX = table.basket().position.x;

	table.key("right", true);
	table.frames(10);
	CHECK(table.basket().position.x == startX + 70.0f);

	table.key("right", false);
	table.frames(10);
	CHECK(table.basket().position.x == startX + 70.0f); // let go: it stays

	table.key("left", true);
	table.frames(200);
	CHECK(table.basket().position.x == 0.0f); // stuck to the left side, key still down

	table.key("left", false);
	table.key("right", true);
	table.frames(200);
	CHECK(table.basket().position.x + table.basket().size.x == kWindowWidth);
}

TEST_CASE("catching a gem scores a point and sends the gem back up", "[gemcatcher]")
{
	Table table;

	table.dropOnto("gems.3");
	table.frames(1);

	CHECK(table.score() == 1.0f);
	CHECK(table.lives() == 3.0f);
	CHECK(table.atStart("gems.3"));
	CHECK(table.game.getObject("score").spriteParams.at(1) == "1");
}

TEST_CASE("catching a bomb costs a life and sends the bomb back up", "[gemcatcher]")
{
	Table table;

	table.dropOnto("bombs.2");
	table.frames(1);

	CHECK(table.lives() == 2.0f);
	CHECK(table.score() == 0.0f);
	CHECK(table.atStart("bombs.2"));
	CHECK(table.game.getObject("lives").spriteParams.at(1) == "2");
}

TEST_CASE("a gem or a bomb that reaches the floor costs nothing and starts over", "[gemcatcher]")
{
	Table table;

	// Well away from the basket, a frame or two from the bottom.
	table.basket().position.x = 600.0f;
	for (const std::string name : { "gems.1", "bombs.1" })
	{
		Object& piece = table.game.getObject(name);
		piece.position.y = kWindowHeight - piece.size.y - 2.0f;
	}
	table.frames(3);

	CHECK(table.score() == 0.0f);
	CHECK(table.lives() == 3.0f);
	// Back up above the top of the screen, at the start of its fall again.
	for (const std::string name : { "gems.1", "bombs.1" })
	{
		const Object& piece = table.game.getObject(name);
		CHECK(piece.position.x == piece.positionOriginal.x);
		CHECK(piece.position.y < 0.0f);
	}
	CHECK(table.game.getCurrentState().name == "playing");
}

TEST_CASE("fifteen gems win the game", "[gemcatcher]")
{
	Table table;

	table.basket().variable["score"] = 14.0f;
	table.dropOnto("gems.1");
	table.frames(1);

	CHECK(table.score() == 15.0f);
	CHECK(table.game.getCurrentState().name == "youwin");
}

TEST_CASE("fourteen gems are not enough", "[gemcatcher]")
{
	Table table;

	table.basket().variable["score"] = 13.0f;
	table.dropOnto("gems.1");
	table.frames(1);

	CHECK(table.score() == 14.0f);
	CHECK(table.game.getCurrentState().name == "playing");
}

TEST_CASE("the third bomb ends the game, and a gem in between changes nothing", "[gemcatcher]")
{
	Table table;

	table.dropOnto("bombs.1");
	table.frames(1);
	table.dropOnto("gems.2");
	table.frames(1);
	table.dropOnto("bombs.3");
	table.frames(1);
	CHECK(table.lives() == 1.0f);
	CHECK(table.game.getCurrentState().name == "playing");

	table.dropOnto("bombs.2");
	table.frames(1);
	CHECK(table.lives() == 0.0f);
	CHECK(table.score() == 1.0f);
	CHECK(table.game.getCurrentState().name == "gameover");
}

TEST_CASE("space starts a new game from either end screen with everything back", "[gemcatcher]")
{
	Table table;

	table.dropOnto("gems.4");
	table.frames(1);
	table.basket().variable["lives"] = 1.0f;
	table.dropOnto("bombs.1");
	table.frames(1);
	REQUIRE(table.game.getCurrentState().name == "gameover");

	table.executor.executeInput(Command{ CmdReset{} }, true);

	CHECK(table.game.getCurrentState().name == "mainmenu");
	CHECK(table.score() == 0.0f);
	CHECK(table.lives() == 3.0f);
	CHECK(table.game.getObject("score").spriteParams.at(1) == "0");
	CHECK(table.game.getObject("lives").spriteParams.at(1) == "3");
	CHECK(table.atStart("gems.4"));
	CHECK(table.atStart("bombs.1"));
}

TEST_CASE("a game left running eventually brings gems down on a moving basket", "[gemcatcher][play]")
{
	// A crude player: sit under the lowest gem. Enough gems fall in twenty
	// seconds of play that the basket, always moving, must have caught some.
	Table table;
	table.basket().variable["lives"] = 1000.0f; // the chaser is not dodging bombs

	for (int frame = 0; frame < 20 * 60; ++frame)
	{
		float targetX = kWindowWidth / 2.0f;
		float lowest = -10000.0f;
		for (int i = 1; i <= 6; ++i)
		{
			const Object& gem = table.game.getObject("gems." + std::to_string(i));
			if (gem.position.y > lowest && gem.position.y < table.basket().position.y)
			{
				lowest = gem.position.y;
				targetX = gem.position.x;
			}
		}
		const float centre = table.basket().position.x + table.basket().size.x / 2.0f;
		table.key("left", centre > targetX + 8.0f);
		table.key("right", centre < targetX - 8.0f);
		table.frames(1);
		if (table.game.getCurrentState().name != "playing") { break; }
	}

	CHECK(table.score() > 0.0f);
}
