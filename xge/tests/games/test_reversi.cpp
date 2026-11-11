// test_reversi.cpp
// XML Game Engine
// author: beefviper
// date: Nov 1, 2026
//
// Catch2 tests for reversi.xml and what it brought: <flank> in a piece's
// <moves>, the picker's <put>, <count class into>, the <put> marks, a side
// that cannot move passing, and the computer playing a game of puts.

#include "played.h"
#include "turns.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

using namespace xge;
using played::Played;

namespace
{
	int cell(float number) { return static_cast<int>(std::lround(number)); }

	using Cells = std::vector<std::pair<int, int>>;

	struct Board : Played
	{
		// Two players at the keyboard, or (computer) black against it.
		explicit Board(bool computer = false) : Played("descriptions/reversi.xml")
		{
			tap(computer ? KeyCode::Space : KeyCode::Num2);
			frames(2);
		}

		Object& cursor() { return get("cursor"); }
		float turn() { return turns::number(game); }
		float score(const std::string& side) { return get("score").variable.at(side); }

		void to(int column, int row)
		{
			while (cell(cursor().cellColumn) < column) { tap(KeyCode::D); }
			while (cell(cursor().cellColumn) > column) { tap(KeyCode::A); }
			while (cell(cursor().cellRow) < row) { tap(KeyCode::S); }
			while (cell(cursor().cellRow) > row) { tap(KeyCode::W); }
		}

		void space(int column, int row)
		{
			to(column, row);
			tap(KeyCode::Space);
			frames(1);
		}

		// The disc on that square: "black", "white" or "".
		std::string at(int column, int row)
		{
			for (Object& object : game.getCurrentObjects())
			{
				if (object.exists && object.isVisible && object.onCells && (object.objClass == "black" || object.objClass == "white")
					&& cell(object.cellColumn) == column && cell(object.cellRow) == row)
				{
					return object.objClass;
				}
			}
			return "";
		}

		float sum(const std::string& side)
		{
			float total = 0.0f;
			for (const Object& object : game.getCurrentObjects())
			{
				if (object.exists && object.isVisible && object.objClass == side) { total += object.moveCount; }
			}
			return total;
		}

		// The four discs written moved to these squares, two of each side.
		struct Disc
		{
			const char* side;
			int column;
			int row;
		};
		void set(std::initializer_list<Disc> discs)
		{
			int black = 0;
			int white = 0;
			for (Object& object : game.getCurrentObjects())
			{
				if (object.objClass != "black" && object.objClass != "white") { continue; }
				int& next = object.objClass == "black" ? black : white;
				int seen = 0;
				for (const Disc& disc : discs)
				{
					if (disc.side != object.objClass || seen++ != next) { continue; }
					object.cellColumn = static_cast<float>(disc.column);
					object.cellRow = static_cast<float>(disc.row);
					object.position = { 44.0f + 60.0f * static_cast<float>(disc.column), 24.0f + 60.0f * static_cast<float>(disc.row) };
					break;
				}
				++next;
			}
			frames(1);
		}

		// The cells holding a copy of that mark.
		Cells marks(const std::string& mark)
		{
			Cells cells;
			for (const Object& object : game.getCurrentObjects())
			{
				if (object.exists && object.copyOf.find(mark) != std::string::npos) { cells.emplace_back(cell(object.cellColumn), cell(object.cellRow)); }
			}
			std::sort(cells.begin(), cells.end());
			return cells;
		}
	};
}

TEST_CASE("reversi.xml: two discs each in the middle, black to move with four places", "[reversi]")
{
	Board board;
	CHECK(board.at(4, 4) == "white");
	CHECK(board.at(5, 4) == "black");
	CHECK(board.at(4, 5) == "black");
	CHECK(board.at(5, 5) == "white");
	CHECK(board.turn() == 1.0f);
	CHECK(board.get("blacks").lookName() == "moving");
	// Each of a side's discs counts the side's places.
	CHECK(board.sum("black") == 8.0f);
	CHECK(board.sum("white") == 8.0f);
	CHECK(board.marks("putmark") == Cells{ { 3, 4 }, { 4, 3 }, { 5, 6 }, { 6, 5 } });
}

TEST_CASE("reversi.xml: a disc put traps a line and turns it, and the turn passes", "[reversi]")
{
	Board board;
	board.space(5, 6);
	CHECK(board.at(5, 6) == "black");
	CHECK(board.at(5, 5) == "black");
	CHECK(board.at(4, 4) == "white");
	CHECK(board.score("black") == 4.0f);
	CHECK(board.score("white") == 1.0f);
	CHECK(board.turn() == 2.0f);
	CHECK(board.get("whites").lookName() == "moving");
	board.frames(1);
	CHECK(board.marks("lastmark") == Cells{ { 5, 6 } });
	CHECK(board.marks("putmark") == Cells{ { 4, 6 }, { 6, 4 }, { 6, 6 } });

	// White turns a line on a slant.
	board.space(6, 6);
	CHECK(board.at(6, 6) == "white");
	CHECK(board.at(5, 5) == "white");
	CHECK(board.at(4, 4) == "white");
	CHECK(board.score("black") == 3.0f);
	CHECK(board.score("white") == 3.0f);
	CHECK(board.turn() == 1.0f);
}

TEST_CASE("reversi.xml: a put that traps nothing, or on a disc, is refused", "[reversi]")
{
	Board board;
	board.space(1, 1);
	CHECK(board.at(1, 1).empty());
	board.space(6, 4);
	CHECK(board.at(6, 4).empty());
	board.space(5, 4);
	CHECK(board.at(5, 4) == "black");
	CHECK(board.turn() == 1.0f);
	CHECK(board.score("black") == 2.0f);
}

TEST_CASE("reversi.xml: a side that cannot move passes", "[reversi]")
{
	Board board;
	board.set({ { "black", 1, 1 }, { "black", 5, 5 }, { "white", 2, 2 }, { "white", 6, 5 } });
	board.space(7, 5);
	CHECK(board.at(6, 5) == "black");
	// White's one disc is hemmed in a corner's line: it has nowhere to go.
	board.frames(2);
	CHECK(board.turn() == 1.0f);
	CHECK(board.get("blacks").lookName() == "again");
	board.space(3, 3);
	CHECK(board.at(2, 2) == "black");
}

TEST_CASE("reversi.xml: when neither side can move, more discs wins", "[reversi]")
{
	Board board;
	board.set({ { "black", 1, 1 }, { "black", 8, 1 }, { "white", 2, 1 }, { "white", 8, 8 } });
	board.space(3, 1);
	board.frames(2);
	CHECK(board.state() == "over");
	CHECK(board.get("winner").lookName() == "black");
	CHECK(board.score("black") == 4.0f);
	CHECK(board.score("white") == 1.0f);
	// Space goes back to the title, and the board is set again.
	board.tap(KeyCode::Space);
	board.frames(2);
	CHECK(board.state() == "title");
	CHECK(board.at(1, 1).empty());
	CHECK(board.score("black") == 2.0f);
}

TEST_CASE("reversi.xml: the option hides where a disc can go", "[reversi][marks]")
{
	Board board;
	board.game.setVariable("options", "moves", 0.0f);
	board.frames(2);
	CHECK(board.marks("putmark").empty());
}

TEST_CASE("reversi.xml: the computer answers black's disc with one of its own", "[reversi][think]")
{
	Board board(true);
	board.space(5, 6);
	REQUIRE(board.turn() == 2.0f);
	board.frames(1);
	// Your keys do not reach the cursor while the computer thinks.
	const int column = cell(board.cursor().cellColumn);
	board.tap(KeyCode::A);
	CHECK(cell(board.cursor().cellColumn) == column);
	for (int i = 0; i < 600 && board.turn() != 1.0f; ++i) { board.frames(1); }
	CHECK(board.turn() == 1.0f);
	CHECK(board.score("black") + board.score("white") == 6.0f);
	CHECK(board.score("white") == 3.0f);
}

TEST_CASE("reversi.xml: the cursor follows the mouse, and a click puts a disc", "[reversi][mouse]")
{
	Board board;
	const auto mouse = [&](float x, float y, bool left)
	{
		Controls controls;
		controls.pointerInside = true;
		controls.pointerX = x;
		controls.pointerY = y;
		board.engine.handleControls(controls);
		if (left)
		{
			board.engine.handleKeyPressed(KeyCode::MouseLeft);
			board.engine.handleKeyReleased(KeyCode::MouseLeft);
		}
		board.frames(1);
	};
	mouse(100.0f + 60.0f * 5.5f, 80.0f + 60.0f * 3.5f, false);
	CHECK(cell(board.cursor().cellColumn) == 6);
	CHECK(cell(board.cursor().cellRow) == 4);
	mouse(100.0f + 60.0f * 5.5f, 80.0f + 60.0f * 4.5f, true);
	CHECK(board.at(6, 5) == "black");
	CHECK(board.at(5, 5) == "black");
	CHECK(board.turn() == 2.0f);
}

TEST_CASE("reversi.xml: a whole game against the computer comes to an end", "[reversi][think]")
{
	Board board(true);
	// Black puts its disc in the first place marked, every turn, passing
	// when it has none, until the game is over.
	for (int i = 0; i < 6000 && board.state() == "playing"; ++i)
	{
		if (board.turn() == 1.0f)
		{
			const Cells places = board.marks("putmark");
			if (!places.empty()) { board.space(places.front().first, places.front().second); }
		}
		board.frames(1);
	}
	REQUIRE(board.state() == "over");
	CHECK(board.score("black") == static_cast<float>(board.count("black")));
	CHECK(board.score("white") == static_cast<float>(board.count("white")));
	CHECK(board.sum("black") == 0.0f);
	CHECK(board.sum("white") == 0.0f);
	const std::string winner = board.get("winner").lookName();
	CHECK(winner == (board.score("black") > board.score("white") ? "black" : board.score("white") > board.score("black") ? "white" : "draw"));
}
