// test_puttpark.cpp
// XML Game Engine
// author: beefviper
// date: Oct 31, 2026
//
// Catch2 tests for puttpark.xml, mini golf: the ball waits on the first
// tee with the arrow on it, Space held builds the power and let go strikes
// along the arrow, a stopped ball gets the arrow back, a bank turns the
// ball, and the cup takes it on to the next hole.

#include "played.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

using namespace xge;
using Catch::Approx;
using played::Played;

namespace
{
	struct Park : Played
	{
		Park() : Played("descriptions/puttpark.xml")
		{
			tap(KeyCode::Space);
			frames(3);
		}

		Object& ball() { return get("ball"); }
		Object& arrow() { return get("arrow"); }
		float card(const char* name) { return get("card").variable.at(name); }

		// Space held for `held` frames, then let go.
		void putt(int held)
		{
			engine.handleKeyPressed(KeyCode::Space);
			frames(held);
			engine.handleKeyReleased(KeyCode::Space);
		}

		// Frames until the arrow is ready again, at most `most`.
		void rollOut(int most = 900)
		{
			for (int i = 0; i < most && arrow().lookName() != "ready"; ++i) { frames(1); }
		}

		Vector2f middle(const Object& object) { return object.position + object.size * 0.5f; }
	};
}

TEST_CASE("puttpark.xml: the ball waits on the first tee, the arrow on it", "[puttpark]")
{
	Park park;
	REQUIRE(park.state() == "playing");
	CHECK(park.middle(park.ball()).x == Approx(150.0f));
	CHECK(park.middle(park.ball()).y == Approx(300.0f));
	CHECK(park.arrow().lookName() == "ready");
	CHECK(park.middle(park.arrow()).x == Approx(150.0f).margin(1.0f));
	CHECK(park.middle(park.arrow()).y == Approx(300.0f).margin(1.0f));
	CHECK(park.card("hole") == 1.0f);
}

TEST_CASE("puttpark.xml: Space held builds the power, let go strikes along the arrow", "[puttpark]")
{
	Park park;
	park.engine.handleKeyPressed(KeyCode::Space);
	park.frames(20);
	CHECK(park.arrow().variable.at("power") == Approx(40.0f));
	CHECK(park.ball().velocity.x == Approx(0.0f));
	park.frames(100);
	CHECK(park.arrow().variable.at("power") == Approx(100.0f)); // no further than 100
	park.engine.handleKeyReleased(KeyCode::Space);

	// Heading 90, to the right, at a tenth of the power.
	CHECK(park.ball().velocity.x == Approx(10.0f));
	CHECK(park.ball().velocity.y == Approx(0.0f).margin(0.001f));
	CHECK(park.arrow().lookName() == "waiting");
	CHECK(park.arrow().variable.at("power") == 0.0f);
	CHECK(park.card("strokes") == 1.0f);

	// While it rolls, Space does nothing.
	park.putt(10);
	CHECK(park.card("strokes") == 1.0f);
}

TEST_CASE("puttpark.xml: a gentle putt rolls to a stop and gets the arrow back", "[puttpark]")
{
	Park park;
	park.putt(10);
	park.rollOut();
	REQUIRE(park.arrow().lookName() == "ready");
	CHECK(park.ball().velocity.x == 0.0f);
	CHECK(park.middle(park.ball()).x > 180.0f);
	CHECK(park.middle(park.arrow()).x == Approx(park.middle(park.ball()).x).margin(1.0f));
}

TEST_CASE("puttpark.xml: straight at the cup drops in and goes on to the next hole's tee", "[puttpark]")
{
	Park park;
	park.putt(35);
	for (int i = 0; i < 600 && park.card("sunk") < 1.0f; ++i) { park.frames(1); }
	REQUIRE(park.card("sunk") == 1.0f);
	park.frames(2);
	CHECK(park.card("hole") == 2.0f);
	CHECK(park.card("total") == 1.0f);
	CHECK(park.card("strokes") == 0.0f);
	CHECK(park.middle(park.ball()).x == Approx(150.0f));
	CHECK(park.middle(park.ball()).y == Approx(180.0f));
	park.rollOut(10);
	CHECK(park.arrow().lookName() == "ready");
}

TEST_CASE("puttpark.xml: the bank on the second hole turns the ball down", "[puttpark]")
{
	Park park;
	park.putt(35);
	for (int i = 0; i < 600 && park.card("hole") < 2.0f; ++i) { park.frames(1); }
	park.rollOut(10);
	REQUIRE(park.card("hole") == 2.0f);

	// Hard to the right along the top run, into the 45 degree bank.
	park.putt(60);
	bool turned = false;
	for (int i = 0; i < 200 && !turned; ++i)
	{
		park.frames(1);
		turned = park.ball().velocity.y > 2.0f;
	}
	CHECK(turned);
	CHECK(park.ball().velocity.y > std::abs(park.ball().velocity.x) * 0.5f);
}

TEST_CASE("puttpark.xml: a post kicks the ball off faster than it came", "[puttpark]")
{
	Park park;
	// Straight to the third hole.
	park.game.enterScene("playing.hole3");
	park.get("card").variable["placed"] = 0.0f;
	park.frames(3);
	REQUIRE(park.card("hole") == 3.0f);
	park.putt(40);
	float before = 0.0f;
	bool kicked = false;
	for (int i = 0; i < 200 && !kicked; ++i)
	{
		before = std::hypot(park.ball().velocity.x, park.ball().velocity.y);
		park.frames(1);
		kicked = park.ball().velocity.x < 0.0f;
	}
	REQUIRE(kicked);
	CHECK(std::hypot(park.ball().velocity.x, park.ball().velocity.y) > before * 1.2f);
}
