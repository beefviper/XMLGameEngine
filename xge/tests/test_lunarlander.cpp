// test_lunarlander.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026
//
// Catch2 tests for games/descriptions/lunarlander.xml, played frame by frame by a real
// xge::Game with no window: a lander, terrain and pads drawn from lines, and
// collisions of type pixel between them; the moon's pull, turning and the
// engine and its fuel; a landing told from a wreck by speed and lean, each
// pad worth its own points; lander after lander until the fuel is gone.
//
// Window::init() normally measures each object's size once a backend exists;
// a Game built on its own has {0, 0}, so measure() gives every shape the size
// its sprite implies, which is what the backends would measure too (a sprite of
// lines measures as the picture it was drawn as).

#include "play/executor.h"
#include "game/game.h"
#include "load/xml.h"

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
			object.size = measureShapeSize(object.picture);
		}
	}

	// A game in the "playing" state, ready to be stepped a frame at a time.
	struct Moon
	{
		Game game{ "descriptions/lunarlander.xml" };
		CommandExecutor executor{ game };

		Moon()
		{
			measure(game);
			game.setCurrentState("playing");
		}

		Object& lander() { return game.getObject("lander"); }
		float fuel() { return lander().variable["fuel"]; }
		float score() { return game.getObject("status").variable["score"]; }
		bool wrecked() { return !lander().isVisible; }
		std::string state() { return game.getCurrentState().name; }

		void key(const char* action, bool down)
		{
			executor.executeInput(Command{ CmdTriggerAction{ "lander", action } }, down);
		}

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { game.updateObjects(); }
		}

		// Over the middle of a pad, upright, coming down at `fall`, with the
		// moon's pull switched off so that nothing but the test moves it.
		void over(const char* pad, float y, float fall)
		{
			const Object& at = game.getObject(pad);
			lander().position = { at.position.x + at.size.x / 2 - lander().size.x / 2, y };
			lander().velocity = { 0, fall };
			lander().acceleration = {};
		}

		// Frames until it is down, one way or the other, or `count` have gone.
		void untilDown(int count)
		{
			for (int i = 0; i < count && state() == "playing" && !wrecked(); ++i) { game.updateObjects(); }
		}
	};
}

TEST_CASE("lunarlander.xml loads with a lander, terrain and three pads, all drawn from lines", "[lunarlander]")
{
	Moon moon;

	for (const char* name : { "terrain", "pad2", "pad3", "pad5", "lander" })
	{
		const Object& object = moon.game.getObject(name);
		CHECK(object.picture.kind == ShapeKind::Line);
		CHECK(object.collisionData.type == CollisionType::Pixel);
		REQUIRE(object.bitmap);
	}
	for (const char* name : { "terrain", "pad2", "pad3", "pad5" })
	{
		CHECK(moon.game.getObject(name).objClass == "ground");
	}

	// The narrower the pad, the more it is worth.
	CHECK(moon.game.getObject("pad5").size.x < moon.game.getObject("pad3").size.x);
	CHECK(moon.game.getObject("pad3").size.x < moon.game.getObject("pad2").size.x);

	CHECK(moon.lander().hasHeading);
	CHECK(moon.lander().heading == 0.0f);
	CHECK(moon.lander().acceleration.y > 0.0f);
	CHECK(moon.fuel() == 1000.0f);
	CHECK(moon.score() == 0.0f);
}

TEST_CASE("lunarlander.xml loads under every XML backend, checked against the schema", "[lunarlander]")
{
	for (const XmlBackend backend : { XmlBackend::Xerces, XmlBackend::TinyXml2, XmlBackend::PugiXml, XmlBackend::RapidXml })
	{
		Game game{ readGame("descriptions/lunarlander.xml", backend) };
		const Object& lander = game.getObject("lander");

		CHECK(lander.collisionData.type == CollisionType::Pixel);
		CHECK(lander.hasHeading);

		// One rule is the landing, kept for slow touches.
		int slow = 0;
		for (const auto& rule : lander.collisionData.others)
		{
			if (rule.slower) { ++slow; }
		}
		CHECK(slow == 1);
	}
}

TEST_CASE("left alone, the lander speeds up as it falls", "[lunarlander]")
{
	Moon moon;
	const float startY = moon.lander().position.y;
	const float drift = moon.lander().velocity.x;

	moon.frames(60);

	CHECK_THAT(moon.lander().velocity.y, WithinAbs(0.6f, 1e-3f)); // 0.01 a frame
	CHECK(moon.lander().position.y > startY + 15.0f);
	CHECK(moon.lander().velocity.x == drift); // nothing pulls it sideways
	CHECK(drift >= 0.5f);
	CHECK(drift <= 1.2f);
}

TEST_CASE("the engine pushes the way the lander points and burns a unit of fuel a frame; turning is free", "[lunarlander]")
{
	Moon moon;
	moon.lander().velocity = {};
	moon.lander().acceleration = {};

	moon.key("thrust", true);
	moon.frames(10);
	moon.key("thrust", false);
	CHECK(moon.fuel() == 990.0f);
	CHECK_THAT(moon.lander().velocity.y, WithinAbs(-0.3f, 1e-4f)); // upright: straight up
	CHECK_THAT(moon.lander().velocity.x, WithinAbs(0.0f, 1e-4f));

	moon.key("right", true);
	moon.frames(30);
	moon.key("right", false);
	CHECK(moon.lander().heading == 90.0f);
	CHECK(moon.fuel() == 990.0f);

	moon.key("thrust", true);
	moon.frames(10);
	moon.key("thrust", false);
	CHECK_THAT(moon.lander().velocity.x, WithinAbs(0.3f, 1e-4f)); // on its side: sideways
	CHECK(moon.fuel() == 980.0f);
}

TEST_CASE("with no fuel the engine does nothing and the moon keeps pulling", "[lunarlander]")
{
	Moon moon;
	moon.lander().variable["fuel"] = 0.0f;
	moon.lander().velocity = {};

	moon.key("thrust", true);
	moon.frames(20);

	CHECK_THAT(moon.lander().velocity.y, WithinAbs(0.2f, 1e-4f));
	CHECK(moon.fuel() == 0.0f);
}

TEST_CASE("the fuel and score shown follow the lander's", "[lunarlander]")
{
	Moon moon;

	moon.key("thrust", true);
	moon.frames(25);

	CHECK(moon.game.getObject("fuel").picture.text == "FUEL 975");
	CHECK(moon.game.getObject("score").picture.text == "SCORE 0");
}

TEST_CASE("it cannot leave the sides or top of the screen", "[lunarlander]")
{
	Moon moon;

	moon.lander().position = { 2, 100 };
	moon.lander().velocity = { -5, 0 };
	moon.frames(3);
	CHECK(moon.lander().position.x >= 0.0f);

	moon.lander().position = { 800 - moon.lander().size.x - 2, 100 };
	moon.lander().velocity = { 5, 0 };
	moon.frames(3);
	CHECK(moon.lander().position.x + moon.lander().size.x <= 800.0f);

	moon.lander().position = { 300, 2 };
	moon.lander().velocity = { 0, -5 };
	moon.frames(3);
	CHECK(moon.lander().position.y >= 0.0f);
}

TEST_CASE("coming down slowly and upright onto a pad is a landing worth that pad's points", "[lunarlander]")
{
	struct Pad { const char* name; float worth; };
	for (const Pad pad : { Pad{ "pad2", 100 }, Pad{ "pad3", 150 }, Pad{ "pad5", 250 } })
	{
		DYNAMIC_SECTION(pad.name)
		{
			Moon moon;
			moon.over(pad.name, moon.game.getObject(pad.name).position.y - 80, 0.8f);

			moon.untilDown(400);

			REQUIRE(moon.state() == "landed");
			CHECK_FALSE(moon.wrecked());
			CHECK(moon.score() == pad.worth);
			CHECK(moon.lander().velocity.y == 0.0f);
			CHECK(moon.game.isShown(moon.game.getObject("landed.message")));
			CHECK(moon.game.getObject("points").picture.text == std::to_string(static_cast<int>(pad.worth)) + " POINTS");

			// It stays where it touched down, drawn still under the message.
			const Vector2f where = moon.lander().position;
			moon.frames(60);
			CHECK(moon.lander().position == where);
			CHECK(moon.game.isStill(moon.lander()));
		}
	}
}

TEST_CASE("after a landing the next lander comes down from the top, with the fuel and score kept", "[lunarlander]")
{
	Moon moon;
	moon.over("pad2", 470, 0.8f);
	moon.key("thrust", true);
	moon.frames(5);
	moon.key("thrust", false);
	moon.untilDown(400);
	REQUIRE(moon.state() == "landed");
	const float fuel = moon.fuel();

	moon.frames(160); // the message is shown for 2.5 seconds

	CHECK(moon.state() == "playing");
	CHECK(moon.lander().position.y < 45.0f);
	CHECK(moon.lander().heading == 0.0f);
	CHECK(moon.lander().acceleration.y > 0.0f);
	CHECK(moon.score() == 100.0f);
	CHECK(moon.fuel() == fuel);
	CHECK(fuel < 1000.0f);

	// Down again, it scores again.
	moon.over("pad3", 420, 0.8f);
	moon.untilDown(400);
	CHECK(moon.state() == "landed");
	CHECK(moon.score() == 250.0f);
}

TEST_CASE("a lander leaning too far is wrecked on the pad, however slowly", "[lunarlander]")
{
	Moon moon;
	moon.over("pad2", 470, 0.8f);
	moon.lander().heading = 30.0f;
	moon.lander().showHeading();

	moon.untilDown(400);

	CHECK(moon.wrecked());
	CHECK(moon.state() == "playing");
	CHECK(moon.score() == 0.0f);
}

TEST_CASE("a little lean is still a landing", "[lunarlander]")
{
	Moon moon;
	moon.over("pad2", 470, 0.8f);
	moon.lander().heading = 354.0f;
	moon.lander().showHeading();

	moon.untilDown(400);

	CHECK(moon.state() == "landed");
}

TEST_CASE("falling onto the pad too fast is a wreck", "[lunarlander]")
{
	Moon moon;
	moon.over("pad2", 470, 2.0f);

	moon.untilDown(400);

	CHECK(moon.wrecked());
	CHECK(moon.score() == 0.0f);
}

TEST_CASE("a wreck breaks the lander up, costs fuel, and the next one comes after the message", "[lunarlander]")
{
	Moon moon;
	// Onto the peak to the left of the 5X pad.
	moon.lander().position = { 85, 300 };
	moon.lander().velocity = { 0, 0.3f };
	moon.lander().acceleration = {};

	moon.untilDown(800);
	REQUIRE(moon.wrecked());
	moon.frames(1);

	CHECK(moon.fuel() == 800.0f);
	CHECK(moon.game.isShown(moon.game.getObject("wrecked")));
	for (int piece = 1; piece <= 4; ++piece)
	{
		CHECK(moon.game.isShown(moon.game.getObject("debris." + std::to_string(piece) + ".1")));
	}
	const float pieceY = moon.game.getObject("debris.1.1").position.y;
	moon.frames(20);
	CHECK(moon.game.getObject("debris.1.1").position.y != pieceY);

	moon.frames(150);

	CHECK_FALSE(moon.wrecked());
	CHECK(moon.lander().position.y < 45.0f);
	CHECK_FALSE(moon.game.isShown(moon.game.getObject("wrecked")));
	CHECK(moon.game.tryGetObject("debris.1.1") == nullptr); // taken away for the next lander
	CHECK(moon.fuel() == 800.0f);
}

TEST_CASE("the boxes overlap long before the pixels do, and nothing is hit until they do", "[lunarlander]")
{
	Moon moon;

	// Low over the 2X pad's valley, inside the box of the whole terrain, but
	// well above any line of it: flying along, not touching anything.
	moon.lander().position = { 262, 450 };
	moon.lander().velocity = { 1, 0 };
	moon.lander().acceleration = {};
	REQUIRE(moon.lander().position.y + moon.lander().size.y > moon.game.getObject("terrain").position.y);

	moon.frames(20);

	CHECK_FALSE(moon.wrecked());
	CHECK(moon.lander().position.x == 282.0f);
	CHECK(moon.state() == "playing");
}

TEST_CASE("a pilot who brakes with the engine brings it down from a drop, and has fuel left", "[lunarlander]")
{
	Moon moon;
	moon.over("pad2", 100, 0);
	moon.lander().acceleration.y = 0.01f;

	bool thrusting = false;
	for (int frame = 0; frame < 1500 && moon.state() == "playing" && !moon.wrecked(); ++frame)
	{
		const bool want = moon.lander().velocity.y > 0.8f;
		if (want != thrusting)
		{
			moon.key("thrust", want);
			thrusting = want;
		}
		moon.frames(1);
	}

	CHECK(moon.state() == "landed");
	CHECK(moon.fuel() > 0.0f);
	CHECK(moon.fuel() < 1000.0f);
}

TEST_CASE("the last lander down with no fuel ends the game, and Space starts it over", "[lunarlander]")
{
	Moon moon;
	moon.game.incrementText("lander.fuel", -850.0f);
	moon.lander().position = { 85, 300 };
	moon.lander().velocity = { 0, 0.3f };

	moon.untilDown(800);
	REQUIRE(moon.wrecked());
	moon.frames(170);

	REQUIRE(moon.state() == "gameover");
	CHECK(moon.fuel() == 0.0f);
	CHECK(moon.game.isShown(moon.game.getObject("gameover.message")));

	moon.game.resetAll();
	CHECK(moon.state() == "mainmenu");
	CHECK(moon.fuel() == 1000.0f);
	CHECK(moon.score() == 0.0f);
	CHECK(moon.lander().isVisible);
	CHECK(moon.lander().heading == 0.0f);
}

TEST_CASE("the pause screen holds the lander where it is", "[lunarlander]")
{
	Moon moon;
	moon.frames(10);
	const Vector2f where = moon.lander().position;

	moon.game.pushState("paused");
	moon.frames(50);

	CHECK(moon.lander().position == where);
	CHECK(moon.game.isStill(moon.lander()));
	CHECK(moon.game.isStill(moon.game.getObject("terrain")));
	CHECK(moon.game.isShown(moon.game.getObject("paused")));
	CHECK_FALSE(moon.game.isStill(moon.game.getObject("title")));

	moon.game.popState();
	moon.frames(10);
	CHECK(moon.lander().position.y > where.y);
}
