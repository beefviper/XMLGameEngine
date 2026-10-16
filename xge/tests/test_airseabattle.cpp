// test_airseabattle.cpp
// XML Game Engine
// author: beefviper
// date: Oct 7, 2026
//
// Catch2 tests for games/descriptions/airseabattle.xml, played through a real
// xge::Engine with no window and the keys two players would press: the
// lanes of planes going round, a gun swung (<turn>) and fired along its
// heading, a plane brought down scoring for the right player and coming back
// (<reveal>), and the clock ending the battle.
//
// Window::init() normally measures each object's size once a backend exists;
// the helper gives every shape the size its sprite implies instead.

#include "engine/engine.h"
#include "game/game.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace xge;
using Catch::Approx;

namespace
{
	class FakeWindow : public Window
	{
	public:
		bool isOpen() const override { return true; }
		void close() override {}
		void init(std::vector<Object>&) override {}
		std::vector<std::pair<KeyCode, bool>> pollEvents() override { return {}; }
		void clear(const std::string&) override {}
		void draw(Object&) override {}
		void display() override {}
	};

	struct AirSeaBattle
	{
		Game game{ "descriptions/airseabattle.xml" };
		Engine engine{ game, std::make_unique<FakeWindow>() };

		AirSeaBattle()
		{
			for (auto& object : game.getCurrentObjects())
			{
				object.size = measureShapeSize(object.spriteParams, object.shapeKind);
			}
		}

		void tap(KeyCode key)
		{
			engine.handleKeyPressed(key);
			engine.handleKeyReleased(key);
		}

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { engine.step(); }
		}

		void hold(KeyCode key, int count)
		{
			engine.handleKeyPressed(key);
			frames(count);
			engine.handleKeyReleased(key);
		}

		void start()
		{
			tap(KeyCode::Space);
			REQUIRE(state() == "battle");
		}

		std::string state() { return game.getCurrentState().name; }
		float score(const char* gun) { return game.getObject(gun).variable.at("score"); }
	};
}

TEST_CASE("airseabattle.xml loads, and the planes fly in and go round", "[airseabattle]")
{
	AirSeaBattle play;
	CHECK(play.state() == "title");
	play.start();

	Object& jet = play.game.getObject("jets.1");
	CHECK(jet.position.x < 0.0f);
	play.frames(120);
	CHECK(jet.position.x > 0.0f);
	CHECK(jet.position.x < 640.0f);

	// Off the right and back in from the left, in its own lane.
	play.frames(240);
	CHECK(jet.isVisible);
	CHECK(jet.position.x < 400.0f);
	CHECK(jet.position.y == Approx(60.0f));

	Object& bomber = play.game.getObject("bombers.1");
	CHECK(bomber.velocity.x < 0.0f);
}

TEST_CASE("a gun swings, and fires along the way it points", "[airseabattle]")
{
	AirSeaBattle play;
	play.start();

	play.tap(KeyCode::W);
	play.frames(1);
	const Object& straight = play.game.getObject("shots1.1");
	REQUIRE(straight.isVisible);
	CHECK(straight.velocity.x == Approx(0.0f).margin(0.01f));
	CHECK(straight.velocity.y == Approx(-6.0f));

	// Thirty degrees to the right.
	play.hold(KeyCode::D, 15);
	CHECK(play.game.getObject("gun1").heading == Approx(30.0f));
	play.tap(KeyCode::W);
	play.frames(1);
	const Object& slanted = play.game.getObject("shots1.2");
	REQUIRE(slanted.isVisible);
	CHECK(slanted.velocity.x == Approx(3.0f).margin(0.01f));
	CHECK(slanted.velocity.y == Approx(-5.196f).margin(0.01f));

	// Two in the air is all there is.
	play.tap(KeyCode::W);
	play.frames(1);
	CHECK(play.game.getObject("shots1.1").isVisible);
	CHECK(play.game.getObject("shots1.2").isVisible);
}

TEST_CASE("a plane shot down scores for whoever shot it, and comes back", "[airseabattle]")
{
	AirSeaBattle play;
	play.start();

	// A blimp over player two's gun.
	Object& blimp = play.game.getObject("blimps.1");
	const Object& gun = play.game.getObject("gun2");
	blimp.position.x = gun.position.x + gun.size.x / 2 - blimp.size.x / 2;
	blimp.velocity = { 0.0f, 0.0f };
	play.tap(KeyCode::Up);
	play.frames(60);
	CHECK(play.score("gun2") == 1);
	CHECK(play.score("gun1") == 0);
	CHECK_FALSE(blimp.isVisible);

	// Back off the side it came from.
	play.frames(90);
	CHECK(blimp.isVisible);
	CHECK(blimp.position.x < 30.0f);
	CHECK(blimp.position.y == Approx(200.0f));
}

TEST_CASE("the battle lasts two minutes", "[airseabattle]")
{
	AirSeaBattle play;
	play.start();
	play.frames(120);
	CHECK(play.game.getObject("status").variable.at("time") == Approx(118.0f));

	play.tap(KeyCode::Space);
	CHECK(play.state() == "paused");
	play.frames(120);
	play.tap(KeyCode::Space);
	CHECK(play.game.getObject("status").variable.at("time") == Approx(118.0f));

	play.game.getObject("status").variable["time"] = 1;
	play.frames(61);
	CHECK(play.state() == "result");
	CHECK(play.game.getObject("result.outcome").lookName() == "draw"); // no hits either side
	play.tap(KeyCode::Space);
	CHECK(play.state() == "title");
}

TEST_CASE("at time up the scores are compared, and the screen says who won", "[airseabattle][conditions]")
{
	const auto endWith = [](float yellow, float white)
	{
		AirSeaBattle play;
		play.start();
		play.game.getObject("gun1").variable["score"] = yellow;
		play.game.getObject("gun2").variable["score"] = white;
		play.game.getObject("status").variable["time"] = 1;
		play.frames(62);
		REQUIRE(play.state() == "result");
		return play.game.getObject("result.outcome").lookName();
	};

	CHECK(endWith(7, 3) == "yellow");
	CHECK(endWith(3, 7) == "white");
	CHECK(endWith(5, 5) == "draw");
	CHECK(endWith(0, 1) == "white");
}

TEST_CASE("paused, the battle is drawn still: the planes stop and the clock never shows", "[airseabattle]")
{
	AirSeaBattle play;
	play.start();
	play.frames(30);
	Object& jet = play.game.getObject("jets.1");
	const Vector2f jetAt = jet.position;

	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "paused");
	play.frames(60);
	CHECK(jet.position == jetAt);
	CHECK(play.game.isStill(jet));
	CHECK(play.game.isStill(play.game.getObject("gun1")));
	CHECK(play.game.isStill(play.game.getObject("clock")));
	CHECK_FALSE(play.game.isShown(jet));
	CHECK(play.game.isShown(play.game.getObject("paused")));
	CHECK_FALSE(play.game.isStill(play.game.getObject("shots1.1"))); // never fired

	// The clock's holder runs in the battle but is never drawn.
	Object& status = play.game.getObject("status");
	CHECK(status.ownerState == "battle");
	CHECK_FALSE(status.isDrawn);

	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "battle");
	play.frames(1);
	CHECK(jet.position != jetAt);
}

TEST_CASE("each end screen draws the battle still under TIME UP and its message", "[airseabattle]")
{
	AirSeaBattle play;
	CHECK(play.game.getObject("keys1").ownerState == "title");
	CHECK(play.game.getObject("timeup").ownerState.empty());
	CHECK_FALSE(play.game.isShown(play.game.getObject("timeup")));

	play.start();
	play.game.getObject("gun1").variable["score"] = 7;
	play.game.getObject("status").variable["time"] = 1;
	play.frames(62);
	REQUIRE(play.state() == "result");
	CHECK(play.game.isStill(play.game.getObject("score1")));
	CHECK(play.game.isStill(play.game.getObject("sea")));
	CHECK(play.game.isShown(play.game.getObject("timeup")));
	CHECK(play.game.isShown(play.game.getObject("again")));
	CHECK(play.game.isShown(play.game.getObject("outcome")));
	CHECK(play.game.getObject("outcome").lookName() == "yellow");
}
