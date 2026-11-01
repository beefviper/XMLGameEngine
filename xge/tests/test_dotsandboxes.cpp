// test_dotsandboxes.cpp
// XML Game Engine
// author: beefviper
// date: Nov 1, 2026
//
// Catch2 tests for dotsandboxes.xml and what it brought: <on class> in a
// piece's <moves> (a put in place of a piece), <put> taking whichever
// piece of its class fits, <close> claiming a box and keeping the turn,
// and the computer taking boxes and going again.

#include "played.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <string>

using namespace xge;
using played::Played;

namespace
{
	int cell(float number) { return static_cast<int>(std::lround(number)); }

	struct Dots : Played
	{
		// Two players at the keyboard, or red against the computer.
		explicit Dots(bool computer = false) : Played("descriptions/dotsandboxes.xml")
		{
			tap(computer ? KeyCode::Space : KeyCode::Num2);
			frames(2);
		}

		void untilRed()
		{
			for (int i = 0; i < 3000 && turn() != 1.0f && state() == "playing"; ++i) { frames(1); }
		}

		Object& cursor() { return get("cursor"); }
		float turn() { return cursor().variable.at("turn"); }
		float score(const std::string& name) { return get("score").variable.at(name); }

		void to(int column, int row)
		{
			while (cell(cursor().cellColumn) < column) { tap(KeyCode::D); }
			while (cell(cursor().cellColumn) > column) { tap(KeyCode::A); }
			while (cell(cursor().cellRow) < row) { tap(KeyCode::S); }
			while (cell(cursor().cellRow) > row) { tap(KeyCode::W); }
		}

		void draw(int column, int row)
		{
			to(column, row);
			tap(KeyCode::Space);
			frames(1);
		}

		// The class of what is in that cell of the grid, or "".
		std::string at(int column, int row)
		{
			for (Object& object : game.getCurrentObjects())
			{
				if (object.exists && object.isVisible && object.onCells && object.mapName == "playing.grid"
					&& cell(object.cellColumn) == column && cell(object.cellRow) == row)
				{
					return object.objClass;
				}
			}
			return "";
		}
	};
}

TEST_CASE("dotsandboxes.xml: a line goes on its place, across or down, and the turn passes", "[dotsandboxes]")
{
	Dots board;
	REQUIRE(board.at(2, 1) == "hslot");
	board.draw(2, 1);
	CHECK(board.at(2, 1) == "redline");
	CHECK(board.turn() == 2.0f);
	board.draw(1, 2);
	CHECK(board.at(1, 2) == "blueline");
	CHECK(board.turn() == 1.0f);
	// Not on a dot, a box or a line drawn already.
	board.draw(1, 1);
	board.draw(2, 2);
	board.draw(2, 1);
	CHECK(board.turn() == 1.0f);
	CHECK(board.at(1, 1) == "dot");
	CHECK(board.at(2, 2) == "box");
}

TEST_CASE("dotsandboxes.xml: closing a box claims it and keeps the turn", "[dotsandboxes]")
{
	Dots board;
	board.draw(2, 1);
	board.draw(1, 2);
	board.draw(3, 2);
	REQUIRE(board.turn() == 2.0f);
	board.draw(2, 3);
	CHECK(board.at(2, 2) == "bluebox");
	CHECK(board.score("blue") == 1.0f);
	CHECK(board.score("open") == 24.0f);
	CHECK(board.turn() == 2.0f);
	CHECK(board.get("whoseturn").lookName() == "blue");
	board.draw(6, 1);
	CHECK(board.turn() == 1.0f);
}

TEST_CASE("dotsandboxes.xml: one line can close two boxes", "[dotsandboxes]")
{
	Dots board;
	// Boxes (2,2) and (4,2) share the line at (3,2).
	board.draw(2, 1);
	board.draw(4, 1);
	board.draw(1, 2);
	board.draw(5, 2);
	board.draw(2, 3);
	board.draw(4, 3);
	REQUIRE(board.turn() == 1.0f);
	board.draw(3, 2);
	CHECK(board.at(2, 2) == "redbox");
	CHECK(board.at(4, 2) == "redbox");
	CHECK(board.score("red") == 2.0f);
	CHECK(board.turn() == 1.0f);
}

TEST_CASE("dotsandboxes.xml: every box claimed ends the game, more boxes wins", "[dotsandboxes]")
{
	Dots board;
	// Every line, row by row: whoever closes boxes goes on.
	for (int row = 1; row <= 11 && board.state() == "playing"; ++row)
	{
		for (int column = 1; column <= 11 && board.state() == "playing"; ++column)
		{
			const std::string here = board.at(column, row);
			if (here == "hslot" || here == "vslot") { board.draw(column, row); }
		}
	}
	board.frames(1);
	CHECK(board.state() == "over");
	CHECK(board.score("open") == 0.0f);
	CHECK(board.score("red") + board.score("blue") == 25.0f);
	CHECK(board.get("winner").lookName() == (board.score("red") > board.score("blue") ? "red" : "blue"));
}

TEST_CASE("dotsandboxes.xml: the computer takes a box it is given, and goes again", "[dotsandboxes][think]")
{
	Dots board;
	// Three sides of box (2,2) drawn, red to move.
	board.draw(2, 1);
	board.draw(1, 2);
	board.draw(3, 2);
	board.draw(10, 11);
	REQUIRE(board.turn() == 1.0f);
	board.game.setVariable("match", "players", 1.0f);
	// Red draws far away; blue (the computer) closes the box, then draws again.
	board.draw(10, 1);
	board.untilRed();
	CHECK(board.at(2, 2) == "bluebox");
	CHECK(board.score("blue") == 1.0f);
	CHECK(board.turn() == 1.0f);
	int blue = 0;
	for (int row = 1; row <= 11; ++row)
	{
		for (int column = 1; column <= 11; ++column) { blue += board.at(column, row) == "blueline" ? 1 : 0; }
	}
	CHECK(blue == 4);
}

TEST_CASE("dotsandboxes.xml: the computer does not draw a box's third side when it need not", "[dotsandboxes][think]")
{
	Dots board(true);
	// Red's lines: one, then the far corner's while there are some.
	board.draw(2, 1);
	board.untilRed();
	for (int i = 0; i < 6 && board.state() == "playing"; ++i)
	{
		const int places[6][2] = { { 10, 11 }, { 11, 10 }, { 8, 11 }, { 9, 10 }, { 11, 8 }, { 10, 9 } };
		if (board.at(places[i][0], places[i][1]).find("slot") != std::string::npos) { board.draw(places[i][0], places[i][1]); }
		board.untilRed();
	}
	// No box has three sides drawn with red to move: the computer gave none away.
	int given = 0;
	for (int row = 2; row <= 10; row += 2)
	{
		for (int column = 2; column <= 10; column += 2)
		{
			int drawn = 0;
			drawn += board.at(column + 1, row).find("line") != std::string::npos ? 1 : 0;
			drawn += board.at(column - 1, row).find("line") != std::string::npos ? 1 : 0;
			drawn += board.at(column, row + 1).find("line") != std::string::npos ? 1 : 0;
			drawn += board.at(column, row - 1).find("line") != std::string::npos ? 1 : 0;
			given += drawn == 3 ? 1 : 0;
		}
	}
	CHECK(given == 0);
}

TEST_CASE("dotsandboxes.xml: a whole game against the computer comes to an end", "[dotsandboxes][think]")
{
	Dots board(true);
	for (int i = 0; i < 200 && board.state() == "playing"; ++i)
	{
		if (board.turn() == 1.0f)
		{
			bool drawn = false;
			for (int row = 1; row <= 11 && !drawn; ++row)
			{
				for (int column = 1; column <= 11 && !drawn; ++column)
				{
					if (board.at(column, row).find("slot") != std::string::npos)
					{
						board.draw(column, row);
						drawn = true;
					}
				}
			}
		}
		board.untilRed();
	}
	board.frames(1);
	CHECK(board.state() == "over");
	CHECK(board.score("red") + board.score("blue") == 25.0f);
}
