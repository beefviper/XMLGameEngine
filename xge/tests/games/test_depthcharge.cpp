// test_depthcharge.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026
//
// Catch2 tests for games/descriptions/depthcharge.xml, played through a real
// xge::Engine with no window and the keys a player would press: a pair of
// charges <spawn>ed off both sides of the destroyer, submarines in four lanes
// worth more the deeper they run, the mines they lay rising under the
// destroyer, a burst where a boat goes down, the clock and the ships.

#include "played.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <string>

using namespace xge;
using namespace played;
using Catch::Approx;

namespace
{
	struct Hunt : Played
	{
		Hunt() : Played("descriptions/depthcharge.xml")
		{
			tap(KeyCode::Space);
			REQUIRE(state() == "playing");
		}

		float status(const char* name) { return get("status").variable.at(name); }

		// Every boat held still where it is.
		void stillWater()
		{
			for (auto& object : game.getCurrentObjects())
			{
				if (object.objClass == "sub") { object.velocity = {}; }
			}
		}
	};
}

TEST_CASE("depthcharge.xml loads a destroyer, ten submarines in four lanes, and no charges or mines until they are spawned", "[depthcharge]")
{
	Hunt hunt;

	int subs = 0;
	for (const auto& object : hunt.game.getCurrentObjects())
	{
		if (object.objClass == "sub") { ++subs; }
	}
	CHECK(subs == 10);
	CHECK(hunt.count("charge") == 0);
	CHECK(hunt.count("mine") == 0);
	CHECK_FALSE(hunt.get("charge").exists);
	CHECK(hunt.status("time") == 90.0f);
	CHECK(hunt.status("ships") == 3.0f);

	// Deeper lanes are worth more.
	CHECK(hunt.get("lane1.1").variable.at("worth") == 20.0f);
	CHECK(hunt.get("lane4.1").variable.at("worth") == 80.0f);
	CHECK(hunt.get("lane4.1").position.y > hunt.get("lane1.1").position.y);
}

TEST_CASE("Space drops a pair, one off each side of the destroyer, sinking slowly", "[depthcharge]")
{
	Hunt hunt;
	const Object& ship = hunt.get("ship");

	hunt.tap(KeyCode::Space);
	hunt.frames(1);

	REQUIRE(hunt.count("charge") == 2);
	const Object& left = hunt.get("charge.1");
	const Object& right = hunt.get("charge.2");
	CHECK(left.objClass == "charge");
	CHECK(left.position.x + left.size.x / 2 == Approx(ship.position.x + 6));
	CHECK(right.position.x + right.size.x / 2 == Approx(ship.position.x + ship.size.x - 6));
	CHECK(left.velocity.y == Approx(2));

	// Three pairs in the water at most.
	hunt.tap(KeyCode::Space);
	hunt.tap(KeyCode::Space);
	hunt.tap(KeyCode::Space);
	hunt.frames(1);
	CHECK(hunt.count("charge") == 6);
	CHECK(hunt.has("charge.6"));
	CHECK_FALSE(hunt.has("charge.7"));
}

TEST_CASE("held keys slide the destroyer, and it stops at the sides", "[depthcharge]")
{
	Hunt hunt;
	const Object& ship = hunt.get("ship");
	const float start = ship.position.x;

	hunt.engine.handleKeyPressed(KeyCode::A);
	hunt.frames(10);
	CHECK(ship.position.x == Approx(start - 40));
	hunt.frames(200);
	hunt.engine.handleKeyReleased(KeyCode::A);
	CHECK(ship.position.x == Approx(0));
}

TEST_CASE("a charge sinks a submarine for its lane's worth, with a burst where it went down", "[depthcharge]")
{
	Hunt hunt;
	hunt.stillWater();
	Object& deep = hunt.get("lane4.1");
	deep.position.x = 300;

	hunt.tap(KeyCode::Space);
	hunt.frames(1);
	hunt.get("charge.1").position = { deep.position.x + deep.size.x / 2, deep.position.y - 10 };
	hunt.game.despawn(hunt.get("charge.2")); // out of the way
	hunt.frames(10);

	CHECK_FALSE(deep.isVisible);
	CHECK(hunt.status("score") == 80.0f);
	CHECK_FALSE(hunt.has("charge.1"));
	CHECK(hunt.count("charge") == 0);
	CHECK(hunt.has("burst.1"));
	hunt.frames(40);
	CHECK_FALSE(hunt.has("burst.1"));

	// It comes back into its lane, on the playing state's timer at two seconds.
	hunt.frames(80);
	CHECK(deep.isVisible);
	CHECK(deep.position.y == Approx(440));
}

TEST_CASE("a mine laid by a submarine rises, and one under the destroyer costs a ship", "[depthcharge]")
{
	Hunt hunt;
	hunt.stillWater();
	const Object& ship = hunt.get("ship");

	// Out in the open it goes off at the top of the water.
	const auto lay = [&](float x)
	{
		Object* mine = hunt.game.spawn("mine");
		REQUIRE(mine);
		mine->position = { x, 130 };
		const std::string name = mine->name;
		hunt.game.settleSpawns();
		return name;
	};
	const std::string open = lay(20);
	CHECK(hunt.get(open).velocity.y == Approx(-1.2f));
	hunt.frames(40);
	CHECK_FALSE(hunt.has(open));
	CHECK(hunt.status("ships") == 3.0f);

	// Under the destroyer it costs a ship.
	const std::string under = lay(ship.position.x + ship.size.x / 2);
	hunt.frames(40);
	CHECK_FALSE(hunt.has(under));
	CHECK(hunt.status("ships") == 2.0f);

	// The boats lay them on their own.
	int laid = 0;
	for (int frame = 0; frame < 600 && laid == 0; ++frame)
	{
		hunt.frames(1);
		laid = hunt.count("mine");
	}
	CHECK(laid > 0);
}

TEST_CASE("the hunt ends when the clock runs out or the ships are gone, and Space starts again", "[depthcharge]")
{
	Hunt hunt;
	hunt.frames(120);
	CHECK(hunt.status("time") == 88.0f);

	hunt.get("status").variable["ships"] = 0;
	hunt.frames(1);
	CHECK(hunt.state() == "gameover");
	CHECK(hunt.game.isStill(hunt.get("ship")));

	hunt.tap(KeyCode::Space);
	CHECK(hunt.state() == "mainmenu");
	CHECK(hunt.status("ships") == 3.0f);
	CHECK(hunt.status("time") == 90.0f);

	hunt.tap(KeyCode::Space);
	hunt.get("status").variable["time"] = 1;
	hunt.frames(61);
	CHECK(hunt.state() == "gameover");
}

TEST_CASE("paused, the sea is drawn still", "[depthcharge]")
{
	Hunt hunt;
	const Object& sub = hunt.get("lane1.1");
	hunt.frames(5);
	const float x = sub.position.x;

	hunt.tap(KeyCode::P);
	REQUIRE(hunt.state() == "paused");
	hunt.frames(30);
	CHECK(sub.position.x == x);
	CHECK(hunt.game.isStill(sub));

	hunt.tap(KeyCode::Escape);
	hunt.frames(5);
	CHECK(sub.position.x != x);
}
