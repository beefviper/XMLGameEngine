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
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace xge;
using played::Played;

namespace
{
	// The falling shape of a game: the one in play that has cells.
	Object* falling(Game& game)
	{
		for (Object& object : game.getCurrentObjects())
		{
			if (object.exists && object.isShape() && object.isCopy()) { return &object; }
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

	// The loose pieces of a map (in no shape), as (column, row).
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
