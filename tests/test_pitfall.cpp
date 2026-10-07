// test_pitfall.cpp
// XML Game Engine
// author: beefviper
// date: Oct 7, 2026
//
// Catch2 tests for games/pitfall.xml, played through a real xge::Engine with
// no window and the keys a player would press: Harry running from screen to
// screen (his edge rules and the screens' conditions), the logs, the tar pit
// leapt over and fallen into, the crocodiles and their jaws, the ladder down
// to the tunnel, the treasures, and the clock.
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
	constexpr float kSurface = 300.0f;
	constexpr float kFloor = 440.0f;

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

	struct Pitfall
	{
		Game game{ "games/pitfall.xml" };
		Engine engine{ game, std::make_unique<FakeWindow>() };

		Pitfall()
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

		void start()
		{
			tap(KeyCode::Space);
			REQUIRE(state() == "screen1");
			frames(5);
		}

		std::string state() { return game.getCurrentState().name; }
		Object& harry() { return game.getObject("harry"); }
		float feet() { return harry().position.y + harry().size.y; }
		float status(const char* name) { return game.getObject("status").variable.at(name); }

		// Puts Harry standing at `x`, on the path or the tunnel floor.
		void standAt(float x, float top = kSurface)
		{
			harry().position = { x, top - harry().size.y };
			harry().velocity = { 0.0f, 0.0f };
			frames(2);
		}

		// Runs off the right of the screen.
		void runRight()
		{
			standAt(600.0f);
			engine.handleKeyPressed(KeyCode::D);
			frames(30);
			engine.handleKeyReleased(KeyCode::D);
			frames(2);
		}
	};
}

TEST_CASE("pitfall.xml loads, and Harry stands on the jungle path", "[pitfall]")
{
	Pitfall play;
	CHECK(play.state() == "title");
	play.start();

	play.frames(60);
	CHECK(play.feet() == Approx(kSurface));
	CHECK(play.harry().grounded);
	CHECK(play.status("lives") == 3);
	CHECK(play.status("score") == 2000);
}

TEST_CASE("running off one side of a screen is the next screen, and the jungle goes round", "[pitfall]")
{
	Pitfall play;
	play.start();

	play.runRight();
	CHECK(play.state() == "screen2");
	CHECK(play.status("screen") == 2);
	CHECK(play.harry().position.x < 100.0f);
	CHECK(play.feet() == Approx(kSurface));

	// Back the way he came.
	play.standAt(10.0f);
	play.engine.handleKeyPressed(KeyCode::A);
	play.frames(20);
	play.engine.handleKeyReleased(KeyCode::A);
	play.frames(2);
	CHECK(play.state() == "screen1");
	CHECK(play.harry().position.x > 500.0f);

	// Left of the first screen is the fourth.
	play.standAt(10.0f);
	play.engine.handleKeyPressed(KeyCode::A);
	play.frames(20);
	play.engine.handleKeyReleased(KeyCode::A);
	play.frames(2);
	CHECK(play.state() == "screen4");
	CHECK(play.status("screen") == 4);

	// And right of the fourth is the first again.
	play.runRight();
	CHECK(play.state() == "screen1");
	CHECK(play.status("screen") == 1);
}

TEST_CASE("a log rolling over Harry costs points while it does", "[pitfall]")
{
	Pitfall play;
	play.start();
	Object& log = play.game.getObject("logs.1");
	play.standAt(log.position.x - 40.0f);

	play.frames(60);
	CHECK(play.status("score") < 2000);
	CHECK(play.status("lives") == 3);
}

TEST_CASE("the tar pit costs a life, unless Harry leaps over it", "[pitfall]")
{
	Pitfall play;
	play.start();
	play.runRight();
	REQUIRE(play.state() == "screen2");
	const float tar = play.game.getObject("tar").position.x;

	// Walking in.
	play.standAt(tar - 40.0f);
	play.engine.handleKeyPressed(KeyCode::D);
	play.frames(30);
	play.engine.handleKeyReleased(KeyCode::D);
	CHECK(play.status("lives") == 2);
	CHECK(play.harry().position.x < 100.0f);

	// Leaping from just short of it.
	play.standAt(tar - 20.0f);
	play.engine.handleKeyPressed(KeyCode::D);
	play.frames(1);
	play.tap(KeyCode::Space);
	play.frames(50);
	play.engine.handleKeyReleased(KeyCode::D);
	play.frames(5);
	CHECK(play.status("lives") == 2);
	CHECK(play.harry().position.x > tar + 48.0f);
	CHECK(play.feet() == Approx(kSurface));
}

TEST_CASE("a crocodile can be stood on until its jaws open; the water drowns", "[pitfall]")
{
	Pitfall play;
	play.start();
	play.game.getObject("status").variable["screen"] = 3;
	play.frames(5);
	REQUIRE(play.state() == "screen3");

	// On the first head, between bites.
	const Object& croc = play.game.getObject("crocs.1");
	play.standAt(croc.position.x + 7.0f, croc.position.y);
	play.frames(60);
	CHECK(play.status("lives") == 3);
	CHECK(play.feet() == Approx(croc.position.y));

	// The jaws open every three seconds.
	play.frames(130);
	CHECK(play.status("lives") == 2);
	CHECK(play.harry().position.x < 100.0f);

	// And off the bank into the pond.
	play.standAt(190.0f);
	play.engine.handleKeyPressed(KeyCode::D);
	play.frames(40);
	play.engine.handleKeyReleased(KeyCode::D);
	CHECK(play.status("lives") == 1);
}

TEST_CASE("the ladder takes Harry down to the tunnel and back up", "[pitfall]")
{
	Pitfall play;
	play.start();
	play.game.getObject("status").variable["screen"] = 4;
	play.frames(5);
	REQUIRE(play.state() == "screen4");

	// The hole is too narrow to fall down.
	const Object& ladder = play.game.getObject("ladder");
	play.standAt(ladder.position.x + 2.0f);
	play.frames(30);
	CHECK(play.feet() == Approx(kSurface));

	play.engine.handleKeyPressed(KeyCode::S);
	play.frames(100);
	play.engine.handleKeyReleased(KeyCode::S);
	play.frames(5);
	CHECK(play.feet() == Approx(kFloor));

	play.engine.handleKeyPressed(KeyCode::W);
	play.frames(100);
	play.engine.handleKeyReleased(KeyCode::W);
	play.frames(5);
	CHECK(play.feet() == Approx(kSurface));
	CHECK(play.harry().grounded);
}

TEST_CASE("a treasure is taken once, and all four win", "[pitfall]")
{
	Pitfall play;
	play.start();

	play.runRight();
	REQUIRE(play.state() == "screen2");
	play.standAt(540.0f);
	play.engine.handleKeyPressed(KeyCode::D);
	play.frames(15);
	play.engine.handleKeyReleased(KeyCode::D);
	CHECK(play.status("score") == 5000);
	CHECK_FALSE(play.game.getObject("silverbar").isVisible);

	play.game.getObject("moneybag").isVisible = false;
	play.game.getObject("goldbar").isVisible = false;
	play.frames(2);
	CHECK(play.state() == "screen2");
	play.game.getObject("ring").isVisible = false;
	play.frames(2);
	CHECK(play.state() == "youwin");
}

TEST_CASE("the clock counts down a second at a time, and running out ends the game", "[pitfall]")
{
	Pitfall play;
	play.start();
	const float time = play.status("time");
	play.frames(120);
	CHECK(play.status("time") == Approx(time - 2.0f));

	play.game.getObject("status").variable["time"] = 1;
	play.frames(61);
	CHECK(play.state() == "gameover");

	play.tap(KeyCode::Space);
	CHECK(play.state() == "title");
	CHECK(play.status("time") == 180);
}

TEST_CASE("the pond can be crossed by leaping from head to head between bites", "[pitfall]")
{
	Pitfall play;
	play.start();
	play.game.getObject("status").variable["screen"] = 3;
	play.frames(5);
	REQUIRE(play.state() == "screen3");
	// The next bite a good while off, so the crossing is clear.
	play.game.getObject("pond").timers.at(0).framesLeft = 170;

	// From the edge of the bank, one leap to each head and one to the far bank.
	play.standAt(200.0f);
	play.engine.handleKeyPressed(KeyCode::D);
	for (int leap = 0; leap < 4; ++leap)
	{
		play.tap(KeyCode::Space);
		play.frames(32);
	}
	play.engine.handleKeyReleased(KeyCode::D);
	play.frames(5);
	CHECK(play.status("lives") == 3);
	CHECK(play.harry().position.x > 490.0f);
	CHECK(play.feet() == Approx(kSurface));
}
