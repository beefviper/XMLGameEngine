// test_dotsandboxes.cpp
// XML Game Engine
// author: beefviper
// date: Nov 1, 2026
//
// Catch2 tests for dotsandboxes.xml and what it brought: <on class> in a
// piece's <moves> (a put in place of a piece), <put> taking whichever
// piece of its class fits, <close> claiming a box and keeping the turn,
// the computer taking boxes and going again (thinking after the player's
// line is shown), and a cursor that stands only on places still free.

#include "played.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cmath>
#include <string>
#include <thread>

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

		int column() { return cell(cursor().cellColumn); }
		int row() { return cell(cursor().cellRow); }

		// By the keys: to the row, then along it (only free places).
		void to(int toColumn, int toRow)
		{
			for (int i = 0; i < 12 && row() != toRow; ++i) { tap(row() < toRow ? KeyCode::S : KeyCode::W); }
			for (int i = 0; i < 12 && column() != toColumn; ++i) { tap(column() < toColumn ? KeyCode::D : KeyCode::A); }
		}

		// The pointer over that place on the grid, and a click if wanted.
		void mouse(float x, float y, bool click = false)
		{
			Controls controls;
			controls.pointerInside = true;
			controls.pointerX = x;
			controls.pointerY = y;
			engine.handleControls(controls);
			if (click)
			{
				engine.handleKeyPressed(KeyCode::MouseLeft);
				engine.handleKeyReleased(KeyCode::MouseLeft);
			}
			frames(1);
		}
		static float middleX(float column) { return 180.0f + 40.0f * (column - 0.5f); }
		static float middleY(float row) { return 110.0f + 40.0f * (row - 0.5f); }

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
	// A line drawn already is no place: Space there does nothing.
	board.engine.handleKeyPressed(KeyCode::Space);
	board.engine.handleKeyReleased(KeyCode::Space);
	board.frames(1);
	CHECK(board.turn() == 1.0f);
}

TEST_CASE("dotsandboxes.xml: the cursor steps over free places only", "[dotsandboxes]")
{
	Dots board;
	REQUIRE(board.column() == 2);
	REQUIRE(board.row() == 1);
	// Along a row of across lines, two cells a step.
	board.tap(KeyCode::D);
	CHECK(board.column() == 4);
	CHECK(board.row() == 1);
	// Down: a row of down lines, then across lines again, keeping near column 4.
	board.tap(KeyCode::S);
	CHECK(board.row() == 2);
	CHECK(board.column() == 3);
	board.tap(KeyCode::S);
	CHECK(board.row() == 3);
	CHECK(board.column() == 4);
	board.tap(KeyCode::A);
	CHECK(board.column() == 2);
	// A line drawn is stepped over; nothing left that way, and it stays.
	board.draw(4, 1);
	board.draw(4, 3);
	board.to(6, 1);
	board.tap(KeyCode::A);
	CHECK(board.column() == 2);
	board.tap(KeyCode::A);
	CHECK(board.column() == 2);
	board.tap(KeyCode::W);
	CHECK(board.row() == 1);
	// Never a dot or a box.
	for (int i = 0; i < 30; ++i)
	{
		const KeyCode keys[4] = { KeyCode::S, KeyCode::D, KeyCode::S, KeyCode::A };
		board.tap(keys[i % 4]);
		const std::string under = board.at(board.column(), board.row());
		CHECK((under == "hslot" || under == "vslot"));
	}
}

TEST_CASE("dotsandboxes.xml: the mouse picks the free place nearest it", "[dotsandboxes][mouse]")
{
	Dots board;
	// Just below the middle of the across line at (4,3): that line.
	board.mouse(Dots::middleX(4), Dots::middleY(3) + 5.0f);
	CHECK(board.column() == 4);
	CHECK(board.row() == 3);
	// On a dot: one of the lines beside it, never the dot.
	board.mouse(Dots::middleX(5), Dots::middleY(5));
	CHECK(board.at(board.column(), board.row()).find("slot") != std::string::npos);
	// Inside box (4,4), nearer its left side: the down line at (3,4); a click draws it.
	board.mouse(Dots::middleX(3.8f), Dots::middleY(4));
	CHECK(board.column() == 3);
	CHECK(board.row() == 4);
	board.mouse(Dots::middleX(3.8f), Dots::middleY(4), true);
	CHECK(board.at(3, 4) == "redline");
	// Drawn, it is no place: the nearest free one is taken instead.
	board.mouse(Dots::middleX(3), Dots::middleY(4));
	CHECK(board.at(board.column(), board.row()).find("slot") != std::string::npos);
}

TEST_CASE("dotsandboxes.xml: against the computer, the keys do nothing on its turn", "[dotsandboxes][think]")
{
	Dots board(true);
	board.draw(2, 1);
	CHECK(board.at(2, 1) == "redline");
	REQUIRE(board.turn() == 2.0f);
	board.frames(1);
	CHECK(board.cursor().lookName() == "cpu");
	CHECK(board.get("whoseturn").lookName() == "thinking");
	const int column = board.column();
	const int row = board.row();
	board.tap(KeyCode::D);
	board.tap(KeyCode::S);
	board.mouse(Dots::middleX(10), Dots::middleY(11));
	CHECK(board.column() == column);
	CHECK(board.row() == row);
	board.untilRed();
	CHECK(board.get("whoseturn").lookName() == "red");
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

TEST_CASE("dotsandboxes.xml: the computer thinks on a thread of its own while the game goes on", "[dotsandboxes][think]")
{
	Dots board(true);
	board.game.thinkInBackground(true);
	board.draw(2, 1);
	CHECK(board.at(2, 1) == "redline");
	// Frames go on while it thinks; then it moves, and it is red's turn.
	const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(60);
	while (board.turn() != 1.0f && board.state() == "playing" && std::chrono::steady_clock::now() < until)
	{
		board.frames(1);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	CHECK(board.turn() == 1.0f);
	int blue = 0;
	for (int row = 1; row <= 11; ++row)
	{
		for (int column = 1; column <= 11; ++column) { blue += board.at(column, row) == "blueline" ? 1 : 0; }
	}
	CHECK(blue == 1);
	// Its turn again, then put back mid-thought: nothing is left running wrong.
	board.draw(board.at(4, 1) == "hslot" ? 4 : 6, 1);
	board.frames(1);
	board.game.thinkInBackground(false);
}
