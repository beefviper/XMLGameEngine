// test_backgammon.cpp
// XML Game Engine
// author: beefviper
// date: Nov 22, 2026
//
// Catch2 tests for backgammon.xml and what it brought: cells several tiles
// tall that stack their pieces, a <route> moved along by <steps> of the
// dice, <hit>, coming in from the start first, bearing off at the end,
// a turn using as many dice as it can, the live number `along`, and the
// game's turns, cube and points.

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
	int whole(float number) { return static_cast<int>(std::lround(number)); }

	using Cell = std::pair<int, int>;

	// Red's points, 1 to 24, as cells of the board (white's point p is red's 25 - p).
	Cell redPoint(int point)
	{
		if (point <= 6) { return { 14 - point, 2 }; }
		if (point <= 12) { return { 13 - point, 2 }; }
		if (point <= 18) { return { point - 12, 1 }; }
		return { point - 11, 1 };
	}
	Cell whitePoint(int point) { return redPoint(25 - point); }
	const Cell kRedBar{ 7, 2 };
	const Cell kWhiteBar{ 7, 1 };
	const Cell kRedTray{ 14, 1 };
	const Cell kWhiteTray{ 14, 2 };

	struct Board : Played
	{
		// Two players at the keyboard, unless against the computer.
		explicit Board(bool computer = false) : Played("descriptions/backgammon.xml")
		{
			tap(computer ? KeyCode::Space : KeyCode::Num2);
			frames(2);
		}

		std::vector<Object*> side(const std::string& objClass)
		{
			std::vector<Object*> pieces;
			for (Object& object : game.getCurrentObjects())
			{
				if (object.objClass == objClass && object.onCells && object.exists) { pieces.push_back(&object); }
			}
			return pieces;
		}

		int count(const std::string& objClass, Cell cell)
		{
			int many = 0;
			for (Object* piece : side(objClass))
			{
				if (whole(game.place(*piece).column) == cell.first && whole(game.place(*piece).row) == cell.second) { ++many; }
			}
			return many;
		}

		// Every checker of a side put where the list says, the rest on its tray.
		void arrange(const std::string& objClass, const std::vector<Cell>& cells, Cell tray)
		{
			std::size_t next = 0;
			for (Object* piece : side(objClass))
			{
				const Cell cell = next < cells.size() ? cells[next] : tray;
				game.cellMaps().putInCell(*piece, cell.first, cell.second);
				++next;
			}
			frames(1);
		}

		// The player on turn rolls, and the dice are made to say these.
		void roll(const std::vector<int>& faces)
		{
			tap(KeyCode::Space);
			frames(1);
			showing(faces);
		}

		// The dice to play made to say these.
		void showing(const std::vector<int>& faces)
		{
			game.holders().sendHeld(Name::of("rolled"), Name::of("used"), 0, true, {});
			for (const int face : faces)
			{
				Object* die = game.holders().heldIn(Name::of("used")).back();
				game.holders().sendItem(game.itemOf(*die).number, Name::of("used"), Name::of("rolled"));
				game.setVariable(die->name, "worth", static_cast<float>(face));
			}
			frames(1);
		}

		int diceLeft() { return static_cast<int>(game.holders().heldIn(Name::of("rolled")).size()); }

		void to(Cell cell)
		{
			Object& cursor = get("cursor");
			for (int i = 0; i < 20 && whole(game.place(cursor).column) < cell.first; ++i) { tap(KeyCode::D); frames(1); }
			for (int i = 0; i < 20 && whole(game.place(cursor).column) > cell.first; ++i) { tap(KeyCode::A); frames(1); }
			for (int i = 0; i < 4 && whole(game.place(cursor).row) < cell.second; ++i) { tap(KeyCode::S); frames(1); }
			for (int i = 0; i < 4 && whole(game.place(cursor).row) > cell.second; ++i) { tap(KeyCode::W); frames(1); }
		}

		void space()
		{
			tap(KeyCode::Space);
			frames(1);
		}

		// A checker from one cell to another: picked up, and put down.
		void move(Cell from, Cell into)
		{
			to(from);
			space();
			to(into);
			space();
		}

		bool turnOf(const std::string& member) { return get(member).variable.valueOr("turn", 0.0f) == 1.0f; }
		std::string phase() { return get("table").lookName().str(); }
		float along(Cell cell, const std::string& objClass)
		{
			for (Object* piece : side(objClass))
			{
				if (whole(game.place(*piece).column) == cell.first && whole(game.place(*piece).row) == cell.second) { return game.pieceMoves(*piece).along; }
			}
			return -2.0f;
		}
	};
}

TEST_CASE("backgammon: the board starts as its rows show, a checker each tile of a tall cell", "[backgammon]")
{
	Board play;
	REQUIRE(play.side("red").size() == 15);
	REQUIRE(play.side("white").size() == 15);
	REQUIRE(play.count("red", redPoint(1)) == 2);
	REQUIRE(play.count("red", redPoint(12)) == 5);
	REQUIRE(play.count("red", redPoint(17)) == 3);
	REQUIRE(play.count("red", redPoint(19)) == 5);
	REQUIRE(play.count("white", whitePoint(1)) == 2);
	REQUIRE(play.count("white", whitePoint(19)) == 5);
	REQUIRE(play.along(redPoint(19), "red") == 19);
	REQUIRE(play.along(whitePoint(19), "white") == 19);

	// A stack: each checker a tile further in from the edge.
	std::vector<float> ys;
	for (Object* piece : play.side("red"))
	{
		if (whole(play.game.place(*piece).column) == redPoint(12).first && whole(play.game.place(*piece).row) == 2) { ys.push_back(piece->position.y); }
	}
	std::sort(ys.begin(), ys.end());
	REQUIRE(ys.size() == 5);
	for (std::size_t i = 1; i < ys.size(); ++i) { REQUIRE(ys[i] - ys[i - 1] == 40.0f); }
	REQUIRE(ys.back() + play.side("red").front()->size.y <= 90 + 480);
}

TEST_CASE("backgammon: a checker goes as far as a die, which is then played; a closed point stops it", "[backgammon]")
{
	Board play;
	REQUIRE(play.turnOf("redside"));
	play.roll({ 3, 5 });
	REQUIRE(play.phase() == "moving");

	// Five on from red's 1 is white's 6: closed.
	play.move(redPoint(1), redPoint(6));
	REQUIRE(play.count("red", redPoint(1)) == 2);
	play.tap(KeyCode::X);
	play.frames(1);

	play.move(redPoint(1), redPoint(4));
	REQUIRE(play.count("red", redPoint(4)) == 1);
	REQUIRE(play.count("red", redPoint(1)) == 1);
	REQUIRE(play.diceLeft() == 1);
	REQUIRE(play.along(redPoint(4), "red") == 4);

	play.move(redPoint(12), redPoint(17));
	REQUIRE(play.count("red", redPoint(17)) == 4);
	// Both dice played: white's turn, to roll.
	play.frames(1);
	REQUIRE(play.turnOf("whiteside"));
	REQUIRE(play.phase() == "rolling");
}

TEST_CASE("backgammon: a lone checker is hit to the bar, and comes in before anything else moves", "[backgammon]")
{
	Board play;
	play.roll({ 3, 1 });
	play.move(redPoint(1), redPoint(4));
	play.move(redPoint(1), redPoint(2));
	play.frames(1);
	REQUIRE(play.turnOf("whiteside"));

	// White's 21 is red's 4: a lone red there, hit.
	play.roll({ 2, 6 });
	play.move(whitePoint(19), whitePoint(21));
	REQUIRE(play.count("red", kRedBar) == 1);
	REQUIRE(play.count("white", whitePoint(21)) == 1);
	REQUIRE(play.along(kRedBar, "red") == 0);
	play.move(whitePoint(12), whitePoint(18));
	play.frames(1);
	REQUIRE(play.turnOf("redside"));

	// Red must bring it in first: nothing else may move.
	play.roll({ 5, 4 });
	play.move(redPoint(12), redPoint(17));
	REQUIRE(play.count("red", redPoint(12)) == 5);
	play.tap(KeyCode::X);
	play.frames(1);
	// In by four onto white's 21: red's 4, hitting the white there.
	play.move(kRedBar, redPoint(4));
	REQUIRE(play.count("red", redPoint(4)) == 1);
	REQUIRE(play.count("white", kWhiteBar) == 1);
}

TEST_CASE("backgammon: no move with the dice left: the turn is stuck until Space passes", "[backgammon]")
{
	Board play;
	// Red on the bar; white holds all six points it could come in on.
	play.arrange("red", { kRedBar, redPoint(19), redPoint(19) }, kRedTray);
	play.arrange("white", { whitePoint(19), whitePoint(19), whitePoint(20), whitePoint(20), whitePoint(21), whitePoint(21),
		whitePoint(22), whitePoint(22), whitePoint(23), whitePoint(23), whitePoint(24), whitePoint(24) }, kWhiteTray);
	play.roll({ 3, 5 });
	play.frames(1);
	REQUIRE(play.phase() == "stuck");
	play.space();
	play.frames(1);
	REQUIRE(play.turnOf("whiteside"));
	REQUIRE(play.phase() == "rolling");
}

TEST_CASE("backgammon: as many dice as can be played must be, the bigger when only one fits", "[backgammon]")
{
	Board play;
	// One red on its 1; white holds red's 4 and 8: 3 then 5 is stopped
	// at once, 5 then 3 is not, so the 3 may not be played first.
	play.arrange("red", { redPoint(1) }, kRedTray);
	play.arrange("white", { redPoint(4), redPoint(4), redPoint(8), redPoint(8) }, kWhiteTray);
	play.roll({ 3, 5 });
	// Taking the 3 first would end on 4: closed anyway. 5 first lands on 6, then 3 to 9.
	play.move(redPoint(1), redPoint(6));
	REQUIRE(play.count("red", redPoint(6)) == 1);

	Board alone;
	// Only one die can be played: 6 would reach 7, 2 reaches 3; after either
	// the other is closed. The 6 must be played.
	alone.arrange("red", { redPoint(1) }, kRedTray);
	alone.arrange("white", { redPoint(9), redPoint(9), redPoint(5), redPoint(5) }, kWhiteTray);
	alone.roll({ 6, 2 });
	alone.move(redPoint(1), redPoint(3));
	REQUIRE(alone.count("red", redPoint(3)) == 0);
	alone.tap(KeyCode::X);
	alone.frames(1);
	alone.move(redPoint(1), redPoint(7));
	REQUIRE(alone.count("red", redPoint(7)) == 1);
}

TEST_CASE("backgammon: bearing off once all are home, past the end only from the furthest back", "[backgammon]")
{
	Board play;
	play.arrange("red", { redPoint(18), redPoint(22), redPoint(23) }, kRedTray);
	// White: one in red's last six, the rest far away.
	play.arrange("white", { whitePoint(5) }, whitePoint(12));
	play.roll({ 2, 6 });
	// Not yet: the one on 18 is outside the last six.
	play.move(redPoint(23), kRedTray);
	REQUIRE(play.count("red", redPoint(23)) == 1);
	play.tap(KeyCode::X);
	play.frames(1);
	play.move(redPoint(18), redPoint(24));
	// Now home: 2 bears off from 23 exactly.
	play.move(redPoint(23), kRedTray);
	REQUIRE(play.count("red", kRedTray) == 13);
	play.frames(1);
	REQUIRE(play.turnOf("whiteside"));

	play.roll({ 1, 2 });
	play.move(whitePoint(12), whitePoint(14));
	play.move(whitePoint(14), whitePoint(15));
	play.frames(1);

	// Checkers on 22 and 24: past the end only from 22, the furthest back,
	// and then from 24.
	play.roll({ 6, 5 });
	play.move(redPoint(24), kRedTray);
	REQUIRE(play.count("red", redPoint(24)) == 1);
	play.tap(KeyCode::X);
	play.frames(1);
	play.move(redPoint(22), kRedTray);
	REQUIRE(play.count("red", redPoint(22)) == 0);
	play.move(redPoint(24), kRedTray);
	play.frames(2);
	REQUIRE(play.state() == "over");
	// White has borne off none, and has one in red's last six: a backgammon.
	REQUIRE(play.get("match").variable.at("won") == 3);
	REQUIRE(play.get("score").variable.at("red") == 3);
}

TEST_CASE("backgammon: the cube doubled and taken, then doubled and dropped", "[backgammon]")
{
	Board play;
	play.tap(KeyCode::C);
	play.frames(2);
	REQUIRE(play.state() == "offered");
	play.tap(KeyCode::Y);
	play.frames(1);
	REQUIRE(play.state() == "playing");
	REQUIRE(play.get("cube").variable.at("value") == 2);
	REQUIRE(play.get("cube").variable.at("owner") == 2);

	// Red may not double again: the cube is white's.
	play.tap(KeyCode::C);
	play.frames(2);
	REQUIRE(play.state() == "playing");

	play.roll({ 3, 1 });
	play.move(redPoint(17), redPoint(20));
	play.move(redPoint(19), redPoint(20));
	play.frames(1);
	REQUIRE(play.turnOf("whiteside"));
	play.tap(KeyCode::C);
	play.frames(2);
	REQUIRE(play.state() == "offered");
	play.tap(KeyCode::N);
	play.frames(1);
	REQUIRE(play.state() == "over");
	REQUIRE(play.get("match").variable.at("winner") == 1);
	REQUIRE(play.get("score").variable.at("white") == 2);
}

TEST_CASE("backgammon: the computer plays white's turn, rolling, moving and passing it back", "[backgammon]")
{
	Board play(true);
	play.roll({ 3, 1 });
	play.move(redPoint(17), redPoint(20));
	play.move(redPoint(19), redPoint(20));
	REQUIRE(play.turnOf("whiteside"));

	const auto whereWhite = [&]
	{
		std::vector<Cell> cells;
		for (Object* piece : play.side("white")) { cells.emplace_back(whole(play.game.place(*piece).column), whole(play.game.place(*piece).row)); }
		std::sort(cells.begin(), cells.end());
		return cells;
	};
	const std::vector<Cell> before = whereWhite();
	for (int i = 0; i < 1200 && !play.turnOf("redside"); ++i) { play.frames(1); }
	REQUIRE(play.turnOf("redside"));
	REQUIRE(play.phase() == "rolling");
	REQUIRE(whereWhite() != before);
}

TEST_CASE("backgammon: the computer's move hits a lone checker", "[backgammon]")
{
	Board play;
	// Without the red checker there, white's best three is from its 8.
	play.arrange("red", { redPoint(12) }, kRedTray);
	play.arrange("white", { whitePoint(8), whitePoint(10) }, kWhiteTray);
	play.showing({ 3 });
	std::vector<std::pair<int, int>> cells;
	REQUIRE(play.game.cellMaps().routePlan(Name::of("white"), cells));
	REQUIRE(cells.size() == 2);
	REQUIRE(cells[0] == whitePoint(10));
	REQUIRE(cells[1] == whitePoint(13));

	// With nothing rolled there is nothing to move.
	play.showing({});
	REQUIRE(play.game.cellMaps().routePlan(Name::of("white"), cells));
	REQUIRE(cells.empty());
}
