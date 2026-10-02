// test_astrosmash.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026
//
// Catch2 tests for games/astrosmash.xml, played frame by frame by a real
// xge::Game with no window. Astrosmash is written with verbs the language
// already had: falling rocks whose speeds and starting heights are drawn by
// <random> at load, a <fire /> gun that <die />s on a rock, and <inc /> and
// <dec /> on the ship for the score and the lives.
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
		Game game{ "games/astrosmash.xml" };
		CommandExecutor executor{ game };

		Table()
		{
			measure(game);
			game.setCurrentState("playing");
		}

		Object& ship() { return game.getObject("ship"); }
		Object& shot() { return game.getObject("shot"); }
		float score() { return ship().variable["score"]; }
		float lives() { return ship().variable["lives"]; }
		bool inFlight() { return shot().isVisible && shot().collisionData.enabled; }

		// For tests that play for a while: enough lives that no run of bad luck
		// ends the game, and a score no run of good luck can win with.
		void keepPlaying()
		{
			ship().variable["lives"] = 1000.0f;
			ship().variable["score"] = -1000.0f;
		}

		void press(const char* action)
		{
			executor.executeInput(Command{ CmdTriggerAction{ "ship", action } }, true);
		}

		void key(const char* direction, bool down)
		{
			executor.executeInput(Command{ CmdTriggerAction{ "ship", direction } }, down);
		}

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { game.updateObjects(); }
		}

		std::vector<Object*> rocks()
		{
			std::vector<Object*> found;
			for (auto& object : game.getCurrentObjects())
			{
				if (object.objClass == "rocks") { found.push_back(&object); }
			}
			return found;
		}

		// Holds every rock still, out of the way at the top of the screen.
		void parkRocks()
		{
			for (Object* rock : rocks())
			{
				rock->velocity = { 0.0f, 0.0f };
				rock->position.y = -400.0f;
			}
		}

		// A rock back at the start of its fall, give or take a frame of falling.
		bool atStart(const std::string& name)
		{
			const Object& rock = game.getObject(name);
			return rock.position.x == rock.positionOriginal.x
				&& rock.position.y >= rock.positionOriginal.y
				&& rock.position.y <= rock.positionOriginal.y + rock.velocity.y;
		}

		// Brings a rock down into the middle of the screen, fires, and puts the
		// shot just under it, heading up into it.
		void fireAt(const std::string& name)
		{
			press("gun");
			Object& rock = game.getObject(name);
			rock.position.y = 200.0f;
			shot().position = { rock.position.x + rock.size.x / 2.0f, rock.position.y + rock.size.y + 4.0f };
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

TEST_CASE("astrosmash.xml loads a ship, four boulders, four pebbles and five lives", "[astrosmash]")
{
	Table table;

	int boulders = 0;
	int pebbles = 0;
	for (const auto& object : table.game.getCurrentObjects())
	{
		if (object.objClass != "rocks") { continue; }
		if (object.name.rfind("boulders.", 0) == 0) { ++boulders; }
		if (object.name.rfind("pebbles.", 0) == 0) { ++pebbles; }
	}
	CHECK(boulders == 4);
	CHECK(pebbles == 4);
	CHECK(table.score() == 0.0f);
	CHECK(table.lives() == 5.0f);
	CHECK(!table.inFlight());

	// Centered along the bottom.
	CHECK(table.ship().position.x + table.ship().size.x / 2.0f == kWindowWidth / 2.0f);
	CHECK(table.ship().position.y + table.ship().size.y < kWindowHeight);
}

TEST_CASE("every rock starts above the screen and falls at a speed drawn from its range", "[astrosmash][random]")
{
	for (int load = 0; load < 20; ++load)
	{
		Table table;
		for (int i = 1; i <= 4; ++i)
		{
			for (const std::string group : { "boulders", "pebbles" })
			{
				const bool boulder = group == "boulders";
				const Object& rock = table.game.getObject(group + "." + std::to_string(i));
				INFO(group << "." << i);
				CHECK(rock.position.y + rock.size.y <= 0.0f);
				CHECK(rock.position.y >= (boulder ? -500.0f : -700.0f));
				CHECK(rock.velocity.x == 0.0f);
				CHECK(rock.velocity.y >= (boulder ? 1.0f : 2.0f));
				CHECK(rock.velocity.y <= (boulder ? 2.0f : 3.5f));
			}
		}
	}
}

TEST_CASE("the rocks do not all draw the same fall", "[astrosmash][random]")
{
	Table table;

	bool speedsDiffer = false;
	bool heightsDiffer = false;
	const Object& first = table.game.getObject("boulders.1");
	for (int i = 2; i <= 4; ++i)
	{
		const Object& other = table.game.getObject("boulders." + std::to_string(i));
		speedsDiffer = speedsDiffer || other.velocity.y != first.velocity.y;
		heightsDiffer = heightsDiffer || other.position.y != first.position.y;
	}
	CHECK(speedsDiffer);
	CHECK(heightsDiffer);
}

TEST_CASE("held keys slide the ship and it stops at the sides", "[astrosmash]")
{
	Table table;
	table.keepPlaying();
	const float startX = table.ship().position.x;

	table.key("right", true);
	table.frames(10);
	CHECK(table.ship().position.x == startX + 70.0f);

	table.key("right", false);
	table.frames(10);
	CHECK(table.ship().position.x == startX + 70.0f);

	table.key("left", true);
	table.frames(200);
	CHECK(table.ship().position.x == 0.0f);

	table.key("left", false);
	table.key("right", true);
	table.frames(200);
	CHECK(table.ship().position.x + table.ship().size.x == kWindowWidth);
}

TEST_CASE("firing launches a shot up from the ship, one at a time", "[astrosmash]")
{
	Table table;
	table.parkRocks();

	table.press("gun");
	CHECK(table.inFlight());
	// Leaves the middle of the ship: the shot's own middle over the ship's.
	CHECK(table.shot().position.x + table.shot().size.x / 2.0f == table.ship().position.x + table.ship().size.x / 2.0f);
	CHECK(table.shot().position.y == table.ship().position.y);
	CHECK(table.shot().velocity.y == -11.0f);

	table.frames(5);
	const float y = table.shot().position.y;
	table.key("right", true);
	table.frames(3);
	table.key("right", false);
	table.press("gun"); // ignored: the first shot is still going
	CHECK(table.shot().position.y == y - 33.0f);
	CHECK(table.shot().position.x != table.ship().position.x + table.ship().size.x / 2.0f);
}

TEST_CASE("a shot that reaches the top of the screen is gone and the gun is ready again", "[astrosmash]")
{
	Table table;
	table.parkRocks();
	table.press("gun");
	table.shot().position.y = 6.0f;
	table.frames(2);

	CHECK(!table.inFlight());
	CHECK(table.score() == 0.0f);
	table.press("gun");
	CHECK(table.inFlight());
}

TEST_CASE("shooting a rock scores a point and sends the rock back up to fall again", "[astrosmash]")
{
	Table table;
	table.parkRocks();

	table.fireAt("boulders.2");
	table.frames(1);

	CHECK(table.score() == 1.0f);
	CHECK(table.lives() == 5.0f);
	CHECK(!table.inFlight());
	CHECK(table.atStart("boulders.2"));
	CHECK(table.game.getObject("score").spriteParams.at(1) == "1");

	// A pebble is just as good.
	table.fireAt("pebbles.3");
	table.frames(1);
	CHECK(table.score() == 2.0f);
	CHECK(table.atStart("pebbles.3"));
}

TEST_CASE("a rock that reaches the ground costs a life and starts over", "[astrosmash]")
{
	Table table;
	table.parkRocks();
	table.ship().position.x = 700.0f; // well away from the rock below

	Object& rock = table.game.getObject("boulders.1");
	rock.velocity.y = 2.0f;
	rock.position.y = kWindowHeight - rock.size.y - 1.0f;
	table.frames(3);

	CHECK(table.lives() == 4.0f);
	CHECK(table.score() == 0.0f);
	CHECK(table.game.getObject("lives").spriteParams.at(1) == "4");
	CHECK(rock.position.y < 0.0f);
	CHECK(table.game.getCurrentState().name == "playing");
}

TEST_CASE("a rock that lands on the ship costs a life and starts over", "[astrosmash]")
{
	Table table;
	table.parkRocks();

	Object& rock = table.game.getObject("pebbles.1");
	rock.velocity.y = 3.0f;
	rock.position = { table.ship().position.x + 10.0f, table.ship().position.y - rock.size.y + 1.0f };
	table.frames(1);

	CHECK(table.lives() == 4.0f);
	CHECK(table.score() == 0.0f);
	CHECK(rock.position.x == rock.positionOriginal.x);
	CHECK(rock.position.y < 0.0f);
}

TEST_CASE("twenty rocks win the game, and nineteen are not enough", "[astrosmash]")
{
	{
		Table table;
		table.parkRocks();
		table.ship().variable["score"] = 19.0f;
		table.fireAt("boulders.1");
		table.frames(1);
		CHECK(table.score() == 20.0f);
		CHECK(table.game.getCurrentState().name == "youwin");
	}
	{
		Table table;
		table.parkRocks();
		table.ship().variable["score"] = 18.0f;
		table.fireAt("boulders.1");
		table.frames(1);
		CHECK(table.score() == 19.0f);
		CHECK(table.game.getCurrentState().name == "playing");
	}
}

TEST_CASE("the last life ends the game", "[astrosmash]")
{
	Table table;
	table.parkRocks();
	table.ship().position.x = 700.0f;
	table.ship().variable["lives"] = 1.0f;

	Object& rock = table.game.getObject("boulders.1");
	rock.velocity.y = 2.0f;
	rock.position.y = kWindowHeight - rock.size.y - 1.0f;
	table.frames(3);

	CHECK(table.lives() == 0.0f);
	CHECK(table.game.getCurrentState().name == "gameover");
}

TEST_CASE("space starts a new game from either end screen with everything back", "[astrosmash]")
{
	Table table;
	table.parkRocks();
	table.fireAt("boulders.3");
	table.frames(1);
	table.ship().variable["lives"] = 1.0f;
	Object& rock = table.game.getObject("boulders.1");
	rock.velocity.y = 2.0f;
	rock.position.y = kWindowHeight - rock.size.y - 1.0f;
	table.ship().position.x = 700.0f;
	table.frames(3);
	REQUIRE(table.game.getCurrentState().name == "gameover");

	table.executor.executeInput(Command{ CmdReset{} }, true);

	CHECK(table.game.getCurrentState().name == "mainmenu");
	CHECK(table.score() == 0.0f);
	CHECK(table.lives() == 5.0f);
	CHECK(table.game.getObject("score").spriteParams.at(1) == "0");
	CHECK(table.game.getObject("lives").spriteParams.at(1) == "5");
	CHECK(!table.inFlight());
	CHECK(table.game.getObject("boulders.1").position.y < 0.0f);
}

TEST_CASE("the keys play the game through the engine", "[astrosmash][engine_input]")
{
	Game game{ "games/astrosmash.xml" };
	measure(game);
	Engine engine(game, std::make_unique<FakeWindow>());
	Object& ship = game.getObject("ship");
	Object& shot = game.getObject("shot");

	CHECK(game.getCurrentState().name == "mainmenu");
	engine.handleKeyPressed(KeyCode::Space); // space starts the game
	REQUIRE(game.getCurrentState().name == "playing");
	engine.handleKeyReleased(KeyCode::Space);
	CHECK(!shot.isVisible);

	engine.handleKeyPressed(KeyCode::Space);
	engine.handleKeyReleased(KeyCode::Space);
	CHECK(shot.isVisible);

	const float startX = ship.position.x;
	engine.handleKeyPressed(KeyCode::A);
	game.updateObjects();
	CHECK(ship.position.x == startX - 7.0f);
	engine.handleKeyReleased(KeyCode::A);
	engine.handleKeyPressed(KeyCode::Right);
	game.updateObjects();
	game.updateObjects();
	CHECK(ship.position.x == startX + 7.0f);
	engine.handleKeyReleased(KeyCode::Right);

	engine.handleKeyPressed(KeyCode::P);
	engine.handleKeyReleased(KeyCode::P);
	CHECK(game.getCurrentState().name == "paused");
	engine.handleKeyPressed(KeyCode::P);
	engine.handleKeyReleased(KeyCode::P);
	CHECK(game.getCurrentState().name == "playing");
}

TEST_CASE("a crude player who shoots the lowest rock in the sights scores", "[astrosmash][play]")
{
	// Sit under the lowest rock above the ship and fire whenever the gun is
	// ready. Over twenty seconds a ship that is always trying must score.
	Table table;
	table.keepPlaying();
	const float startScore = table.score();

	for (int frame = 0; frame < 20 * 60; ++frame)
	{
		float targetX = kWindowWidth / 2.0f;
		float lowest = -10000.0f;
		for (Object* rock : table.rocks())
		{
			if (rock->position.y > lowest && rock->position.y < table.ship().position.y)
			{
				lowest = rock->position.y;
				targetX = rock->position.x + rock->size.x / 2.0f;
			}
		}
		const float centre = table.ship().position.x + table.ship().size.x / 2.0f;
		table.key("left", centre > targetX + 6.0f);
		table.key("right", centre < targetX - 6.0f);
		if (!table.inFlight() && centre > targetX - 10.0f && centre < targetX + 10.0f) { table.press("gun"); }
		table.frames(1);
		if (table.game.getCurrentState().name != "playing") { break; }
	}

	CHECK(table.score() > startScore);
}
