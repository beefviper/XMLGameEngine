// test_checkers.cpp
// XML Game Engine
// author: beefviper
// date: Oct 31, 2026
//
// Catch2 tests for checkers.xml and what it brought: a board game piece's
// <moves> (steps, jumps, forced jumps, a king's own set, <promote>), the
// cursor's <pick> and <place>, a run of jumps that must be finished, the
// live number `moves` that tells a side it is stuck, and the computer's
// <think>; the marks, options, put back and mouse play.

#include "played.h"

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

	struct Board : Played
	{
		// Two players at the keyboard, or (computer) red against it.
		explicit Board(bool computer = false) : Played("descriptions/checkers.xml")
		{
			tap(computer ? KeyCode::Space : KeyCode::Num2);
			frames(2);
		}

		// Frames until it is red's turn again, at most `most`.
		void untilRed(int most = 600)
		{
			for (int i = 0; i < most && cursor().variable.at("turn") != 1.0f && state() == "playing"; ++i) { frames(1); }
		}

		Object& cursor() { return get("cursor"); }

		// The cursor walked to that square with the keys.
		void to(int column, int row)
		{
			while (cell(cursor().cellColumn) < column) { tap(KeyCode::D); }
			while (cell(cursor().cellColumn) > column) { tap(KeyCode::A); }
			while (cell(cursor().cellRow) < row) { tap(KeyCode::S); }
			while (cell(cursor().cellRow) > row) { tap(KeyCode::W); }
		}

		// Space on that square.
		void space(int column, int row)
		{
			to(column, row);
			tap(KeyCode::Space);
			frames(1);
		}

		// The piece of that side on that square, or null.
		Object* at(const std::string& side, int column, int row)
		{
			for (Object& object : game.getCurrentObjects())
			{
				if (object.exists && object.isVisible && object.objClass == side && object.onCells
					&& cell(object.cellColumn) == column && cell(object.cellRow) == row)
				{
					return &object;
				}
			}
			return nullptr;
		}

		// Every piece off the board but these: (side, column, row) each.
		struct Piece
		{
			const char* side;
			int column;
			int row;
		};
		void only(std::initializer_list<Piece> pieces)
		{
			int red = 0;
			int white = 0;
			for (Object& object : game.getCurrentObjects())
			{
				if (object.objClass != "red" && object.objClass != "white") { continue; }
				const Piece* keep = nullptr;
				int seen = 0;
				for (const Piece& piece : pieces)
				{
					if (piece.side != object.objClass) { continue; }
					if (seen++ == (object.objClass == "red" ? red : white)) { keep = &piece; }
				}
				int& next = object.objClass == "red" ? red : white;
				if (!keep) { game.despawn(object.name); continue; }
				++next;
				object.cellColumn = static_cast<float>(keep->column);
				object.cellRow = static_cast<float>(keep->row);
				object.position = { 100.0f + 60.0f * static_cast<float>(keep->column) + 4.0f, 20.0f + 60.0f * static_cast<float>(keep->row) + 4.0f };
			}
			frames(1);
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
	};
}

TEST_CASE("checkers.xml: the board is set, red to move, the cursor over red's men", "[checkers]")
{
	Board board;
	REQUIRE(board.state() == "playing");
	CHECK(board.count("red") == 12);
	CHECK(board.count("white") == 12);
	CHECK(board.cursor().lookName() == "redfree");
	CHECK(board.get("whoseturn").lookName() == "red");
	CHECK(board.at("red", 1, 8));
	CHECK(board.at("white", 8, 1));
	CHECK_FALSE(board.at("red", 2, 8));
	// Only the front row can move: one way from the edge, two from the others.
	CHECK(board.sum("red") == 7.0f);
	CHECK(board.sum("white") == 7.0f);
	CHECK(board.at("red", 1, 6)->moveCount == 1.0f);
	CHECK(board.at("red", 3, 6)->moveCount == 2.0f);
	CHECK(board.at("red", 2, 7)->moveCount == 0.0f);
}

TEST_CASE("checkers.xml: a man steps forward on a slant, and the turn passes", "[checkers]")
{
	Board board;
	board.space(3, 6);
	CHECK(board.cursor().lookName() == "redheld");
	CHECK(board.cursor().picked == board.at("red", 3, 6)->name);
	board.space(4, 5);
	CHECK(board.at("red", 4, 5));
	CHECK_FALSE(board.at("red", 3, 6));
	CHECK(board.cursor().lookName() == "whitefree");
	CHECK(board.cursor().variable.at("turn") == 2.0f);
	CHECK(board.get("whoseturn").lookName() == "white");
	CHECK(board.cursor().picked.empty());
}

TEST_CASE("checkers.xml: what is no move is refused, and so is the other side's piece", "[checkers]")
{
	Board board;
	// A white piece on red's turn, and a red man that cannot move.
	board.space(2, 3);
	CHECK(board.cursor().lookName() == "redfree");
	board.space(2, 7);
	CHECK(board.cursor().lookName() == "redfree");

	// Straight ahead, backwards, two squares: none is a move.
	board.space(3, 6);
	REQUIRE(board.cursor().lookName() == "redheld");
	board.space(3, 5);
	board.space(4, 7);
	board.space(5, 4);
	CHECK(board.at("red", 3, 6));
	CHECK(board.cursor().lookName() == "redheld");

	// Space on another of red's own: that one is held instead.
	board.space(5, 6);
	CHECK(board.cursor().picked == board.at("red", 5, 6)->name);
	board.space(6, 5);
	CHECK(board.at("red", 6, 5));
	CHECK(board.cursor().lookName() == "whitefree");
}

TEST_CASE("checkers.xml: a jump must be made, and takes the piece jumped", "[checkers]")
{
	Board board;
	board.space(3, 6);
	board.space(4, 5);
	board.space(6, 3);
	board.space(5, 4);
	REQUIRE(board.cursor().lookName() == "redfree");

	// Red can take: no other piece may move, and the man that can jump
	// may only jump.
	board.space(7, 6);
	CHECK(board.cursor().lookName() == "redfree");
	CHECK(board.sum("red") > 0.0f);
	board.space(4, 5);
	REQUIRE(board.cursor().lookName() == "redheld");
	board.space(3, 4);
	CHECK(board.at("red", 4, 5));
	board.space(6, 3);
	CHECK(board.at("red", 6, 3));
	CHECK_FALSE(board.at("white", 5, 4));
	CHECK(board.count("white") == 11);
	CHECK(board.cursor().lookName() == "whitefree");
}

TEST_CASE("checkers.xml: a piece that can jump again must, before the turn passes", "[checkers]")
{
	Board board;
	board.only({ { "red", 2, 7 }, { "red", 1, 8 }, { "white", 3, 6 }, { "white", 5, 4 }, { "white", 8, 1 } });
	board.space(2, 7);
	REQUIRE(board.cursor().lookName() == "redheld");
	board.space(4, 5);
	CHECK(board.at("red", 4, 5));
	CHECK_FALSE(board.at("white", 3, 6));
	// Still red's, the same man held.
	CHECK(board.cursor().lookName() == "redheld");
	CHECK(board.cursor().pickChain);
	CHECK(board.cursor().variable.at("turn") == 1.0f);

	// Not another piece, not a step.
	board.space(1, 8);
	CHECK(board.cursor().picked == board.at("red", 4, 5)->name);
	board.space(3, 4);
	CHECK(board.at("red", 4, 5));

	board.space(6, 3);
	CHECK(board.at("red", 6, 3));
	CHECK(board.count("white") == 1);
	CHECK(board.cursor().lookName() == "whitefree");
	CHECK_FALSE(board.cursor().pickChain);
}

TEST_CASE("checkers.xml: a man on the far row is crowned, and a king goes backwards too", "[checkers]")
{
	Board board;
	board.only({ { "red", 3, 2 }, { "white", 8, 1 }, { "white", 1, 6 } });
	board.space(3, 2);
	board.space(4, 1);
	Object* king = board.at("red", 4, 1);
	REQUIRE(king);
	CHECK(king->lookName() == "king");
	CHECK(king->moveCount == 2.0f);

	// White's turn, then the king steps back down.
	board.space(1, 6);
	board.space(2, 7);
	REQUIRE(board.cursor().lookName() == "redfree");
	board.space(4, 1);
	board.space(5, 2);
	CHECK(board.at("red", 5, 2));
}

TEST_CASE("checkers.xml: a crowning jump ends the move", "[checkers]")
{
	Board board;
	// Crowned on (4,1) by the jump: as a king it could jump on over (3,2),
	// but the move is over.
	board.only({ { "red", 6, 3 }, { "white", 5, 2 }, { "white", 3, 2 }, { "white", 8, 7 } });
	board.space(6, 3);
	board.space(4, 1);
	REQUIRE(board.at("red", 4, 1));
	CHECK(board.at("red", 4, 1)->lookName() == "king");
	CHECK(board.at("white", 3, 2));
	CHECK(board.cursor().lookName() == "whitefree");
	CHECK_FALSE(board.cursor().pickChain);
}

TEST_CASE("checkers.xml: taking the last piece wins", "[checkers]")
{
	Board board;
	board.only({ { "red", 3, 6 }, { "white", 4, 5 } });
	board.space(3, 6);
	board.space(5, 4);
	REQUIRE(board.state() == "over");
	CHECK(board.get("winner").lookName() == "red");
	board.tap(KeyCode::Space);
	CHECK(board.state() == "title");
	board.tap(KeyCode::Space);
	board.frames(2);
	CHECK(board.count("red") == 12);
	CHECK(board.count("white") == 12);
	CHECK(board.cursor().lookName() == "redfree");
}

TEST_CASE("checkers.xml: a side with no move on its turn has lost", "[checkers]")
{
	Board board;
	// White's one man is hemmed in on the edge: two deep on its only way.
	board.only({ { "red", 7, 2 }, { "red", 6, 3 }, { "red", 1, 8 }, { "white", 8, 1 } });
	CHECK(board.sum("white") == 0.0f);
	CHECK(board.state() == "playing");
	board.space(1, 8);
	board.space(2, 7);
	REQUIRE(board.state() == "over");
	CHECK(board.get("winner").lookName() == "red");
}

TEST_CASE("checkers.xml: against the computer, white answers red's move", "[checkers][think]")
{
	Board board(true);
	REQUIRE(board.get("match").variable.at("players") == 1.0f);
	board.space(3, 6);
	board.space(4, 5);
	REQUIRE(board.cursor().variable.at("turn") == 2.0f);
	board.frames(1);
	CHECK(board.cursor().lookName() == "cpufree");
	// Your keys do nothing on its turn.
	board.space(2, 3);
	CHECK(board.cursor().lookName() == "cpufree");
	board.untilRed();
	CHECK(board.cursor().variable.at("turn") == 1.0f);
	CHECK(board.cursor().lookName() == "redfree");
	CHECK(board.count("white") == 12);
	CHECK(board.count("red") == 12);
	// One white man has stepped down off the third row.
	int moved = 0;
	for (int column = 1; column <= 8; ++column) { moved += board.at("white", column, 4) ? 1 : 0; }
	CHECK(moved == 1);
}

TEST_CASE("checkers.xml: the computer does not step where it would be taken", "[checkers][think]")
{
	Board board(true);
	// White's man on (2,3) can go to (1,4), or to (3,4) where red's man on
	// (4,5) would take it; white's other man is hemmed in.
	board.only({ { "red", 4, 5 }, { "red", 7, 8 }, { "red", 7, 2 }, { "red", 6, 3 }, { "white", 2, 3 }, { "white", 8, 1 } });
	board.space(7, 8);
	board.space(6, 7);
	board.untilRed();
	REQUIRE(board.cursor().variable.at("turn") == 1.0f);
	CHECK(board.at("white", 1, 4));
	CHECK_FALSE(board.at("white", 3, 4));
}

TEST_CASE("checkers.xml: the computer takes, and finishes a run of jumps to win", "[checkers][think]")
{
	Board board(true);
	board.only({ { "red", 6, 3 }, { "red", 4, 5 }, { "red", 1, 8 }, { "white", 7, 2 }, { "white", 8, 1 } });
	board.space(1, 8);
	board.space(2, 7);
	// Over (6,3), (4,5) and the man that just left (1,8): all three, and
	// the game.
	board.untilRed();
	CHECK(board.at("white", 1, 8));
	CHECK(board.count("red") == 0);
	REQUIRE(board.state() == "over");
	CHECK(board.get("winner").lookName() == "white");
}

TEST_CASE("checkers.xml: with two players the computer leaves white alone", "[checkers][think]")
{
	Board board;
	REQUIRE(board.get("match").variable.at("players") == 2.0f);
	board.space(3, 6);
	board.space(4, 5);
	board.frames(300);
	CHECK(board.cursor().variable.at("turn") == 2.0f);
	CHECK(board.cursor().lookName() == "whitefree");
}

namespace
{
	// The cells holding a copy of that mark.
	std::vector<std::pair<int, int>> marksOf(Played& board, const std::string& mark)
	{
		std::vector<std::pair<int, int>> cells;
		for (const Object& object : board.game.getCurrentObjects())
		{
			if (object.exists && object.copyOf.find(mark) != std::string::npos) { cells.emplace_back(cell(object.cellColumn), cell(object.cellRow)); }
		}
		std::sort(cells.begin(), cells.end());
		return cells;
	}

	using Cells = std::vector<std::pair<int, int>>;
}

TEST_CASE("checkers.xml: the piece held is ringed, where it can go dotted, and the last move cornered", "[checkers][marks]")
{
	Board board;
	board.space(3, 6);
	board.frames(1);
	CHECK(marksOf(board, "heldmark") == Cells{ { 3, 6 } });
	CHECK(marksOf(board, "movemark") == Cells{ { 2, 5 }, { 4, 5 } });
	board.space(4, 5);
	board.frames(1);
	CHECK(marksOf(board, "heldmark").empty());
	CHECK(marksOf(board, "movemark").empty());
	CHECK(marksOf(board, "lastmark") == Cells{ { 3, 6 }, { 4, 5 } });
}

TEST_CASE("checkers.xml: the options turn the marks for moves and for pieces that can move", "[checkers][marks]")
{
	Board board;
	board.game.setVariable("options", "moves", 0.0f);
	board.game.setVariable("options", "movable", 1.0f);
	board.frames(2);
	CHECK(marksOf(board, "movablemark") == Cells{ { 1, 6 }, { 3, 6 }, { 5, 6 }, { 7, 6 } });
	board.space(3, 6);
	board.frames(1);
	CHECK(marksOf(board, "movablemark").empty());
	CHECK(marksOf(board, "movemark").empty());
	CHECK(marksOf(board, "heldmark") == Cells{ { 3, 6 } });
}

TEST_CASE("checkers.xml: X puts the piece held back, but not in a run of jumps", "[checkers]")
{
	Board board;
	board.space(3, 6);
	REQUIRE(board.cursor().lookName() == "redheld");
	board.tap(KeyCode::X);
	CHECK(board.cursor().lookName() == "redfree");
	CHECK(board.cursor().picked.empty());
	CHECK(board.at("red", 3, 6));

	Board chain;
	chain.only({ { "red", 2, 7 }, { "white", 3, 6 }, { "white", 5, 4 }, { "white", 8, 1 } });
	chain.space(2, 7);
	chain.space(4, 5);
	REQUIRE(chain.cursor().pickChain);
	chain.tap(KeyCode::Backspace);
	CHECK(chain.cursor().lookName() == "redheld");
	CHECK_FALSE(chain.cursor().picked.empty());
}

TEST_CASE("checkers.xml: with the option on, the only piece that can move is picked up for you", "[checkers]")
{
	Board board;
	board.game.setVariable("options", "jumper", 1.0f);
	// Red has two men but only one can move: it jumps.
	board.only({ { "red", 4, 5 }, { "red", 1, 8 }, { "red", 2, 7 }, { "white", 5, 4 }, { "white", 8, 1 } });
	board.frames(2);
	CHECK(board.cursor().lookName() == "redheld");
	CHECK(board.cursor().picked == board.at("red", 4, 5)->name);
	board.space(6, 3);
	CHECK(board.at("red", 6, 3));
	CHECK(board.count("white") == 1);
}

TEST_CASE("checkers.xml: the cursor follows the mouse, and a click picks up and puts down", "[checkers][mouse]")
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
	// The middle of the square in column 3, row 6, then of column 4, row 5.
	mouse(160.0f + 60.0f * 2.5f, 80.0f + 60.0f * 5.5f, false);
	CHECK(cell(board.cursor().cellColumn) == 3);
	CHECK(cell(board.cursor().cellRow) == 6);
	mouse(160.0f + 60.0f * 2.5f, 80.0f + 60.0f * 5.5f, true);
	CHECK(board.cursor().lookName() == "redheld");
	mouse(160.0f + 60.0f * 3.5f, 80.0f + 60.0f * 4.5f, true);
	CHECK(board.at("red", 4, 5));
	CHECK(board.cursor().lookName() == "whitefree");
	// Off the board, the cursor stays where it was.
	mouse(20.0f, 20.0f, false);
	CHECK(cell(board.cursor().cellColumn) == 4);
	CHECK(cell(board.cursor().cellRow) == 5);
}
