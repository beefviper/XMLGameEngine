// test_berserk.cpp
// XML Game Engine
// author: beefviper
// date: Oct 4, 2026
//
// Catch2 tests for games/descriptions/berserk.xml, rewritten on 2026-10-04 after Stern's
// Berzerk (the first version was one of the games another AI wrote from the
// schema alone, see docs/designs/55-games-written-by-ai.md). Played frame by frame through a real
// xge::Engine with no window, with the keys a player would press: Space
// starts, W, A, S, D (or the arrows) walk and set which way the man faces,
// Space fires, P pauses.
//
// The rooms are the scenes of the playing state (playing.room1 to room4),
// each with its own maze of walls and exits and its own robots, named by
// room and colour: y1 to y3 in room one, r1 to r4, c1 to c5, g1 to g6
// (playing.room2.r3).
//
// Window::init() normally measures each object's size once a backend exists;
// measure() gives every shape the size its sprite implies instead.

#include "engine/engine.h"
#include "game/game.h"

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

	// A room is 5 cells across and 3 down, each cell 112 by 128 with an 8
	// pixel wall, starting 48 pixels down the window (below the score); the
	// exits are the middle of each side. The man is 14 by 22 and the robots
	// 14 by 18. Every room keeps the cell of the left exit and the one
	// beside it (cells 0 and 1 of row 1) open between them, and no robot
	// stands in a cell the man comes in at (the middle of each side).
	constexpr float cellWidth = 112.0f;
	constexpr float cellHeight = 128.0f;
	constexpr float mazeTop = 48.0f;
	constexpr float roomRight = 568.0f;
	constexpr float roomBottom = 440.0f;

	struct Play
	{
		Game game;
		Engine engine;

		explicit Play(const char* file = "descriptions/berserk.xml")
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
		// The room the man is in: 1 to 4, or 0 when the game is not being played.
		int room()
		{
			if (state() != "playing") { return 0; }
			const std::string scene = game.currentScene("playing");
			return scene.back() - '0';
		}
		static std::string inRoom(int n, const std::string& name) { return "playing.room" + std::to_string(n) + "." + name; }
		// A robot of a room, by its number in the room: y1, r1, c1 or g1 for
		// the first of rooms one to four.
		static std::string robot(int n, int member) { return inRoom(n, std::string(1, "yrcg"[n - 1]) + std::to_string(member)); }
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

		// The robots of a room still standing.
		int robotsLeft(int n)
		{
			return alive("robot", inRoom(n, ""));
		}

		int alive(const std::string& objClass, const std::string& prefix)
		{
			int k = 0;
			for (auto& object : game.getCurrentObjects())
			{
				if (object.objClass == objClass && object.isVisible && object.name.rfind(prefix, 0) == 0) { ++k; }
			}
			return k;
		}

		// Puts the man in the middle of a cell, counting cells from 0 across
		// and down, without a walk.
		void placeMan(int cellX, int cellY)
		{
			man().position = { static_cast<float>(cellX) * cellWidth + 53.0f, mazeTop + static_cast<float>(cellY) * cellHeight + 57.0f };
		}

		void placeRobot(const std::string& name, int cellX, int cellY)
		{
			object(name).position = { static_cast<float>(cellX) * cellWidth + 53.0f, mazeTop + static_cast<float>(cellY) * cellHeight + 59.0f };
		}

		// Holds every robot of the room still and keeps it from firing, so a
		// test is not about a patrol walking into the thing it checks.
		void freezeRobots()
		{
			for (auto& object : game.getCurrentObjects())
			{
				if (object.objClass == "robot")
				{
					object.velocity = { 0.0f, 0.0f };
					object.timers.clear();
				}
			}
		}

		// Plays a frame with the robots standing where they are (they keep
		// their timers, so they still fire), and the man's lives kept up.
		void standingFrame()
		{
			frames(1);
			man().variable["lives"] = 3.0f;
			for (auto& object : game.getCurrentObjects())
			{
				if (object.objClass == "robot") { object.velocity = { 0.0f, 0.0f }; }
			}
		}

		// Takes every robot of a room out of play, as if they were all shot.
		void killRobots(int n)
		{
			const std::string prefix = inRoom(n, "");
			for (auto& object : game.getCurrentObjects())
			{
				if (object.objClass == "robot" && object.name.rfind(prefix, 0) == 0) { object.isVisible = false; object.collisionData.enabled = false; }
			}
		}

		// Starts the game and holds the robots still.
		void begin(bool freezeThem = true)
		{
			tap(KeyCode::Space);
			if (freezeThem) { freezeRobots(); }
		}

		// Walks out of the room by one of the exits from just in front of it,
		// and stops once through (the man is put back at the start of the
		// next room, still walking, so a longer walk would go out again).
		void leaveByTheLeft() { placeMan(0, 1); man().position.x = 10.0f; walk(KeyCode::A, 6); }
		void leaveByTheRight() { placeMan(4, 1); man().position.x = roomRight - 24.0f; walk(KeyCode::D, 6); }
		void leaveByTheTop() { placeMan(2, 0); man().position.y = mazeTop + 10.0f; walk(KeyCode::W, 6); }
		void leaveByTheBottom() { placeMan(2, 2); man().position.y = roomBottom - 34.0f; walk(KeyCode::S, 6); }

		// The rooms are two by two, going round: 1 and 2 on top, 3 and 4
		// below. On round all four, 1, 2, 4, 3 and 1 again: west from the
		// top left and the bottom right, north from the other two.
		void leaveForTheNext()
		{
			if (room() == 1 || room() == 4) { leaveByTheLeft(); }
			else { leaveByTheTop(); }
		}

		// Walks into the top wall of the second cell of the first row.
		void touchTheWall() { placeMan(1, 0); man().position.y = mazeTop + 9.0f; walk(KeyCode::W, 2); }

		// The man in the cell of the left exit facing the robot he has put
		// in the next cell, and a shot fired at it.
		void shootRobot(const std::string& name)
		{
			placeMan(0, 1);
			placeRobot(name, 1, 1);
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

TEST_CASE("berserk.xml loads the man, four rooms of robots, exits, an Otto and a robot shot to copy", "[berserk]")
{
	Play play;

	CHECK(play.state() == "title");
	CHECK(play.man().isVisible);
	CHECK(play.lives() == 3.0f);
	CHECK(play.score() == 0.0f);
	CHECK_FALSE(play.bullet().isVisible);
	CHECK_FALSE(play.object("otto").isVisible);
	CHECK(play.alive("robotshot") == 0);
	CHECK(play.game.currentScene("playing") == "playing.room1");

	// The man and what is his are the playing state's, there in every room.
	CHECK(play.man().ownerState == "playing");
	CHECK(play.man().ownerScene.empty());
	CHECK(play.object("score").ownerState == "playing");
	CHECK_FALSE(play.game.isShown(play.object("score")));

	// Each room is a map of the same frame: a pillar at every corner of a
	// cell, the walls between them, more robots than the room before, and
	// four exits that are never drawn.
	for (int room = 1; room <= 4; ++room)
	{
		Object& corner = play.object(Play::inRoom(room, "post.1"));
		CHECK(corner.position.x == 0.0f);
		CHECK(corner.position.y == mazeTop);
		CHECK(corner.size.x == 8.0f);
		Object& last = play.object(Play::inRoom(room, "post.24"));
		CHECK(last.position.x == roomRight - 8.0f);
		CHECK(last.position.y == roomBottom - 8.0f);
		CHECK(play.object(Play::inRoom(room, "across.1")).size.x == cellWidth - 8.0f);
		CHECK(play.object(Play::inRoom(room, "down.1")).size.y == cellHeight - 8.0f);
		CHECK(play.object(Play::inRoom(room, "down.1")).isDrawn);
		CHECK(play.robotsLeft(room) == room + 2);
		for (const char* side : { "north", "south", "west", "east" })
		{
			CHECK(play.alive(std::string(side) + "exit", Play::inRoom(room, "")) == 1);
			CHECK_FALSE(play.object(Play::inRoom(room, std::string(side) + ".1")).isDrawn);
		}
	}
	CHECK(count(play, "playing.robotshot.") == 0); // made when a robot fires
	CHECK(play.object("playing.robotshot").most == 4);
}

TEST_CASE("Space starts Berserk, and the man walks the four ways and faces the way he walked", "[berserk]")
{
	Play play;
	play.begin();
	CHECK(play.room() == 1);

	Object& man = play.man();
	play.placeMan(1, 1);
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
	play.placeMan(1, 1);

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
		play.placeMan(1, 1);
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

	Object& robot = play.object(Play::robot(1, 1));
	REQUIRE(robot.isVisible);
	play.shootRobot(Play::robot(1, 1));

	CHECK_FALSE(robot.isVisible);
	CHECK(play.robotsLeft(1) == 2);
	CHECK(play.score() == 50.0f);
	// The man counts the robots gone in this room.
	CHECK(play.variable("kills") == 1.0f);
	CHECK_FALSE(play.bullet().isVisible);
}

TEST_CASE("a robot touching the man costs a life and sends him back to the start", "[berserk]")
{
	Play play;
	play.begin();

	Object& man = play.man();
	const Vector2f start = man.position;
	const std::string robot = Play::robot(1, 1);
	play.placeMan(0, 1);
	play.placeRobot(robot, 1, 1);
	man.position.x = play.object(robot).position.x - 20.0f;
	play.walk(KeyCode::D, 4);

	CHECK(play.lives() == 2.0f);
	CHECK(man.position.x == Approx(start.x).margin(10.0));
	CHECK(man.position.y == Approx(start.y).margin(4.0));
	CHECK(play.object(robot).isVisible);
	CHECK(play.score() == 0.0f);
}

TEST_CASE("the walls are electrified", "[berserk]")
{
	Play play;
	play.begin();
	Object& man = play.man();
	const Vector2f start = man.position;

	// The top wall of the second cell (the exit is the third's).
	play.touchTheWall();
	CHECK(play.lives() == 2.0f);
	CHECK(man.position.x == Approx(start.x).margin(4.0));
	CHECK(man.position.y == Approx(start.y).margin(4.0));
}

TEST_CASE("the man is safe at the start of every room, and can walk out of each", "[berserk]")
{
	Play play;
	play.begin();

	for (int room : { 1, 2, 4, 3 })
	{
		REQUIRE(play.room() == room);
		play.frames(120);
		CHECK(play.lives() == 3.0f);
		play.killRobots(room);
		play.leaveForTheNext();
	}
	CHECK(play.room() == 1);
	CHECK(play.lives() == 3.0f);
}

// ---------------------------------------------------------------- the robots

TEST_CASE("robots stand a while, then come for the man, and one that walks into a wall is destroyed", "[berserk]")
{
	Play play;
	play.begin(false);

	// Each first turns to the man between one and two and a half seconds in.
	const std::string robot = Play::robot(1, 1);
	const Vector2f started = play.object(robot).position;
	play.frames(59);
	CHECK(play.object(robot).position.x == started.x);
	CHECK(play.object(robot).position.y == started.y);
	CHECK(play.object(robot).velocity.x == 0.0f);

	const auto apart = [&](const Object& object) { return std::hypot(object.position.x - play.man().position.x, object.position.y - play.man().position.y); };
	const float before = apart(play.object(robot));
	play.frames(150);
	play.man().variable["lives"] = 3.0f;
	CHECK(apart(play.object(robot)) < before);

	// One walked into the wall between the first two cells of row one is gone,
	// and scores as if shot.
	Play again;
	again.begin();
	Object& walker = again.object(robot);
	walker.position = { 2 * cellWidth - 18.0f, mazeTop + cellHeight + 59.0f };
	walker.velocity = { 1.0f, 0.0f };
	again.frames(6);
	CHECK_FALSE(walker.isVisible);
	CHECK(again.score() == 50.0f);
	CHECK(again.variable("kills") == 1.0f);
	CHECK(again.robotsLeft(1) == 2);
}

TEST_CASE("the first room's robots never fire; the next room's fire at the man, and a shot that reaches him costs a life", "[berserk]")
{
	Play play;
	play.begin(false);
	for (int frame = 0; frame < 600; ++frame) { play.standingFrame(); }
	CHECK(play.alive("robotshot") == 0);

	// In room two, within ten seconds a robot has fired.
	play.leaveByTheLeft();
	REQUIRE(play.room() == 2);
	bool fired = false;
	for (int frame = 0; frame < 600 && !fired; ++frame)
	{
		play.standingFrame();
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
	// between 2.75 and 6.5 seconds at depth 1, between 0.8 and 2 at depth 12
	// (every robot's first wait starts on the first frame of the room, so
	// the depth is given before the man goes into room two and it counts one
	// more). Otto's wait is 14 seconds, less the depth, but never under 6.
	const auto firstShot = [](float depth)
	{
		Play play;
		play.game.incrementText("player.depth", depth);
		play.begin(false);
		play.leaveByTheLeft();
		int frame = 0;
		while (play.alive("robotshot") == 0 && frame < 1000)
		{
			play.standingFrame();
			++frame;
		}
		return frame;
	};
	CHECK(firstShot(0.0f) >= 155);
	CHECK(firstShot(11.0f) <= 2 * 60 + 2);

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

TEST_CASE("walking through a gap leaves the room: the next room, the man in at the side opposite, that doorway shut", "[berserk]")
{
	Play play;
	play.begin();
	REQUIRE(play.room() == 1);
	const auto shut = [&](const char* door) { return play.object(door).isVisible; };
	for (const char* door : { "northdoor", "southdoor", "westdoor", "eastdoor" }) { CHECK_FALSE(shut(door)); }

	// Out by the left, in at the right, with the right-hand doorway shut.
	play.leaveByTheLeft();
	CHECK(play.room() == 2);
	CHECK(play.variable("depth") == 1.0f);
	CHECK(play.lives() == 3.0f);
	CHECK(play.man().position.x > 500.0f);
	CHECK(play.man().position.y == Approx(233));
	CHECK(shut("eastdoor"));
	CHECK_FALSE(shut("westdoor"));

	// Out by the top, in at the bottom of the room below; the doorway before
	// opens again.
	play.leaveByTheTop();
	CHECK(play.room() == 4);
	CHECK(play.man().position.x == Approx(277));
	CHECK(play.man().position.y > 380.0f);
	CHECK(shut("southdoor"));
	CHECK_FALSE(shut("eastdoor"));

	// East of the bottom right is the bottom left, round the side.
	play.leaveByTheRight();
	CHECK(play.room() == 3);
	CHECK(play.man().position.x < 40.0f);
	CHECK(shut("westdoor"));

	// South of the bottom left is the top left, round the bottom.
	play.leaveByTheBottom();
	CHECK(play.room() == 1);
	CHECK(play.man().position.y < 100.0f);
	CHECK(shut("northdoor"));
	CHECK(play.variable("depth") == 4.0f);
	CHECK(play.lives() == 3.0f);

	// The shut doorway is electrified like the walls.
	play.man().position.y = mazeTop + 20.0f;
	play.walk(KeyCode::W, 4);
	CHECK(play.lives() == 2.0f);
	CHECK(play.room() == 1);
}

TEST_CASE("only the room the man is in plays: the others' robots are away and stand still", "[berserk]")
{
	Play play;
	play.begin();
	play.leaveByTheLeft();
	REQUIRE(play.room() == 2);

	Object& left = play.object(Play::robot(1, 1));
	CHECK(play.game.isAway(left));
	CHECK_FALSE(play.game.isShown(left));
	CHECK(play.game.isShown(play.object(Play::robot(2, 1))));
	CHECK_FALSE(play.game.isShown(play.object(Play::inRoom(1, "post.1"))));
	CHECK(play.game.isShown(play.object(Play::inRoom(2, "post.1"))));

	const Vector2f robotBefore = left.position;
	play.frames(60);
	CHECK(left.position.x == robotBefore.x);
	CHECK(left.position.y == robotBefore.y);
}

TEST_CASE("a room left keeps where it was, and is put back as it started when the man comes in again", "[berserk]")
{
	Play play;
	play.begin(false);

	// Room one's first robot walks off from where it started.
	Object& robot = play.object(Play::robot(1, 1));
	const Vector2f started = robot.position;
	play.frames(200);
	play.man().variable["lives"] = 3.0f;

	// Away, it stands where it was left.
	play.leaveByTheLeft();
	REQUIRE(play.room() == 2);
	const Vector2f left = robot.position;
	REQUIRE(std::hypot(left.x - started.x, left.y - started.y) > 10.0f);
	play.frames(60);
	CHECK(robot.position.x == Approx(left.x));
	CHECK(robot.position.y == Approx(left.y));

	// Round the four rooms, the way out of room three puts room one back: the
	// robot is at its start again, and stands there a while.
	for (int step = 0; step < 3; ++step) { play.leaveForTheNext(); }
	REQUIRE(play.room() == 1);
	CHECK(robot.position.x == Approx(started.x).margin(4));
	CHECK(robot.position.y == Approx(started.y).margin(4));
}

TEST_CASE("a room with no robot left pays ten more for each destroyed in it, and leaving puts the count back", "[berserk]")
{
	Play play;
	play.begin();

	// Two of room one's three gone is only fifty each.
	play.game.incrementText("player.kills", 2.0f);
	play.frames(2);
	CHECK(play.score() == 0.0f);

	// Leaving forgets them: room two counts its own.
	play.leaveByTheLeft();
	REQUIRE(play.room() == 2);
	CHECK(play.variable("kills") == 0.0f);

	// All four of room two gone.
	play.killRobots(2);
	play.game.incrementText("player.kills", 4.0f);
	play.frames(2);
	CHECK(play.score() == 40.0f);
	CHECK(play.variable("kills") == 0.0f);
	play.frames(10);
	CHECK(play.score() == 40.0f);
}

TEST_CASE("three shots at three robots clear room one and make the bonus", "[berserk]")
{
	Play play;
	play.begin();

	for (int member = 1; member <= 3; ++member)
	{
		const std::string name = Play::robot(1, member);
		REQUIRE(play.object(name).isVisible);
		play.shootRobot(name);
		CHECK_FALSE(play.object(name).isVisible);
	}
	play.frames(2);
	CHECK(play.robotsLeft(1) == 0);
	CHECK(play.score() == 3 * 50.0f + 30.0f);
}

TEST_CASE("coming round again, the rooms have their robots back", "[berserk]")
{
	Play play;
	play.begin();
	play.killRobots(1);
	REQUIRE(play.robotsLeft(1) == 0);

	for (int step = 0; step < 4; ++step) { play.leaveForTheNext(); }
	REQUIRE(play.room() == 1);
	CHECK(play.robotsLeft(1) == 3);
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
	play.placeMan(0, 1);
	otto.position = { play.man().position.x + 20.0f, play.man().position.y - 2.0f };
	otto.velocity = { 0.0f, 0.0f };
	play.walk(KeyCode::D, 4);
	CHECK(play.lives() == 2.0f);
}

TEST_CASE("each room's wait for Otto starts when the man comes in", "[berserk]")
{
	Play play;
	play.begin();

	// Ten seconds in room one, then into room two at depth one: its wait is
	// thirteen seconds from the door, not what was left of room one's.
	play.frames(10 * 60);
	play.leaveByTheLeft();
	REQUIRE(play.room() == 2);
	Object& otto = play.object("otto");
	REQUIRE_FALSE(otto.isVisible);
	int frame = 0;
	while (!otto.isVisible && frame < 2000)
	{
		play.frames(1);
		++frame;
	}
	CHECK(frame == Approx(13 * 60).margin(6));
}

TEST_CASE("Otto comes at once when every robot is gone, goes when the man leaves, and cannot be shot", "[berserk]")
{
	Play play;
	play.begin();
	CHECK_FALSE(play.object("otto").isVisible);

	play.killRobots(1);
	play.frames(2);
	REQUIRE(play.object("otto").isVisible);

	// He goes when the man leaves the room, and room two's robots keep him away.
	play.leaveByTheLeft();
	CHECK(play.room() == 2);
	CHECK_FALSE(play.object("otto").isVisible);
	play.frames(2);
	CHECK_FALSE(play.object("otto").isVisible);

	// A shot meeting him is put away and he stays.
	Object& otto = play.object("otto");
	otto.isVisible = true;
	otto.collisionData.enabled = true;
	otto.velocity = { 0.0f, 0.0f };
	play.placeMan(0, 1);
	otto.position = { play.man().position.x + 60.0f, play.man().position.y - 2.0f };
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
	play.shootRobot(Play::robot(1, 1));
	CHECK(play.lives() == 4.0f);
	CHECK(play.variable("lifepoints") == 0.0f);
	play.frames(10);
	CHECK(play.lives() == 4.0f);
}

TEST_CASE("no men left is game over, over the room standing still, and Space plays again", "[berserk]")
{
	Play play;
	play.begin();
	play.leaveByTheLeft();
	REQUIRE(play.room() == 2);

	play.man().variable["lives"] = 1.0f;
	play.touchTheWall();
	play.frames(2);
	CHECK(play.state() == "gameover");
	CHECK(play.game.isStill(play.object(Play::inRoom(2, "post.1"))));
	CHECK(play.game.isShown(play.object("gameover")));
	CHECK(play.game.isStill(play.object("score")));

	play.tap(KeyCode::Space);
	CHECK(play.state() == "title");
	CHECK(play.lives() == 3.0f);
	CHECK(play.score() == 0.0f);
	CHECK(play.variable("depth") == 0.0f);
	CHECK(play.robotsLeft(1) == 3);
	CHECK(play.game.currentScene("playing") == "playing.room1");
}

TEST_CASE("P pauses over the room standing still, and Space or P carries on", "[berserk]")
{
	Play play;
	play.begin(false);

	play.tap(KeyCode::P);
	CHECK(play.state() == "paused");
	const std::string robot = Play::robot(1, 1);
	const Vector2f standing = play.object(robot).position;
	play.frames(60);
	CHECK(play.object(robot).position.x == standing.x);

	// The room, the man and the score are drawn still under PAUSED.
	CHECK(play.game.isStill(play.object(robot)));
	CHECK(play.game.isStill(play.man()));
	CHECK(play.game.isStill(play.object(Play::inRoom(1, "post.1"))));
	CHECK_FALSE(play.game.isStill(play.object(Play::inRoom(2, "post.1"))));
	CHECK(play.game.isShown(play.object("paused")));
	CHECK(play.game.isStill(play.object("score")));

	play.tap(KeyCode::Space);
	CHECK(play.state() == "playing");
	play.tap(KeyCode::Escape);
	CHECK(play.state() == "paused");
	play.tap(KeyCode::Escape);
	CHECK(play.state() == "playing");
}
