// test_store.cpp
// XML Game Engine
// author: beefviper
// date: Nov 17, 2026
//
// An object's handle (ObjectId): found while it is in the game, nothing
// once it is gone, and a slot used again is a generation on.

#include "played.h"
#include "game/store.h"

#include <catch2/catch_test_macros.hpp>

#include <set>
#include <string>
#include <vector>

using namespace xge;

namespace
{
	// A shot going up from the ship, gone at the top.
	const std::string kShot = "<object name=\"shot\" class=\"bullet\"><sprite><rectangle><width>4</width><height>8</height></rectangle></sprite>"
		"<velocity><x>0</x><y>-300</y></velocity>"
		"<collisions><collision edge=\"top\"><despawn /></collision></collisions></object>";
}

TEST_CASE("an object's handle finds it until it is despawned, and a slot used again is a generation on", "[store]")
{
	const played::Scratch file(played::box("ship", 300, 400),
		"<spawn object=\"shot\"><position><x>ship.x</x><y>ship.y</y></position></spawn>", {}, kShot);
	played::Played play(file.path);

	// The file's objects have handles from the start.
	Object& ship = play.get("ship");
	REQUIRE(ship.id);
	CHECK(play.game.find(ship.id) == &ship);
	CHECK(play.game.find(ObjectId{}) == nullptr);

	// Found as it is spawned, and after it joins.
	play.tap(KeyCode::Space);
	REQUIRE(play.game.justSpawned().size() == 1);
	const ObjectId first = play.game.justSpawned().front().id;
	REQUIRE(first);
	REQUIRE(play.game.find(first) != nullptr);
	CHECK(play.game.find(first)->name == "playing.shot.1");
	play.frames(1);
	REQUIRE(play.game.find(first) != nullptr);
	CHECK(play.game.find(first) == &play.get("shot.1"));

	// Gone off the top: nothing.
	play.frames(120);
	CHECK_FALSE(play.has("shot.1"));
	CHECK(play.game.find(first) == nullptr);

	// The next copy takes the freed slot, a generation on; the old handle
	// still finds nothing.
	play.tap(KeyCode::Space);
	REQUIRE(play.game.justSpawned().size() == 1);
	const std::string again = play.game.justSpawned().front().name;
	play.frames(1);
	const ObjectId second = play.game.object(again).id;
	CHECK(second.index == first.index);
	CHECK(second.generation == first.generation + 1);
	CHECK(play.game.find(second) == &play.game.object(again));
	CHECK(play.game.find(first) == nullptr);
	CHECK(play.game.find(ship.id) == &play.get("ship"));
}

TEST_CASE("a store holds a part for only the objects given one, in object order", "[store]")
{
	struct Part { int value{}; };
	Store<Part> store;
	const ObjectId a{ 0, 0 };
	const ObjectId b{ 1, 0 };
	const ObjectId c{ 2, 0 };

	CHECK_FALSE(store.has(a));
	CHECK(store.find(a) == nullptr);
	CHECK_THROWS(store.get(a));

	// Given out of order: walked in object order.
	store.add(c, 3, { 30 });
	store.add(a, 1, { 10 });
	store.add(b, 2, { 20 });
	CHECK(store.size() == 3);
	CHECK(store.get(b).value == 20);
	const auto walked = [&]
	{
		std::vector<int> values;
		store.each([&](ObjectId, Part& part) { values.push_back(part.value); });
		return values;
	};
	CHECK(walked() == std::vector<int>{ 10, 20, 30 });

	// Taken away: the rest stay found and in order.
	store.remove(b);
	CHECK_FALSE(store.has(b));
	CHECK(store.get(c).value == 30);
	CHECK(walked() == std::vector<int>{ 10, 30 });

	// The slot used again a generation on: the old handle finds nothing.
	const ObjectId again{ 1, 1 };
	store.add(again, 4, { 40 });
	CHECK(store.find(b) == nullptr);
	CHECK(store.get(again).value == 40);
	CHECK(walked() == std::vector<int>{ 10, 30, 40 });

	// A copy changes alone.
	Store<Part> copy = store;
	copy.get(a).value = 11;
	CHECK(store.get(a).value == 10);

	// A reset puts back the start that was kept.
	store.keepStart(a);
	store.get(a).value = 99;
	store.restart(a);
	CHECK(store.get(a).value == 10);
	store.get(c).value = 31;
	store.restart(c);   // no start kept: left as it is
	CHECK(store.get(c).value == 31);
}

TEST_CASE("only the objects that hold others have a holder part", "[store]")
{
	// Pong holds nothing: no holder parts at all.
	{
		played::Played pong("descriptions/pong.xml");
		CHECK(pong.game.parts().holders.size() == 0);
		CHECK_FALSE(pong.game.isHolder(pong.get("ball")));
	}

	// Klondike's piles are holders; its cards are not.
	played::Played klondike("descriptions/klondike.xml");
	std::size_t holders = 0;
	for (Object& object : klondike.game.getCurrentObjects())
	{
		if (klondike.game.isHolder(object)) { ++holders; }
	}
	CHECK(holders > 0);
	CHECK(holders < klondike.game.getCurrentObjects().size());
	CHECK(holders == klondike.game.parts().holders.size());
}

TEST_CASE("a part a key gives is added on the first press, and a reset takes it away", "[store]")
{
	played::Played pong("descriptions/pong.xml");
	pong.tap(KeyCode::Num2);
	pong.frames(1);
	REQUIRE(pong.state() == "playing");
	const ObjectId paddle = pong.get("paddle2").id;
	CHECK_FALSE(pong.game.parts().drives.has(paddle));

	pong.engine.handleKeyPressed(KeyCode::Down);
	pong.frames(2);
	REQUIRE(pong.game.parts().drives.has(paddle));
	// Nothing drives the ball, ever.
	CHECK_FALSE(pong.game.parts().drives.has(pong.get("ball").id));

	pong.game.resetObject("paddle2");
	CHECK_FALSE(pong.game.parts().drives.has(paddle));
}

TEST_CASE("only an object whose keys pick up held items has a hand part", "[store]")
{
	{
		played::Played pong("descriptions/pong.xml");
		CHECK(pong.game.parts().hands.size() == 0);
	}
	played::Played klondike("descriptions/klondike.xml");
	CHECK(klondike.game.isHand(klondike.get("hand")));
	CHECK(klondike.game.parts().hands.size() == 1);
}

TEST_CASE("only an object written with <maze> has a maze part", "[store]")
{
	{
		played::Played pong("descriptions/pong.xml");
		CHECK(pong.game.parts().mazes.size() == 0);
	}
	played::Played pacman("descriptions/pacman.xml");
	CHECK(pacman.game.mazeOf(pacman.get("playing.pacman")) != nullptr);
	CHECK(pacman.game.mazeOf(pacman.get("playing.status")) == nullptr);
}

TEST_CASE("only a piece of a cell map, a chain or a shape has a cell part", "[store]")
{
	{
		played::Played pong("descriptions/pong.xml");
		CHECK(pong.game.parts().cells.size() == 0);
		CHECK_FALSE(pong.game.isShape(pong.get("ball")));
	}
	// Pac-Man's maze is a map of tiles, not of cells.
	{
		played::Played pacman("descriptions/pacman.xml");
		CHECK(pacman.game.parts().cells.size() == 0);
	}
	played::Played tetris("descriptions/tetris.xml");
	CHECK(tetris.game.parts().cells.size() > 0);
	CHECK(tetris.game.parts().cells.size() < tetris.game.getCurrentObjects().size());
}

TEST_CASE("board moves, a swing and a light's pasts are parts of only the objects that have them", "[store]")
{
	{
		played::Played pong("descriptions/pong.xml");
		CHECK(pong.game.parts().moves.size() == 0);
		CHECK(pong.game.parts().swings.size() == 0);
		CHECK(pong.game.parts().lightPasts.size() == 0);
	}
	{
		played::Played checkers("descriptions/checkers.xml");
		CHECK(checkers.game.parts().moves.size() > 0);
		CHECK(checkers.game.parts().moves.size() < checkers.game.getCurrentObjects().size());
	}
	played::Played hauntedhouse("descriptions/hauntedhouse.xml");
	CHECK(hauntedhouse.game.parts().lightPasts.size() > 0);
}

TEST_CASE("the members of a group share one kind: its rules and actions are held once", "[store][kind]")
{
	played::Played invaders("descriptions/spaceinvaders.xml");
	const Kind* aliens = nullptr;
	std::size_t members = 0;
	std::set<const Kind*> kinds;
	for (const Object& object : invaders.game.getCurrentObjects())
	{
		kinds.insert(object.kind.get());
		if (object.objClass != "aliens") { continue; }
		++members;
		if (!aliens) { aliens = object.kind.get(); }
		CHECK(object.kind.get() == aliens);
	}
	CHECK(members == 55);
	CHECK(kinds.size() < invaders.game.getCurrentObjects().size() - 50);
}

TEST_CASE("a spawned copy shows the looks of the piece it is copied from, held once", "[store][kind]")
{
	played::Played mario("descriptions/mariobros.xml");
	const Object* coin = nullptr;
	for (const Object& object : mario.game.getCurrentObjects())
	{
		if (object.objClass == "coin" && !object.isCopy() && !object.looks.all().empty()) { coin = &object; break; }
	}
	REQUIRE(coin != nullptr);
	const std::string name = coin->name;
	const Object* made = mario.game.spawn(name);
	REQUIRE(made != nullptr);
	const Object& original = mario.get(name);
	CHECK(made->looks.set == original.looks.set);
	CHECK(made->kind == original.kind);
}
