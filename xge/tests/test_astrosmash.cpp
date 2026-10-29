// test_astrosmash.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026
//
// Catch2 tests for games/descriptions/astrosmash.xml, played frame by frame by a real
// xge::Game with no window: rocks falling at a slant from <random> heights,
// big ones breaking into chips, a rock landing taking half its worth off the
// score, spinners and rocks costing a base, a spare base every 1,000 points,
// the peak score, and game over.
//
// Window::init() normally measures each object's size once a backend exists;
// measure() gives every shape the size its sprite implies instead.

#include "play/executor.h"
#include "engine/engine.h"
#include "game/game.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
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
			object.size = measureShapeSize(object.picture);
		}
	}

	struct Table
	{
		Game game{ "descriptions/astrosmash.xml" };
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
		// ends the game.
		void keepPlaying()
		{
			ship().variable["lives"] = 1000.0f;
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
				if (object.objClass == "rocks" || object.objClass == "spinner") { found.push_back(&object); }
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
			return std::abs(rock.position.x - rock.positionOriginal.x) <= std::abs(rock.velocity.x)
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

TEST_CASE("astrosmash.xml loads a base, four big rocks, four small, two spinners and four bases", "[astrosmash]")
{
	Table table;

	int boulders = 0;
	int pebbles = 0;
	int chips = 0;
	for (const auto& object : table.game.getCurrentObjects())
	{
		if (object.objClass != "rocks") { continue; }
		// The rocks are the playing state's own: playing.boulders.1.1 and so on.
		if (object.name.rfind("playing.boulders.", 0) == 0) { ++boulders; }
		if (object.name.rfind("playing.pebbles.", 0) == 0) { ++pebbles; }
		if (object.name.rfind("playing.chip", 0) == 0) { ++chips; CHECK_FALSE(object.exists); }
	}
	CHECK(boulders == 4);
	CHECK(pebbles == 4);
	CHECK(chips == 1); // only the one written; chips are made as rocks break
	CHECK(table.game.getObject("spinners.2.1").objClass == "spinner");
	CHECK(table.score() == 0.0f);
	CHECK(table.lives() == 4.0f);
	CHECK(!table.inFlight());

	// Centered on the ground.
	CHECK(table.ship().position.x + table.ship().size.x / 2.0f == kWindowWidth / 2.0f);
	CHECK(table.ship().position.y + table.ship().size.y == kWindowHeight - 40.0f);
}

TEST_CASE("every rock starts above the screen and falls at a slant drawn from its range", "[astrosmash][random]")
{
	for (int load = 0; load < 20; ++load)
	{
		Table table;
		for (int i = 1; i <= 4; ++i)
		{
			for (const std::string group : { "boulders", "pebbles" })
			{
				const bool boulder = group == "boulders";
				const Object& rock = table.game.getObject(group + "." + std::to_string(i) + ".1");
				INFO(group << "." << i);
				CHECK(rock.position.y + rock.size.y <= 0.0f);
				CHECK(rock.position.y >= (boulder ? -600.0f : -800.0f));
				CHECK(std::abs(rock.velocity.x) <= 1.0f);
				CHECK(rock.velocity.y >= (boulder ? 1.0f : 2.0f));
				CHECK(rock.velocity.y <= (boulder ? 2.5f : 3.5f));
			}
		}
	}
}

TEST_CASE("the rocks do not all draw the same fall", "[astrosmash][random]")
{
	Table table;

	bool speedsDiffer = false;
	bool heightsDiffer = false;
	const Object& first = table.game.getObject("boulders.1.1");
	for (int i = 2; i <= 4; ++i)
	{
		const Object& other = table.game.getObject("boulders." + std::to_string(i) + ".1");
		speedsDiffer = speedsDiffer || other.velocity.y != first.velocity.y;
		heightsDiffer = heightsDiffer || other.position.y != first.position.y;
	}
	CHECK(speedsDiffer);
	CHECK(heightsDiffer);
}

TEST_CASE("held keys slide the base and it stops at the sides", "[astrosmash]")
{
	Table table;
	table.keepPlaying();
	table.parkRocks();
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

TEST_CASE("firing launches a shot up from the base, one at a time", "[astrosmash]")
{
	Table table;
	table.parkRocks();

	table.press("gun");
	CHECK(table.inFlight());
	// Leaves the middle of the base: the shot's own middle over the base's.
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

TEST_CASE("a big rock shot scores 10, starts over and breaks into two chips", "[astrosmash]")
{
	Table table;
	table.parkRocks();

	table.fireAt("boulders.2.1");
	table.frames(1);

	CHECK(table.score() == 10.0f);
	CHECK(table.game.getObject("peak").picture.text.find("10") != std::string::npos);
	CHECK(!table.inFlight());
	CHECK(table.atStart("boulders.2.1"));
	CHECK(table.game.getObject("chip.1").isVisible);
	CHECK(table.game.getObject("chip.2").isVisible);
	CHECK(table.game.tryGetObject("chip.3") == nullptr);
	CHECK(table.game.getObject("chip.1").velocity.y > 0.0f);

	// A chip shot scores 20 and is gone (the other one out of the way).
	table.game.getObject("chip.2").velocity = { 0.0f, 0.0f };
	table.game.getObject("chip.2").position = { 20.0f, 20.0f };
	table.fireAt("chip.1");
	table.game.getObject("chip.1").velocity = { 0.0f, 0.0f };
	table.frames(1);
	CHECK(table.score() == 30.0f);
	CHECK(table.game.tryGetObject("chip.1") == nullptr);
}

TEST_CASE("a small rock shot scores 20 and breaks into nothing", "[astrosmash]")
{
	Table table;
	table.parkRocks();
	table.fireAt("pebbles.3.1");
	table.frames(1);
	CHECK(table.score() == 20.0f);
	CHECK(table.atStart("pebbles.3.1"));
	CHECK(table.game.tryGetObject("chip.1") == nullptr);
}

TEST_CASE("a rock that lands takes half its worth off the score and starts over", "[astrosmash]")
{
	Table table;
	table.parkRocks();
	table.ship().position.x = 700.0f; // well away from the rock below
	table.game.setVariable("ship", "score", 100.0f);

	Object& rock = table.game.getObject("pebbles.1.1");
	rock.velocity = { 0.0f, 2.0f };
	rock.position.y = kWindowHeight - 40.0f - rock.size.y - 1.0f;
	table.frames(3);

	CHECK(table.lives() == 4.0f);
	CHECK(table.score() == 90.0f);
	CHECK(rock.position.y < 0.0f);

	// never below nought
	table.game.setVariable("ship", "score", 2.0f);
	rock.velocity = { 0.0f, 2.0f };
	rock.position.y = kWindowHeight - 40.0f - rock.size.y - 1.0f;
	table.frames(3);
	CHECK(table.score() == 0.0f);
	CHECK(table.game.getCurrentState().name == "playing");
}

TEST_CASE("a rock on the base costs it, and the sky clears", "[astrosmash]")
{
	Table table;
	table.parkRocks();

	Object& rock = table.game.getObject("pebbles.1.1");
	rock.velocity = { 0.0f, 3.0f };
	rock.position = { table.ship().position.x + 10.0f, table.ship().position.y - rock.size.y + 1.0f };
	Object& other = table.game.getObject("boulders.2.1");
	other.position.y = 300.0f;
	table.frames(1);

	CHECK(table.lives() == 3.0f);
	CHECK(table.score() == 0.0f);
	CHECK(rock.position.y < 0.0f);
	CHECK(other.position.y < 0.0f);
}

TEST_CASE("a spinner that lands blows up the base, and one shot scores 40", "[astrosmash]")
{
	Table table;
	table.parkRocks();
	table.ship().position.x = 0.0f;

	Object& spinner = table.game.getObject("spinners.2.1");
	spinner.velocity = { 0.0f, 2.0f };
	spinner.position.y = kWindowHeight - 40.0f - spinner.size.y - 1.0f;
	table.frames(3);
	CHECK(table.lives() == 3.0f);
	CHECK(spinner.position.y < 0.0f);

	table.parkRocks();
	table.fireAt("spinners.1.1");
	table.frames(1);
	CHECK(table.score() == 40.0f);
}

TEST_CASE("every 1,000 points is a spare base, and the peak stays when the score falls", "[astrosmash]")
{
	Table table;
	table.parkRocks();
	table.game.setVariable("ship", "score", 995.0f);
	table.fireAt("boulders.1.1");
	table.frames(2);
	CHECK(table.score() == 1005.0f);
	CHECK(table.lives() == 5.0f);
	CHECK(table.ship().variable["spare"] == 2000.0f);
	CHECK(table.ship().variable["peak"] == 1005.0f);

	table.ship().position.x = 700.0f;
	Object& rock = table.game.getObject("boulders.1.1");
	rock.velocity = { 0.0f, 2.0f };
	rock.position.y = kWindowHeight - 40.0f - rock.size.y - 1.0f;
	table.frames(3);
	CHECK(table.score() == 1000.0f);
	CHECK(table.ship().variable["peak"] == 1005.0f);
	CHECK(table.lives() == 5.0f);
}

TEST_CASE("the last base ends the game", "[astrosmash]")
{
	Table table;
	table.parkRocks();
	table.ship().variable["lives"] = 1.0f;

	Object& rock = table.game.getObject("pebbles.1.1");
	rock.velocity = { 0.0f, 3.0f };
	rock.position = { table.ship().position.x + 10.0f, table.ship().position.y - rock.size.y + 1.0f };
	table.frames(2);

	CHECK(table.lives() == 0.0f);
	CHECK(table.game.getCurrentState().name == "gameover");
}

TEST_CASE("space starts a new game from game over with everything back", "[astrosmash]")
{
	Table table;
	table.parkRocks();
	table.fireAt("boulders.3.1");
	table.frames(1);
	table.ship().variable["lives"] = 0.0f;
	table.frames(1);
	REQUIRE(table.game.getCurrentState().name == "gameover");

	table.executor.executeInput(Command{ CmdReset{} }, true);

	CHECK(table.game.getCurrentState().name == "mainmenu");
	CHECK(table.score() == 0.0f);
	CHECK(table.lives() == 4.0f);
	CHECK(table.game.getObject("score").picture.text == "0");
	CHECK(table.game.getObject("lives").picture.text == "4");
	CHECK(!table.inFlight());
	CHECK(table.game.tryGetObject("chip.1") == nullptr);
	CHECK(table.game.getObject("boulders.1.1").position.y < 0.0f);
}

TEST_CASE("the keys play the game through the engine", "[astrosmash][engine_input]")
{
	Game game{ "descriptions/astrosmash.xml" };
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

TEST_CASE("each state has its own objects: the title, the sky, the signs", "[astrosmash][states]")
{
	Game game{ "descriptions/astrosmash.xml" };
	game.setCurrentState("mainmenu");
	CHECK(game.isShown(game.getObject("help")));
	CHECK_FALSE(game.isShown(game.getObject("ship")));

	game.pushState("playing");
	CHECK(game.getObject("ship").name == "playing.ship");
	CHECK(game.getObject("pebbles.3.1").name == "playing.pebbles.3.1");
	CHECK(game.isShown(game.getObject("pebbles.3.1")));
	CHECK(game.isShown(game.getObject("playing.lives")));
	CHECK_FALSE(game.isShown(game.getObject("help")));
}

TEST_CASE("paused and the endings draw the sky still underneath, with the score", "[astrosmash][states]")
{
	Table table;
	Object& rock = table.game.getObject("boulders.1.1");
	const float y = rock.position.y;

	table.game.pushState("paused");
	table.frames(30);
	CHECK(rock.position.y == y);
	CHECK(table.game.isStill(rock));
	CHECK(table.game.isStill(table.ship()));
	CHECK(table.game.isShown(table.game.getObject("paused.paused")));
	table.game.popState();
	CHECK(table.game.isShown(rock));

	table.ship().variable["lives"] = 0.0f;
	table.frames(1);
	REQUIRE(table.game.getCurrentState().name == "gameover");
	CHECK(table.game.isStill(table.ship()));
	CHECK(table.game.isStill(table.game.getObject("playing.score")));
	CHECK(table.game.isShown(table.game.getObject("gameover.message")));
	CHECK(table.game.isShown(table.game.getObject("gameover.again")));
}
