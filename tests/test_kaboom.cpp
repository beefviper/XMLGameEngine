// test_kaboom.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026
//
// Catch2 tests for games/kaboom.xml, played frame by frame by a real
// xge::Game with no window. Kaboom! is described with verbs the language
// already had: held <move>, <stick />, <inc /> and <dec />, <reset /> (for a
// bomb that starts its fall again and, from a condition, for a whole wave of
// them going off at once), a state per wave, and variable conditions for the
// waves and the two ends of the game. Fall speeds and starting heights are
// drawn from <random> once at load, so these tests are about the game working
// whatever those draws were.
//
// Window::init() normally measures each object's size once a backend exists;
// measure() gives every shape the size its sprite implies instead.

#include "command_executor.h"
#include "engine.h"
#include "game.h"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <vector>

using namespace xge;

namespace
{
	constexpr float kWindowWidth = 800.0f;
	constexpr float kWindowHeight = 600.0f;

	void measure(Game& game)
	{
		for (auto& object : game.getCurrentObjects())
		{
			object.size = measureShapeSize(object.spriteParams, object.shapeKind);
		}
	}

	struct Table
	{
		Game game{ "games/kaboom.xml" };
		CommandExecutor executor{ game };

		Table()
		{
			measure(game);
			game.setCurrentState("playing");
		}

		Object& bucket() { return game.getObject("bucket"); }
		float score() { return bucket().variable["score"]; }
		float buckets() { return bucket().variable["buckets"]; }
		float missed() { return game.getObject("tally").variable["missed"]; }
		std::string state() { return game.getCurrentState().name; }

		// For tests that slide the bucket about while the bombs keep falling:
		// enough buckets that no run of bad luck ends the game.
		void keepPlaying() { bucket().variable["buckets"] = 1000.0f; }

		void key(const char* direction, bool down)
		{
			executor.executeInput(Command{ CmdTriggerAction{ "bucket", direction } }, down);
		}

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { game.updateObjects(); }
		}

		// Puts a bomb inside the bucket, ready to be caught on the next frame.
		void dropOnto(const std::string& name)
		{
			Object& bomb = game.getObject(name);
			bomb.position = { bucket().position.x + 10.0f, bucket().position.y - bomb.size.y / 2.0f };
		}

		// Puts a bomb a frame or two from the floor, well away from the bucket.
		void dropToFloor(const std::string& name)
		{
			bucket().position.x = 700.0f - bucket().size.x / 2.0f;
			Object& bomb = game.getObject(name);
			bomb.position.y = kWindowHeight - bomb.size.y - 2.0f;
		}

		// A reset puts a bomb back at the start of its fall, and the rest of
		// that frame's move still happens.
		bool atStart(const std::string& name)
		{
			const Object& bomb = game.getObject(name);
			return bomb.position.x == bomb.positionOriginal.x
				&& bomb.position.y >= bomb.positionOriginal.y
				&& bomb.position.y <= bomb.positionOriginal.y + bomb.velocity.y;
		}
	};

	// Just enough of a Window for Engine's constructor.
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
}

TEST_CASE("kaboom.xml loads a bucket, a bomber and three waves of six bombs", "[kaboom]")
{
	Table table;

	int bombs = 0;
	for (const auto& object : table.game.getCurrentObjects())
	{
		if (object.objClass == "bombs") { ++bombs; }
	}
	CHECK(bombs == 18);
	CHECK(table.score() == 0.0f);
	CHECK(table.buckets() == 3.0f);
	CHECK(table.missed() == 0.0f);

	// Centered along the bottom.
	CHECK(table.bucket().position.x + table.bucket().size.x / 2.0f == kWindowWidth / 2.0f);
	CHECK(table.bucket().position.y + table.bucket().size.y < kWindowHeight);
}

TEST_CASE("every bomb starts above the screen and each wave falls faster than the last", "[kaboom][random]")
{
	const float lowest[3] = { 2.0f, 3.5f, 5.0f };
	const float highest[3] = { 3.5f, 5.0f, 7.0f };
	for (int load = 0; load < 20; ++load)
	{
		Table table;
		for (int wave = 1; wave <= 3; ++wave)
		{
			for (int i = 1; i <= 6; ++i)
			{
				const Object& bomb = table.game.getObject("bombs" + std::to_string(wave) + "." + std::to_string(i));
				INFO("bombs" << wave << "." << i);
				CHECK(bomb.position.y + bomb.size.y <= 0.0f);
				CHECK(bomb.position.y >= -600.0f);
				CHECK(bomb.velocity.x == 0.0f);
				CHECK(bomb.velocity.y >= lowest[wave - 1]);
				CHECK(bomb.velocity.y <= highest[wave - 1]);
			}
		}
	}
}

TEST_CASE("the bombs do not all draw the same fall", "[kaboom][random]")
{
	Table table;

	bool speedsDiffer = false;
	bool heightsDiffer = false;
	const Object& first = table.game.getObject("bombs1.1");
	for (int i = 2; i <= 6; ++i)
	{
		const Object& other = table.game.getObject("bombs1." + std::to_string(i));
		speedsDiffer = speedsDiffer || other.velocity.y != first.velocity.y;
		heightsDiffer = heightsDiffer || other.position.y != first.position.y;
	}
	CHECK(speedsDiffer);
	CHECK(heightsDiffer);
}

TEST_CASE("the bomber paces along the top and turns at the sides", "[kaboom]")
{
	Table table;
	Object& bomber = table.game.getObject("bomber");
	CHECK(bomber.velocity.x == 4.0f);

	bomber.position.x = kWindowWidth - bomber.size.x - 2.0f;
	table.frames(2);
	CHECK(bomber.velocity.x == -4.0f);

	bomber.position.x = 2.0f;
	table.frames(2);
	CHECK(bomber.velocity.x == 4.0f);
}

TEST_CASE("held keys slide the bucket and it stops at the sides", "[kaboom]")
{
	Table table;
	table.keepPlaying();
	const float startX = table.bucket().position.x;

	table.key("right", true);
	table.frames(10);
	CHECK(table.bucket().position.x == startX + 80.0f);

	table.key("right", false);
	table.frames(10);
	CHECK(table.bucket().position.x == startX + 80.0f);

	table.key("left", true);
	table.frames(200);
	CHECK(table.bucket().position.x == 0.0f);

	table.key("left", false);
	table.key("right", true);
	table.frames(200);
	CHECK(table.bucket().position.x + table.bucket().size.x == kWindowWidth);
}

TEST_CASE("catching a bomb scores a point in the first wave and sends the bomb back up", "[kaboom]")
{
	Table table;

	table.dropOnto("bombs1.3");
	table.frames(1);

	CHECK(table.score() == 1.0f);
	CHECK(table.buckets() == 3.0f);
	CHECK(table.atStart("bombs1.3"));
	CHECK(table.game.getObject("scorevalue").spriteParams.at(1) == "1");
	CHECK(table.state() == "playing");
}

TEST_CASE("a bomb is worth two points in the second wave and three in the third", "[kaboom]")
{
	{
		Table table;
		table.game.setCurrentState("wave2");
		table.bucket().variable["score"] = 12.0f;
		table.dropOnto("bombs2.4");
		table.frames(1);
		CHECK(table.score() == 14.0f);
		CHECK(table.atStart("bombs2.4"));
	}
	{
		Table table;
		table.game.setCurrentState("wave3");
		table.bucket().variable["score"] = 33.0f;
		table.dropOnto("bombs3.1");
		table.frames(1);
		CHECK(table.score() == 36.0f);
		CHECK(table.atStart("bombs3.1"));
	}
}

TEST_CASE("a bomb that reaches the floor costs a bucket", "[kaboom]")
{
	Table table;

	table.dropToFloor("bombs1.1");
	table.frames(3);

	CHECK(table.buckets() == 2.0f);
	CHECK(table.score() == 0.0f);
	CHECK(table.atStart("bombs1.1"));
	CHECK(table.game.getObject("bucketsvalue").spriteParams.at(1) == "2");
	CHECK(table.state() == "playing");
}

TEST_CASE("a missed bomb sets off every bomb in the wave, and the count starts again", "[kaboom]")
{
	Table table;

	// Bombs up in the air, none near the bucket, and one about to hit the floor.
	for (int i = 2; i <= 6; ++i)
	{
		Object& bomb = table.game.getObject("bombs1." + std::to_string(i));
		bomb.position.y = 100.0f + 50.0f * static_cast<float>(i);
	}
	table.dropToFloor("bombs1.1");
	table.frames(3);

	CHECK(table.buckets() == 2.0f); // one bucket, not six
	CHECK(table.missed() == 0.0f);  // the tally is cleared for the next miss
	for (int i = 1; i <= 6; ++i)
	{
		INFO("bombs1." << i);
		CHECK(table.atStart("bombs1." + std::to_string(i)));
	}

	// And the next miss costs the next bucket.
	table.dropToFloor("bombs1.4");
	table.frames(3);
	CHECK(table.buckets() == 1.0f);
}

TEST_CASE("the explosion belongs to the current wave's bombs", "[kaboom]")
{
	Table table;
	table.game.setCurrentState("wave2");
	table.dropToFloor("bombs2.1");
	Object& other = table.game.getObject("bombs2.5");
	other.position.y = 200.0f;
	table.frames(3);

	CHECK(table.buckets() == 2.0f);
	CHECK(table.atStart("bombs2.5"));
}

TEST_CASE("the third miss ends the game, and a catch in between changes nothing", "[kaboom]")
{
	Table table;

	table.dropToFloor("bombs1.1");
	table.frames(3);
	table.bucket().position.x = 300.0f;
	table.dropOnto("bombs1.2");
	table.frames(1);
	table.dropToFloor("bombs1.3");
	table.frames(3);
	CHECK(table.buckets() == 1.0f);
	CHECK(table.score() == 1.0f);
	CHECK(table.state() == "playing");

	table.dropToFloor("bombs1.5");
	table.frames(3);
	CHECK(table.buckets() == 0.0f);
	CHECK(table.state() == "gameover");
}

TEST_CASE("the score carries the game from wave to wave and to a win", "[kaboom]")
{
	Table table;

	table.bucket().variable["score"] = 9.0f;
	table.dropOnto("bombs1.1");
	table.frames(1);
	CHECK(table.score() == 10.0f);
	CHECK(table.state() == "wave2");

	table.bucket().variable["score"] = 28.0f;
	table.dropOnto("bombs2.1");
	table.frames(1);
	CHECK(table.score() == 30.0f);
	CHECK(table.state() == "wave3");

	table.bucket().variable["score"] = 56.0f;
	table.dropOnto("bombs3.1");
	table.frames(1);
	CHECK(table.score() == 59.0f);
	CHECK(table.state() == "wave3"); // one short

	table.dropOnto("bombs3.2");
	table.frames(1);
	CHECK(table.score() == 62.0f);
	CHECK(table.state() == "youwin");
}

TEST_CASE("one point short of a wave does not start it", "[kaboom]")
{
	Table table;

	table.bucket().variable["score"] = 8.0f;
	table.dropOnto("bombs1.1");
	table.frames(1);

	CHECK(table.score() == 9.0f);
	CHECK(table.state() == "playing");
}

TEST_CASE("space starts a new game from either end screen with everything back", "[kaboom]")
{
	Table table;

	table.dropOnto("bombs1.4");
	table.frames(1);
	table.bucket().variable["buckets"] = 1.0f;
	table.dropToFloor("bombs1.1");
	table.frames(3);
	REQUIRE(table.state() == "gameover");

	table.executor.executeInput(Command{ CmdReset{} }, true);

	CHECK(table.state() == "mainmenu");
	CHECK(table.score() == 0.0f);
	CHECK(table.buckets() == 3.0f);
	CHECK(table.missed() == 0.0f);
	CHECK(table.game.getObject("scorevalue").spriteParams.at(1) == "0");
	CHECK(table.game.getObject("bucketsvalue").spriteParams.at(1) == "3");
	CHECK(table.atStart("bombs1.4"));
	CHECK(table.atStart("bombs1.1"));
}

TEST_CASE("the keys play the game through the engine, across a wave change", "[kaboom][engine_input]")
{
	Game game{ "games/kaboom.xml" };
	measure(game);
	Engine engine(game, std::make_unique<FakeWindow>());
	Object& bucket = game.getObject("bucket");

	engine.handleKeyPressed(KeyCode::Space); // mainmenu -> playing
	REQUIRE(game.getCurrentState().name == "playing");
	engine.handleKeyReleased(KeyCode::Space);

	const float startX = bucket.position.x;
	engine.handleKeyPressed(KeyCode::D);
	game.updateObjects();
	CHECK(bucket.position.x == startX + 8.0f);

	// The second wave binds the same keys: the held key carries on.
	game.setCurrentState("wave2");
	game.updateObjects();
	CHECK(bucket.position.x == startX + 16.0f);
	engine.handleKeyReleased(KeyCode::D);
	game.updateObjects();
	CHECK(bucket.position.x == startX + 16.0f);

	engine.handleKeyPressed(KeyCode::Left);
	game.updateObjects();
	CHECK(bucket.position.x == startX + 8.0f);
	engine.handleKeyReleased(KeyCode::Left);

	engine.handleKeyPressed(KeyCode::P);
	engine.handleKeyReleased(KeyCode::P);
	CHECK(game.getCurrentState().name == "paused");
	engine.handleKeyPressed(KeyCode::Space);
	engine.handleKeyReleased(KeyCode::Space);
	CHECK(game.getCurrentState().name == "wave2");
}

TEST_CASE("a game left running eventually brings bombs down on a moving bucket", "[kaboom][play]")
{
	// A crude player: sit under the lowest bomb. Enough bombs fall in twenty
	// seconds of play that a bucket that is always moving must have caught some.
	Table table;
	table.keepPlaying();

	for (int frame = 0; frame < 20 * 60; ++frame)
	{
		float targetX = kWindowWidth / 2.0f;
		float lowest = -10000.0f;
		for (const auto& object : table.game.getCurrentObjects())
		{
			if (object.objClass != "bombs" || !object.isVisible) { continue; }
			if (object.position.y > lowest && object.position.y < table.bucket().position.y)
			{
				lowest = object.position.y;
				targetX = object.position.x;
			}
		}
		const float centre = table.bucket().position.x + table.bucket().size.x / 2.0f;
		table.key("left", centre > targetX + 10.0f);
		table.key("right", centre < targetX - 10.0f);
		table.frames(1);
		if (table.state() == "youwin" || table.state() == "gameover") { break; }
	}

	CHECK(table.score() > 0.0f);
}
