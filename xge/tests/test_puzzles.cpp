// test_puzzles.cpp
// XML Game Engine
// author: beefviper
// date: Nov 9, 2026
//
// Catch2 tests for the turn puzzles on cells: <shove />, <merge>, a
// <slide class>, <count> and <trigger> about the cells round a piece,
// conditions in actions and <undo />. Crates
// (games/descriptions/crates.xml), Doubles (doubles.xml) and Minefield
// (minefield.xml) are played.

#include "played.h"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <utility>

using namespace xge;
using played::Played;

namespace
{
	// A piece of a room, by its name in the room (worker, board.crates.1).
	Object& inRoom(Played& play, const std::string& room, const std::string& name)
	{
		return play.get("playing." + room + "." + name);
	}

	std::pair<int, int> cellOf(const Object& piece)
	{
		return { static_cast<int>(piece.cellColumn), static_cast<int>(piece.cellRow) };
	}
}

TEST_CASE("Crates: the worker shoves a crate along, and a crate on its spot finishes the room", "[puzzles][crates]")
{
	Played play("descriptions/crates.xml");
	play.tap(KeyCode::Space);
	play.frames(1);
	REQUIRE(play.state() == "playing");

	Object& worker = inRoom(play, "room1", "worker");
	CHECK(cellOf(worker) == std::pair{ 3, 3 });
	play.tap(KeyCode::D);
	play.frames(1);
	CHECK(cellOf(worker) == std::pair{ 4, 3 });
	CHECK(cellOf(inRoom(play, "room1", "board.crates.1")) == std::pair{ 5, 3 });
	CHECK(play.get("status").variable.at("moves") == 1.0f);

	play.tap(KeyCode::D);
	play.frames(1);
	CHECK(inRoom(play, "room1", "board.crates.1").lookName() == "crate");
	play.tap(KeyCode::D);
	play.frames(2);
	CHECK(cellOf(inRoom(play, "room1", "board.crates.1")) == std::pair{ 7, 3 });
	CHECK(inRoom(play, "room1", "board.crates.1").lookName() == "home");
	CHECK(play.get("status").variable.at("room") == 2.0f);
	CHECK(play.get("status").variable.at("moves") == 0.0f);
}

TEST_CASE("Crates: a crate against a wall stays, and so does the worker", "[puzzles][crates]")
{
	Played play("descriptions/crates.xml");
	play.tap(KeyCode::Space);
	play.frames(1);
	Object& worker = inRoom(play, "room1", "worker");
	for (int i = 0; i < 6; ++i) { play.tap(KeyCode::A); play.frames(1); }
	// Room 1 is one crate to the right; walk round it and push it left.
	play.tap(KeyCode::W); play.frames(1);
	for (int i = 0; i < 3; ++i) { play.tap(KeyCode::D); play.frames(1); }
	play.tap(KeyCode::S); play.frames(1);
	const auto crate = cellOf(inRoom(play, "room1", "board.crates.1"));
	for (int i = 0; i < 5; ++i) { play.tap(KeyCode::A); play.frames(1); }
	CHECK(cellOf(inRoom(play, "room1", "board.crates.1")) == std::pair{ 2, 3 });
	CHECK(cellOf(worker) == std::pair{ 3, 3 });
	CHECK(crate == std::pair{ 4, 3 });
}

TEST_CASE("Crates: <undo /> takes back moves one at a time, the count with them", "[puzzles][crates][undo]")
{
	Played play("descriptions/crates.xml");
	play.tap(KeyCode::Space);
	play.frames(1);
	Object& worker = inRoom(play, "room1", "worker");
	play.tap(KeyCode::D); play.frames(1);
	play.tap(KeyCode::S); play.frames(1);
	// A walk into the wall is no move, so nothing is kept for it.
	play.tap(KeyCode::S); play.frames(1);
	CHECK(play.get("status").variable.at("moves") == 2.0f);

	play.tap(KeyCode::U); play.frames(1);
	CHECK(cellOf(worker) == std::pair{ 4, 3 });
	CHECK(play.get("status").variable.at("moves") == 1.0f);
	play.tap(KeyCode::U); play.frames(1);
	CHECK(cellOf(worker) == std::pair{ 3, 3 });
	CHECK(cellOf(inRoom(play, "room1", "board.crates.1")) == std::pair{ 4, 3 });
	CHECK(play.get("status").variable.at("moves") == 0.0f);
	// Nothing left: refused, nothing changes.
	play.tap(KeyCode::U); play.frames(1);
	CHECK(cellOf(worker) == std::pair{ 3, 3 });
}

TEST_CASE("Crates: every room can be finished, and the last one wins the game", "[puzzles][crates]")
{
	Played play("descriptions/crates.xml");
	play.tap(KeyCode::Space);
	play.frames(1);
	// Each room's shortest answer, found by a search over its rows.
	const std::string answers[] = { "rrr", "luuldduurrrrdd", "uuruldrrrurrdulldrrdru", "luulrddlluulurdrrrrrurd" };
	for (std::size_t room = 0; room < 4; ++room)
	{
		INFO("room " << room + 1);
		CHECK(play.get("status").variable.at("room") == static_cast<float>(room + 1));
		for (const char way : answers[room])
		{
			play.tap(way == 'u' ? KeyCode::W : way == 'd' ? KeyCode::S : way == 'l' ? KeyCode::A : KeyCode::D);
			play.frames(1);
		}
		play.frames(2);
	}
	CHECK(play.state() == "won");
}
