// test_dark.cpp
// XML Game Engine
// author: beefviper
// date: Oct 17, 2026
//
// Catch2 tests for <light> and <dark>: what is dark is drawn only where a
// light reaches it, or, unseen, from the first time one does; then the dark
// house of games/descriptions/hauntedhouse.xml.

#include "played.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <string>
#include <vector>

using namespace xge;
using namespace played;
using Catch::Approx;
using Catch::Matchers::ContainsSubstring;

namespace
{
	// A window that notes what it drew.
	class SeeingWindow : public FakeWindow
	{
	public:
		void draw(Object& object) override { drawn.push_back(object.name); }
		std::vector<std::string> drawn;
	};

	bool wasDrawn(const std::vector<std::string>& names, const std::string& name)
	{
		return std::find(names.begin(), names.end(), "playing." + name) != names.end();
	}
}

TEST_CASE("what is <dark> is drawn only while a <light> reaches some part of it, or from the first time, if unseen", "[dark]")
{
	const Scratch file(box("lamp", 100, 100, "<velocity><x>4</x><y>0</y></velocity><light>30</light>")
		+ box("near", 135, 100, "<dark>unlit</dark>") + box("far", 400, 100, "<dark>unlit</dark>")
		+ box("map", 200, 100, "<dark>unseen</dark>"), "<set variable=\"lamp.light\">0</set>");
	Game game(file.path);
	auto window = std::make_unique<SeeingWindow>();
	SeeingWindow& seen = *window;
	Engine engine(game, std::move(window));
	for (auto& object : game.getCurrentObjects()) { object.size = measureShapeSize(object.spriteParams, object.shapeKind); }

	const auto frame = [&]
	{
		engine.step();
		seen.drawn.clear();
		engine.render();
		return seen.drawn;
	};

	// The lamp's middle is at 109 after a frame; near's left side 135 is
	// within 30 of it, the rest are not.
	std::vector<std::string> names = frame();
	CHECK(wasDrawn(names, "lamp"));
	CHECK(wasDrawn(names, "near"));
	CHECK_FALSE(wasDrawn(names, "far"));
	CHECK_FALSE(wasDrawn(names, "map"));

	// Moving on: past near (out of reach behind), lighting the map, which stays.
	for (int i = 0; i < 20; ++i) { names = frame(); }
	CHECK(wasDrawn(names, "map"));
	for (int i = 0; i < 20; ++i) { names = frame(); }
	CHECK_FALSE(wasDrawn(names, "near"));
	CHECK(wasDrawn(names, "map"));

	// A light put out lights nothing; a reset forgets what was seen.
	engine.handleKeyPressed(KeyCode::Space);
	engine.handleKeyReleased(KeyCode::Space);
	game.getObject("lamp").position.x = 125;
	game.getObject("lamp").velocity.x = 0;
	names = frame();
	CHECK_FALSE(wasDrawn(names, "near"));
	game.resetObject("map");
	names = frame();
	CHECK_FALSE(wasDrawn(names, "map"));
	CHECK(game.getObject("lamp").light == Approx(0));
	game.resetObject("lamp");
	CHECK(game.getObject("lamp").light == Approx(30));
}

TEST_CASE("a <dark> object still moves and touches while it is not drawn", "[dark]")
{
	const Scratch file(box("ghost", 100, 100, "<velocity><x>5</x><y>0</y></velocity><dark>unlit</dark><collisions><enabled>true</enabled>"
		"<collision object=\"wall\"><stop /></collision></collisions>") + box("wall", 150, 100, "<collisions><enabled>true</enabled></collisions>"), "<pop />");
	Played play(file.path);
	play.frames(20);
	CHECK(play.get("ghost").inDark);
	CHECK(play.get("ghost").velocity.x == Approx(0));
	CHECK(play.get("ghost").position.x < 150);
}

TEST_CASE("a <dark> is unlit or unseen, and a <light> reaches 0 or more", "[dark]")
{
	const auto load = [](const std::string& objects)
	{
		const Scratch file(objects, "<pop />");
		Game game(file.path);
	};
	CHECK_THROWS(load(box("ghost", 0, 0, "<dark>gloomy</dark>")));
	CHECK_THROWS_WITH(load(box("lamp", 0, 0, "<light>-5</light>")), ContainsSubstring("<light> reaches -5"));
}

namespace
{
	// Haunted House started, one frame played. Tiles are 40 pixels and the
	// floor's map starts a tile down the window.
	struct House : Played
	{
		House() : Played("descriptions/hauntedhouse.xml")
		{
			tap(KeyCode::Space);
			frames(1);
		}

		Object& eyes() { return get("player"); }
		float variable(const char* name) { return eyes().variable.at(name); }
		std::string floor() { return game.currentScene("playing"); }

		// Puts the eyes in the middle of a tile of the map, column and row.
		void place(int column, int row)
		{
			eyes().position = { static_cast<float>(column) * 40.0f + 11.0f, static_cast<float>(row + 1) * 40.0f + 15.0f };
		}

		void walk(KeyCode key, int count)
		{
			engine.handleKeyPressed(key);
			frames(count);
			engine.handleKeyReleased(key);
		}

		// Walks up into the stairs at the top of a column, from the tile under them.
		void climb(int column) { place(column, 2); walk(KeyCode::W, 25); }

		int lit(const std::string& objClass)
		{
			int count = 0;
			for (const Object& object : game.getCurrentObjects())
			{
				if (object.objClass == objClass && game.isShown(object) && !object.inDark) { ++count; }
			}
			return count;
		}

		// Holds every creature of the floor still, so a test is not about one
		// walking into what it checks.
		void stillCreatures()
		{
			for (Object& object : game.getCurrentObjects())
			{
				if (object.objClass == "creature") { object.velocity = { 0.0f, 0.0f }; object.timers.clear(); object.position = { 0.0f, 0.0f }; }
			}
		}
	};
}

TEST_CASE("hauntedhouse.xml: the walls show near the eyes, and a match shows more, burns down and is counted", "[dark][hauntedhouse]")
{
	House play;
	REQUIRE(play.state() == "playing");
	CHECK(play.floor() == "playing.floor1");
	CHECK(play.variable("lives") == Approx(9));

	// Beside the eyes at the door the walls are lit; across the house they are dark.
	const int bySelf = play.lit("wall");
	CHECK(bySelf > 0);
	CHECK(bySelf < 10);

	// A match lights much further, and burns down again.
	play.tap(KeyCode::Space);
	play.frames(1);
	CHECK(play.eyes().light == Approx(260));
	CHECK(play.variable("matches") == Approx(1));
	CHECK(play.lit("wall") > bySelf * 2);
	play.frames(300);
	CHECK(play.eyes().light == Approx(40));
	CHECK(play.lit("wall") == bySelf);

	// There are always more matches; each one counts.
	for (int i = 0; i < 12; ++i) { play.tap(KeyCode::Space); }
	CHECK(play.variable("matches") == Approx(13));
	CHECK(play.eyes().light > 200);
}

TEST_CASE("hauntedhouse.xml: the stairs lead up and down the four floors, each walls of its own colour", "[dark][hauntedhouse]")
{
	House play;
	play.stillCreatures();
	const std::string groundWall = play.get("playing.floor1.house.walls.1").spriteParams.back();

	play.climb(18);
	REQUIRE(play.floor() == "playing.floor2");
	CHECK(play.get("playing.floor2.house.walls.1").spriteParams.back() != groundWall);
	// Up the stairs the eyes are where they were, which on this floor is no stairs.
	play.frames(30);
	CHECK(play.floor() == "playing.floor2");

	play.stillCreatures();
	play.climb(1);
	REQUIRE(play.floor() == "playing.floor3");
	play.stillCreatures();
	play.climb(18);
	REQUIRE(play.floor() == "playing.floor4");

	// And down again, by the stairs of the top floor.
	play.stillCreatures();
	play.climb(15);
	CHECK(play.floor() == "playing.floor3");
}

TEST_CASE("hauntedhouse.xml: the key goes with the eyes and opens a locked door, and an urn piece found stays on the map", "[dark][hauntedhouse]")
{
	House play;
	play.stillCreatures();

	// A locked door stops the eyes without the key.
	{
		House keyless;
		keyless.stillCreatures();
		keyless.climb(18);
		REQUIRE(keyless.floor() == "playing.floor2");
		keyless.stillCreatures();
		keyless.place(14, 5);
		keyless.walk(KeyCode::A, 40);
		CHECK(keyless.get("playing.floor2.door").isVisible);
		CHECK(keyless.eyes().position.x > 12 * 40.0f);
	}

	// The key, picked up on the ground floor, is carried up the stairs.
	Object& key = play.get("playing.floor1.key");
	play.place(4, 5);
	play.walk(KeyCode::D, 10);
	play.climb(18);
	REQUIRE(play.floor() == "playing.floor2");
	CHECK(play.game.isShown(key));

	// The door of the locked room opens to the eyes with the key.
	Object& door = play.get("playing.floor2.door");
	REQUIRE(door.isVisible);
	play.stillCreatures();
	play.place(14, 5);
	play.walk(KeyCode::A, 40);
	CHECK_FALSE(door.isVisible);

	// The piece inside: dark until a light finds it, then always seen.
	Object& piece = play.get("playing.floor2.piece1");
	REQUIRE(piece.isVisible);
	play.place(12, 7);
	play.frames(1);
	CHECK(piece.inDark);
	play.tap(KeyCode::Space);
	play.frames(1);
	CHECK_FALSE(piece.inDark);
	play.frames(300);
	CHECK_FALSE(piece.inDark);
	play.eyes().position = { piece.position.x + 25.0f, piece.position.y };
	play.walk(KeyCode::A, 15);
	CHECK(play.variable("urns") == Approx(1));
	CHECK_FALSE(piece.isVisible);
}

TEST_CASE("hauntedhouse.xml: a creature that reaches the eyes costs a life, blows out the match and goes back", "[dark][hauntedhouse]")
{
	House play;
	Object& spider = play.get("playing.floor1.spider");
	const Vector2f start = spider.position;
	play.frames(120);
	CHECK(spider.position.x != start.x);

	play.tap(KeyCode::Space);
	play.frames(1);
	REQUIRE(play.eyes().light > 200);
	spider.position = { play.eyes().position.x + 4.0f, play.eyes().position.y };
	play.frames(2);
	CHECK(play.variable("lives") == Approx(8));
	CHECK(play.eyes().light == Approx(40));
	CHECK(spider.position.x == Approx(start.x));
	CHECK(spider.position.y == Approx(start.y));
}

TEST_CASE("hauntedhouse.xml: the whole urn brought back to the door wins, and no lives left loses", "[dark][hauntedhouse]")
{
	House play;
	play.stillCreatures();
	play.game.incrementText("player.urns", 3.0f);
	play.place(10, 10);
	play.frames(2);
	CHECK(play.state() == "playing");
	play.place(1, 11);
	play.frames(2);
	CHECK(play.state() == "won");
	play.tap(KeyCode::Space);
	CHECK(play.state() == "mainmenu");

	House again;
	again.game.incrementText("player.lives", -9.0f);
	again.frames(2);
	CHECK(again.state() == "lost");
}
