// test_lunarlander.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026
//
// Catch2 tests for games/lunarlander.xml, played frame by frame by a real
// xge::Game with no window: a lander drawn from lines, terrain drawn from
// lines, a pad drawn from a line, and collisions of type pixel between them;
// the moon's pull, the thrusters and their fuel; a landing and a crash told
// apart by speed on the pad; and the states that follow from each.
//
// Window::init() normally measures each object's size once a backend exists;
// a Game built on its own has {0, 0}, so measure() gives every shape the size
// its sprite implies, which is what the backends would measure too (a sprite of
// lines measures as the picture it was drawn as).

#include "command_executor.h"
#include "game.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <string>

using namespace xge;
using Catch::Matchers::WithinAbs;

namespace
{
	void measure(Game& game)
	{
		for (auto& object : game.getCurrentObjects())
		{
			object.size = measureShapeSize(object.spriteParams, object.shapeKind);
		}
	}

	// A game in the "playing" state, ready to be stepped a frame at a time.
	struct Moon
	{
		Game game{ "games/lunarlander.xml" };
		CommandExecutor executor{ game };

		Moon()
		{
			measure(game);
			game.setCurrentState("playing");
		}

		Object& lander() { return game.getObject("lander"); }
		float fuel() { return lander().variable["fuel"]; }
		bool landed() { return lander().variable["landed"] >= 1.0f; }
		bool crashed() { return lander().variable["crashed"] >= 1.0f; }
		std::string state() { return game.getCurrentState().name; }

		void key(const char* action, bool down)
		{
			executor.executeInput(Command{ CmdTriggerAction{ "lander", action } }, down);
		}

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { game.updateObjects(); }
		}

		// Some place in the sky, at rest, with the moon's pull switched off so
		// that nothing but what the test does moves it.
		void hover(float x, float y, float velocityX = 0.0f, float velocityY = 0.0f)
		{
			lander().position = { x, y };
			lander().velocity = { velocityX, velocityY };
			lander().acceleration = {};
		}

		// A pilot: holds the main thruster while it is falling faster than `limit`,
		// lets go otherwise, for at most `count` frames or until it is down.
		int fly(float limit, int count)
		{
			bool thrusting = false;
			int used = 0;
			for (; used < count && !landed() && !crashed(); ++used)
			{
				const bool want = lander().velocity.y > limit;
				if (want != thrusting)
				{
					key("thrust", want);
					thrusting = want;
				}
				game.updateObjects();
			}
			return used;
		}
	};
}

TEST_CASE("lunarlander.xml loads with a lander, some terrain and a pad, all drawn from lines", "[lunarlander]")
{
	Moon moon;

	const Object& terrain = moon.game.getObject("terrain");
	const Object& pad = moon.game.getObject("pad");
	const Object& lander = moon.lander();

	for (const Object* object : { &terrain, &pad, &lander })
	{
		CHECK(object->shapeKind == ShapeKind::Line);
		CHECK(object->collisionData.type == CollisionType::Pixel);
		REQUIRE(object->bitmap);
	}

	CHECK(terrain.bitmap->width == 800);
	CHECK(pad.size.x == 80.0f);
	CHECK(pad.size.y == 4.0f);
	CHECK(lander.size.x == 32.0f);
	CHECK(lander.size.y == 31.0f);

	CHECK(lander.acceleration.y > 0.0f);
	CHECK(moon.fuel() == 500.0f);
	CHECK(moon.state() == "playing");
}

TEST_CASE("lunarlander.xml loads under every XML backend, checked against the schema", "[lunarlander]")
{
	for (const XmlBackend backend : { XmlBackend::Xerces, XmlBackend::TinyXml2, XmlBackend::PugiXml, XmlBackend::RapidXml })
	{
		Game game{ "games/lunarlander.xml", backend };
		const Object& lander = game.getObject("lander");

		CHECK(lander.collisionData.type == CollisionType::Pixel);
		CHECK(lander.acceleration.y > 0.0f);
		CHECK(game.getObject("terrain").bitmap->width == 800);

		// Both rules about the pad came through with their speeds.
		int slow = 0;
		int fast = 0;
		for (const auto& rule : lander.collisionData.basic)
		{
			if (rule.slower) { ++slow; }
			if (rule.faster) { ++fast; }
		}
		CHECK(slow == 1);
		CHECK(fast == 1);
	}
}

TEST_CASE("left alone, the lander speeds up as it falls", "[lunarlander]")
{
	Moon moon;
	const float startY = moon.lander().position.y;

	moon.frames(60);

	CHECK_THAT(moon.lander().velocity.y, WithinAbs(0.6f, 1e-3f)); // 0.01 a frame
	CHECK(moon.lander().position.y > startY + 15.0f);
	CHECK(moon.lander().velocity.x == 0.5f); // nothing pulls it sideways
}

TEST_CASE("the main thruster slows a fall and burns one unit of fuel a frame", "[lunarlander]")
{
	Moon moon;

	moon.key("thrust", true);
	moon.frames(10);

	CHECK(moon.fuel() == 490.0f);
	CHECK_THAT(moon.lander().velocity.y, WithinAbs(10 * (0.01f - 0.035f), 1e-4f)); // going up now

	moon.key("thrust", false);
	moon.frames(10);
	CHECK(moon.fuel() == 490.0f);
}

TEST_CASE("the side thrusters push it sideways", "[lunarlander]")
{
	Moon moon;

	moon.key("left", true);
	moon.frames(30);
	moon.key("left", false);

	CHECK_THAT(moon.lander().velocity.x, WithinAbs(0.5f - 30 * 0.02f, 1e-4f));
	CHECK(moon.fuel() == 470.0f);

	moon.key("right", true);
	moon.key("left", true);
	moon.frames(5);

	CHECK_THAT(moon.lander().velocity.x, WithinAbs(0.5f - 30 * 0.02f, 1e-4f)); // opposite pushes cancel
	CHECK(moon.fuel() == 460.0f); // each of the two burns, every frame
}

TEST_CASE("with no fuel the thrusters do nothing and the moon keeps pulling", "[lunarlander]")
{
	Moon moon;
	moon.lander().variable["fuel"] = 0.0f;

	moon.key("thrust", true);
	moon.key("right", true);
	moon.frames(20);

	CHECK_THAT(moon.lander().velocity.y, WithinAbs(0.2f, 1e-4f));
	CHECK(moon.lander().velocity.x == 0.5f);
	CHECK(moon.fuel() == 0.0f);
}

TEST_CASE("the fuel shown on screen follows the fuel that is left", "[lunarlander]")
{
	Moon moon;

	moon.key("thrust", true);
	moon.frames(25);

	CHECK(moon.game.getObject("fuelvalue").spriteParams.at(1) == "475");
}

TEST_CASE("it cannot leave the sides or top of the screen", "[lunarlander]")
{
	Moon moon;

	moon.hover(2, 100, -5, 0);
	moon.frames(3);
	CHECK(moon.lander().position.x >= 0.0f);
	CHECK(moon.lander().velocity.x >= 0.0f);

	moon.hover(766, 100, 5, 0);
	moon.frames(3);
	CHECK(moon.lander().position.x + moon.lander().size.x <= 800.0f);

	moon.hover(300, 2, 0, -5);
	moon.frames(3);
	CHECK(moon.lander().position.y >= 0.0f);
}

TEST_CASE("the boxes overlap long before the pixels do, and nothing is hit until they do", "[lunarlander]")
{
	Moon moon;

	// Low over the ledge, inside the box of the whole terrain, but well above
	// any line of it: flying along, not touching anything.
	moon.hover(250, 420, 1, 0);
	REQUIRE(moon.lander().position.y + moon.lander().size.y > moon.game.getObject("terrain").position.y);

	moon.frames(20);

	CHECK_FALSE(moon.crashed());
	CHECK_FALSE(moon.landed());
	CHECK(moon.lander().position.x == 270.0f);
	CHECK(moon.state() == "playing");
}

TEST_CASE("the lander drops into a valley and is wrecked where its feet meet the ledge", "[lunarlander]")
{
	Moon moon;
	moon.lander().position = { 262, 300 };
	moon.lander().velocity = {};

	moon.frames(400);

	REQUIRE(moon.crashed());
	CHECK(moon.state() == "crashed");

	// The ledge's line is at y 560 and 561; the feet are the lowest row of the ship.
	CHECK(moon.lander().position.y + 30.0f >= 559.0f);
	CHECK(moon.lander().position.y + 30.0f <= 561.5f);
	CHECK(moon.lander().velocity.y == 0.0f);
}

TEST_CASE("a peak wrecks it however gently it arrives", "[lunarlander]")
{
	Moon moon;
	moon.lander().position = { 134, 300 };
	moon.lander().velocity = { 0, 0.3f };
	moon.lander().acceleration = {};

	moon.frames(800);

	CHECK(moon.crashed());
	CHECK_FALSE(moon.landed());
	CHECK(moon.lander().position.y > 380.0f);
	CHECK(moon.state() == "crashed");
}

TEST_CASE("falling onto the pad is a crash when it is too fast", "[lunarlander]")
{
	Moon moon;
	moon.hover(524, 300, 0, 2.0f);

	moon.frames(300);

	CHECK(moon.crashed());
	CHECK_FALSE(moon.landed());
	CHECK(moon.state() == "crashed");

	// On the pad, not the ground beside it.
	CHECK(moon.lander().position.x == 524.0f);
	CHECK(moon.lander().position.y + 30.0f >= 559.0f);
}

TEST_CASE("coming down slowly onto the pad is a landing", "[lunarlander]")
{
	Moon moon;
	moon.hover(524, 300, 0, 0.8f);

	moon.frames(400);

	CHECK(moon.landed());
	CHECK_FALSE(moon.crashed());
	CHECK(moon.state() == "landed");
	CHECK(moon.lander().velocity.y == 0.0f);

	// It stays where it touched down.
	const Vector2f where = moon.lander().position;
	moon.frames(120);
	CHECK(moon.lander().position == where);
	CHECK(moon.state() == "landed");
}

TEST_CASE("a pilot who brakes with the thruster brings it down from a drop, and has fuel left", "[lunarlander]")
{
	Moon moon;
	moon.lander().position = { 524, 100 };
	moon.lander().velocity = { 0, 0 };

	const int used = moon.fly(0.8f, 1500);

	REQUIRE(moon.landed());
	CHECK_FALSE(moon.crashed());
	CHECK(used < 1500);
	CHECK(moon.fuel() > 0.0f);
	CHECK(moon.fuel() < 500.0f);
	CHECK(moon.state() == "landed");
}

TEST_CASE("the same drop with no braking is a crash", "[lunarlander]")
{
	Moon moon;
	moon.lander().position = { 524, 100 };
	moon.lander().velocity = { 0, 0 };

	moon.frames(1500);

	CHECK(moon.crashed());
	CHECK_FALSE(moon.landed());
}

TEST_CASE("a foot over the edge of the pad is on the ground, and the ground is a crash", "[lunarlander]")
{
	Moon moon;
	// The pad is x 500 to 580; with the left foot past its end the lander
	// touches the slope beside it first, even falling slowly.
	moon.hover(486, 300, 0, 0.5f);

	moon.frames(600);

	CHECK(moon.crashed());
	CHECK_FALSE(moon.landed());
}

TEST_CASE("a thruster still held at the moment of landing does nothing afterwards", "[lunarlander]")
{
	Moon moon;
	moon.hover(524, 520, 0, 0.5f);
	moon.key("right", true);

	moon.frames(60);
	REQUIRE(moon.landed());

	const float fuel = moon.fuel();
	const Vector2f where = moon.lander().position;
	moon.frames(30);

	CHECK(moon.fuel() == fuel); // stopped for good: no burning, no moving
	CHECK(moon.lander().velocity.x == 0.0f);
	CHECK(moon.lander().position == where);
}

TEST_CASE("the pause screen holds the lander where it is", "[lunarlander]")
{
	Moon moon;
	moon.frames(10);
	const Vector2f where = moon.lander().position;
	const float velocity = moon.lander().velocity.y;

	moon.game.pushState("paused");
	moon.frames(50);

	CHECK(moon.lander().position == where);
	CHECK(moon.lander().velocity.y == velocity);

	moon.game.popState();
	moon.frames(10);
	CHECK(moon.lander().position.y > where.y);
}

TEST_CASE("starting over puts the lander, its fuel and the moon's pull back", "[lunarlander]")
{
	Moon moon;
	moon.lander().position = { 134, 300 };
	moon.frames(800);
	REQUIRE(moon.crashed());

	moon.key("thrust", true); // held as it is started over
	moon.game.resetAll();

	CHECK(moon.state() == "mainmenu");
	CHECK(moon.lander().position == moon.lander().positionOriginal);
	CHECK(moon.lander().velocity.x == 0.5f);
	CHECK(moon.lander().velocity.y == 0.0f);
	CHECK(moon.lander().acceleration.y == moon.lander().accelerationOriginal.y);
	CHECK(moon.lander().acceleration.y > 0.0f);
	CHECK(moon.fuel() == 500.0f);
	CHECK_FALSE(moon.crashed());
	CHECK_FALSE(moon.landed());
	CHECK(moon.lander().isVisible);

	// Nothing of the old thrust is left to push it.
	moon.game.setCurrentState("playing");
	moon.frames(10);
	CHECK_THAT(moon.lander().velocity.y, WithinAbs(0.1f, 1e-4f));
	CHECK(moon.fuel() == 500.0f);
}
