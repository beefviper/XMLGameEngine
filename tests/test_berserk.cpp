// test_berserk.cpp
// XML Game Engine
// author: beefviper
// date: Oct 4, 2026
//
// Catch2 tests for games/berserk.xml, rewritten on 2026-10-04 after Stern's
// Berzerk (the first version was one of the games another AI wrote from the
// schema alone, see docs/designs/games-written-by-ai.md). Played frame by frame through a real
// xge::Engine with no window, with the keys a player would press: Space
// starts, W, A, S, D (or the arrows) walk and set which way the man faces,
// Space fires, P pauses.
//
// Window::init() normally measures each object's size once a backend exists;
// measure() gives every shape the size its sprite implies instead.

#include "engine.h"
#include "game.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
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

	// The maze is 12 cells across and 8 down, each cell a 40 pixel floor and
	// an 8 pixel wall, starting 48 pixels down the window (below the score).
	// The man is 14 by 22 and the robots 14 by 18. Every room keeps the first
	// three cells of the row of the left exit (cells 0 to 2 of row 3) open.
	constexpr float cellSize = 48.0f;
	constexpr float mazeTop = 48.0f;

	struct Play
	{
		Game game;
		Engine engine;

		explicit Play(const char* file = "games/berserk.xml")
			: game(file), engine(game, std::make_unique<FakeWindow>())
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

		// Holds a key down for some frames.
		void walk(KeyCode key, int count)
		{
			engine.handleKeyPressed(key);
			frames(count);
			engine.handleKeyReleased(key);
		}

		std::string state() { return game.getCurrentState().name; }
		Object& object(const std::string& name) { return game.getObject(name); }
		Object& man() { return object("player"); }
		Object& bullet() { return object("bullet"); }
		float variable(const char* name) { return man().variable[name]; }
		float lives() { return variable("lives"); }
		float score() { return variable("score"); }

		int alive(const std::string& objClass)
		{
			int n = 0;
			for (auto& object : game.getCurrentObjects())
			{
				if (object.objClass == objClass && object.isVisible) { ++n; }
			}
			return n;
		}

		// Puts the man in the middle of a cell, counting cells from 0 across
		// and down, without a walk.
		void placeMan(int cellX, int cellY)
		{
			man().position = { static_cast<float>(cellX) * cellSize + 8.0f + 13.0f, mazeTop + static_cast<float>(cellY) * cellSize + 8.0f + 9.0f };
		}

		void placeRobot(const std::string& name, int cellX, int cellY)
		{
			object(name).position = { static_cast<float>(cellX) * cellSize + 8.0f + 13.0f, mazeTop + static_cast<float>(cellY) * cellSize + 8.0f + 11.0f };
		}

		// Holds every robot of the room still and keeps it from firing, so a
		// test is not about a patrol walking into the thing it checks.
		void freezeRobots()
		{
			for (auto& object : game.getCurrentObjects())
			{
				if (object.objClass.rfind("robots", 0) == 0)
				{
					object.velocity = { 0.0f, 0.0f };
					object.timers.clear();
				}
			}
		}

		// Takes every robot of a class out of play, as if they were all shot.
		void killRobots(const std::string& objClass)
		{
			for (auto& object : game.getCurrentObjects())
			{
				if (object.objClass == objClass) { object.isVisible = false; object.collisionData.enabled = false; }
			}
		}

		// Starts the game and holds the robots still.
		void begin(bool freezeThem = true)
		{
			tap(KeyCode::Space);
			if (freezeThem) { freezeRobots(); }
		}

		// Walks out of the room by one of the exits from the cell in front of
		// it, and stops once through (the man is put back at the start of the
		// next room, still walking, so a longer walk would go out again).
		void leaveByTheLeft() { placeMan(0, 3); walk(KeyCode::A, 9); }
		void leaveByTheRight() { placeMan(11, 3); walk(KeyCode::D, 9); }
		void leaveByTheTop() { placeMan(5, 0); walk(KeyCode::W, 6); }
		void leaveByTheBottom() { placeMan(5, 7); walk(KeyCode::S, 6); }

		// The man in the first cell of the row of the exit facing the robot
		// he has put two cells along, and a shot fired at it.
		void shootRobot(const std::string& name)
		{
			placeMan(0, 3);
			placeRobot(name, 2, 3);
			walk(KeyCode::D, 1);
			tap(KeyCode::Space);
			frames(60);
		}
	};

	int count(Play& play, const std::string& prefix)
	{
		int n = 0;
		for (auto& object : play.game.getCurrentObjects())
		{
			if (object.name.rfind(prefix, 0) == 0) { ++n; }
		}
		return n;
	}
}

// ------------------------------------------------------------------ the game

TEST_CASE("berserk.xml loads the man, four rooms of robots, exits, an Otto and a pool of robot shots", "[berserk]")
{
	Play play;

	CHECK(play.state() == "title");
	CHECK(play.man().isVisible);
	CHECK(play.lives() == 3.0f);
	CHECK(play.score() == 0.0f);
	CHECK_FALSE(play.bullet().isVisible);
	CHECK_FALSE(play.object("otto").isVisible);
	CHECK(play.alive("robotshot") == 0);
	CHECK(play.object("walls1").size.x == 584.0f);
	CHECK(play.object("walls1").size.y == 392.0f);

	// Each room has more robots than the one before, and four exits.
	CHECK(play.alive("robots1") == 3);
	CHECK(play.alive("robots2") == 4);
	CHECK(play.alive("robots3") == 5);
	CHECK(play.alive("robots4") == 6);
	for (int room = 1; room <= 4; ++room) { CHECK(play.alive("exit" + std::to_string(room)) == 4); }
	CHECK(count(play, "robotshots.") == 4);
}

TEST_CASE("Space starts Berserk, and the man walks the four ways and faces the way he walked", "[berserk]")
{
	Play play;
	play.begin();
	CHECK(play.state() == "room1");

	Object& man = play.man();
	play.placeMan(1, 3);
	const Vector2f start = man.position;

	play.walk(KeyCode::D, 5);
	CHECK(man.position.x > start.x);
	CHECK(man.facing == Direction::Right);
	CHECK(man.lookName() == "right");

	play.walk(KeyCode::Up, 5);
	CHECK(man.position.y < start.y);
	CHECK(man.facing == Direction::Up);
	CHECK(man.lookName() == "up");

	play.walk(KeyCode::A, 5);
	CHECK(man.facing == Direction::Left);
	CHECK(man.lookName() == "left");

	play.walk(KeyCode::S, 5);
	CHECK(man.facing == Direction::Down);
	CHECK(man.lookName() == "down");
}

TEST_CASE("the man shoots the way he faces, one shot at a time", "[berserk]")
{
	Play play;
	play.begin();
	Object& bullet = play.bullet();
	play.placeMan(5, 4);

	struct Way { KeyCode key; float vx; float vy; };
	for (const Way& way : { Way{ KeyCode::A, -1.0f, 0.0f }, Way{ KeyCode::D, 1.0f, 0.0f }, Way{ KeyCode::W, 0.0f, -1.0f }, Way{ KeyCode::S, 0.0f, 1.0f } })
	{
		play.walk(way.key, 1);
		play.tap(KeyCode::Space);
		REQUIRE(bullet.isVisible);
		CHECK((bullet.velocity.x > 0.0f) == (way.vx > 0.0f));
		CHECK((bullet.velocity.x < 0.0f) == (way.vx < 0.0f));
		CHECK((bullet.velocity.y > 0.0f) == (way.vy > 0.0f));
		CHECK((bullet.velocity.y < 0.0f) == (way.vy < 0.0f));

		// It flies on until it meets a wall (or the edge of the window).
		play.frames(120);
		CHECK_FALSE(bullet.isVisible);
		play.placeMan(5, 4);
	}

	// While one is in flight another cannot be fired.
	play.tap(KeyCode::Space);
	REQUIRE(bullet.isVisible);
	play.frames(2);
	const Vector2f flying = bullet.position;
	play.tap(KeyCode::Space);
	play.frames(1);
	CHECK(bullet.position.x == Approx(flying.x + bullet.velocity.x).margin(0.01));
	CHECK(bullet.position.y == Approx(flying.y + bullet.velocity.y).margin(0.01));
}

TEST_CASE("a shot robot is gone and scores fifty, and the shot goes with it", "[berserk]")
{
	Play play;
	play.begin();

	Object& robot = play.object("robots1left.1");
	REQUIRE(robot.isVisible);
	play.shootRobot("robots1left.1");

	CHECK_FALSE(robot.isVisible);
	CHECK(play.alive("robots1") == 2);
	CHECK(play.score() == 50.0f);
	CHECK(play.object("tally1").variable["kills"] == 1.0f);
	CHECK_FALSE(play.bullet().isVisible);
}

TEST_CASE("a robot touching the man costs a life and sends him back to the start", "[berserk]")
{
	Play play;
	play.begin();

	Object& man = play.man();
	const Vector2f start = man.position;
	play.placeMan(0, 3);
	play.placeRobot("robots1left.1", 2, 3);
	man.position.x = play.object("robots1left.1").position.x - 20.0f;
	play.walk(KeyCode::D, 4);

	CHECK(play.lives() == 2.0f);
	CHECK(man.position.x == Approx(start.x).margin(10.0));
	CHECK(man.position.y == Approx(start.y).margin(4.0));
	CHECK(play.object("robots1left.1").isVisible);
	CHECK(play.score() == 0.0f);
}

TEST_CASE("the walls are electrified", "[berserk]")
{
	Play play;
	play.begin();
	Object& man = play.man();
	const Vector2f start = man.position;

	// The top wall is a few steps up from the first row of cells (the exit
	// is in the fifth column; this is the third).
	play.placeMan(2, 0);
	play.walk(KeyCode::W, 6);
	CHECK(play.lives() == 2.0f);
	CHECK(man.position.x == Approx(start.x).margin(4.0));
	CHECK(man.position.y == Approx(start.y).margin(4.0));
}

TEST_CASE("the man is safe at the start of every room, and can walk out of each", "[berserk]")
{
	Play play;
	play.begin();

	for (int room = 1; room <= 4; ++room)
	{
		REQUIRE(play.state() == "room" + std::to_string(room));
		play.frames(120);
		CHECK(play.lives() == 3.0f);
		play.killRobots("robots" + std::to_string(room));
		play.leaveByTheLeft();
	}
	CHECK(play.state() == "room1");
	CHECK(play.lives() == 3.0f);
}

// ---------------------------------------------------------------- the robots

TEST_CASE("robots patrol their corridors and turn at the walls instead of leaving", "[berserk]")
{
	Play play;
	play.begin(false);

	// The man stands at the start; any robot that reaches him only costs a
	// life, which this gives back.
	for (int block = 0; block < 20; ++block)
	{
		play.frames(180);
		play.man().variable["lives"] = 3.0f;
		for (auto& object : play.game.getCurrentObjects())
		{
			if (object.objClass != "robots1" || !object.isVisible) { continue; }
			CHECK(object.position.x > 8.0f);
			CHECK(object.position.x + object.size.x < 584.0f - 8.0f);
			CHECK(object.position.y > mazeTop + 8.0f);
			CHECK(object.position.y + object.size.y < mazeTop + 392.0f - 8.0f);
		}
	}
	CHECK(play.alive("robots1") == 3);
}

TEST_CASE("robots fire at the man, and a shot that reaches the man costs a life", "[berserk]")
{
	Play play;
	play.begin(false);

	// Within ten seconds a robot has fired.
	bool fired = false;
	for (int frame = 0; frame < 600 && !fired; ++frame)
	{
		play.man().variable["lives"] = 3.0f;
		play.frames(1);
		fired = play.alive("robotshot") > 0;
	}
	REQUIRE(fired);

	// Straight at him (<aim>), at any angle: towards his middle.
	Object* shot = nullptr;
	for (auto& object : play.game.getCurrentObjects())
	{
		if (object.objClass == "robotshot" && object.isVisible) { shot = &object; break; }
	}
	REQUIRE(shot);
	const Vector2f toMan = (play.man().position + play.man().size * 0.5f) - (shot->position + shot->size * 0.5f);
	const float along = (toMan.x * shot->velocity.x + toMan.y * shot->velocity.y)
		/ (std::hypot(toMan.x, toMan.y) * std::hypot(shot->velocity.x, shot->velocity.y));
	CHECK(along > 0.99f);

	// Nothing else fires while this is looked at: the robots' waits are random,
	// and another robot's shot would come out of the pool in the very slot this
	// one is put away into (which is what made this test fail one run in fifty).
	for (auto& object : play.game.getCurrentObjects())
	{
		for (auto& timer : object.timers) { timer.framesLeft = 1000000; }
	}

	// And one that meets the man ends a life (a wall in the way would do as much).
	play.man().variable["lives"] = 3.0f;
	play.man().position = { shot->position.x - 4.0f, shot->position.y - 8.0f };
	play.frames(3);
	CHECK(play.lives() < 3.0f);
	CHECK_FALSE(shot->isVisible);
}

TEST_CASE("robots fire more often, and Otto comes sooner, the deeper the man has gone", "[berserk]")
{
	// A robot's wait is worked out when its timer starts, from player.depth:
	// between 3 and 7 seconds at depth 0, between 0.8 and 2 at depth 12 (every
	// robot's first wait starts on the first frame of the room, so the depth
	// is given before it). Otto's wait is 14 seconds, less the depth, but never
	// under 6.
	const auto firstShot = [](float depth)
	{
		Play play;
		play.game.incrementText("player.depth", depth);
		play.begin(false);
		int frame = 0;
		while (play.alive("robotshot") == 0 && frame < 1000)
		{
			play.frames(1);
			++frame;
			play.man().variable["lives"] = 3.0f;
		}
		return frame;
	};
	CHECK(firstShot(0.0f) >= 3 * 60);
	CHECK(firstShot(12.0f) <= 2 * 60 + 2);

	const auto ottoAt = [](float depth)
	{
		Play play;
		play.game.incrementText("player.depth", depth);
		play.begin();
		int frame = 0;
		while (!play.object("otto").isVisible && frame < 2000)
		{
			play.frames(1);
			++frame;
		}
		return frame;
	};
	CHECK(ottoAt(0.0f) == Approx(14 * 60).margin(2));
	CHECK(ottoAt(5.0f) == Approx(9 * 60).margin(2));
	CHECK(ottoAt(20.0f) == Approx(6 * 60).margin(2));
}

// ------------------------------------------------------------ rooms and exits

TEST_CASE("walking through a gap leaves the room: the next room, the man back at its door", "[berserk]")
{
	Play play;
	play.begin();
	REQUIRE(play.state() == "room1");

	play.leaveByTheLeft();
	CHECK(play.state() == "room2");
	CHECK(play.variable("depth") == 1.0f);
	CHECK(play.variable("exited") == 0.0f);
	CHECK(play.lives() == 3.0f);
	CHECK(play.man().position.x < 40.0f);

	// The other exits do the same, and after room four comes room one.
	play.leaveByTheTop();
	CHECK(play.state() == "room3");
	play.leaveByTheRight();
	CHECK(play.state() == "room4");
	play.leaveByTheBottom();
	CHECK(play.state() == "room1");
	CHECK(play.variable("depth") == 4.0f);
}

TEST_CASE("shooting every robot of a room pays ten more for each, and leaving puts the count back", "[berserk]")
{
	Play play;
	play.begin();

	// Room one has three robots: two shot is only fifty each.
	Object& tally = play.object("tally1");
	tally.variable["kills"] = 2.0f;
	play.frames(2);
	CHECK(play.score() == 0.0f);

	// Leaving puts the count back, so the two do not add to the next visit's.
	play.leaveByTheLeft();
	REQUIRE(play.state() == "room2");
	CHECK(tally.variable["kills"] == 0.0f);

	// Room two has four robots.
	Object& tally2 = play.object("tally2");
	tally2.variable["kills"] = 4.0f;
	play.frames(2);
	CHECK(play.score() == 40.0f);
	CHECK(tally2.variable["kills"] == 0.0f);
	play.frames(10);
	CHECK(play.score() == 40.0f);
}

TEST_CASE("three shots at three robots clear room one and make the bonus", "[berserk]")
{
	Play play;
	play.begin();

	for (const char* name : { "robots1left.1", "robots1up.1", "robots1up.2" })
	{
		REQUIRE(play.object(name).isVisible);
		play.shootRobot(name);
		CHECK_FALSE(play.object(name).isVisible);
	}
	play.frames(2);
	CHECK(play.alive("robots1") == 0);
	CHECK(play.score() == 3 * 50.0f + 30.0f);
}

TEST_CASE("coming round again, the rooms have their robots back", "[berserk]")
{
	Play play;
	play.begin();
	play.killRobots("robots1");
	REQUIRE(play.alive("robots1") == 0);

	for (int room = 1; room <= 4; ++room) { play.leaveByTheLeft(); }
	REQUIRE(play.state() == "room1");
	CHECK(play.alive("robots1") == 3);
}

TEST_CASE("the robots of one room stand still while the man is in another", "[berserk]")
{
	Play play;
	play.begin();
	play.leaveByTheLeft();
	REQUIRE(play.state() == "room2");

	const Vector2f robotBefore = play.object("robots1left.1").position;
	play.frames(60);
	CHECK(play.object("robots1left.1").position.x == robotBefore.x);
	CHECK(play.object("robots1left.1").position.y == robotBefore.y);
}

// ----------------------------------------------------------------- Evil Otto

TEST_CASE("Evil Otto turns up after a while, and one touch costs the man a life", "[berserk]")
{
	Play play;
	play.begin();
	Object& otto = play.object("otto");
	CHECK_FALSE(otto.isVisible);

	play.frames(13 * 60);
	CHECK_FALSE(otto.isVisible);
	play.frames(2 * 60);
	REQUIRE(otto.isVisible);

	// He comes for the man (<chase>), and meeting him is the end of one life.
	const auto apart = [&] { return std::hypot(otto.position.x - play.man().position.x, otto.position.y - play.man().position.y); };
	const float before = apart();
	play.frames(30);
	CHECK(apart() < before - 25.0f);
	play.placeMan(0, 3);
	otto.position = { play.man().position.x + 20.0f, play.man().position.y - 2.0f };
	otto.velocity = { 0.0f, 0.0f };
	play.walk(KeyCode::D, 4);
	CHECK(play.lives() == 2.0f);
}

TEST_CASE("Otto comes at once when every robot is gone, goes when the man leaves, and cannot be shot", "[berserk]")
{
	Play play;
	play.begin();
	CHECK_FALSE(play.object("otto").isVisible);

	play.killRobots("robots1");
	play.frames(2);
	REQUIRE(play.object("otto").isVisible);

	// He goes when the man leaves the room.
	play.leaveByTheLeft();
	CHECK(play.state() == "room2");
	CHECK_FALSE(play.object("otto").isVisible);

	// A shot meeting him is put away and he stays.
	Object& otto = play.object("otto");
	otto.isVisible = true;
	otto.collisionData.enabled = true;
	otto.velocity = { 0.0f, 0.0f };
	play.placeMan(0, 3);
	otto.position = { 2 * cellSize + 21.0f, mazeTop + 3 * cellSize + 8.0f + 5.0f };
	play.walk(KeyCode::D, 1);
	play.tap(KeyCode::Space);
	play.frames(40);
	CHECK(otto.isVisible);
	CHECK_FALSE(play.bullet().isVisible);
	CHECK(play.lives() == 3.0f);
}

// ------------------------------------------------------------- score and men

TEST_CASE("an extra man at every two thousand points", "[berserk]")
{
	Play play;
	play.begin();

	// A robot is fifty points; the fortieth makes the man.
	play.man().variable["lifepoints"] = 1950.0f;
	play.shootRobot("robots1left.1");
	CHECK(play.lives() == 4.0f);
	CHECK(play.variable("lifepoints") == 0.0f);
	play.frames(10);
	CHECK(play.lives() == 4.0f);
}

TEST_CASE("no men left is game over, and Space plays again", "[berserk]")
{
	Play play;
	play.begin();

	play.man().variable["lives"] = 1.0f;
	play.placeMan(2, 0);
	play.walk(KeyCode::W, 6);
	play.frames(2);
	CHECK(play.state() == "gameover");

	play.tap(KeyCode::Space);
	CHECK(play.state() == "title");
	CHECK(play.lives() == 3.0f);
	CHECK(play.score() == 0.0f);
	CHECK(play.alive("robots1") == 3);
}

TEST_CASE("P pauses, and Space or P carries on", "[berserk]")
{
	Play play;
	play.begin(false);

	play.tap(KeyCode::P);
	CHECK(play.state() == "paused");
	const Vector2f robot = play.object("robots1left.1").position;
	play.frames(60);
	CHECK(play.object("robots1left.1").position.x == robot.x);

	play.tap(KeyCode::Space);
	CHECK(play.state() == "room1");
	play.tap(KeyCode::Escape);
	CHECK(play.state() == "paused");
	play.tap(KeyCode::Escape);
	CHECK(play.state() == "room1");
}
