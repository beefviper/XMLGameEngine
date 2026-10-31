// test_pinball.cpp
// XML Game Engine
// author: beefviper
// date: Oct 31, 2026
//
// Catch2 tests for pinball.xml: the ball waits on the plunger, Space held
// pulls it back and let go shoots the ball up the lane, a flipper sends a
// ball coming down onto it back up the table, bumpers and lanes score, and
// a ball down the drain costs one of three.

#include "played.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

using namespace xge;
using Catch::Approx;
using played::Played;

namespace
{
	struct Table : Played
	{
		Table() : Played("descriptions/pinball.xml")
		{
			tap(KeyCode::Space);
			frames(3);
		}

		Object& ball() { return get("ball"); }
		float table(const char* name) { return get("table").variable.at(name); }
		Vector2f middle(const Object& object) { return object.position + object.size * 0.5f; }

		// The ball's middle put at x, y, going by vx, vy.
		void put(float x, float y, float vx, float vy)
		{
			ball().position = { x - 6.0f, y - 6.0f };
			ball().velocity = { vx, vy };
		}
	};
}

TEST_CASE("pinball.xml: the ball waits on the plunger, and Space held then let go shoots it", "[pinball]")
{
	Table table;
	REQUIRE(table.state() == "playing");
	CHECK(table.middle(table.ball()).x == Approx(445.0f));
	CHECK(table.middle(table.ball()).y == Approx(594.0f));
	CHECK(table.get("plunger").lookName() == "loaded");

	table.engine.handleKeyPressed(KeyCode::Space);
	table.frames(80);
	// Pulled back as far as it goes, the ball sitting on it as it goes.
	CHECK(table.get("plunger").variable.at("power") == Approx(100.0f));
	CHECK(table.middle(table.ball()).y == Approx(614.0f).margin(1.0f));
	table.engine.handleKeyReleased(KeyCode::Space);
	CHECK(table.get("plunger").lookName() == "empty");
	CHECK(table.ball().velocity.y == Approx(-18.0f));

	// Up the lane, and off the curve at the top into the table.
	for (int i = 0; i < 80 && table.middle(table.ball()).x > 430.0f; ++i) { table.frames(1); }
	CHECK(table.middle(table.ball()).x < 430.0f);
	CHECK(table.middle(table.ball()).y < 130.0f);
	CHECK(table.ball().velocity.x < 0.0f);
}

TEST_CASE("pinball.xml: a flipper sends a ball coming down onto it back up the table", "[pinball]")
{
	Table table;
	// Falling onto the middle of the left flipper.
	table.put(185.0f, 520.0f, 0.0f, 3.0f);
	table.engine.handleKeyPressed(KeyCode::A);
	table.frames(4);
	CHECK(table.get("leftflipper").heading == Approx(342.0f));
	CHECK(table.ball().velocity.y < -6.0f);

	table.frames(20);
	CHECK(table.middle(table.ball()).y < 400.0f);
}

TEST_CASE("pinball.xml: a ball lying on a flipper rolls off it, and through the gap at the bottom", "[pinball]")
{
	Table table;
	table.put(170.0f, 540.0f, 0.0f, 0.0f);
	const float balls = table.table("balls");
	table.frames(200);
	CHECK(table.table("balls") == balls - 1.0f);
	// The next ball, on the plunger.
	CHECK(table.middle(table.ball()).x == Approx(445.0f));
	CHECK(table.get("plunger").lookName() == "loaded");
}

TEST_CASE("pinball.xml: a bumper scores, lights and kicks the ball away faster", "[pinball]")
{
	Table table;
	// Straight down onto the top of the lowest bumper.
	table.put(225.0f, 220.0f, 0.0f, 4.0f);
	for (int i = 0; i < 10 && table.ball().velocity.y > 0.0f; ++i) { table.frames(1); }
	CHECK(table.table("score") == 100.0f);
	CHECK(table.get("bumper3").lookName() == "lit");
	CHECK(table.ball().velocity.y < -4.0f);
	table.frames(12);
	CHECK(table.get("bumper3").lookName() == "dark");
}

TEST_CASE("pinball.xml: the three lanes lit give a bonus, flash and go dark again", "[pinball]")
{
	Table table;
	for (const char* lane : { "lane1", "lane2", "lane3" })
	{
		const Object& it = table.get(lane);
		table.put(it.position.x + 4.0f, it.position.y - 10.0f, 0.0f, 2.0f);
		table.frames(8);
	}
	CHECK(table.table("score") == 1150.0f);
	CHECK(table.table("lit") == 0.0f);
	CHECK(table.get("lane3").lookName() == "flash");
	table.frames(40);
	CHECK(table.get("lane1").lookName() == "dark");
	CHECK(table.table("score") == 1150.0f);
}

TEST_CASE("pinball.xml: the third ball down the drain ends the game", "[pinball]")
{
	Table table;
	for (int i = 0; i < 3; ++i)
	{
		table.put(225.0f, 610.0f, 0.0f, 6.0f);
		table.frames(10);
	}
	CHECK(table.state() == "over");
	table.tap(KeyCode::Space);
	CHECK(table.state() == "title");
}

TEST_CASE("pinball.xml: a ball dropped onto a flipper is never thrown further than it was going", "[pinball]")
{
	Table table;
	table.put(185.0f, 530.0f, 0.0f, 0.0f);
	for (int i = 0; i < 40; ++i)
	{
		const Vector2f was = table.ball().position;
		const float speed = std::hypot(table.ball().velocity.x, table.ball().velocity.y);
		table.frames(1);
		const Vector2f moved = table.ball().position - was;
		CHECK(std::hypot(moved.x, moved.y) <= speed + 1.0f);
	}
}

TEST_CASE("pinball.xml: a ball caught on a raised flipper rolls down to the hinge and settles there", "[pinball]")
{
	Table table;
	table.engine.handleKeyPressed(KeyCode::A);
	table.frames(6);
	table.put(185.0f, 500.0f, 0.0f, 0.0f);
	table.frames(120);
	const Vector2f settled = table.middle(table.ball());
	CHECK(settled.x < 155.0f);
	for (int i = 0; i < 30; ++i)
	{
		table.frames(1);
		const Vector2f now = table.middle(table.ball());
		CHECK(std::abs(now.x - settled.x) < 1.5f);
		CHECK(std::abs(now.y - settled.y) < 1.5f);
	}
}
