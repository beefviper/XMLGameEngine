// test_cannonhill.cpp
// XML Game Engine
// author: beefviper
// date: Oct 31, 2026
//
// Catch2 tests for cannonhill.xml: the cannons stand on the hill, the
// barrel turns from its elevation, a held then let go Space fires a shell
// from the tip with the wind in it, a landing blows a crater in the hill
// and passes the turn, and a shell by a cannon wins the other player the
// point and the next field.

#include "played.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

using namespace xge;
using Catch::Approx;
using played::Played;

namespace
{
	struct Duel : Played
	{
		Duel() : Played("descriptions/cannonhill.xml")
		{
			tap(KeyCode::Space);
			frames(3);
		}

		float table(const char* name) { return get("table").variable.at(name); }
		Vector2f middle(const Object& object) { return object.position + object.size * 0.5f; }
		Object* shell()
		{
			for (auto& object : game.getCurrentObjects())
			{
				if (object.exists && calls(object.name, "shell.1")) { return &object; }
			}
			return nullptr;
		}

		// Space held for that many frames, then let go.
		void fire(int held)
		{
			engine.handleKeyPressed(KeyCode::Space);
			frames(held);
			engine.handleKeyReleased(KeyCode::Space);
		}

		// Frames until the turn is someone's to aim again, at most `most`.
		void untilAiming(int most = 600)
		{
			for (int i = 0; i < most && table("phase") != 0.0f; ++i) { frames(1); }
		}
	};
}

TEST_CASE("cannonhill.xml: the cannons stand on the first field, player 1 to aim", "[cannonhill]")
{
	Duel duel;
	REQUIRE(duel.state() == "playing");
	CHECK(duel.table("phase") == 0.0f);
	CHECK(duel.table("turn") == 1.0f);
	CHECK(duel.get("barrel1").lookName() == "ready");
	CHECK(duel.get("barrel2").lookName() == "idle");

	// Each cannon's foot on the ground, and its barrel's middle on its top.
	const Object& cannon = duel.get("cannon1");
	CHECK(cannon.position.y + cannon.size.y == Approx(440.0f));
	CHECK(duel.middle(cannon).x == Approx(100.0f));
	CHECK(duel.middle(duel.get("barrel1")).x == Approx(100.0f));
	CHECK(duel.middle(duel.get("barrel1")).y == Approx(cannon.position.y));
	CHECK(duel.get("barrel1").heading == Approx(45.0f));
	CHECK(duel.get("barrel2").heading == Approx(315.0f));
}

TEST_CASE("cannonhill.xml: the barrel turns from its elevation, kept from level to straight up", "[cannonhill]")
{
	Duel duel;
	Object& barrel = duel.get("barrel1");
	duel.engine.handleKeyPressed(KeyCode::W);
	duel.frames(10);
	duel.engine.handleKeyReleased(KeyCode::W);
	CHECK(barrel.variable.at("elevation") == Approx(55.0f));
	CHECK(barrel.heading == Approx(35.0f));
	duel.engine.handleKeyPressed(KeyCode::W);
	duel.frames(80);
	duel.engine.handleKeyReleased(KeyCode::W);
	CHECK(barrel.variable.at("elevation") == Approx(90.0f));
	CHECK(barrel.heading == Approx(0.0f));
	duel.engine.handleKeyPressed(KeyCode::S);
	duel.frames(200);
	duel.engine.handleKeyReleased(KeyCode::S);
	CHECK(barrel.variable.at("elevation") == Approx(0.0f));
	CHECK(barrel.heading == Approx(90.0f));
	// The other barrel, not ready, never moved.
	CHECK(duel.get("barrel2").heading == Approx(315.0f));
}

TEST_CASE("cannonhill.xml: a shell leaves the tip along the barrel, at the charge's speed, with the wind in it", "[cannonhill]")
{
	Duel duel;
	duel.game.setVariable("playing.table", "wind", 3.0f);
	duel.fire(40);
	duel.frames(1);
	Object* shell = duel.shell();
	REQUIRE(shell);
	CHECK(duel.get("barrel1").variable.at("last") == Approx(60.0f));
	CHECK(duel.get("barrel1").lookName() == "idle");
	CHECK(duel.table("phase") == 1.0f);
	const float speed = 60.0f * 0.15f;
	CHECK(shell->acceleration.x == Approx(3.0f * 0.004f));
	CHECK(shell->acceleration.y == Approx(0.12f));
	CHECK(std::hypot(shell->velocity.x, shell->velocity.y) == Approx(speed).margin(0.2f));
	CHECK(shell->velocity.x > 0.0f);
	CHECK(shell->velocity.y < 0.0f);
	// From the tip, 16 out from the barrel's middle at 45 degrees.
	const Vector2f from = duel.middle(duel.get("barrel1"));
	const Vector2f at = duel.middle(*shell) - shell->velocity;
	CHECK(std::hypot(at.x - from.x, at.y - from.y) == Approx(16.0f).margin(2.0f));
}

TEST_CASE("cannonhill.xml: a shell that lands wide blows a crater and passes the turn with a new wind", "[cannonhill]")
{
	Duel duel;
	duel.game.setVariable("playing.table", "wind", 0.0f);
	// A soft shot: it lands on the slope in front.
	duel.fire(20);
	for (int i = 0; i < 300 && duel.table("phase") == 1.0f; ++i) { duel.frames(1); }
	REQUIRE(duel.table("phase") >= 2.0f);
	const Object& ground = duel.get("ground");
	REQUIRE(ground.bitmap);
	duel.untilAiming();
	CHECK(duel.table("turn") == 2.0f);
	CHECK(duel.get("barrel2").lookName() == "ready");
	CHECK(duel.get("barrel1").lookName() == "idle");
	CHECK(duel.table("score1") == 0.0f);
	CHECK(duel.table("score2") == 0.0f);
	CHECK(std::abs(duel.table("wind")) <= 5.0f);
	CHECK(duel.table("wind") == std::round(duel.table("wind")));
}

TEST_CASE("cannonhill.xml: the crater is cut where the shell landed", "[cannonhill]")
{
	Duel duel;
	duel.game.setVariable("playing.table", "wind", 0.0f);
	duel.fire(20);
	Vector2f landed{};
	for (int i = 0; i < 300 && duel.table("phase") == 1.0f; ++i)
	{
		if (Object* shell = duel.shell()) { landed = duel.middle(*shell); }
		duel.frames(1);
	}
	REQUIRE(duel.table("phase") >= 2.0f);
	const Object& ground = duel.get("ground");
	const int x = static_cast<int>(landed.x - ground.position.x);
	const int y = static_cast<int>(landed.y - ground.position.y);
	// Ground a little under where it struck is gone.
	CHECK_FALSE(ground.bitmap->solidAt(x, y + 6));
	// Far below the crater, still ground.
	CHECK(ground.bitmap->solidAt(x, y + 60));
}

TEST_CASE("cannonhill.xml: a shell landing by the other cannon wins the point and the next field, whole", "[cannonhill]")
{
	Duel duel;
	// Dropped onto cannon 2.
	duel.fire(5);
	duel.frames(1);
	Object* shell = duel.shell();
	REQUIRE(shell);
	const Object& target = duel.get("cannon2");
	shell->position = { duel.middle(target).x - 3.0f, target.position.y - 30.0f };
	shell->velocity = { 0.0f, 3.0f };
	shell->acceleration = { 0.0f, 0.12f };
	for (int i = 0; i < 60 && duel.state() == "playing"; ++i) { duel.frames(1); }
	REQUIRE(duel.state() == "point");
	CHECK(duel.table("score1") == 1.0f);
	CHECK(duel.get("pointshown").lookName() == "one");

	duel.frames(100);
	REQUIRE(duel.state() == "playing");
	duel.untilAiming();
	// The second field, its cannons on its own ground, and the turn passed.
	CHECK(duel.get("cannon1").position.y + duel.get("cannon1").size.y == Approx(470.0f));
	CHECK(duel.table("turn") == 2.0f);
	CHECK(duel.table("score1") == 1.0f);
}

TEST_CASE("cannonhill.xml: three points win the match", "[cannonhill]")
{
	Duel duel;
	duel.game.setVariable("playing.table", "score2", 2.0f);
	duel.fire(5);
	duel.frames(1);
	Object* shell = duel.shell();
	REQUIRE(shell);
	// Player 1's own shell back onto their own cannon: player 2's point.
	const Object& own = duel.get("cannon1");
	shell->position = { duel.middle(own).x - 3.0f, own.position.y - 30.0f };
	shell->velocity = { 0.0f, 3.0f };
	for (int i = 0; i < 200 && duel.state() != "over"; ++i) { duel.frames(1); }
	REQUIRE(duel.state() == "over");
	CHECK(duel.get("winner").lookName() == "two");
	duel.tap(KeyCode::Space);
	CHECK(duel.state() == "title");
}
