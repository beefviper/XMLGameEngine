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

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

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

namespace
{
	// The numbers on Doubles' board, by cell (0 for none).
	std::map<std::pair<int, int>, int> numbers(Played& play)
	{
		std::map<std::pair<int, int>, int> found;
		for (const Object& object : play.game.getCurrentObjects())
		{
			if (object.exists && object.isCopy() && object.objClass == "tile") { found[cellOf(object)] = static_cast<int>(object.variable.at("value")); }
		}
		return found;
	}
}

TEST_CASE("Doubles: two numbers to begin, and a slide that moves brings a new one", "[puzzles][doubles]")
{
	Played play("descriptions/doubles.xml");
	play.tap(KeyCode::Space);
	play.frames(2);
	REQUIRE(numbers(play).size() == 2);
	// Some way moves something: four tries, the board grows by one each time it did.
	std::size_t before = numbers(play).size();
	for (const KeyCode key : { KeyCode::A, KeyCode::D, KeyCode::W, KeyCode::S })
	{
		play.tap(key);
		play.frames(1);
		const std::size_t now = numbers(play).size();
		CHECK((now == before + 1 || now == before || now == before - 1));
		before = now;
	}
	CHECK(play.state() == "playing");
}

namespace
{
	// A row of numbers (t a 2, f a 4) that Space slides left.
	std::string numberRow(const std::string& row)
	{
		const std::string number = "<sprite><rectangle><width>10</width><height>10</height></rectangle></sprite>";
		return "<object name=\"holder\"><actions><action name=\"left\"><slide class=\"tile\" direction=\"left\" /><inc variable=\"holder.moved\" /></action></actions>"
			"<variables><variable name=\"moved\">0</variable><variable name=\"score\">0</variable></variables></object>"
			"<map name=\"board\"><tile><width>10</width><height>10</height></tile><position><x>0</x><y>0</y></position><cells /><row>" + row + "</row>"
			"<group name=\"tiles\" class=\"tile\">" + number +
			"<collisions><collision class=\"tile\"><merge variable=\"value\" /><inc variable=\"holder.score\">self.value</inc></collision></collisions>"
			"<variables><variable name=\"value\">2</variable></variables>"
			"<group name=\"twos\" char=\"t\" /><group name=\"fours\" char=\"f\"><variables><variable name=\"value\">4</variable></variables></group></group></map>";
	}

	// Each number left on the row, from the left (0 for an empty cell).
	std::vector<int> rowOf(Played& play, int width)
	{
		std::vector<int> row(static_cast<std::size_t>(width), 0);
		for (const Object& object : play.game.getCurrentObjects())
		{
			if (object.exists && object.objClass == "tile") { row[static_cast<std::size_t>(object.cellColumn) - 1] = static_cast<int>(object.variable.at("value")); }
		}
		return row;
	}
}

TEST_CASE("<merge>: equal numbers that meet in a slide add up, each once a slide", "[puzzles][merge]")
{
	SECTION("four twos make two fours, not an eight")
	{
		played::Scratch file(numberRow("tttt"), "<trigger object=\"holder\" action=\"left\" />");
		Played play(file.path);
		play.tap(KeyCode::Space);
		play.frames(1);
		CHECK(rowOf(play, 4) == std::vector<int>{ 4, 4, 0, 0 });
	}
	SECTION("a four made does not take the four beside it")
	{
		played::Scratch file(numberRow("ttf."), "<trigger object=\"holder\" action=\"left\" />");
		Played play(file.path);
		play.tap(KeyCode::Space);
		play.frames(1);
		CHECK(rowOf(play, 4) == std::vector<int>{ 4, 4, 0, 0 });
		play.tap(KeyCode::Space);
		play.frames(1);
		CHECK(rowOf(play, 4) == std::vector<int>{ 8, 0, 0, 0 });
	}
	SECTION("unequal numbers only close up")
	{
		played::Scratch file(numberRow(".t.f"), "<trigger object=\"holder\" action=\"left\" />");
		Played play(file.path);
		play.tap(KeyCode::Space);
		play.frames(1);
		CHECK(rowOf(play, 4) == std::vector<int>{ 2, 4, 0, 0 });
	}
}

TEST_CASE("<slide class> that moves nothing is refused, so what follows it does not run", "[puzzles][merge]")
{
	played::Scratch file(numberRow("tf.."), "<trigger object=\"holder\" action=\"left\" />");
	Played play(file.path);
	play.tap(KeyCode::Space);
	play.frames(1);
	CHECK(rowOf(play, 4) == std::vector<int>{ 2, 4, 0, 0 });
	CHECK(play.get("holder").variable.at("moved") == 0.0f);
}

namespace
{
	// Minefield's mines and squares, by cell.
	struct Field
	{
		std::set<std::pair<int, int>> mines;
		std::map<std::pair<int, int>, const Object*> squares;

		explicit Field(Played& play)
		{
			for (const Object& object : play.game.getCurrentObjects())
			{
				if (!object.exists || !object.isVisible) { continue; }
				if (object.objClass == "mine") { mines.insert(cellOf(object)); }
				if (object.objClass == "square") { squares[cellOf(object)] = &object; }
			}
		}

		int around(std::pair<int, int> cell) const
		{
			int count = 0;
			for (int down = -1; down <= 1; ++down)
			{
				for (int across = -1; across <= 1; ++across)
				{
					if ((across != 0 || down != 0) && mines.count({ cell.first + across, cell.second + down })) { ++count; }
				}
			}
			return count;
		}
	};
}

TEST_CASE("Minefield: ten mines are laid, and an opened square counts the mines round it", "[puzzles][minefield]")
{
	Played play("descriptions/minefield.xml");
	play.tap(KeyCode::Space);
	play.frames(12);
	REQUIRE(play.state() == "playing");
	Field before(play);
	REQUIRE(before.mines.size() == 10);
	REQUIRE(before.squares.size() == 81);

	// The cursor starts in the middle: open there.
	play.tap(KeyCode::Space);
	play.frames(1);
	Field after(play);
	const Object& middle = *after.squares.at({ 5, 5 });
	CHECK(middle.variable.at("open") == 1.0f);
	if (after.mines.count({ 5, 5 }))
	{
		CHECK(middle.lookName() == "boom");
		CHECK(play.state() == "lost");
		return;
	}
	CHECK(middle.variable.at("near") == static_cast<float>(after.around({ 5, 5 })));

	// The flood: every open square shows its own count, an open square with
	// none round it has every neighbour open, and no mine is opened.
	int opened = 0;
	for (const auto& [cell, square] : after.squares)
	{
		if (square->variable.at("open") != 1.0f) { continue; }
		++opened;
		INFO("square " << cell.first << ", " << cell.second);
		CHECK_FALSE(after.mines.count(cell));
		CHECK(square->variable.at("near") == static_cast<float>(after.around(cell)));
		CHECK(square->lookName() == "n" + std::to_string(after.around(cell)));
		if (after.around(cell) != 0) { continue; }
		for (int down = -1; down <= 1; ++down)
		{
			for (int across = -1; across <= 1; ++across)
			{
				const auto next = after.squares.find({ cell.first + across, cell.second + down });
				if (next != after.squares.end()) { CHECK(next->second->variable.at("open") == 1.0f); }
			}
		}
	}
	CHECK(play.get("status").variable.at("opened") == static_cast<float>(opened));
}

TEST_CASE("Minefield: a flag goes on and off, and a flagged square does not open", "[puzzles][minefield]")
{
	Played play("descriptions/minefield.xml");
	play.tap(KeyCode::Space);
	play.frames(12);
	play.tap(KeyCode::F);
	play.frames(1);
	const Object& middle = *Field(play).squares.at({ 5, 5 });
	CHECK(middle.lookName() == "flag");
	CHECK(play.get("status").variable.at("flags") == 1.0f);
	play.tap(KeyCode::Space);
	play.frames(1);
	CHECK(middle.lookName() == "flag");
	CHECK(play.get("status").variable.at("opened") == 0.0f);
	play.tap(KeyCode::F);
	play.frames(1);
	CHECK(middle.lookName() == "closed");
	CHECK(play.get("status").variable.at("flags") == 0.0f);
}

TEST_CASE("Doubles: every number shows its value, and slides only add equal ones", "[puzzles][doubles]")
{
	Played play("descriptions/doubles.xml");
	play.tap(KeyCode::Space);
	play.frames(2);
	const KeyCode keys[] = { KeyCode::A, KeyCode::W, KeyCode::D, KeyCode::S, KeyCode::A, KeyCode::A, KeyCode::S, KeyCode::D };
	for (int turn = 0; turn < 120 && play.state() == "playing"; ++turn)
	{
		const auto before = numbers(play);
		int sumBefore = 0;
		for (const auto& [cell, number] : before) { sumBefore += number; }
		play.tap(keys[turn % 8]);
		play.frames(1);
		const auto after = numbers(play);
		int sumAfter = 0;
		for (const auto& [cell, number] : after) { sumAfter += number; }
		INFO("turn " << turn);
		// Nothing made or lost but the new 2.
		CHECK((sumAfter == sumBefore || sumAfter == sumBefore + 2));
		for (const Object& object : play.game.getCurrentObjects())
		{
			if (!object.exists || !object.isCopy() || object.objClass != "tile") { continue; }
			INFO(object.name);
			CHECK(object.picture.text == std::to_string(static_cast<int>(object.variable.at("value"))));
		}
	}
}
