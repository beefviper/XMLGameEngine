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

TEST_CASE("depthcharge.xml loads a destroyer, ten submarines in four lanes, six charges and six mines", "[depthcharge]")
{
	Hunt hunt;

	int subs = 0;
	for (const auto& object : hunt.game.getCurrentObjects())
	{
		if (object.objClass == "sub") { ++subs; }
	}
	CHECK(subs == 10);
	CHECK(hunt.get("charges.6").objClass == "charge");
	CHECK(hunt.get("mines.6").objClass == "mine");
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

	const Object& left = hunt.get("charges.1");
	const Object& right = hunt.get("charges.2");
	REQUIRE(left.isVisible);
	REQUIRE(right.isVisible);
	CHECK(left.position.x + left.size.x / 2 == Approx(ship.position.x + 6));
	CHECK(right.position.x + right.size.x / 2 == Approx(ship.position.x + ship.size.x - 6));
	CHECK(left.velocity.y == Approx(2));

	// Three pairs in the water at most.
	hunt.tap(KeyCode::Space);
	hunt.tap(KeyCode::Space);
	hunt.tap(KeyCode::Space);
	hunt.frames(1);
	CHECK(hunt.get("charges.6").isVisible);
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
	Object& charge = hunt.get("charges.1");
	charge.position = { deep.position.x + deep.size.x / 2, deep.position.y - 10 };
	Object& other = hunt.get("charges.2"); // out of the way
	other.isVisible = false;
	other.collisionData.enabled = false;
	hunt.frames(10);

	CHECK_FALSE(deep.isVisible);
	CHECK(hunt.status("score") == 80.0f);
	CHECK_FALSE(charge.isVisible);
	CHECK(hunt.get("bursts.1").isVisible);
	hunt.frames(40);
	CHECK_FALSE(hunt.get("bursts.1").isVisible);

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
	Object& mine = hunt.get("mines.1");
	mine.isVisible = true;
	mine.collisionData.enabled = true;
	mine.velocity = { 0, -1.2f };
	mine.position = { 20, 130 };
	hunt.frames(40);
	CHECK_FALSE(mine.isVisible);
	CHECK(hunt.status("ships") == 3.0f);

	// Under the destroyer it costs a ship.
	mine.isVisible = true;
	mine.collisionData.enabled = true;
	mine.position = { ship.position.x + ship.size.x / 2, 130 };
	hunt.frames(40);
	CHECK_FALSE(mine.isVisible);
	CHECK(hunt.status("ships") == 2.0f);

	// The boats lay them on their own.
	int laid = 0;
	for (int frame = 0; frame < 600 && laid == 0; ++frame)
	{
		hunt.frames(1);
		for (int i = 1; i <= 6; ++i)
		{
			if (hunt.get("mines." + std::to_string(i)).isVisible) { ++laid; }
		}
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
