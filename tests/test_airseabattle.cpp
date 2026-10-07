// test_airseabattle.cpp
// XML Game Engine
// author: beefviper
// date: Oct 7, 2026
//
// Catch2 tests for games/airseabattle.xml, played through a real
// xge::Engine with no window and the keys two players would press: the
// lanes of planes going round, a gun swung (<turn>) and fired along its
// heading, a plane brought down scoring for the right player and coming back
// (<reveal>), and the clock ending the battle.
//
// Window::init() normally measures each object's size once a backend exists;
// the helper gives every shape the size its sprite implies instead.

#include "engine.h"
#include "game.h"

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
		Game game{ "games/airseabattle.xml" };
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
	CHECK(play.state() == "timeup");
	play.tap(KeyCode::Space);
	CHECK(play.state() == "title");
}
