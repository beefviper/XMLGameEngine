// test_pacman.cpp
// XML Game Engine
// author: beefviper
// date: Oct 15, 2026
//
// Catch2 tests for games/descriptions/pacman.xml, played frame by frame by a
// real xge::Game with no window, and for what it is the first game with: a
// <map> of characters that places the walls, dots, Pac-Man and the ghosts;
// a bitmap's <color char>; objects that walk the map (<maze>) by <go>,
// <hunt> and <wander>; timers that follow a look; <become from> and
// <trigger from>; and a maze's <slow>. Its maze is the arcade's, 28 tiles
// by 31.
//
// Window::init() normally measures each object's size once a backend exists;
// measure() gives every shape the size its sprite implies instead.

#include "scratch.h"
#include "play/executor.h"
#include "engine/folder.h"
#include "game/game.h"
#include "xml/document.h"
#include "load/xsd.h"
#include "picture/bitmap.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace xge;
using Catch::Approx;
using Catch::Matchers::ContainsSubstring;

namespace
{
	constexpr float kTile = 16.0f;
	constexpr float kTop = 48.0f;

	void measure(Game& game)
	{
		for (auto& object : game.getCurrentObjects())
		{
			object.size = measureShapeSize(object.picture);
		}
	}

	// The middle of a tile of the maze, column and row from 0.
	Vector2f tile(int column, int row)
	{
		return { (static_cast<float>(column) + 0.5f) * kTile, kTop + (static_cast<float>(row) + 0.5f) * kTile };
	}

	Vector2f middleOf(const Object& object) { return object.position + object.size * 0.5f; }

	struct Maze
	{
		Game game{ "descriptions/pacman.xml" };
		CommandExecutor executor{ game };

		Maze()
		{
			measure(game);
			game.setCurrentState(0);
			game.pushState("playing");
		}

		Object& pacman() { return game.getObject("playing.pacman"); }
		float status(const char* variable) { return game.getObject("playing.status").variable[variable]; }
		Object& ghost(const std::string& name) { return game.getObject("playing.maze." + name + ".1"); }
		std::string state() { return game.getCurrentState().name; }

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { game.updateObjects(); }
		}

		// Pac-Man, out of the maze's hands, runs into `other` from its left:
		// touching happens on the move. Both touch with their hitbox if they
		// have one.
		void touch(const Object& other)
		{
			const auto box = [](const Object& object, Vector2f& middle, float& half)
			{
				const bool own = object.collisionData.hasHitbox;
				const Vector2f size = own ? object.collisionData.hitboxSize : object.size;
				middle = object.position + (own ? object.collisionData.hitboxOffset : Vector2f{}) + size * 0.5f;
				half = size.x * 0.5f;
			};
			Object& p = pacman();
			Vector2f mine{};
			Vector2f theirs{};
			float myHalf = 0.0f;
			float theirHalf = 0.0f;
			box(p, mine, myHalf);
			box(other, theirs, theirHalf);
			p.walk = {};
			p.position += theirs - mine - Vector2f{ myHalf + theirHalf + 1.0f, 0.0f };
			p.velocity = { 3.0f, 0.0f };
			frames(1);
			p.velocity = {};
			p.position = { 2000.0f, 2000.0f };
		}

		void press(const char* action) { executor.executeInput(Command{ CmdTriggerAction{ "playing.pacman", action } }, true); }

		// Every ghost but `keep` out of play, so that a test of one is not
		// caught by another.
		void onlyGhost(const std::string& keep)
		{
			for (const char* name : { "blinky", "pinky", "inky", "clyde" })
			{
				if (name != keep) { ghost(name).isVisible = false; }
			}
		}

		int count(const std::string& group)
		{
			int found = 0;
			for (auto& object : game.getCurrentObjects())
			{
				if (object.inGroup(group) && object.isVisible) { ++found; }
			}
			return found;
		}
	};

	std::string loadError(const std::string& xml)
	{
		const std::filesystem::path path = scratchName("descriptions/pacman_scratch.xml");
		{
			std::ofstream out(path);
			out << xml;
		}
		std::string error;
		try
		{
			Game game(path.string());
		}
		catch (const std::exception& thrown)
		{
			error = thrown.what();
		}
		std::error_code ignored;
		std::filesystem::remove(path, ignored);
		return error;
	}

	// A small game around a <map>, with `objects` in place of OBJECTS.
	std::string mapGame(const std::string& objects)
	{
		return R"(<game>
  <window name="t"><width>200</width><height>200</height><background>color.black</background><fullscreen>false</fullscreen><framerate>60</framerate></window>
  <variables><variable name="tile">10</variable></variables>
  <states>
    <state name="playing">
      <objects>
)" + objects + R"(
      </objects>
      <inputs><input button="space"><pop /></input></inputs>
    </state>
  </states>
</game>)";
	}

	const std::string kWallSprite = "<sprite><rectangle><width>10</width><height>10</height><color>color.blue</color></rectangle></sprite>";
	const std::string kDotSprite = "<sprite><rectangle><width>2</width><height>2</height><color>color.white</color></rectangle></sprite>";
}

TEST_CASE("pacman: the map puts every piece on its tile", "[pacman][map]")
{
	Maze maze;

	// Pac-Man's '@' is on column 14 of row 23, and he sits in the middle of it.
	CHECK(middleOf(maze.pacman()).x == Approx(tile(14, 23).x));
	CHECK(middleOf(maze.pacman()).y == Approx(tile(14, 23).y));
	CHECK(maze.pacman().inGroup("playing.maze"));

	// The arcade's maze: 240 dots and four pellets, one group of food.
	CHECK(maze.count("playing.dots") == 240);
	CHECK(maze.count("playing.pellets") == 4);
	CHECK(maze.count("playing.food") == 244);
	CHECK(maze.count("playing.walls") == 480);
	CHECK(middleOf(maze.game.getObject("playing.maze.pellets.1")).x == Approx(tile(1, 3).x));
	CHECK(middleOf(maze.game.getObject("playing.maze.pellets.1")).y == Approx(tile(1, 3).y));
	CHECK(middleOf(maze.game.getObject("playing.maze.door.1")).x == Approx(tile(13, 12).x));
	CHECK(maze.game.getObject("playing.maze.door.1").inGroup("playing.walls"));
	CHECK(maze.game.getObject("playing.maze.door.1").objClass == "door");

	// The ghosts: Blinky above the door, the others in the house.
	CHECK(middleOf(maze.ghost("blinky")).x == Approx(tile(14, 11).x));
	CHECK(middleOf(maze.ghost("inky")).x == Approx(tile(12, 14).x));
	CHECK(middleOf(maze.ghost("clyde")).x == Approx(tile(16, 14).x));
	CHECK(maze.ghost("clyde").inGroup("playing.ghosts"));
	CHECK(maze.ghost("blinky").lookName() == "normal");

	// A wall piece gives only its rows: its scale and colour are the walls'.
	const Object& corner = maze.game.getObject("playing.maze.topleft.1");
	CHECK(corner.size.x == Approx(kTile));
	CHECK(corner.size.y == Approx(kTile));
}

TEST_CASE("pacman: Pac-Man goes the way pressed, turns at a gap and stops at a wall", "[pacman][maze]")
{
	Maze maze;
	maze.onlyGhost("");

	// Nothing until a key: then left along row 23 at his speed, in the lane.
	maze.frames(5);
	CHECK(middleOf(maze.pacman()).x == Approx(tile(14, 23).x));
	maze.press("left");
	maze.frames(10);
	CHECK(middleOf(maze.pacman()).x == Approx(tile(14, 23).x - 20.0f));
	CHECK(middleOf(maze.pacman()).y == Approx(tile(14, 23).y));
	CHECK(maze.pacman().facing == Direction::Left);
	CHECK(maze.pacman().heading == Approx(270.0f));

	// Row 23 is open from column 6 to 21: he stops in the middle of column 6.
	maze.frames(60);
	CHECK(middleOf(maze.pacman()).x == Approx(tile(6, 23).x));
	CHECK(maze.pacman().velocity.x == 0.0f);

	// Up is wished before the gap: column 6 is open above, so he goes up there.
	maze.press("right");
	maze.press("up");
	maze.frames(30);
	CHECK(middleOf(maze.pacman()).x == Approx(tile(6, 23).x));
	CHECK(middleOf(maze.pacman()).y < tile(6, 23).y);
	CHECK(maze.pacman().facing == Direction::Up);

	// Straight back at once.
	const float y = middleOf(maze.pacman()).y;
	maze.press("down");
	maze.frames(1);
	CHECK(middleOf(maze.pacman()).y == Approx(y + 2.0f));
}

TEST_CASE("pacman: a press for a way that is not open waits for the first gap", "[pacman][maze]")
{
	Maze maze;
	maze.onlyGhost("");

	// Going right along row 23 with down wished: the first gap below is
	// column 18 (columns 15 to 17 have walls under them), 64 pixels on.
	maze.press("right");
	maze.frames(2);
	maze.press("down");
	maze.frames(24);
	CHECK(middleOf(maze.pacman()).x == Approx(tile(14, 23).x + 52.0f));
	CHECK(middleOf(maze.pacman()).y == Approx(tile(14, 23).y));
	maze.frames(20);
	CHECK(middleOf(maze.pacman()).x == Approx(tile(18, 23).x));
	CHECK(middleOf(maze.pacman()).y > tile(18, 23).y);
	CHECK(maze.pacman().facing == Direction::Down);
}

TEST_CASE("pacman: dots are eaten as he goes", "[pacman]")
{
	Maze maze;
	maze.onlyGhost("");
	maze.press("left");
	maze.frames(70);
	// Columns 12 to 6 of row 23: seven dots (column 13 has none).
	CHECK(maze.count("playing.dots") == 233);
	CHECK(maze.status("score") == 70.0f);
}

TEST_CASE("pacman: the tunnel runs off the map and round", "[pacman][maze]")
{
	Maze maze;
	maze.onlyGhost("");
	Object& pacman = maze.pacman();
	pacman.position = tile(6, 14) - pacman.size * 0.5f;
	maze.press("left");
	maze.frames(70);
	// Off the map he goes straight on, and wraps round to the right side.
	CHECK(middleOf(pacman).y == Approx(tile(6, 14).y));
	CHECK(middleOf(pacman).x > tile(21, 14).x);
	maze.frames(30);
	CHECK(middleOf(pacman).x == Approx(tile(24, 14).x).margin(30.0f));
	CHECK(pacman.facing == Direction::Left);
}

TEST_CASE("pacman: Blinky scatters to his corner, then hunts Pac-Man through the maze", "[pacman][hunt]")
{
	Maze maze;
	maze.onlyGhost("blinky");
	Object& blinky = maze.ghost("blinky");

	// He sets off at once, for the top right corner, and never steps onto a wall.
	maze.frames(1);
	CHECK(blinky.walk.mode == Walk::Mode::Hunt);
	CHECK(blinky.walk.hunt.target == "playing.topright");
	bool chased = false;
	for (int i = 0; i < 1500 && maze.state() == "playing"; ++i)
	{
		maze.frames(1);
		chased = chased || blinky.walk.hunt.target == "playing.pacman";
		const Vector2f middle = middleOf(blinky);
		const int column = static_cast<int>(middle.x / kTile);
		const int row = static_cast<int>((middle.y - kTop) / kTile);
		for (auto& wall : maze.game.getCurrentObjects())
		{
			if (wall.objClass == "wall" && wall.mapColumn == column && wall.mapRow == row)
			{
				FAIL("Blinky is on a wall at column " << column << ", row " << row);
			}
		}
	}

	// After seven seconds he chases, and catches him: a life lost, and READY!
	// over everyone back where they started.
	CHECK(chased);
	CHECK(maze.status("lives") == 2.0f);
	CHECK(maze.state() == "caught");
	CHECK(middleOf(blinky).x == Approx(tile(14, 11).x));
	maze.frames(130);
	CHECK(maze.state() == "playing");
}

TEST_CASE("pacman: the ghosts in the house come out one by one", "[pacman][hunt]")
{
	Maze maze;
	maze.onlyGhost("pinky");
	maze.ghost("inky").isVisible = true;
	Object& pinky = maze.ghost("pinky");
	Object& inky = maze.ghost("inky");
	const float door = tile(13, 12).y;
	maze.frames(30);
	CHECK(middleOf(pinky).y > door);
	maze.frames(40);
	CHECK(pinky.isFollowing());
	for (int i = 0; i < 200 && pinky.isFollowing(); ++i) { maze.frames(1); }
	CHECK_FALSE(pinky.isFollowing());
	CHECK(middleOf(pinky).y == Approx(tile(14, 11).y));
	CHECK(middleOf(inky).y > door);
}

TEST_CASE("pacman: the ghosts scatter and chase in waves, turning round at each change", "[pacman][hunt]")
{
	Maze maze;
	maze.onlyGhost("blinky");
	Object& blinky = maze.ghost("blinky");
	Object& mode = maze.game.getObject("playing.mode");
	maze.pacman().position = { 2000.0f, 2000.0f };
	maze.frames(2);
	REQUIRE(mode.lookName() == "scatter");
	CHECK(blinky.walk.hunt.target == "playing.topright");

	// Seven seconds of scatter, twenty of chase, and he turns round at each.
	maze.frames(7 * 60 - 4);
	const Vector2f before = blinky.velocity;
	maze.frames(4);
	CHECK(mode.lookName() == "chase");
	CHECK(blinky.walk.hunt.target == "playing.pacman");
	CHECK(blinky.velocity.x * before.x + blinky.velocity.y * before.y <= 0.0f);
	maze.frames(20 * 60);
	CHECK(mode.lookName() == "scatter");
	CHECK(blinky.walk.hunt.target == "playing.topright");

	// The last two scatters are five seconds; after the fourth chase is for good.
	maze.frames(7 * 60 + 20 * 60);
	CHECK(mode.lookName() == "scatter");
	maze.frames(5 * 60 + 2);
	CHECK(mode.lookName() == "chase");
	maze.frames(20 * 60 + 5 * 60 + 2);
	CHECK(mode.lookName() == "chase");
	maze.frames(60 * 60);
	CHECK(mode.lookName() == "chase");
	CHECK(blinky.walk.hunt.target == "playing.pacman");
}

TEST_CASE("pacman: a blue ghost keeps wandering through a change of mode, and takes up the mode after", "[pacman][hunt]")
{
	Maze maze;
	maze.onlyGhost("blinky");
	Object& blinky = maze.ghost("blinky");
	maze.pacman().position = { 2000.0f, 2000.0f };
	maze.game.setVariable("playing.status", "fright", 30.0f);
	maze.frames(2);
	maze.game.become(blinky.name, "blue");
	maze.frames(7 * 60);
	CHECK(maze.game.getObject("playing.mode").lookName() == "chase");
	CHECK(blinky.lookName() == "blue");
	CHECK(blinky.walk.mode == Walk::Mode::Wander);
	maze.game.become(blinky.name, "normal");
	maze.frames(2);
	CHECK(blinky.walk.mode == Walk::Mode::Hunt);
	CHECK(blinky.walk.hunt.target == "playing.pacman");
}

TEST_CASE("pacman: Inky comes out after thirty dots and Clyde after sixty more; four idle seconds count as thirty", "[pacman][hunt]")
{
	Maze maze;
	maze.onlyGhost("pinky");
	Object& inky = maze.ghost("inky");
	Object& clyde = maze.ghost("clyde");
	inky.isVisible = true;
	clyde.isVisible = true;
	maze.pacman().position = { 2000.0f, 2000.0f };
	maze.frames(2);
	CHECK_FALSE(inky.isFollowing());

	maze.game.setVariable("playing.status", "eaten", 29.0f);
	maze.frames(1);
	CHECK_FALSE(inky.isFollowing());
	maze.game.setVariable("playing.status", "eaten", 30.0f);
	maze.frames(1);
	CHECK(inky.isFollowing());
	CHECK_FALSE(clyde.isFollowing());
	maze.game.setVariable("playing.status", "eaten", 89.0f);
	maze.frames(1);
	CHECK_FALSE(clyde.isFollowing());
	maze.game.setVariable("playing.status", "eaten", 90.0f);
	maze.frames(1);
	CHECK(clyde.isFollowing());
	CHECK(maze.status("released") == 3.0f);

	// With no dot eaten for four seconds, thirty are counted.
	Maze idle;
	idle.pacman().position = { 2000.0f, 2000.0f };
	idle.onlyGhost("inky");
	idle.frames(4 * 60 - 10);
	CHECK(idle.status("eaten") == 0.0f);
	CHECK_FALSE(idle.ghost("inky").isFollowing());
	idle.frames(12);
	CHECK(idle.status("eaten") == 30.0f);
	CHECK(idle.ghost("inky").isFollowing());
}

TEST_CASE("pacman: a ghost goes at half speed in the tunnel", "[pacman][hunt]")
{
	Maze maze;
	maze.onlyGhost("blinky");
	Object& blinky = maze.ghost("blinky");
	maze.pacman().position = { 2000.0f, 2000.0f };
	maze.frames(2);
	const float pace = maze.status("pace");
	CHECK(std::abs(blinky.velocity.x) + std::abs(blinky.velocity.y) == Approx(pace));

	// Set in the middle of the left tunnel, going left.
	blinky.position = tile(3, 14) - blinky.size * 0.5f;
	blinky.velocity = { -pace, 0.0f };
	blinky.walk.going = Direction::Left;
	maze.frames(2);
	CHECK(blinky.velocity.x == Approx(-pace * 0.5f));
	CHECK(blinky.velocity.y == Approx(0.0f));
}

TEST_CASE("pacman: a pellet turns the ghosts blue, and a blue ghost is eaten", "[pacman][looks]")
{
	Maze maze;
	maze.onlyGhost("blinky");
	Object& blinky = maze.ghost("blinky");

	maze.frames(10);
	CHECK(blinky.walk.mode == Walk::Mode::Hunt);
	Object& pellet = maze.game.getObject("playing.maze.pellets.3");
	maze.touch(pellet);
	CHECK_FALSE(pellet.isVisible);
	CHECK(blinky.lookName() == "blue");
	maze.frames(1);
	CHECK(blinky.lookName() == "blue");
	CHECK(blinky.walk.mode == Walk::Mode::Wander);
	CHECK(blinky.walk.speed == Approx(1.0f));

	// The eyes of one not blue stay eyes (<become from>), and a second pellet
	// starts the blue over (the timer follows the look).
	Object& clyde = maze.ghost("clyde");
	maze.frames(300);
	CHECK(blinky.lookName() == "blue");
	showLookNamed(clyde, "eyes");
	maze.touch(maze.game.getObject("playing.maze.pellets.4"));
	CHECK(clyde.lookName() == "eyes");
	maze.frames(200);
	CHECK(blinky.lookName() == "blue");
	maze.frames(200);
	CHECK(blinky.lookName() == "normal");
	maze.frames(1);
	CHECK(blinky.walk.mode == Walk::Mode::Hunt);

	// Blue and touched: two hundred points, and its eyes head home through
	// the door, where it is a ghost again and comes back out.
	maze.executor.executeInput(Command{ CmdBecome{ "blue", "playing.ghosts", "normal", {} } }, true);
	maze.frames(1);
	const float before = maze.status("score");
	const int dots = maze.count("playing.dots");
	const int pellets = maze.count("playing.pellets");
	maze.touch(blinky);
	// Where the ghost wandered to, Pac-Man may eat a dot or a pellet on the
	// way: the ghost is 200 on top of what those are worth.
	const float eaten = 10.0f * float(dots - maze.count("playing.dots")) + 50.0f * float(pellets - maze.count("playing.pellets"));
	CHECK(maze.status("score") - before == 200.0f + eaten);
	CHECK(maze.status("worth") == 400.0f);
	CHECK(blinky.lookName() == "eyes");
	CHECK(maze.status("lives") == 3.0f);
	bool home = false;
	for (int i = 0; i < 900 && !home; ++i)
	{
		maze.frames(1);
		home = blinky.lookName() == "normal";
	}
	CHECK(home);
	CHECK(blinky.isFollowing());
	maze.frames(50);
	CHECK_FALSE(blinky.isFollowing());
	CHECK(middleOf(blinky).y == Approx(tile(14, 11).y));
	CHECK(blinky.walk.mode == Walk::Mode::Hunt);
}

TEST_CASE("pacman: each ghost eaten on one pellet is worth twice the one before", "[pacman][looks]")
{
	Maze maze;
	maze.touch(maze.game.getObject("playing.maze.pellets.1"));
	maze.frames(1);
	const float before = maze.status("score");
	maze.touch(maze.ghost("pinky"));
	maze.touch(maze.ghost("inky"));
	maze.touch(maze.ghost("clyde"));
	CHECK(maze.status("score") - before == 200.0f + 400.0f + 800.0f);
	CHECK(maze.status("worth") == 1600.0f);

	// The next pellet starts again at 200.
	maze.touch(maze.game.getObject("playing.maze.pellets.2"));
	CHECK(maze.status("worth") == 200.0f);
}

TEST_CASE("pacman: the cherry shows after 70 dots, for a while, and scores 100", "[pacman][looks]")
{
	Maze maze;
	maze.onlyGhost("");
	Object& fruit = maze.game.getObject("playing.fruit");
	CHECK(fruit.lookName() == "none");

	int eaten = 0;
	for (auto& object : maze.game.getCurrentObjects())
	{
		if (object.inGroup("playing.dots") && eaten < 70)
		{
			object.isVisible = false;
			++eaten;
		}
	}
	maze.frames(1);
	CHECK(fruit.lookName() == "cherry");
	CHECK(maze.status("fruits") == 1.0f);
	maze.frames(5);
	CHECK(fruit.lookName() == "cherry"); // shown once, not over and over

	const float before = maze.status("score");
	maze.touch(fruit);
	CHECK(maze.status("score") - before == 100.0f);
	CHECK(fruit.lookName() == "none");

	// After 170, again, gone by itself in nine and a half seconds.
	for (auto& object : maze.game.getCurrentObjects())
	{
		if (object.inGroup("playing.dots") && object.isVisible && eaten < 170)
		{
			object.isVisible = false;
			++eaten;
		}
	}
	maze.frames(1);
	CHECK(fruit.lookName() == "cherry");
	maze.frames(580);
	CHECK(fruit.lookName() == "none");
}

TEST_CASE("pacman: caught three times is game over; a cleared maze is the next round", "[pacman]")
{
	SECTION("caught")
	{
		Maze maze;
		maze.onlyGhost("blinky");
		for (int i = 0; i < 3; ++i)
		{
			REQUIRE(maze.state() == "playing");
			maze.touch(maze.ghost("blinky"));
			maze.frames(130);
		}
		CHECK(maze.status("lives") == 0.0f);
		CHECK(maze.state() == "gameover");
	}
	SECTION("cleared")
	{
		Maze maze;
		maze.press("left");
		maze.frames(20);
		for (auto& object : maze.game.getCurrentObjects())
		{
			if (object.objClass == "food") { object.isVisible = false; }
		}
		maze.frames(1);
		CHECK(maze.state() == "caught");
		CHECK(maze.count("playing.food") == 244);
		CHECK(middleOf(maze.pacman()).x == Approx(tile(14, 23).x));
		CHECK(maze.status("pace") == Approx(1.9375f));
		CHECK(maze.status("fright") == 5.0f);
		CHECK(maze.status("lives") == 3.0f);
	}
}

TEST_CASE("pacman: a life is won at 10,000 points, once, and the best score is kept", "[pacman]")
{
	Maze maze;
	maze.onlyGhost("");
	maze.game.getObject("playing.status").variable["score"] = 10000.0f;
	maze.frames(1);
	CHECK(maze.status("lives") == 4.0f);
	maze.frames(5);
	CHECK(maze.status("lives") == 4.0f);
	CHECK(maze.game.getObject("best").variable["score"] == 10000.0f);
	maze.game.resetAll();
	CHECK(maze.game.getObject("best").variable["score"] == 10000.0f);
}

TEST_CASE("map: what a <map> turns away", "[map]")
{
	const std::string walls = "<group name=\"walls\" char=\"#\" class=\"wall\">" + kWallSprite + "</group>";
	const auto map = [](const std::string& rows, const std::string& pieces)
	{
		return "<map name=\"maze\"><tile><width>tile</width><height>tile</height></tile><position><x>0</x><y>0</y></position>" + rows + pieces + "</map>";
	};

	CHECK(loadError(mapGame(map("<row>###</row><row>#.#</row><row>###</row>", walls))).empty());
	CHECK_THAT(loadError(mapGame(map("<row>###</row><row>#.</row>", walls))), ContainsSubstring("every row is as wide as the map"));
	CHECK_THAT(loadError(mapGame(map("<row>###</row>", walls + "<group name=\"more\" char=\"#\">" + kWallSprite + "</group>"))), ContainsSubstring("is picked by"));
	CHECK_THAT(loadError(mapGame(map("<row>#x#</row>", walls + "<group name=\"none\" char=\"y\">" + kDotSprite + "</group>"))), ContainsSubstring("on no tile"));
	CHECK_THAT(loadError(mapGame(map("<row>#xx</row>", walls + "<object name=\"p\" char=\"x\">" + kDotSprite + "</object>"))), ContainsSubstring("which is on 2 tiles"));
	CHECK_THAT(loadError(mapGame(map("<row>#x#</row>", walls + "<object name=\"p\" char=\"x\">" + kDotSprite + "<position><x>0</x><y>0</y></position></object>"))),
		ContainsSubstring("has no <position>"));
	CHECK_THAT(loadError(mapGame(map("<row>###</row>", ""))), ContainsSubstring("has nothing on it"));
	CHECK_THAT(loadError(mapGame("<object name=\"p\" char=\"x\">" + kDotSprite + "<position><x>0</x><y>0</y></position></object>")), ContainsSubstring("on no <map>"));
}

TEST_CASE("maze: what a walker's verbs and a look's timer turn away", "[map][maze]")
{
	const std::string piece = "<object name=\"p\">" + kDotSprite + "<position><x>0</x><y>0</y></position>";
	const std::string map = "<map name=\"maze\"><tile><width>tile</width><height>tile</height></tile><position><x>0</x><y>0</y></position><row>#.#</row>"
		"<group name=\"walls\" char=\"#\" class=\"wall\">" + kWallSprite + "</group></map>";

	CHECK_THAT(loadError(mapGame(map + piece + "<actions><action name=\"l\"><go direction=\"left\">1</go></action></actions></object>")),
		ContainsSubstring("walks none"));
	CHECK_THAT(loadError(mapGame(map + piece + "<maze map=\"mase\"><wall class=\"wall\" /></maze></object>")), ContainsSubstring("no <map> of that name"));
	CHECK(loadError(mapGame(map + piece + "<maze map=\"maze\"><wall class=\"wall\" /></maze><actions><action name=\"l\"><go direction=\"left\">1</go></action></actions></object>")).empty());
	CHECK_THAT(loadError(mapGame(map + piece + "<timers><timer sprite=\"blue\"><after>1</after><die /></timer></timers></object>")), ContainsSubstring("no look of that name"));
	CHECK_THAT(loadError(mapGame(map + "<object name=\"p\">" + kDotSprite + "<position><x>0</x><y>0</y></position><timers><timer><after>1</after><become sprite=\"a\" from=\"b\" /></timer></timers></object>")),
		ContainsSubstring("no look of that name"));
}

TEST_CASE("map: a group on a map has every part an object has, heading to seek", "[map]")
{
	const std::string xml = mapGame("<map name=\"maze\"><tile><width>tile</width><height>tile</height></tile><position><x>0</x><y>0</y></position><row>#oo@</row>"
		"<group name=\"walls\" char=\"#\" class=\"wall\">" + kWallSprite + "</group>"
		"<group name=\"mines\" char=\"o\">" + kDotSprite + "<heading>90</heading><drag>0.5</drag><mass>2</mass><pull object=\"p\">0.25</pull><seek object=\"p\">3</seek></group>"
		"<object name=\"p\" char=\"@\">" + kDotSprite + "</object></map>");
	const std::filesystem::path path = scratchName("descriptions/pacman_scratch.xml");
	{
		std::ofstream out(path);
		out << xml;
	}
	struct Gone
	{
		std::filesystem::path file;
		~Gone() { std::error_code ignored; std::filesystem::remove(file, ignored); }
	} gone{ path };

	Game game(path.string());
	for (const char* name : { "mines.1", "mines.2" })
	{
		const Object& mine = game.getObject(name);
		CHECK(mine.heading == Approx(90));
		CHECK(mine.drag == Approx(0.5));
		CHECK(mine.mass == Approx(2));
		CHECK(mine.seekTarget == "playing.p");
		CHECK(mine.seekRate == Approx(3));
	}
}

TEST_CASE("bitmap: a <color char> draws that character in its own color", "[map][bitmap]")
{
	const std::vector<std::string> rows{ "*w.", "w**" };
	const Bitmap bitmap = rasterizeRows(rows, 1, Color{ 255, 0, 0, 255 }, { { 'w', Color{ 255, 255, 255, 255 } } });
	REQUIRE(bitmap.width == 3);
	CHECK(bitmap.rgba.at(0) == 255); // * red
	CHECK(bitmap.rgba.at(1) == 0);
	CHECK(bitmap.rgba.at(4 + 1) == 255); // w white
	CHECK_FALSE(bitmap.solidAt(2, 0)); // . clear
	CHECK_THROWS(rasterizeRows(rows, 1, Color{ 255, 0, 0, 255 }, {}));
}

TEST_CASE("pacman: both schema checkers take the game", "[pacman][schema]")
{
	auto weak = XmlDocumentFactory::create(XmlBackend::TinyXml2);
	REQUIRE(weak->load("descriptions/pacman.xml"));
	XsdLiteValidator validator;
	REQUIRE(validator.loadSchema(findSchema("descriptions/pong.xml", "../xgedef.xsd").string(), XmlBackend::TinyXml2));
	CHECK(validator.validate(*weak->getRootElement()));

	auto strong = XmlDocumentFactory::create(XmlBackend::Xerces);
	CHECK(strong->load("descriptions/pacman.xml"));
}
