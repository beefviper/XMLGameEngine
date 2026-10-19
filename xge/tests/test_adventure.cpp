// test_adventure.cpp
// XML Game Engine
// author: beefviper
// date: Oct 14, 2026
//
// Catch2 tests for games/descriptions/adventure.xml, played through a real
// xge::Engine with no window and the keys a player would press: <carry />
// (picking a thing up by walking into it, taking it along, one at a time,
// from another carrier), <drop /> (and not picking it up again until walked
// off), a key opening its castle's gate on its own way there, in through the
// gate to the hall and out again, the sword slaying a dragon, a dragon
// eating the hero, the five rooms (scenes) walked between with what is
// carried, and the chalice carried home from the black castle's dungeon.
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

		// Into the first room, with the dragons gone unless a test wants them.
		void start(bool withDragons = false)
		{
			tap(KeyCode::Space);
			REQUIRE(state() == "playing");
			if (!withDragons)
			{
				die(object("yorgle"));
				die(object("grundle"));
			}
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
	play.put("hero", 80, 380);
	play.put("sword", 100, 380);

	play.hold(KeyCode::D, 10);
	REQUIRE(play.carrying("sword"));
	const Vector2f held = play.object("sword").position - play.hero().position;
	CHECK(held.x > 0.0f);

	// Up and to the right: the sword goes too, the same distance from him.
	play.hold(KeyCode::W, 30);
	play.hold(KeyCode::D, 30);
	CHECK(play.hero().position.y == Approx(290.0f));
	CHECK(play.object("sword").position.x == Approx(play.hero().position.x + held.x));
	CHECK(play.object("sword").position.y == Approx(play.hero().position.y + held.y));
}

TEST_CASE("one thing at a time: picking up another puts the first down where it is", "[adventure][carry]")
{
	Adventure play;
	play.start();
	play.put("hero", 80, 380);
	play.put("sword", 100, 380);
	play.hold(KeyCode::Right, 5);
	REQUIRE(play.carrying("sword"));

	// Into the field with it, onto the gold key.
	play.enter("field");
	play.put("hero", 300, 300);
	play.put("goldkey", 300, 220);
	play.hold(KeyCode::Up, 40);
	CHECK(play.carrying("goldkey"));
	const Vector2f swordLeft = play.object("sword").position;
	CHECK(play.object("sword").ownerScene == "playing.field");

	play.hold(KeyCode::Down, 20);
	CHECK(play.object("sword").position == swordLeft);
	CHECK(play.object("goldkey").position.y > 220.0f);
}

TEST_CASE("Space puts it down, and it is not picked up again until he has walked off it", "[adventure][carry]")
{
	Adventure play;
	play.start();
	play.put("hero", 120, 380);
	play.put("sword", 105, 382);

	play.hold(KeyCode::Left, 2);
	REQUIRE(play.carrying("sword"));

	play.tap(KeyCode::Space);
	play.frames(1);
	CHECK(play.hero().carrying.empty());
	const Vector2f swordAt = play.object("sword").position;

	// Still on it: walking does not pick it up.
	play.hold(KeyCode::Left, 3);
	CHECK(play.hero().carrying.empty());
	CHECK(play.object("sword").position == swordAt);

	// Off it, and back onto it: picked up.
	play.hold(KeyCode::Left, 20);
	play.hold(KeyCode::Right, 25);
	CHECK(play.carrying("sword"));
}

TEST_CASE("the portcullis stops the hero, and rises for its own key he carries", "[adventure][carry]")
{
	Adventure play;
	play.start();
	Object& gate = play.object("goldgate");
	const float gateBottom = gate.position.y + gate.size.y;

	// No key: stopped at the gate.
	play.put("hero", 315, 300);
	play.hold(KeyCode::Up, 60);
	CHECK(play.hero().position.y == Approx(gateBottom).margin(0.01));
	CHECK(play.game.isShown(gate));

	// The black key does nothing to it.
	play.put("blackkey", 312, 270);
	play.object("blackkey").ownerScene = "playing.castle";
	play.put("hero", 315, 290);
	play.hold(KeyCode::Up, 30);
	CHECK(play.game.isShown(gate));
	play.tap(KeyCode::Space);
	play.object("blackkey").ownerScene = "playing.hall";
	play.put("blackkey", 300, 200);

	// The gold key held ahead of him reaches the gate first, which rises.
	play.enter("field");
	play.put("hero", 300, 300);
	play.put("goldkey", 297, 280);
	play.hold(KeyCode::Up, 5);
	REQUIRE(play.carrying("goldkey"));
	play.enter("castle");
	play.put("hero", 315, 300);
	play.hold(KeyCode::Up, 40);
	CHECK_FALSE(play.game.isShown(gate));
	CHECK(play.hero().position.y < gateBottom);
}

TEST_CASE("through the castle's arch into its hall, and out at the bottom of the hall", "[adventure][scenes]")
{
	Adventure play;
	play.start();
	die(play.object("goldgate"));

	play.put("hero", 315, 260);
	play.hold(KeyCode::Up, 45);
	REQUIRE(play.room() == "playing.hall");
	CHECK(play.hero().position.y > 380.0f);
	CHECK(play.game.isShown(play.object("blackkey")));

	play.hold(KeyCode::Down, 40);
	REQUIRE(play.room() == "playing.castle");
	CHECK(play.hero().position.y > 200.0f);
	CHECK(play.hero().position.y < 300.0f);
}

TEST_CASE("a dragon chases the hero and eats him; the sword slays it", "[adventure][carry]")
{
	{
		Adventure play;
		play.start(true);
		play.enter("field");
		play.put("hero", 300, 200);
		play.put("yorgle", 350, 200);
		play.frames(120);
		CHECK(play.state() == "eaten");
	}
	{
		// The sword lying between them: the dragon runs into it.
		Adventure play;
		play.start(true);
		play.enter("field");
		play.object("sword").ownerScene = "playing.field";
		play.put("hero", 300, 200);
		play.put("sword", 330, 215);
		play.put("yorgle", 380, 200);
		play.frames(120);
		CHECK_FALSE(play.object("yorgle").isVisible);
		CHECK(play.state() == "playing");
	}
}

TEST_CASE("a dragon sleeps until the hero comes near, then wakes once, with a roar, and chases him", "[adventure][conditions]")
{
	Adventure play;
	play.start(true);
	play.enter("field");
	Object& dragon = play.object("yorgle");
	play.put("hero", 100, 300);
	play.put("yorgle", 400, 300); // 300 apart
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

	// Grundle is still asleep in his dungeon.
	CHECK(play.object("grundle").lookName() == "asleep");

	// A new game: asleep again.
	play.game.resetAll();
	CHECK(dragon.lookName() == "asleep");
	CHECK(dragon.variable.at("awake") == 0.0f);
}

TEST_CASE("walking through a side enters the next room, and the one left waits", "[adventure][scenes]")
{
	Adventure play;
	play.start(true);
	REQUIRE(play.room() == "playing.castle");
	CHECK(play.game.isAway(play.object("yorgle")));
	CHECK_FALSE(play.game.isShown(play.object("yorgle")));
	CHECK(play.game.isShown(play.object("sword")));
	const Vector2f dragonAt = play.object("yorgle").position;

	// East, through the castle room's unseen side: in at the west of the field.
	play.put("hero", 560, 300);
	play.hold(KeyCode::Right, 40);
	CHECK(play.room() == "playing.field");
	CHECK(play.hero().position.x < 100.0f);
	CHECK(play.game.isShown(play.object("yorgle")));
	CHECK(play.game.isAway(play.object("sword")));
	CHECK(play.object("yorgle").position == dragonAt); // asleep, he is far off

	// It wakes when he is near.
	play.put("yorgle", play.hero().position.x + 120, play.hero().position.y);
	play.frames(1);
	const Vector2f dragonWoke = play.object("yorgle").position;

	// And straight back, before the dragon gets far.
	play.hold(KeyCode::Left, 30);
	CHECK(play.room() == "playing.castle");
	CHECK(play.hero().position.x > 500.0f);
	const Vector2f dragonLeft = play.object("yorgle").position;
	CHECK(dragonLeft != dragonWoke);
	play.frames(60);
	CHECK(play.object("yorgle").position == dragonLeft);

	// On east twice: the black castle, whose east side is a wall.
	play.enter("field");
	play.put("hero", 560, 400);
	play.hold(KeyCode::Right, 40);
	CHECK(play.room() == "playing.blackcastle");
	play.hold(KeyCode::Right, 250);
	CHECK(play.room() == "playing.blackcastle");
}

TEST_CASE("the chalice carried from the dungeon into the gold castle's hall wins, and a new game starts with nothing carried", "[adventure][carry][scenes]")
{
	Adventure play;
	play.start();
	die(play.object("goldgate"));
	die(play.object("blackgate"));

	// Picked up in the dungeon and carried out through the bottom.
	play.enter("dungeon");
	play.put("hero", 315, 400);
	play.put("chalice", 292, 400);
	play.hold(KeyCode::Left, 3);
	REQUIRE(play.carrying("chalice"));
	play.hold(KeyCode::Down, 40);
	REQUIRE(play.room() == "playing.blackcastle");
	CHECK(play.carrying("chalice"));
	CHECK(play.object("chalice").ownerScene == "playing.blackcastle");

	// West, west, and up through the gold castle's arch.
	play.hold(KeyCode::Left, 120);
	REQUIRE(play.room() == "playing.field");
	play.hold(KeyCode::Left, 220);
	REQUIRE(play.room() == "playing.castle");
	REQUIRE(play.carrying("chalice"));
	play.put("hero", 318, 230);
	play.hold(KeyCode::Up, 60);
	CHECK(play.state() == "won");
	CHECK(play.game.isStill(play.hero()));

	play.tap(KeyCode::Space);
	CHECK(play.state() == "title");
	CHECK(play.hero().carrying.empty());
	CHECK(play.room() == "playing.castle");
	CHECK(play.object("chalice").position == play.object("chalice").positionOriginal);
	CHECK(play.object("chalice").ownerScene == "playing.dungeon");
}

TEST_CASE("paused, the room is drawn still underneath and nothing in it moves", "[adventure][scenes]")
{
	Adventure play;
	play.start();
	play.enter("dungeon");
	play.tap(KeyCode::P);
	REQUIRE(play.state() == "paused");
	CHECK(play.game.isStill(play.hero()));
	CHECK(play.game.isStill(play.object("chalice")));
	CHECK_FALSE(play.game.isStill(play.object("sword")));
	CHECK_FALSE(play.game.isShown(play.hero()));
}

TEST_CASE("a carrier out of play puts down what it carried", "[adventure][carry]")
{
	Adventure play;
	play.start();
	play.put("hero", 80, 380);
	play.put("sword", 100, 380);
	play.hold(KeyCode::D, 10);
	REQUIRE(play.carrying("sword"));

	die(play.hero());
	play.frames(1);
	CHECK(play.hero().carrying.empty());
}
