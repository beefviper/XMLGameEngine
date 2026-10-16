// test_adventure.cpp
// XML Game Engine
// author: beefviper
// date: Oct 14, 2026
//
// Catch2 tests for games/descriptions/adventure.xml, played through a real
// xge::Engine with no window and the keys a player would press: <carry />
// (picking a thing up by walking into it, taking it along, one at a time,
// from another carrier), <drop /> (and not picking it up again until walked
// off), the key opening the gate on its own way there, the sword slaying the
// dragon, the dragon eating the hero, the two rooms (scenes) walked between
// with what is carried, and the chalice carried home from the field.
//
// Window::init() normally measures each object's size once a backend exists;
// the helper gives every shape the size its sprite implies instead.

#include "engine/engine.h"
#include "game/game.h"
#include "play/verbs.h"

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

	struct Adventure
	{
		Game game{ "descriptions/adventure.xml" };
		Engine engine{ game, std::make_unique<FakeWindow>() };

		Adventure()
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

		// Into the room, with the dragon gone unless a test wants it.
		void start(bool withDragon = false)
		{
			tap(KeyCode::Space);
			REQUIRE(state() == "playing");
			if (!withDragon) { die(object("dragon")); }
		}

		Object& object(const std::string& name) { return game.getObject(name); }
		Object& hero() { return object("hero"); }
		bool carrying(const std::string& name) { return hero().carrying == object(name).name; }

		std::string room() { return game.currentScene("playing"); }
		void enter(const std::string& room) { game.enterScene("playing." + room); }

		// Puts the hero (and anything) somewhere, standing still.
		void put(const std::string& name, float x, float y)
		{
			object(name).position = { x, y };
		}

		std::string state() { return game.getCurrentState().name; }
	};
}

TEST_CASE("the hero picks up what he walks into and takes it along, held where it was", "[adventure][carry]")
{
	Adventure play;
	play.start();
	play.put("hero", 520, 420);
	play.put("key", 540, 420);

	play.hold(KeyCode::D, 10);
	REQUIRE(play.carrying("key"));
	const Vector2f held = play.object("key").position - play.hero().position;
	CHECK(held.x > 0.0f);

	// Up and to the left: the key goes too, the same distance from him.
	play.hold(KeyCode::W, 30);
	play.hold(KeyCode::A, 30);
	CHECK(play.hero().position.y == Approx(330.0f));
	CHECK(play.object("key").position.x == Approx(play.hero().position.x + held.x));
	CHECK(play.object("key").position.y == Approx(play.hero().position.y + held.y));
}

TEST_CASE("one thing at a time: picking up another puts the first down where it is", "[adventure][carry]")
{
	Adventure play;
	play.start();
	play.enter("field");
	play.put("hero", 100, 300);
	play.put("chalice", 120, 300);
	play.put("sword", 100, 200);

	play.hold(KeyCode::Right, 5);
	REQUIRE(play.carrying("chalice"));

	play.hold(KeyCode::Up, 40);
	CHECK(play.carrying("sword"));
	const Vector2f chaliceLeft = play.object("chalice").position;

	play.hold(KeyCode::Down, 20);
	CHECK(play.object("chalice").position == chaliceLeft);
	CHECK(play.object("sword").position.y > 200.0f);
}

TEST_CASE("Space puts it down, and it is not picked up again until he has walked off it", "[adventure][carry]")
{
	Adventure play;
	play.start();
	play.put("hero", 520, 420);
	play.put("key", 525, 422);

	play.hold(KeyCode::Left, 2);
	REQUIRE(play.carrying("key"));

	play.tap(KeyCode::Space);
	play.frames(1);
	CHECK(play.hero().carrying.empty());
	const Vector2f keyAt = play.object("key").position;

	// Still on it: walking does not pick it up.
	play.hold(KeyCode::Left, 3);
	CHECK(play.hero().carrying.empty());
	CHECK(play.object("key").position == keyAt);

	// Off it, and back onto it: picked up.
	play.hold(KeyCode::Left, 20);
	play.hold(KeyCode::Right, 25);
	CHECK(play.carrying("key"));
}

TEST_CASE("the gate stops the hero, and opens to the key he carries", "[adventure][carry]")
{
	Adventure play;
	play.start();
	const float gateBottom = play.object("gate").position.y + play.object("gate").size.y;

	// No key: stopped at the gate.
	play.put("hero", 315, 250);
	play.hold(KeyCode::Up, 60);
	CHECK(play.hero().position.y == Approx(gateBottom).margin(0.01));
	CHECK(play.game.isShown(play.object("gate")));

	// The key held ahead of him reaches the gate first, which lifts.
	play.put("hero", 315, 300);
	play.put("key", 312, 280);
	play.hold(KeyCode::Up, 5);
	REQUIRE(play.carrying("key"));
	play.hold(KeyCode::Up, 60);
	CHECK_FALSE(play.object("gate").collisionData.enabled);
	CHECK(play.hero().position.y < gateBottom);
}

TEST_CASE("the dragon chases the hero and eats him; the sword slays it", "[adventure][carry]")
{
	{
		Adventure play;
		play.start(true);
		play.enter("field");
		play.put("hero", 400, 200);
		play.put("dragon", 450, 200);
		play.frames(120);
		CHECK(play.state() == "eaten");
	}
	{
		// The sword lying between them: the dragon runs into it.
		Adventure play;
		play.start(true);
		play.enter("field");
		play.put("hero", 400, 200);
		play.put("sword", 430, 205);
		play.put("dragon", 480, 200);
		play.frames(120);
		CHECK_FALSE(play.object("dragon").isVisible);
		CHECK(play.state() == "playing");
	}
}

TEST_CASE("the dragon sleeps until the hero comes near, then wakes once, with a roar, and chases him", "[adventure][conditions]")
{
	Adventure play;
	play.start(true);
	play.enter("field");
	Object& dragon = play.object("dragon");
	play.put("hero", 100, 300);
	play.put("dragon", 400, 300); // 300 apart
	play.frames(60);
	CHECK(dragon.lookName() == "asleep");
	CHECK(dragon.position.x == Approx(400.0f));

	// Within 160: awake (with a roar), and after him.
	play.put("hero", 260, 300);
	play.frames(1);
	CHECK(dragon.lookName() == "awake");
	CHECK(dragon.variable.at("awake") == 1.0f);
	play.frames(30);
	CHECK(dragon.position.x < 400.0f);
	CHECK(dragon.variable.at("awake") == 1.0f); // woken once

	// A new game: asleep again.
	play.game.resetAll();
	CHECK(dragon.lookName() == "asleep");
	CHECK(dragon.variable.at("awake") == 0.0f);
}

TEST_CASE("the field and the castle are two rooms: walking through a side enters the other, and the one left waits", "[adventure][scenes]")
{
	Adventure play;
	play.start(true);
	REQUIRE(play.room() == "playing.castle");
	CHECK(play.game.isAway(play.object("dragon")));
	CHECK_FALSE(play.game.isShown(play.object("dragon")));
	CHECK(play.game.isShown(play.object("key")));
	const Vector2f dragonAt = play.object("dragon").position;

	// East, through the castle's unseen side: in at the west of the field.
	play.put("hero", 560, 300);
	play.hold(KeyCode::Right, 40);
	CHECK(play.room() == "playing.field");
	CHECK(play.hero().position.x < 100.0f);
	CHECK(play.game.isShown(play.object("dragon")));
	CHECK(play.game.isAway(play.object("key")));
	CHECK(play.object("dragon").position == dragonAt); // asleep, he is far off

	// It wakes when he is near.
	play.put("dragon", play.hero().position.x + 120, play.hero().position.y);
	play.frames(1);
	const Vector2f dragonWoke = play.object("dragon").position;

	// And straight back, before the dragon gets far.
	play.hold(KeyCode::Left, 30);
	CHECK(play.room() == "playing.castle");
	CHECK(play.hero().position.x > 500.0f);
	const Vector2f dragonLeft = play.object("dragon").position;
	CHECK(dragonLeft != dragonWoke);
	play.frames(60);
	CHECK(play.object("dragon").position == dragonLeft);
}

TEST_CASE("the chalice carried from the field into the hall wins, and a new game starts with nothing carried", "[adventure][carry][scenes]")
{
	Adventure play;
	play.start();
	die(play.object("gate"));

	// Picked up in the field and carried west through its side into the castle.
	play.enter("field");
	play.put("hero", 80, 300);
	play.put("chalice", 50, 300);
	play.hold(KeyCode::Left, 3);
	REQUIRE(play.carrying("chalice"));
	play.hold(KeyCode::Left, 30);
	REQUIRE(play.room() == "playing.castle");
	CHECK(play.carrying("chalice"));
	CHECK(play.game.isShown(play.object("chalice")));
	CHECK(play.object("chalice").ownerScene == "playing.castle");

	// Along to the gate, and up into the hall.
	play.hold(KeyCode::Left, 96);
	REQUIRE(play.carrying("chalice"));
	play.hold(KeyCode::Up, 120);
	CHECK(play.state() == "won");
	CHECK(play.game.isStill(play.hero()));

	play.tap(KeyCode::Space);
	CHECK(play.state() == "title");
	CHECK(play.hero().carrying.empty());
	CHECK(play.room() == "playing.castle");
	CHECK(play.object("chalice").position == play.object("chalice").positionOriginal);
	CHECK(play.object("chalice").ownerScene == "playing.field");
}

TEST_CASE("paused, the room is drawn still underneath and nothing in it moves", "[adventure][scenes]")
{
	Adventure play;
	play.start();
	play.enter("field");
	play.tap(KeyCode::P);
	REQUIRE(play.state() == "paused");
	CHECK(play.game.isStill(play.hero()));
	CHECK(play.game.isStill(play.object("chalice")));
	CHECK_FALSE(play.game.isStill(play.object("key")));
	CHECK_FALSE(play.game.isShown(play.hero()));
}

TEST_CASE("a carrier out of play puts down what it carried", "[adventure][carry]")
{
	Adventure play;
	play.start();
	play.put("hero", 520, 420);
	play.put("key", 540, 420);
	play.hold(KeyCode::D, 10);
	REQUIRE(play.carrying("key"));

	die(play.hero());
	play.frames(1);
	CHECK(play.hero().carrying.empty());
}
