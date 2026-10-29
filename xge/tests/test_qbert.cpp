// test_qbert.cpp
// XML Game Engine
// author: beefviper
// date: Oct 23, 2026
//
// Catch2 tests for games/descriptions/qbert.xml, played frame by frame: a
// hop that arcs up and lands on the next cube and lights it, the mirror
// that turns him going left, a hop off the pyramid, Coily hatching and
// hunting, and a disc.

#include "played.h"
#include "play/executor.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace xge;
using namespace played;
using Catch::Approx;

namespace
{
	// The game started, with the ball and Coily out of the way.
	struct Qbert : Played
	{
		Qbert() : Played("descriptions/qbert.xml")
		{
			tap(KeyCode::Space);
			REQUIRE(state() == "playing");
			for (const char* enemy : { "ball", "coily" })
			{
				get(enemy).isVisible = false;
				get(enemy).collisionData.enabled = false;
			}
			frames(2);
		}

		Object& qbert() { return get("qbert"); }
		float score() { return get("player").variable.at("score"); }
		float lives() { return get("player").variable.at("lives"); }

		// One hop, there and landed.
		void hop(KeyCode key)
		{
			tap(key);
			frames(20);
		}
	};
}

TEST_CASE("qbert.xml: a hop arcs up off its way and lands on the next cube, which lights", "[qbert]")
{
	Qbert play;
	const Vector2f start = play.qbert().position;
	REQUIRE(play.score() == Approx(0.0f));

	play.tap(KeyCode::D);
	play.frames(9);
	CHECK(play.qbert().isAirborne());
	CHECK(play.qbert().altitude == Approx(10.0f).margin(0.5f));
	play.frames(11);
	CHECK_FALSE(play.qbert().isAirborne());
	CHECK(play.qbert().altitude == 0.0f);
	CHECK(play.qbert().position.x == Approx(start.x + 16.0f));
	CHECK(play.qbert().position.y == Approx(start.y + 24.0f));
	CHECK(play.qbert().lookName() == "front");
	CHECK(play.score() == Approx(25.0f));
	CHECK(play.lives() == Approx(3.0f));

	// Back up lights the top; down again onto a lit cube scores nothing.
	play.hop(KeyCode::W);
	play.hop(KeyCode::D);
	CHECK(play.score() == Approx(50.0f));
	CHECK(play.qbert().lookName() == "front");
}

TEST_CASE("qbert.xml: going left he is drawn the other way round", "[qbert][mirror]")
{
	Qbert play;
	CHECK_FALSE(play.qbert().mirrored);
	play.hop(KeyCode::S);
	CHECK(play.qbert().mirrored);
	CHECK(play.qbert().lookName() == "front");
	play.hop(KeyCode::W);
	CHECK_FALSE(play.qbert().mirrored);
	CHECK(play.qbert().lookName() == "back");
	play.hop(KeyCode::A);
	CHECK(play.qbert().mirrored);
	CHECK(play.qbert().lookName() == "back");
}

TEST_CASE("qbert.xml: a hop down a diagonal lights the cube landed on, and one off the pyramid costs a life", "[qbert]")
{
	Played play("descriptions/qbert.xml");
	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "playing");

	// The red ball and Coily out of the way, where their hops go nowhere.
	for (const char* enemy : { "ball", "coily" })
	{
		play.get(enemy).position = { 232, 210 };
		play.get(enemy).positionOriginal = { 232, 210 };
	}

	// A hop takes 0.3 seconds, 18 frames, through the air.
	const auto hop = [&](KeyCode key)
	{
		play.tap(key);
		play.frames(19);
	};

	// The cubes above the bottom row are numbered in reading order: 1 the top,
	// 2 and 3 the row below.
	play.tap(KeyCode::D);
	play.frames(8);
	CHECK(play.get("qbert").isAirborne());
	CHECK(play.get("upper.3").lookName() == "unlit"); // not until he lands
	play.frames(11);
	CHECK(play.get("qbert").position.x == Approx(136));
	CHECK(play.get("qbert").position.y == Approx(62));
	CHECK(play.get("qbert").lookName() == "front");
	CHECK(play.get("upper.3").lookName() == "lit");
	CHECK(play.get("upper.2").lookName() == "unlit");
	CHECK(play.get("upper.1").lookName() == "unlit");
	CHECK(play.get("player").variable.at("score") == Approx(25));

	// Down and left: only the top he lands on is touched.
	hop(KeyCode::S);
	CHECK(play.get("upper.5").lookName() == "lit");
	CHECK(play.get("upper.2").lookName() == "unlit");
	CHECK(play.get("upper.1").lookName() == "unlit");
	CHECK(play.get("player").variable.at("score") == Approx(50));

	// Back up onto the top, and a second landing on a lit cube scores nothing.
	hop(KeyCode::W);
	hop(KeyCode::A);
	CHECK(play.get("qbert").lookName() == "back");
	CHECK(play.get("qbert").mirrored);
	CHECK(play.get("upper.1").lookName() == "lit");
	CHECK(play.get("player").variable.at("score") == Approx(75));
	hop(KeyCode::D);
	hop(KeyCode::A);
	CHECK(play.get("player").variable.at("score") == Approx(75));

	// Up and left off the top: into the void, and he curses.
	hop(KeyCode::A);
	CHECK(play.get("player").variable.at("lives") == Approx(2));
	CHECK(play.get("curse").lookName() == "said");
	CHECK(play.get("qbert").position.x == Approx(120));
	CHECK(play.get("qbert").position.y == Approx(38));
}

TEST_CASE("qbert.xml: Coily hatches on the bottom row and hunts Q*bert a hop at a time", "[qbert]")
{
	Played play("descriptions/qbert.xml");
	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "playing");
	play.get("ball").position = { 232, 210 };
	play.get("ball").positionOriginal = { 232, 210 };

	// The egg, on the left of the sixth row, hops down and left onto the
	// bottom row (that cube's top centred at 32, 196) and hatches there.
	Object& coily = play.get("coily");
	coily.position = { 40, 150 };
	CHECK(coily.lookName() == "egg");
	CommandExecutor executor(play.game);
	executor.executeInput(CmdTriggerAction{ "coily", "left" }, true);
	play.frames(23);
	CHECK(coily.position.x == Approx(24));
	CHECK(coily.position.y == Approx(174));
	CHECK(coily.lookName() == "snake");
	CHECK(coily.variable.at("snake") == Approx(1));

	// The snake's hops go the diagonal that lands nearest Q*bert, up on the
	// top cube: up and right each time, a row nearer.
	play.frames(70);
	CHECK(coily.position.y < 174.0f);
	CHECK(coily.position.x > 24.0f);
	CHECK(coily.mirrored); // his pictures face left, and he goes right
	CHECK(play.get("player").variable.at("lives") == Approx(3));

	// Until it lands on him: a life, and Coily back to an egg at the start.
	play.frames(400);
	CHECK(play.get("player").variable.at("lives") < 3.0f);
	CHECK(coily.lookName() == "egg");
}

TEST_CASE("qbert.xml: a disc carries Q*bert back to the top once, and Coily chasing him falls off", "[qbert]")
{
	Played play("descriptions/qbert.xml");
	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "playing");
	for (const char* enemy : { "ball", "coily" })
	{
		play.get(enemy).position = { 232, 210 };
		play.get(enemy).positionOriginal = { 232, 210 };
	}
	Object& qbert = play.get("qbert");
	Object& coily = play.get("coily");

	// On the left cube of the sixth row, up and left onto the disc beside
	// the fifth: back on the top, no life lost, and Coily, a snake by now,
	// after him.
	coily.variable["snake"] = 1.0f;
	qbert.position = { 40, 158 };
	play.frames(2);
	const float before = play.get("player").variable.at("score");
	play.tap(KeyCode::A);
	play.frames(20);
	CHECK(qbert.position.x == Approx(120));
	CHECK(qbert.position.y == Approx(38));
	CHECK(play.get("player").variable.at("lives") == Approx(3));
	CHECK(play.get("discs.1").lookName() == "used");
	CHECK(play.get("discs.2").lookName() == "ready");
	// 500 for Coily, and 25 for the top cube he is set down on: put there,
	// he has moved, so he lands on it as from a hop.
	CHECK(play.get("player").variable.at("score") - before == Approx(525));
	CHECK(coily.variable.at("snake") == Approx(0));

	// The disc is gone: the same hop again falls.
	qbert.position = { 40, 158 };
	play.frames(2);
	play.tap(KeyCode::A);
	play.frames(20);
	CHECK(play.get("player").variable.at("lives") == Approx(2));
}
