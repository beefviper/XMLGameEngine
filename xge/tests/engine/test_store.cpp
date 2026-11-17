// test_store.cpp
// XML Game Engine
// author: beefviper
// date: Nov 17, 2026
//
// An object's handle (ObjectId): found while it is in the game, nothing
// once it is gone, and a slot used again is a generation on.

#include "played.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

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
	const ObjectId second = play.game.getObject(again).id;
	CHECK(second.index == first.index);
	CHECK(second.generation == first.generation + 1);
	CHECK(play.game.find(second) == &play.game.getObject(again));
	CHECK(play.game.find(first) == nullptr);
	CHECK(play.game.find(ship.id) == &play.get("ship"));
}
