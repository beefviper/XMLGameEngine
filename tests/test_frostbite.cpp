// test_frostbite.cpp
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026
//
// Catch2 tests for games/frostbite.xml, played through a real xge::Engine
// with no window and the keys a player would press. Bailey jumps (<jump>)
// between the shore and four rows of drifting ice; a row he lands on while
// it is white turns blue (<become>, and a rule with sprite="white") and puts
// a block on the igloo (<reveal>); the water drowns him unless he is on a
// floe; a state <timer> takes a degree a second off the thermometer.
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
	constexpr int kFramerate = 60;
	constexpr int kJumpFrames = 21; // 0.35 seconds
	constexpr float kRowGap = 90.0f;

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

	struct Frostbite
	{
		Game game{ "games/frostbite.xml" };
		Engine engine{ game, std::make_unique<FakeWindow>() };

		Frostbite()
		{
			for (auto& object : game.getCurrentObjects())
			{
				object.size = measureShapeSize(object.spriteParams, object.shapeKind);
			}
		}

		// Starts a game with the ice, the geese and the fish held still, so a
		// test is about Bailey and not about where a floe has drifted to.
		void startStill()
		{
			tap(KeyCode::Space);
			REQUIRE(state() == "playing");
			for (auto& object : game.getCurrentObjects())
			{
				if (object.objClass == "floe" || object.objClass == "goose" || object.objClass == "fish") { object.velocity = { 0, 0 }; }
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

		// One jump, all the way to its landing and a frame after.
		void jump(KeyCode key)
		{
			tap(key);
			frames(kJumpFrames + 1);
		}

		std::string state() { return game.getCurrentState().name; }
		Object& bailey() { return game.getObject("bailey"); }
		float variable(const char* name) { return bailey().variable.at(name); }
		float degrees() { return game.getObject("thermometer").variable.at("degrees"); }

		int visible(const std::string& group)
		{
			int n = 0;
			for (const auto& object : game.getCurrentObjects())
			{
				if (object.groupName == group && object.isVisible) { ++n; }
			}
			return n;
		}

		// Every floe of a row shows this look.
		bool rowIs(const std::string& row, const std::string& look)
		{
			for (const auto& object : game.getCurrentObjects())
			{
				if (object.groupName == row && object.lookName() != look) { return false; }
			}
			return true;
		}

		// Puts Bailey on the shore, right above the first floe of a row, so
		// that jumping down `rows` times lands on that floe's row.
		void standAbove(const std::string& floe)
		{
			Object& target = game.getObject(floe);
			bailey().position.x = target.position.x + 40.0f;
		}
	};
}

TEST_CASE("frostbite.xml loads a shore, four rows of white ice, a hidden igloo and door, and Bailey", "[frostbite]")
{
	Frostbite play;
	CHECK(play.state() == "mainmenu");

	for (const char* row : { "row1", "row2", "row3", "row4" })
	{
		CHECK(play.visible(row) == 3);
		CHECK(play.rowIs(row, "white"));
	}
	CHECK(play.visible("igloo") == 0);
	CHECK_FALSE(play.game.getObject("door").isVisible);
	CHECK(play.degrees() == 45);
	CHECK(play.variable("lives") == 4);

	// Bailey stands on the shore, clear of the water.
	CHECK(play.bailey().position.y + play.bailey().size.y <= 125.0f);
}

TEST_CASE("a jump down lands on the ice a row below, taking its time, and the row turns blue and builds a block", "[frostbite]")
{
	Frostbite play;
	play.startStill();
	play.standAbove("row1.1");
	const float startY = play.bailey().position.y;

	play.tap(KeyCode::S);
	play.frames(10);
	CHECK(play.bailey().isAirborne());
	CHECK(play.bailey().position.y > startY);
	CHECK(play.bailey().position.y < startY + kRowGap);
	CHECK(play.rowIs("row1", "white"));

	play.frames(kJumpFrames - 10 + 1);
	CHECK_FALSE(play.bailey().isAirborne());
	CHECK(play.bailey().position.y == Approx(startY + kRowGap));
	CHECK(play.variable("lives") == 4); // on the ice, not in the water

	CHECK(play.rowIs("row1", "blue"));
	CHECK(play.rowIs("row2", "white"));
	CHECK(play.variable("score") == 10);
	CHECK(play.visible("igloo") == 1);

	// Standing on it, or landing on it again, earns nothing more.
	play.frames(60);
	CHECK(play.variable("score") == 10);
	play.jump(KeyCode::W);
	play.jump(KeyCode::S);
	CHECK(play.variable("score") == 10);
	CHECK(play.visible("igloo") == 1);
}

TEST_CASE("a jump into the water costs a life, the warmth comes back, and Bailey is home again", "[frostbite]")
{
	Frostbite play;
	play.startStill();
	const Vector2f home = play.bailey().positionOriginal;

	// Between the first two floes of the top row.
	play.bailey().position.x = 200;
	play.frames(90);
	REQUIRE(play.degrees() < 45);

	play.jump(KeyCode::S);
	CHECK(play.variable("lives") == 3);
	CHECK(play.degrees() == 45);
	CHECK(play.bailey().position.x == Approx(home.x));
	CHECK(play.bailey().position.y == Approx(home.y));
}

TEST_CASE("nothing is touched in mid-air: jumping across a row with a goose in it is safe", "[frostbite]")
{
	Frostbite play;
	play.startStill();
	play.standAbove("row1.1");
	play.jump(KeyCode::S);
	REQUIRE(play.variable("lives") == 4);

	// The first goose sits on row 2, right where Bailey would pass by and land.
	Object& goose = play.game.getObject("geese.1");
	Object& floe = play.game.getObject("row2.1");
	floe.position.x = play.bailey().position.x - 40.0f;
	goose.position.x = play.bailey().position.x + 30.0f;

	const float x = play.bailey().position.x;
	play.tap(KeyCode::S);
	play.frames(kJumpFrames - 1);
	CHECK(play.bailey().position.x == Approx(x));
}

TEST_CASE("up from the shore goes nowhere", "[frostbite]")
{
	Frostbite play;
	play.startStill();
	const float y = play.bailey().position.y;

	play.jump(KeyCode::W);
	CHECK(play.bailey().position.y == Approx(y));
	CHECK(play.variable("lives") == 4);
}

TEST_CASE("a floe carries Bailey along, and a goose pushes him", "[frostbite]")
{
	Frostbite play;
	play.tap(KeyCode::Space);
	play.standAbove("row1.1");
	play.game.getObject("row1.1").position.x = play.bailey().position.x - 40.0f;
	play.jump(KeyCode::S);
	REQUIRE(play.variable("lives") == 4);

	const float x = play.bailey().position.x;
	play.frames(30);
	CHECK(play.bailey().position.x == Approx(x + 30 * 1.2f).margin(2));

	// A goose catching him carries him along at its own speed.
	Object& goose = play.game.getObject("geese.1");
	goose.position = { play.bailey().position.x - 30.0f, play.bailey().position.y + 10.0f };
	const float before = play.bailey().position.x;
	play.frames(5);
	CHECK(play.bailey().position.x > before + 5 * 1.2f + 1.0f);
}

TEST_CASE("once all four rows are blue, they all turn white again", "[frostbite]")
{
	Frostbite play;
	play.startStill();

	// Line a floe of every row up under Bailey, and jump down through them.
	for (const char* floe : { "row1.1", "row2.1", "row3.1", "row4.1" })
	{
		play.game.getObject(floe).position.x = play.bailey().position.x - 40.0f;
	}

	for (int row = 1; row <= 3; ++row)
	{
		play.jump(KeyCode::S);
		REQUIRE(play.variable("lives") == 4);
	}
	CHECK(play.rowIs("row3", "blue"));

	play.jump(KeyCode::S);
	play.frames(1);
	CHECK(play.variable("score") == 40);
	CHECK(play.visible("igloo") == 4);
	for (const char* row : { "row1", "row2", "row3", "row4" })
	{
		CHECK(play.rowIs(row, "white"));
	}
	CHECK(play.variable("blue") == 0);
}

TEST_CASE("the cold takes a degree a second; at zero Bailey freezes, loses a life and starts warm on the shore", "[frostbite]")
{
	Frostbite play;
	play.startStill();

	play.frames(kFramerate * 3);
	CHECK(play.degrees() == 42);

	play.bailey().position.x = 300;
	play.game.getObject("thermometer").variable["degrees"] = 1;
	play.frames(kFramerate + 1);

	CHECK(play.variable("lives") == 3);
	CHECK(play.degrees() >= 44);
	CHECK(play.bailey().position.x == Approx(play.bailey().positionOriginal.x));
}

TEST_CASE("a finished igloo opens its door, and walking in wins", "[frostbite]")
{
	Frostbite play;
	play.startStill();

	play.bailey().variable["blocks"] = 15;
	play.frames(1);
	Object& door = play.game.getObject("door");
	REQUIRE(door.isVisible);

	// Walk right along the shore until he reaches it.
	play.engine.handleKeyPressed(KeyCode::D);
	for (int frame = 0; frame < 400 && play.state() == "playing"; ++frame) { play.frames(1); }
	play.engine.handleKeyReleased(KeyCode::D);
	CHECK(play.state() == "youwin");
}

TEST_CASE("the fish is worth a bonus, and comes back a while later", "[frostbite]")
{
	Frostbite play;
	play.startStill();
	Object& fish = play.game.getObject("fish");
	fish.velocity = fish.velocityOriginal; // swimming: touches are only checked for what moves

	// Put Bailey on the ice of the third row, on the fish.
	Object& floe = play.game.getObject("row3.1");
	play.bailey().position = { fish.position.x, floe.position.y - 2.0f };
	floe.position.x = fish.position.x - 40.0f;
	play.frames(2);

	CHECK(play.variable("score") >= 100);
	CHECK_FALSE(fish.isVisible);

	play.bailey().position = play.bailey().positionOriginal;
	play.frames(12 * kFramerate);
	CHECK(fish.isVisible);
}

TEST_CASE("no lives left is game over, and Space starts over with the ice white and the igloo gone", "[frostbite]")
{
	Frostbite play;
	play.startStill();
	play.standAbove("row1.1");
	play.jump(KeyCode::S);
	REQUIRE(play.rowIs("row1", "blue"));

	play.bailey().variable["lives"] = 0;
	play.frames(1);
	CHECK(play.state() == "gameover");

	play.tap(KeyCode::Space);
	CHECK(play.state() == "mainmenu");
	CHECK(play.rowIs("row1", "white"));
	CHECK(play.visible("igloo") == 0);
	CHECK(play.variable("lives") == 4);
	CHECK(play.degrees() == 45);
}
