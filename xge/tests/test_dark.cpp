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

	bool drawn(const std::vector<std::string>& names, const std::string& name)
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
	CHECK(drawn(names, "lamp"));
	CHECK(drawn(names, "near"));
	CHECK_FALSE(drawn(names, "far"));
	CHECK_FALSE(drawn(names, "map"));

	// Moving on: past near (out of reach behind), lighting the map, which stays.
	for (int i = 0; i < 20; ++i) { names = frame(); }
	CHECK(drawn(names, "map"));
	for (int i = 0; i < 20; ++i) { names = frame(); }
	CHECK_FALSE(drawn(names, "near"));
	CHECK(drawn(names, "map"));

	// A light put out lights nothing; a reset forgets what was seen.
	engine.handleKeyPressed(KeyCode::Space);
	engine.handleKeyReleased(KeyCode::Space);
	game.getObject("lamp").position.x = 125;
	game.getObject("lamp").velocity.x = 0;
	names = frame();
	CHECK_FALSE(drawn(names, "near"));
	game.resetObject("map");
	names = frame();
	CHECK_FALSE(drawn(names, "map"));
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

TEST_CASE("hauntedhouse.xml: the walls show near the eyes, a match shows more, and found urn pieces stay", "[dark][hauntedhouse]")
{
	Played play("descriptions/hauntedhouse.xml");
	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "playing");
	play.frames(1);

	// Beside the eyes at the door the walls are lit; across the house they are dark.
	const auto litWalls = [&]
	{
		int count = 0;
		for (const Object& object : play.game.getCurrentObjects())
		{
			if (object.objClass == "wall" && !object.inDark) { ++count; }
		}
		return count;
	};
	const int bySelf = litWalls();
	CHECK(bySelf > 0);
	CHECK(bySelf < 10);

	// A match lights much further, and burns down again.
	play.tap(KeyCode::Space);
	play.frames(1);
	CHECK(play.get("player").light == Approx(170));
	CHECK(play.get("player").variable.at("matches") == Approx(9));
	CHECK(litWalls() > bySelf * 2);
	play.frames(150);
	CHECK(play.get("player").light == Approx(40));
	CHECK(litWalls() == bySelf);

	// Every match struck: then no light comes of striking.
	for (int i = 0; i < 12; ++i) { play.tap(KeyCode::Space); }
	CHECK(play.get("player").variable.at("matches") == Approx(0));
	play.frames(150);
	play.tap(KeyCode::Space);
	play.frames(1);
	CHECK(play.get("player").light < 50);

	// An urn piece found in the light stays on the map, and picking it up counts.
	Object& piece = play.get("urn.1");
	CHECK(piece.inDark);
	Object& eyes = play.get("player");
	eyes.position = { piece.position.x - 30, piece.position.y };
	play.frames(1);
	CHECK_FALSE(piece.inDark);
	eyes.position = { 60, 100 };
	play.frames(1);
	CHECK_FALSE(piece.inDark);
	eyes.position = { piece.position.x - 25, piece.position.y };
	play.engine.handleKeyPressed(KeyCode::D);
	play.frames(20);
	play.engine.handleKeyReleased(KeyCode::D);
	CHECK(play.get("player").variable.at("urns") == Approx(1));
	CHECK_FALSE(piece.isVisible);
}
