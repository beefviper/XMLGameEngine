// test_spawn.cpp
// XML Game Engine
// author: beefviper
// date: Oct 23, 2026
//
// Catch2 tests for objects that come and go while a game runs: an object
// that does not exist until a <spawn> makes a copy of it, the copies'
// names and their own rules, <despawn />, <most>, <fire> and <release>
// making copies, a group spawned in turn, what other rules and conditions
// see of copies, and resets.

#include "played.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <string>

using namespace xge;
using played::box;
using played::Played;
using played::Scratch;
using Catch::Approx;
using Catch::Matchers::ContainsSubstring;

namespace
{
	// A shot that does not exist at the start, going up, counting its own
	// frames in a variable of its own, and gone at the top.
	const std::string kShot = "<object name=\"shot\" class=\"bullet\"><sprite><rectangle><width>4</width><height>8</height></rectangle></sprite>"
		"<velocity><x>0</x><y>-5</y></velocity><exists>false</exists>"
		"<collisions><enabled>true</enabled><collision edge=\"top\"><despawn /></collision></collisions>"
		"<variables><variable name=\"age\">0</variable></variables>"
		"<timers><timer><every>0.01</every><inc variable=\"self.age\" /></timer></timers></object>";
}

TEST_CASE("an object that does not exist is not in play until a copy of it is spawned", "[spawn]")
{
	const Scratch file(box("ship", 300, 400) + kShot,
		"<spawn object=\"shot\"><position><x>ship.x</x><y>ship.y</y></position></spawn>");
	Played play(file.path);

	CHECK_FALSE(play.get("shot").exists);
	CHECK_FALSE(play.game.isShown(play.get("shot")));
	CHECK(play.count("shot") == 0);

	// Made at once, in the game when the frame is over.
	play.tap(KeyCode::Space);
	REQUIRE(play.game.justSpawned().size() == 1);
	CHECK(play.game.justSpawned().front().name == "playing.shot.1");
	play.frames(1);
	REQUIRE(play.has("shot.1"));
	const Object& copy = play.get("shot.1");
	CHECK(copy.isCopy());
	CHECK(copy.objClass == "bullet");
	CHECK(copy.inGroup("playing.shot"));
	CHECK(play.game.isShown(copy));
	CHECK(copy.position.x == Approx(300));

	play.tap(KeyCode::Space);
	play.frames(1);
	CHECK(play.has("shot.2"));
	CHECK(play.count("shot") == 2);
	CHECK_FALSE(play.get("shot").exists); // the one written is still only a description
}

TEST_CASE("a copy's own rules and values are about the copy, not the object it was copied from", "[spawn]")
{
	const Scratch file(box("ship", 300, 400) + kShot, "<spawn object=\"shot\"><position><y>500</y></position></spawn>");
	Played play(file.path);

	play.tap(KeyCode::Space);
	play.frames(10);
	play.tap(KeyCode::Space);
	play.frames(5);

	// Each counts its own frames (self.age is its own).
	CHECK(play.get("shot.1").variable.at("age") == Approx(15).margin(1));
	CHECK(play.get("shot.2").variable.at("age") == Approx(5).margin(1));
	CHECK(play.get("shot").variable.at("age") == 0.0f);
}

TEST_CASE("<despawn /> takes a copy out for good, and its name is used again", "[spawn]")
{
	const Scratch file(box("ship", 300, 100) + kShot,
		"<spawn object=\"shot\"><position><x>ship.x</x><y>ship.y</y></position></spawn>");
	Played play(file.path);

	play.tap(KeyCode::Space);
	play.frames(1);
	const std::size_t with = play.game.getCurrentObjects().size();

	// Up and off the top: despawned, and gone from the objects.
	play.frames(30);
	CHECK_FALSE(play.has("shot.1"));
	CHECK(play.game.getCurrentObjects().size() == with - 1);

	play.tap(KeyCode::Space);
	play.frames(1);
	CHECK(play.has("shot.1"));
	CHECK(play.get("shot.1").variable.at("age") <= 2.0f); // a new one, starting as the file says
}

TEST_CASE("<despawn object> takes out an object the file wrote, until a reset", "[spawn]")
{
	const Scratch file(box("wall", 100, 100) + box("door", 200, 100), "<despawn object=\"door\" />");
	Played play(file.path);

	play.tap(KeyCode::Space);
	play.frames(1);
	CHECK_FALSE(play.get("door").exists);
	CHECK_FALSE(play.game.isShown(play.get("door")));
	CHECK(play.get("wall").exists);

	// It stays among the objects, not existing; a reset brings it back.
	play.game.resetObject("door");
	CHECK(play.get("door").exists);
	CHECK(play.game.isShown(play.get("door")));
}

TEST_CASE("<most> caps how many copies exist, and a refused spawn stops what comes after", "[spawn]")
{
	const Scratch file(box("ship", 300, 400, "<actions><action name=\"shoot\"><spawn object=\"shot\" /><inc variable=\"self.shots\" /></action></actions>"
		"<variables><variable name=\"shots\">0</variable></variables>")
		+ "<object name=\"shot\"><sprite><rectangle><width>4</width><height>8</height></rectangle></sprite><exists>false</exists><most>2</most></object>",
		"<trigger object=\"ship\" action=\"shoot\" />");
	Played play(file.path);

	for (int i = 0; i < 4; ++i) { play.tap(KeyCode::Space); }
	play.frames(1);
	CHECK(play.count("shot") == 2);
	CHECK(play.get("ship").variable.at("shots") == 2.0f);

	play.game.despawn(play.get("shot.1"));
	play.frames(1);
	play.tap(KeyCode::Space);
	play.frames(1);
	CHECK(play.count("shot") == 2);
	CHECK(play.has("shot.1"));

	// By the end of its path, as a test or a front end names it.
	play.game.despawn("shot");
	play.frames(1);
	CHECK(play.count("shot") == 0);
}

TEST_CASE("<fire> and <release> of an object that does not exist make copies of it", "[spawn]")
{
	const Scratch file(box("ship", 300, 400, "<collisions><enabled>true</enabled></collisions>"
		"<actions><action name=\"shoot\"><fire object=\"shot\" /></action></actions>")
		+ "<object name=\"shot\"><sprite><rectangle><width>4</width><height>8</height></rectangle></sprite>"
		"<velocity><x>0</x><y>-6</y></velocity><exists>false</exists><most>1</most></object>"
		+ "<object name=\"piece\"><sprite><rectangle><width>2</width><height>2</height></rectangle></sprite>"
		"<velocity><x>1</x><y>1</y></velocity><exists>false</exists></object>"
		+ box("rock", 300, 380, "<collisions><enabled>true</enabled><collision object=\"ship\"><release object=\"piece\">3</release><despawn /></collision></collisions>"),
		"<trigger object=\"ship\" action=\"shoot\" />");
	Played play(file.path);

	play.tap(KeyCode::Space);
	play.tap(KeyCode::Space); // one in flight at most
	play.frames(1);
	REQUIRE(play.count("shot") == 1);
	const Object& shot = play.get("shot.1");
	CHECK(shot.position.x + 2 == Approx(305)); // from the ship's top middle
	CHECK(shot.velocity.y == Approx(-6));

	// The ship touches the rock: it breaks into three pieces.
	play.get("ship").velocity = { 0, -3 };
	play.frames(8);
	CHECK(play.count("piece") == 3);
}

TEST_CASE("other rules and conditions see the copies by the name of what they were copied from", "[spawn]")
{
	const Scratch file(box("ship", 300, 400, "<collisions><enabled>true</enabled></collisions>")
		+ "<object name=\"rock\"><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite><exists>false</exists>"
		"<collisions><enabled>true</enabled><collision object=\"ship\"><despawn /></collision></collisions></object>"
		+ box("target", 0, 0, "<collisions><enabled>true</enabled></collisions><variables><variable name=\"done\">0</variable></variables>"),
		"<spawn object=\"rock\"><position><x>300</x><y>380</y></position><velocity><y>4</y></velocity></spawn>",
		"<conditions><condition object=\"rock\"><remaining>0</remaining><set variable=\"target.done\">1</set></condition></conditions>");
	Played play(file.path);

	// None yet: none remaining.
	play.frames(1);
	CHECK(play.get("target").variable.at("done") == 1.0f);
	play.get("target").variable["done"] = 0;

	play.tap(KeyCode::Space);
	play.frames(1);
	CHECK(play.count("rock") == 1);
	play.frames(1);
	CHECK(play.get("target").variable.at("done") == 0.0f);

	// It falls onto the ship and despawns itself; then none remain.
	play.frames(10);
	CHECK(play.count("rock") == 0);
	CHECK(play.get("target").variable.at("done") == 1.0f);
}

TEST_CASE("a group spawned from makes its members in turn", "[spawn]")
{
	const Scratch file(box("ship", 300, 400)
		+ "<group name=\"rocks\"><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite><exists>false</exists>"
		"<member name=\"big\"><velocity><x>1</x></velocity></member><member name=\"small\"><velocity><x>2</x></velocity></member></group>",
		"<spawn object=\"rocks\" />");
	Played play(file.path);

	play.tap(KeyCode::Space);
	play.tap(KeyCode::Space);
	play.tap(KeyCode::Space);
	play.frames(1);
	CHECK(play.has("big.1"));
	CHECK(play.has("small.1"));
	CHECK(play.has("big.2"));
	CHECK(play.count("rocks") == 3);
	CHECK(play.get("small.1").velocity.x == Approx(2));
}

TEST_CASE("a new game takes away every copy, and a spawned object can be of one that exists", "[spawn]")
{
	const Scratch file(box("ball", 100, 100, "<velocity><x>1</x><y>0</y></velocity>"), "<spawn object=\"ball\"><position><y>300</y></position></spawn>");
	Played play(file.path);

	play.tap(KeyCode::Space);
	play.frames(1);
	REQUIRE(play.has("ball.1"));
	CHECK(play.get("ball.1").position.y == Approx(300));
	CHECK(play.get("ball").exists);

	play.game.resetAll();
	CHECK_FALSE(play.has("ball.1"));
	CHECK(play.count("ball") == 1);
}

TEST_CASE("a <despawn object> must name an object, and one that does not exist may leave out its <position>", "[spawn]")
{
	const auto load = [](const std::string& objects, const std::string& onSpace)
	{
		const Scratch file(objects, onSpace);
		Game game(file.path);
	};

	CHECK_THROWS_WITH(load(box("ship", 0, 0), "<despawn object=\"shots\" />"), ContainsSubstring("<despawn> names 'shots'"));
	CHECK_THROWS_WITH(load(box("ship", 0, 0), "<despawn />"), ContainsSubstring("<despawn />"));
	CHECK_NOTHROW(load(box("ship", 0, 0) + kShot, "<spawn object=\"shot\" />"));
	CHECK_THROWS_WITH(load(box("ship", 0, 0) + "<object name=\"shot\"><sprite><rectangle><width>4</width><height>8</height></rectangle></sprite></object>", "<pop />"),
		ContainsSubstring("missing <position>"));
}
