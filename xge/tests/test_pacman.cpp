// test_pacman.cpp
// XML Game Engine
// author: beefviper
// date: Oct 15, 2026
//
// Catch2 tests for games/descriptions/pacman.xml, played frame by frame by a
// real xge::Game with no window, and for what it is the first game with: a
// <map> of characters that places the walls, dots, Pac-Man and the ghosts;
// a bitmap's <color char>; objects that walk the map (<maze>) by <go>,
// <hunt> and <wander>; timers that follow a look; and <become from>.
//
// Window::init() normally measures each object's size once a backend exists;
// measure() gives every shape the size its sprite implies instead.

#include "command_executor.h"
#include "data_folder.h"
#include "game.h"
#include "xml_document.h"
#include "xsd_lite.h"
#include "bitmap.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace xge;
using Catch::Approx;
using Catch::Matchers::ContainsSubstring;

namespace
{
	constexpr float kTile = 24.0f;
	constexpr float kTop = 44.0f;

	void measure(Game& game)
	{
		for (auto& object : game.getCurrentObjects())
		{
			object.size = measureShapeSize(object.spriteParams, object.shapeKind);
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
		Object& ghost(const std::string& name) { return game.getObject("playing." + name + ".1"); }
		std::string state() { return game.getCurrentState().name; }

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { game.updateObjects(); }
		}

		// Pac-Man, out of the maze's hands, runs into `other` from its left:
		// touching happens on the move.
		void touch(const Object& other)
		{
			Object& p = pacman();
			p.walk = {};
			p.position = middleOf(other) - p.size * 0.5f - Vector2f{ (p.size.x + other.size.x) * 0.5f + 1.0f, 0.0f };
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
		const std::filesystem::path path = "descriptions/pacman_scratch.xml";
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

	// Pac-Man's '@' is on column 8 of row 16, and he sits in the middle of it.
	CHECK(middleOf(maze.pacman()).x == Approx(tile(8, 16).x));
	CHECK(middleOf(maze.pacman()).y == Approx(tile(8, 16).y));
	CHECK(maze.pacman().inGroup("playing.maze"));

	// A member on each tile of its character, numbered in reading order.
	CHECK(maze.count("playing.dots") == 150);
	CHECK(maze.count("playing.pellets") == 4);
	CHECK(maze.count("playing.walls") == 192);
	CHECK(middleOf(maze.game.getObject("playing.pellets.1")).x == Approx(tile(1, 2).x));
	CHECK(middleOf(maze.game.getObject("playing.pellets.1")).y == Approx(tile(1, 2).y));
	CHECK(middleOf(maze.game.getObject("playing.door.1")).x == Approx(tile(9, 8).x));

	// The ghosts: Blinky above the door, the others in the house.
	CHECK(middleOf(maze.ghost("blinky")).x == Approx(tile(9, 7).x));
	CHECK(middleOf(maze.ghost("inky")).x == Approx(tile(8, 9).x));
	CHECK(middleOf(maze.ghost("clyde")).x == Approx(tile(10, 9).x));
	CHECK(maze.ghost("clyde").inGroup("playing.ghosts"));

	// A ghost's eyes are white on its own color (<color char>).
	CHECK(maze.ghost("blinky").lookName() == "normal");
}

TEST_CASE("pacman: Pac-Man goes the way pressed, turns at a gap and stops at a wall", "[pacman][maze]")
{
	Maze maze;
	maze.onlyGhost("");

	// Nothing until a key: then left along row 16 at his speed, in the lane.
	maze.frames(5);
	CHECK(middleOf(maze.pacman()).x == Approx(tile(8, 16).x));
	maze.press("left");
	maze.frames(10);
	CHECK(middleOf(maze.pacman()).x == Approx(tile(8, 16).x - 25.0f));
	CHECK(middleOf(maze.pacman()).y == Approx(tile(8, 16).y));
	CHECK(maze.pacman().facing == Direction::Left);
	CHECK(maze.pacman().heading == Approx(270.0f));

	// Row 16 is open from column 4 to 14: he stops in the middle of column 4.
	maze.frames(60);
	CHECK(middleOf(maze.pacman()).x == Approx(tile(4, 16).x));
	CHECK(maze.pacman().velocity.x == 0.0f);

	// Up is wished before the gap: column 4 is open above, so he goes up there.
	maze.press("right");
	maze.press("up");
	maze.frames(30);
	CHECK(middleOf(maze.pacman()).x == Approx(tile(4, 16).x));
	CHECK(middleOf(maze.pacman()).y < tile(4, 16).y);
	CHECK(maze.pacman().facing == Direction::Up);

	// Straight back at once.
	const float y = middleOf(maze.pacman()).y;
	maze.press("down");
	maze.frames(1);
	CHECK(middleOf(maze.pacman()).y == Approx(y + 2.5f));
}

TEST_CASE("pacman: a press for a way that is not open waits for the first gap", "[pacman][maze]")
{
	Maze maze;
	maze.onlyGhost("");

	// Going right along row 16 with down wished: the first gap below is
	// column 12 (columns 9 to 11 have walls under them), 96 pixels on.
	maze.press("right");
	maze.frames(2);
	maze.press("down");
	maze.frames(30);
	CHECK(middleOf(maze.pacman()).x == Approx(tile(8, 16).x + 80.0f));
	CHECK(middleOf(maze.pacman()).y == Approx(tile(8, 16).y));
	maze.frames(20);
	CHECK(middleOf(maze.pacman()).x == Approx(tile(12, 16).x));
	CHECK(middleOf(maze.pacman()).y > tile(12, 16).y);
	CHECK(maze.pacman().facing == Direction::Down);
}

TEST_CASE("pacman: dots and pellets are eaten", "[pacman]")
{
	Maze maze;
	maze.onlyGhost("");
	maze.press("left");
	maze.frames(60);
	CHECK(maze.count("playing.dots") == 146);
	CHECK(maze.status("score") == 40.0f);
}

TEST_CASE("pacman: the tunnel runs off the map and round", "[pacman][maze]")
{
	Maze maze;
	maze.onlyGhost("");
	Object& pacman = maze.pacman();
	pacman.position = tile(4, 9) - pacman.size * 0.5f;
	maze.press("left");
	maze.frames(60);
	// Off the map he goes straight on, and wraps round to the right side.
	CHECK(middleOf(pacman).y == Approx(tile(4, 9).y));
	CHECK(middleOf(pacman).x > tile(14, 9).x);
	maze.frames(30);
	CHECK(middleOf(pacman).x == Approx(tile(14, 9).x).margin(30.0f));
	CHECK(pacman.facing == Direction::Left);
}

TEST_CASE("pacman: Blinky hunts Pac-Man through the maze", "[pacman][hunt]")
{
	Maze maze;
	maze.onlyGhost("blinky");
	Object& blinky = maze.ghost("blinky");

	// He sets off at once, along row 7, and never steps onto a wall.
	maze.frames(1);
	CHECK((blinky.velocity.x != 0.0f || blinky.velocity.y != 0.0f));
	for (int i = 0; i < 600 && maze.state() == "playing"; ++i)
	{
		maze.frames(1);
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

	// And he catches him: a life lost, everyone back where they started.
	CHECK(maze.status("lives") < 3.0f);
}

TEST_CASE("pacman: the ghosts in the house come out one by one", "[pacman][hunt]")
{
	Maze maze;
	maze.onlyGhost("pinky");
	maze.ghost("inky").isVisible = true;
	Object& pinky = maze.ghost("pinky");
	Object& inky = maze.ghost("inky");
	maze.frames(30);
	CHECK(middleOf(pinky).y > tile(9, 8).y);
	maze.frames(60);
	CHECK(pinky.isFollowing());
	maze.frames(40);
	CHECK(middleOf(pinky).y < tile(9, 8).y);
	CHECK_FALSE(pinky.isFollowing());
	CHECK(middleOf(inky).y > tile(9, 8).y);
}

TEST_CASE("pacman: a pellet turns the ghosts blue, and a blue ghost is eaten", "[pacman][looks]")
{
	Maze maze;
	maze.onlyGhost("blinky");
	Object& blinky = maze.ghost("blinky");

	maze.frames(10);
	CHECK(blinky.walk.mode == Walk::Mode::Hunt);
	Object& pellet = maze.game.getObject("playing.pellets.3");
	maze.touch(pellet);
	CHECK_FALSE(pellet.isVisible);
	CHECK(blinky.lookName() == "blue");
	maze.frames(1);
	CHECK(blinky.walk.mode == Walk::Mode::Wander);
	CHECK(blinky.walk.speed == Approx(1.25f));

	// The eyes of one not blue stay eyes (<become from>), and a second pellet
	// starts the blue over (the timer follows the look).
	Object& clyde = maze.ghost("clyde");
	maze.frames(300);
	CHECK(blinky.lookName() == "blue");
	showLookNamed(clyde, "eyes");
	maze.touch(maze.game.getObject("playing.pellets.4"));
	CHECK(clyde.lookName() == "eyes");
	maze.frames(200);
	CHECK(blinky.lookName() == "blue");
	maze.frames(200);
	CHECK(blinky.lookName() == "normal");
	maze.frames(1);
	CHECK(blinky.walk.mode == Walk::Mode::Hunt);

	// Blue and touched: two hundred points, and its eyes head home through
	// the door, where it is a ghost again and comes back out.
	maze.executor.executeInput(Command{ CmdBecome{ "blue", "playing.ghosts", "normal" } }, true);
	maze.frames(1);
	const float before = maze.status("score");
	maze.touch(blinky);
	// (and maybe a dot on the way in)
	CHECK(maze.status("score") - before >= 200.0f);
	CHECK(maze.status("score") - before <= 210.0f);
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
	CHECK(middleOf(blinky).y == Approx(tile(9, 7).y));
	CHECK(blinky.walk.mode == Walk::Mode::Hunt);
}

TEST_CASE("pacman: caught three times is game over; every dot eaten wins", "[pacman]")
{
	SECTION("caught")
	{
		Maze maze;
		maze.onlyGhost("blinky");
		for (int i = 0; i < 3; ++i)
		{
			maze.touch(maze.ghost("blinky"));
		}
		CHECK(maze.status("lives") == 0.0f);
		maze.frames(1);
		CHECK(maze.state() == "gameover");
	}
	SECTION("won")
	{
		Maze maze;
		for (auto& object : maze.game.getCurrentObjects())
		{
			if (object.objClass == "food") { object.isVisible = false; }
		}
		maze.frames(1);
		CHECK(maze.state() == "youwin");
	}
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
		ContainsSubstring("needs an object="));
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
	REQUIRE(validator.loadSchema(findSchema("descriptions/pong.xml", "../../xge/xgedef.xsd").string(), XmlBackend::TinyXml2));
	CHECK(validator.validate(*weak->getRootElement()));

	auto strong = XmlDocumentFactory::create(XmlBackend::Xerces);
	CHECK(strong->load("descriptions/pacman.xml"));
}
