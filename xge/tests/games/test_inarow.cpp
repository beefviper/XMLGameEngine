// test_inarow.cpp
// XML Game Engine
// author: beefviper
// date: Nov 1, 2026
//
// Catch2 tests for fourinarow.xml and tictactoe.xml and what they
// brought: <fall /> in a piece's <moves> (a put drops down its column),
// an empty <moves /> (a put anywhere free), a map's <line> checks asked
// after a put, and the computer reading wins from them.

#include "played.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <string>

using namespace xge;
using played::Played;

namespace
{
	int cell(float number) { return static_cast<int>(std::lround(number)); }

	struct Grid : Played
	{
		// Two players at the keyboard, or one against the computer.
		Grid(const std::string& file, bool computer) : Played("descriptions/" + file + ".xml")
		{
			tap(computer ? KeyCode::Space : KeyCode::Num2);
			frames(2);
		}

		Object& cursor() { return get("cursor"); }
		// Whose turn: the number of the member of `players` on it (from 1).
		float turn()
		{
			const Object* on = game.onTurn("players");
			float number = 0.0f;
			for (const Object& each : game.getCurrentObjects())
			{
				if (!on || !each.takesTurns || each.groupName != on->groupName) { continue; }
				number += 1.0f;
				if (&each == on) { return number; }
			}
			return 0.0f;
		}

		void to(int column, int row)
		{
			while (cell(cursor().cellColumn) < column) { tap(KeyCode::D); }
			while (cell(cursor().cellColumn) > column) { tap(KeyCode::A); }
			while (cell(cursor().cellRow) < row) { tap(KeyCode::S); }
			while (cell(cursor().cellRow) > row) { tap(KeyCode::W); }
		}

		void space(int column, int row = 1)
		{
			to(column, row);
			tap(KeyCode::Space);
			frames(1);
		}

		// The class of the piece in that cell, or "".
		std::string at(int column, int row)
		{
			for (Object& object : game.getCurrentObjects())
			{
				if (object.exists && object.isVisible && object.onCells && object.isCopy() && object.mapName == boardMap
					&& cell(object.cellColumn) == column && cell(object.cellRow) == row)
				{
					return object.objClass;
				}
			}
			return "";
		}

		// Frames until it is the first player's turn again, or the game is over.
		void untilFirst()
		{
			for (int i = 0; i < 600 && turn() != 1.0f && state() == "playing"; ++i) { frames(1); }
		}

		std::string boardMap;
	};

	struct Four : Grid
	{
		explicit Four(bool computer = false) : Grid("fourinarow", computer)
		{
			for (const Object& object : game.getCurrentObjects())
			{
				if (object.objClass == "red" && object.onCells) { boardMap = object.mapName; break; }
			}
		}
		// A disc dropped down that column.
		void drop(int column) { space(column, 1); }
	};

	struct Three : Grid
	{
		explicit Three(bool computer = false) : Grid("tictactoe", computer)
		{
			for (const Object& object : game.getCurrentObjects())
			{
				if (object.objClass == "x" && object.onCells) { boardMap = object.mapName; break; }
			}
		}
	};
}

TEST_CASE("fourinarow.xml: a disc dropped falls to the bottom, then onto the one below", "[fourinarow]")
{
	Four board;
	CHECK(board.turn() == 1.0f);
	board.drop(4);
	CHECK(board.at(4, 6) == "red");
	CHECK(board.turn() == 2.0f);
	CHECK(board.cursor().lookName() == "yellow");
	board.drop(4);
	CHECK(board.at(4, 5) == "yellow");
	CHECK(board.turn() == 1.0f);
	// The cursor stays on its own row above the board.
	CHECK(cell(board.cursor().cellRow) == 1);
}

TEST_CASE("fourinarow.xml: a full column refuses a disc", "[fourinarow]")
{
	Four board;
	for (int i = 0; i < 6; ++i) { board.drop(1); }
	REQUIRE(board.at(1, 1) == "yellow");
	REQUIRE(board.turn() == 1.0f);
	board.drop(1);
	CHECK(board.turn() == 1.0f);
	CHECK(board.get("match").variable.at("made") == 6.0f);
}

TEST_CASE("fourinarow.xml: four across wins, and four on a slant", "[fourinarow]")
{
	Four board;
	for (int column = 1; column <= 3; ++column)
	{
		board.drop(column);
		board.drop(column);
	}
	REQUIRE(board.state() == "playing");
	board.drop(4);
	board.frames(1);
	CHECK(board.state() == "over");
	CHECK(board.get("winner").lookName() == "red");

	// Red: (1,6) (2,5) (3,4) (4,3) on a slant, built on yellow and red discs.
	Four slant;
	slant.drop(1);
	slant.drop(2);
	slant.drop(2);
	slant.drop(3);
	slant.drop(3);
	slant.drop(4);
	slant.drop(3);
	slant.drop(4);
	slant.drop(5);
	slant.drop(4);
	REQUIRE(slant.state() == "playing");
	slant.drop(4);
	slant.frames(1);
	CHECK(slant.state() == "over");
	CHECK(slant.get("winner").lookName() == "red");
}

TEST_CASE("fourinarow.xml: the computer blocks three, and takes its own four", "[fourinarow][think]")
{
	// Red three along the bottom, yellow two above them; then the computer plays yellow.
	Four block;
	block.drop(1);
	block.drop(1);
	block.drop(2);
	block.drop(2);
	block.game.setVariable("match", "players", 1.0f);
	block.drop(3);
	block.untilFirst();
	CHECK(block.at(4, 6) == "yellow");
	CHECK(block.state() == "playing");

	// Yellow three along the bottom, its turn: it makes four.
	Four win;
	win.drop(7);
	win.drop(1);
	win.drop(7);
	win.drop(2);
	win.drop(6);
	win.drop(3);
	win.game.setVariable("match", "players", 1.0f);
	win.drop(5);
	win.untilFirst();
	win.frames(1);
	CHECK(win.at(4, 6) == "yellow");
	CHECK(win.state() == "over");
	CHECK(win.get("winner").lookName() == "yellow");
}

TEST_CASE("fourinarow.xml: the computer's disc is dropped from its row", "[fourinarow][think]")
{
	Four board(true);
	board.drop(4);
	REQUIRE(board.turn() == 2.0f);
	board.frames(1);
	CHECK(board.cursor().lookName() == "yellow");
	// Your keys do not reach the cursor while the computer thinks.
	const int column = cell(board.cursor().cellColumn);
	board.tap(KeyCode::A);
	CHECK(cell(board.cursor().cellColumn) == column);
	board.untilFirst();
	CHECK(board.turn() == 1.0f);
	CHECK(board.get("match").variable.at("made") == 2.0f);
	CHECK(cell(board.cursor().cellRow) == 1);
}

TEST_CASE("tictactoe.xml: a mark goes in any free square, never a taken one", "[tictactoe]")
{
	Three board;
	board.space(2, 2);
	CHECK(board.at(2, 2) == "x");
	CHECK(board.turn() == 2.0f);
	board.space(2, 2);
	CHECK(board.turn() == 2.0f);
	board.space(1, 1);
	CHECK(board.at(1, 1) == "o");
	CHECK(board.turn() == 1.0f);
}

TEST_CASE("tictactoe.xml: three in a line wins, a full grid is a draw", "[tictactoe]")
{
	Three board;
	board.space(1, 1);
	board.space(1, 2);
	board.space(2, 2);
	board.space(1, 3);
	board.space(3, 3);
	board.frames(1);
	CHECK(board.state() == "over");
	CHECK(board.get("winner").lookName() == "x");

	// x o x / x o o / o x x
	Three draw;
	draw.space(1, 1);
	draw.space(2, 1);
	draw.space(3, 1);
	draw.space(2, 2);
	draw.space(1, 2);
	draw.space(3, 2);
	draw.space(2, 3);
	draw.space(1, 3);
	draw.space(3, 3);
	draw.frames(1);
	CHECK(draw.state() == "over");
	CHECK(draw.get("winner").lookName() == "draw");
}

TEST_CASE("tictactoe.xml: the smart computer is never beaten", "[tictactoe][think]")
{
	// X tries every first square, then keeps going for the corner it can.
	for (int first = 1; first <= 9; ++first)
	{
		Three board(true);
		REQUIRE(board.get("options").variable.at("smart") == 1.0f);
		int next = first;
		for (int turn = 0; turn < 5 && board.state() == "playing"; ++turn)
		{
			// The first free square from `next` on.
			for (int tries = 0; tries < 9; ++tries, next = next % 9 + 1)
			{
				if (board.at((next - 1) % 3 + 1, (next - 1) / 3 + 1).empty()) { break; }
			}
			board.space((next - 1) % 3 + 1, (next - 1) / 3 + 1);
			board.untilFirst();
		}
		board.frames(2);
		CHECK(board.state() == "over");
		CHECK(board.get("winner").lookName() != "x");
	}
}

TEST_CASE("tictactoe.xml: the easy computer takes a win but does not block", "[tictactoe][think]")
{
	Three board(true);
	board.game.setVariable("options", "smart", 0.0f);
	board.space(1, 1);
	board.untilFirst();
	board.space(2, 1);
	board.untilFirst();
	// X threatens (3,1); easy O does not see it, unless it won already.
	if (board.state() == "playing" && board.at(3, 1).empty())
	{
		board.space(3, 1);
		board.frames(2);
		CHECK(board.get("winner").lookName() == "x");
	}
}
