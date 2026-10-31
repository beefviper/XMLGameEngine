// test_cells.cpp
// XML Game Engine
// author: beefviper
// date: Oct 30, 2026
//
// Catch2 tests for maps with <cells />: pieces that move a cell at a time,
// a move refused as a touch, shapes and their turns, <settle />, <slide>,
// and the map's checks. Tetris (games/descriptions/tetris.xml) is played,
// and small wells written out here test one thing each.

#include "played.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace xge;
using played::Played;

namespace
{
	// The falling shape of a game: the one in play that has cells, on its
	// own map (not waiting in a next-piece box).
	Object* falling(Game& game)
	{
		for (Object& object : game.getCurrentObjects())
		{
			if (object.exists && object.isShape() && object.isCopy() && !object.awayFromHome()) { return &object; }
		}
		return nullptr;
	}

	// Where a shape's cells are, as (column, row).
	std::set<std::pair<int, int>> cellsOf(Game& game, const Object& shape)
	{
		std::set<std::pair<int, int>> cells;
		for (const std::string& part : shape.shapeParts)
		{
			const Object& cell = game.getObject(part);
			cells.insert({ static_cast<int>(cell.cellColumn), static_cast<int>(cell.cellRow) });
		}
		return cells;
	}

	// The loose pieces (in no shape), as (column, row).
	std::set<std::pair<int, int>> loose(Game& game)
	{
		std::set<std::pair<int, int>> cells;
		for (const Object& object : game.getCurrentObjects())
		{
			if (object.exists && object.onCells && object.isCopy() && !object.isShape() && object.shapeOwner.empty())
			{
				cells.insert({ static_cast<int>(object.cellColumn), static_cast<int>(object.cellRow) });
			}
		}
		return cells;
	}

	// A well `columns` wide and `rows` deep, of one kind of block (b) and
	// the shapes given (each a <group> with a <shape>), whose pieces fall
	// a cell every `every` seconds and settle on a block or the floor; a
	// full row is cleared and what is above comes down.
	std::string well(int columns, int rows, const std::string& shapes, const std::string& every = "100")
	{
		return "<object name=\"tally\"><variables><variable name=\"rows\">0</variable></variables></object>"
			"<map name=\"well\"><tile><width>10</width><height>10</height></tile><position><x>100</x><y>100</y></position>"
			"<columns>" + std::to_string(columns) + "</columns><rows>" + std::to_string(rows) + "</rows><cells />"
			"<object name=\"block\" class=\"block\" char=\"b\"><sprite><rectangle><width>8</width><height>8</height></rectangle></sprite></object>"
			"<group name=\"pieces\" class=\"falling\">"
			"<collisions><collision class=\"block\" side=\"bottom\"><settle /></collision><collision edge=\"bottom\"><settle /></collision></collisions>"
			"<actions><action name=\"left\"><hop direction=\"left\" /></action><action name=\"right\"><hop direction=\"right\" /></action>"
			"<action name=\"turn\"><turn direction=\"right\" /></action><action name=\"back\"><turn direction=\"left\" /></action>"
			"<action name=\"drop\"><slide direction=\"down\" /></action></actions>"
			"<timers><timer><every>" + every + "</every><hop direction=\"down\" /></timer></timers>"
			+ shapes + "</group>"
			"<checks><full class=\"block\"><clear /><collapse /><inc variable=\"tally.rows\" /></full></checks></map>";
	}

	const std::string bar = "<group name=\"bar\"><shape><row>bb</row></shape></group>";
	const std::string tee = "<group name=\"tee\"><shape><row>.b.</row><row>bbb</row><row>...</row></shape></group>";

	// What Space does: spawn the shape in that cell; and the keys for the
	// shape's actions, as inputs after Space's (the last one closed by
	// Scratch, which closes Space's).
	std::string spawnAt(const std::string& shape, int column, int row)
	{
		return "<spawn object=\"" + shape + "\"><column>" + std::to_string(column) + "</column><row>" + std::to_string(row) + "</row></spawn></input>"
			"<input button=\"a\"><trigger object=\"pieces\" action=\"left\" /></input>"
			"<input button=\"d\"><trigger object=\"pieces\" action=\"right\" /></input>"
			"<input button=\"w\"><trigger object=\"pieces\" action=\"turn\" /></input>"
			"<input button=\"q\"><trigger object=\"pieces\" action=\"back\" /></input>"
			"<input button=\"s\"><trigger object=\"pieces\" action=\"drop\" />";
	}
}

TEST_CASE("A map with columns and rows and <cells /> loads its pieces waiting to be spawned", "[cells]")
{
	played::Scratch file(well(4, 6, bar), spawnAt("bar", 1, 1));
	Played play(file.path);

	const Object& block = play.get("block");
	CHECK_FALSE(block.exists);
	CHECK(block.onCells);
	CHECK(block.mapColumns == 4);
	CHECK(block.mapRows == 6);

	const Object& shape = play.get("well.bar");
	CHECK(shape.isShape());
	CHECK(shape.shapeCells.size() == 2);
	CHECK_FALSE(shape.exists);
}

TEST_CASE("A shape spawned on a cell map brings its cells, and falls a cell at a time", "[cells]")
{
	played::Scratch file(well(4, 6, bar, "0.5"), spawnAt("bar", 2, 1));
	Played play(file.path);
	play.tap(KeyCode::Space);
	play.frames(1);

	Object* shape = falling(play.game);
	REQUIRE(shape);
	CHECK(cellsOf(play.game, *shape) == std::set<std::pair<int, int>>{ { 2, 1 }, { 3, 1 } });
	// The block in column 2, row 1 is drawn in the middle of its tile.
	const Object& cell = play.get(shape->shapeParts.front());
	CHECK(cell.position.x == 100.0f + 10.0f + 1.0f);
	CHECK(cell.position.y == 100.0f + 1.0f);

	play.frames(31);
	shape = falling(play.game);
	REQUIRE(shape);
	CHECK(cellsOf(play.game, *shape) == std::set<std::pair<int, int>>{ { 2, 2 }, { 3, 2 } });
}

TEST_CASE("A cell move off the map is refused, and it stays", "[cells]")
{
	played::Scratch file(well(4, 6, bar), spawnAt("bar", 1, 1));
	Played play(file.path);
	play.tap(KeyCode::Space);
	play.frames(1);
	Object* shape = falling(play.game);
	REQUIRE(shape);

	play.tap(KeyCode::A);
	play.frames(1);
	CHECK(cellsOf(play.game, *falling(play.game)) == std::set<std::pair<int, int>>{ { 1, 1 }, { 2, 1 } });

	play.tap(KeyCode::D);
	play.frames(1);
	play.tap(KeyCode::D);
	play.frames(1);
	play.tap(KeyCode::D);
	play.frames(1);
	CHECK(cellsOf(play.game, *falling(play.game)) == std::set<std::pair<int, int>>{ { 3, 1 }, { 4, 1 } });
}

TEST_CASE("A slide goes down to the floor, where the refused step settles it", "[cells]")
{
	played::Scratch file(well(4, 6, bar), spawnAt("bar", 1, 1));
	Played play(file.path);
	play.tap(KeyCode::Space);
	play.frames(1);
	play.tap(KeyCode::S);
	play.frames(1);

	CHECK_FALSE(falling(play.game));
	CHECK(loose(play.game) == std::set<std::pair<int, int>>{ { 1, 6 }, { 2, 6 } });
}

TEST_CASE("A refused fall onto a block settles the shape on it", "[cells]")
{
	played::Scratch file(well(4, 6, bar), spawnAt("bar", 1, 1));
	Played play(file.path);
	play.tap(KeyCode::Space);
	play.frames(1);
	play.tap(KeyCode::S);
	play.frames(1);
	play.tap(KeyCode::Space);
	play.frames(1);
	play.tap(KeyCode::D);
	play.frames(1);
	play.tap(KeyCode::S);
	play.frames(1);

	CHECK(loose(play.game) == std::set<std::pair<int, int>>{ { 1, 6 }, { 2, 6 }, { 2, 5 }, { 3, 5 } });
}

TEST_CASE("A full row is cleared and what is above comes down", "[cells]")
{
	played::Scratch file(well(4, 6, bar), spawnAt("bar", 1, 1));
	Played play(file.path);

	// Two bars side by side fill the bottom row.
	play.tap(KeyCode::Space);
	play.frames(1);
	play.tap(KeyCode::S);
	play.frames(1);
	CHECK(loose(play.game) == std::set<std::pair<int, int>>{ { 1, 6 }, { 2, 6 } });
	play.tap(KeyCode::Space);
	play.frames(1);
	play.tap(KeyCode::D);
	play.frames(1);
	play.tap(KeyCode::D);
	play.frames(1);
	play.tap(KeyCode::S);
	play.frames(1);
	// The second bar lands in columns 3 and 4: full, cleared.
	play.frames(1);
	CHECK(loose(play.game).empty());
	CHECK(play.get("tally").variable.at("rows") == 1.0f);
}

TEST_CASE("A full row with something above it brings it down a row", "[cells]")
{
	played::Scratch file(well(4, 6, bar), spawnAt("bar", 1, 1));
	Played play(file.path);

	const auto drop = [&](int rightBy)
	{
		play.tap(KeyCode::Space);
		play.frames(1);
		for (int i = 0; i < rightBy; ++i)
		{
			play.tap(KeyCode::D);
			play.frames(1);
		}
		play.tap(KeyCode::S);
		play.frames(1);
	};
	drop(0);
	drop(0);
	CHECK(loose(play.game) == std::set<std::pair<int, int>>{ { 1, 6 }, { 2, 6 }, { 1, 5 }, { 2, 5 } });
	drop(2);
	play.frames(1);
	CHECK(loose(play.game) == std::set<std::pair<int, int>>{ { 1, 6 }, { 2, 6 } });
	CHECK(play.get("tally").variable.at("rows") == 1.0f);
}

TEST_CASE("A shape turns a quarter about the middle of its square", "[cells]")
{
	played::Scratch file(well(6, 8, tee), spawnAt("tee", 2, 2));
	Played play(file.path);
	play.tap(KeyCode::Space);
	play.frames(1);
	Object* shape = falling(play.game);
	REQUIRE(shape);
	CHECK(cellsOf(play.game, *shape) == std::set<std::pair<int, int>>{ { 3, 2 }, { 2, 3 }, { 3, 3 }, { 4, 3 } });

	// Clockwise: the bar stands up in the middle column, the nub to the right.
	play.tap(KeyCode::W);
	play.frames(1);
	CHECK(cellsOf(play.game, *falling(play.game)) == std::set<std::pair<int, int>>{ { 3, 2 }, { 3, 3 }, { 3, 4 }, { 4, 3 } });
	CHECK(falling(play.game)->quarter == 1);

	// And back.
	play.tap(KeyCode::Q);
	play.frames(1);
	CHECK(cellsOf(play.game, *falling(play.game)) == std::set<std::pair<int, int>>{ { 3, 2 }, { 2, 3 }, { 3, 3 }, { 4, 3 } });
	CHECK(falling(play.game)->quarter == 0);
}

TEST_CASE("A turn against a wall kicks the shape away from it", "[cells]")
{
	played::Scratch file(well(6, 8, tee), spawnAt("tee", 2, 2));
	Played play(file.path);
	play.tap(KeyCode::Space);
	play.frames(1);

	// Stood up with the nub to the right, then against the left wall:
	// the T's square sticks out past it, so turning back must kick.
	play.tap(KeyCode::W);
	play.frames(1);
	play.tap(KeyCode::A);
	play.frames(1);
	play.tap(KeyCode::A);
	play.frames(1);
	CHECK(cellsOf(play.game, *falling(play.game)) == std::set<std::pair<int, int>>{ { 1, 2 }, { 1, 3 }, { 1, 4 }, { 2, 3 } });

	// Clockwise again from there (R to 2): in place it fits; to the left
	// (counter, R to 0) the flat T needs column 0, so it is kicked right.
	play.tap(KeyCode::Q);
	play.frames(1);
	CHECK(cellsOf(play.game, *falling(play.game)) == std::set<std::pair<int, int>>{ { 2, 2 }, { 1, 3 }, { 2, 3 }, { 3, 3 } });
}

TEST_CASE("Tetris plays: a shape comes in, falls, lands and the next comes", "[cells][tetris]")
{
	Played play("descriptions/tetris.xml");
	CHECK(play.state() == "title");
	play.tap(KeyCode::Space);
	play.frames(2);
	CHECK(play.state() == "playing");

	Object* shape = falling(play.game);
	REQUIRE(shape);
	CHECK(shape->shapeParts.size() == 4);
	const std::string first = shape->name;

	// Space drops it at once: it lands on the floor, the next comes in.
	play.tap(KeyCode::Space);
	play.frames(2);
	Object* next = falling(play.game);
	REQUIRE(next);
	CHECK(loose(play.game).size() == 4);
	for (const auto& [column, row] : loose(play.game)) { CHECK(row >= 19); }
	CHECK(play.get("playing.status").variable.at("over") == 0.0f);
}

TEST_CASE("Tetris shows the next shape in its box, still, and sends it in when one lands", "[cells][tetris]")
{
	Played play("descriptions/tetris.xml");
	play.tap(KeyCode::Space);
	play.frames(2);

	// The next shape: on the box's map, away from the well it was written on.
	const auto waiting = [&]() -> Object*
	{
		for (Object& object : play.game.getCurrentObjects())
		{
			if (object.exists && object.isShape() && object.isCopy() && object.awayFromHome()) { return &object; }
		}
		return nullptr;
	};
	Object* next = waiting();
	REQUIRE(next);
	CHECK(next->mapName == "playing.next");
	CHECK(next->shapeParts.size() == 4);
	const std::string nextName = next->name;
	const auto shown = cellsOf(play.game, *next);

	// Two seconds: the well's shape falls and answers the keys; the box's
	// does neither.
	const auto before = cellsOf(play.game, *falling(play.game));
	play.tap(KeyCode::D);
	play.frames(120);
	CHECK(cellsOf(play.game, *falling(play.game)) != before);
	REQUIRE(waiting());
	CHECK(waiting()->name == nextName);
	CHECK(cellsOf(play.game, *waiting()) == shown);

	// Dropped: the shape that was next comes into the well at column 4,
	// and another takes its place in the box.
	play.tap(KeyCode::Space);
	play.frames(2);
	REQUIRE(falling(play.game));
	CHECK(falling(play.game)->name == nextName);
	CHECK(falling(play.game)->mapName == "playing.board");
	CHECK(static_cast<int>(falling(play.game)->cellColumn) == 4);
	REQUIRE(waiting());
	CHECK(waiting()->name != nextName);
}

TEST_CASE("Tetris is over when the next shape has no room to come in", "[cells][tetris]")
{
	Played play("descriptions/tetris.xml");
	play.tap(KeyCode::Space);
	play.frames(2);

	// Dropped straight down each time, the shapes pile up in the middle
	// until one cannot come in.
	for (int i = 0; i < 40 && play.state() == "playing"; ++i)
	{
		play.tap(KeyCode::Space);
		play.frames(2);
	}
	CHECK(play.state() == "gameover");

	// Space plays again, with an empty well.
	play.tap(KeyCode::Space);
	play.frames(2);
	CHECK(play.state() == "title");
	CHECK(loose(play.game).empty());
}

namespace
{
	// A map of gems drawn by its rows (r red, g green, b blue, each a
	// group of its own), with a <line> check of three alike that clears
	// them, lets the rest fall and counts each round; its shapes fall a
	// cell every 100 seconds, and S drops one.
	std::string gems(const std::vector<std::string>& rows, const std::string& shapes)
	{
		std::string map = "<object name=\"tally\"><variables><variable name=\"lines\">0</variable></variables></object>"
			"<map name=\"field\"><tile><width>10</width><height>10</height></tile><position><x>100</x><y>100</y></position><cells />";
		for (const std::string& row : rows) { map += "<row>" + row + "</row>"; }
		const auto gem = [](const std::string& name, const std::string& character, const std::string& color)
		{
			return "<group name=\"" + name + "\" class=\"gem\" char=\"" + character + "\"><sprite><circle><radius>4</radius><color>color." + color + "</color></circle></sprite></group>";
		};
		return map + gem("red", "r", "red") + gem("green", "g", "green") + gem("blue", "b", "blue")
			+ "<group name=\"pieces\" class=\"falling\">"
			"<collisions><collision class=\"gem\" side=\"bottom\"><settle /></collision><collision edge=\"bottom\"><settle /></collision></collisions>"
			"<actions><action name=\"drop\"><slide direction=\"down\" /></action><action name=\"cycle\"><cycle /></action></actions>"
			"<timers><timer><every>100</every><hop direction=\"down\" /></timer></timers>"
			+ shapes + "</group>"
			"<checks><line class=\"gem\"><atleast>3</atleast><clear /><fall /><inc variable=\"tally.lines\" /></line></checks></map>";
	}

	// What Space does: that shape in that cell; S drops it, W cycles it.
	std::string dropAt(const std::string& shape, int column)
	{
		return "<spawn object=\"" + shape + "\"><column>" + std::to_string(column) + "</column><row>1</row></spawn></input>"
			"<input button=\"w\"><trigger object=\"pieces\" action=\"cycle\" /></input>"
			"<input button=\"s\"><trigger object=\"pieces\" action=\"drop\" />";
	}

	// The loose gems: where each is, and its group's name.
	std::map<std::pair<int, int>, std::string> gemsOf(Game& game)
	{
		std::map<std::pair<int, int>, std::string> found;
		for (const Object& object : game.getCurrentObjects())
		{
			if (object.exists && object.onCells && !object.isShape() && object.shapeOwner.empty())
			{
				const std::string group = object.groupName.substr(object.groupName.rfind('.') + 1);
				found[{ static_cast<int>(object.cellColumn), static_cast<int>(object.cellRow) }] = group;
			}
		}
		return found;
	}

	using Gems = std::map<std::pair<int, int>, std::string>;
}

TEST_CASE("A line of three alike across is cleared, and what is above it falls", "[cells]")
{
	const std::string one = "<group name=\"one\"><shape><row>r</row></shape></group>";
	played::Scratch file(gems({ "...", "...", "g..", "rr." }, one), dropAt("one", 3));
	Played play(file.path);
	play.tap(KeyCode::Space);
	play.frames(1);
	play.tap(KeyCode::S);
	play.frames(2);

	CHECK(gemsOf(play.game) == Gems{ { { 1, 4 }, "green" } });
	CHECK(play.get("tally").variable.at("lines") == 1.0f);
}

TEST_CASE("A line the fall makes is cleared too: a chain", "[cells]")
{
	const std::string two = "<group name=\"two\"><shape><row>g</row><row>r</row></shape></group>";
	played::Scratch file(gems({ "...", "...", "..r", "ggr" }, two), dropAt("two", 3));
	Played play(file.path);
	play.tap(KeyCode::Space);
	play.frames(1);
	play.tap(KeyCode::S);
	play.frames(2);

	// The three reds down column 3 go, the green above them falls to the
	// bottom, and the greens are a line across.
	CHECK(gemsOf(play.game).empty());
	CHECK(play.get("tally").variable.at("lines") == 2.0f);
}

TEST_CASE("A line of three alike on a slant is cleared", "[cells]")
{
	const std::string one = "<group name=\"one\"><shape><row>r</row></shape></group>";
	played::Scratch file(gems({ "...", ".rg", "rgg" }, one), dropAt("one", 3));
	Played play(file.path);
	play.tap(KeyCode::Space);
	play.frames(1);
	play.tap(KeyCode::S);
	play.frames(2);

	CHECK(gemsOf(play.game) == Gems{ { { 2, 3 }, "green" }, { { 3, 2 }, "green" }, { { 3, 3 }, "green" } });
	CHECK(play.get("tally").variable.at("lines") == 1.0f);
}

TEST_CASE("A cycle moves each cell of a shape on one, the last to the first", "[cells]")
{
	const std::string three = "<group name=\"three\"><shape><row>r</row><row>g</row><row>b</row></shape></group>";
	played::Scratch file(gems({ "...", "...", "...", "...", "...", "..." }, three), dropAt("three", 2));
	Played play(file.path);
	play.tap(KeyCode::Space);
	play.frames(1);
	play.tap(KeyCode::W);
	play.frames(1);
	play.tap(KeyCode::S);
	play.frames(2);

	CHECK(gemsOf(play.game) == Gems{ { { 2, 4 }, "blue" }, { { 2, 5 }, "red" }, { { 2, 6 }, "green" } });
}

TEST_CASE("A group of groups picking one character is any of them in a shape", "[cells]")
{
	const std::string objects = "<map name=\"field\"><tile><width>10</width><height>10</height></tile><position><x>0</x><y>0</y></position>"
		"<columns>3</columns><rows>9</rows><cells />"
		"<group name=\"jewels\" class=\"jewel\" char=\"j\"><sprite><circle><radius>4</radius></circle></sprite>"
		"<group name=\"red\"><sprite><circle><color>color.red</color></circle></sprite></group>"
		"<group name=\"blue\"><sprite><circle><color>color.blue</color></circle></sprite></group></group>"
		"<group name=\"pieces\"><group name=\"column\"><shape><row>j</row><row>j</row><row>j</row></shape></group></group></map>";
	played::Scratch file(objects, "<spawn object=\"pieces\"><column>2</column><row>1</row></spawn>");
	Played play(file.path);

	// Each colour waits, picked by j.
	CHECK_FALSE(play.get("field.red").exists);
	CHECK_FALSE(play.get("field.blue").exists);

	play.tap(KeyCode::Space);
	play.frames(1);
	Object* shape = falling(play.game);
	REQUIRE(shape);
	REQUIRE(shape->shapeParts.size() == 3);
	for (const std::string& part : shape->shapeParts)
	{
		const std::string of = play.get(part).copyOf;
		CHECK((of == "playing.field.red" || of == "playing.field.blue"));
	}
}

TEST_CASE("Columns plays: a column comes in, cycles, lands and the next comes", "[cells][columns]")
{
	Played play("descriptions/columns.xml");
	play.tap(KeyCode::Space);
	play.frames(2);
	CHECK(play.state() == "playing");

	Object* column = falling(play.game);
	REQUIRE(column);
	REQUIRE(column->shapeParts.size() == 3);
	const std::string name = column->name;
	const std::string top = column->shapeParts.front();
	play.tap(KeyCode::W);
	play.frames(1);
	CHECK(falling(play.game)->shapeParts[1] == top);

	// Left to fall, it lands within thirteen rows' time, and the next comes.
	for (int i = 0; i < 13 * 50 && falling(play.game) && falling(play.game)->name == name; ++i) { play.frames(1); }
	REQUIRE(falling(play.game));
	CHECK(falling(play.game)->name != name);
	CHECK(gemsOf(play.game).size() == 3);
}
