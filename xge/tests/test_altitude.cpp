// test_altitude.cpp
// XML Game Engine
// author: beefviper
// date: Oct 17, 2026
//
// Catch2 tests for <altitude>: a thing in the air over where it is, rising,
// falling and bouncing, drawn raised by how high it is, with a <shadow>
// under it; a rule's <condition> of how high it is; then a rally of
// games/descriptions/tennis.xml.

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
	// A window that notes where each object was drawn.
	class SeeingWindow : public FakeWindow
	{
	public:
		void draw(Object& object) override { drawn.push_back({ object.name, object.position }); }
		std::vector<std::pair<std::string, Vector2f>> drawn;
	};

	const std::string inTheAir = box("ball", 100, 200, "<altitude><start>50</start><fall>1</fall><bounce>0.5</bounce></altitude>");
}

TEST_CASE("what has an <altitude> falls by its fall, stops at the ground, and bounces back up with its share", "[altitude]")
{
	const Scratch file(inTheAir, "<set variable=\"ball.rise\">10</set>");
	Played play(file.path);
	Object& thing = play.get("ball");
	REQUIRE(thing.altitude == Approx(50));

	play.frames(1);
	CHECK(thing.rise == Approx(-1));
	CHECK(thing.altitude == Approx(49));
	CHECK(thing.position.y == Approx(200)); // where it is over the ground does not change

	// 50 = n(n+1)/2 at n = 9.5: down on the tenth frame, coming down at 10,
	// and back up at 5.
	play.frames(9);
	CHECK(thing.altitude == Approx(0));
	CHECK(thing.rise == Approx(5));

	// Bounce after bounce, until it rests.
	play.frames(100);
	CHECK(thing.altitude == Approx(0));
	CHECK(thing.rise == Approx(0));

	// <set> sends it up again, and a reset puts it back where it started.
	play.tap(KeyCode::Space);
	play.frames(1);
	CHECK(thing.altitude == Approx(9));
	play.game.resetObject("ball");
	CHECK(thing.altitude == Approx(50));
	CHECK(thing.rise == Approx(0));
}

TEST_CASE("it is drawn raised by its altitude, and its <shadow> lies under it on the ground", "[altitude][shadow]")
{
	const Scratch file("<object name=\"shade\"><sprite><circle><radius>4</radius></circle></sprite><position><x>0</x><y>0</y></position>"
		"<shadow object=\"ball\" /></object>" + inTheAir, "<pop />");
	Game game(file.path);
	auto window = std::make_unique<SeeingWindow>();
	SeeingWindow& seen = *window;
	Engine engine(game, std::move(window));
	for (auto& object : game.getCurrentObjects()) { object.size = measureShapeSize(object.spriteParams, object.shapeKind); }

	engine.step();
	engine.render();
	REQUIRE(seen.drawn.size() == 2);
	CHECK(seen.drawn[0].first == "playing.shade");
	CHECK(seen.drawn[0].second.x == Approx(101));
	CHECK(seen.drawn[0].second.y == Approx(201));
	CHECK(seen.drawn[1].first == "playing.ball");
	CHECK(seen.drawn[1].second.y == Approx(200 - 49));
	CHECK(game.getObject("ball").position.y == Approx(200));

	// Out of play, and so is its shadow.
	game.getObject("ball").isVisible = false;
	engine.step();
	CHECK_FALSE(game.getObject("shade").isVisible);
}

TEST_CASE("a rule can check how high it is: what flies over goes past", "[altitude]")
{
	const Scratch file(box("ball", 100, 100, "<velocity><x>4</x><y>0</y></velocity><altitude><start>30</start></altitude>"
		"<collisions><enabled>true</enabled><collision object=\"wall\"><condition variable=\"altitude\"><below>10</below></condition><bounce /></collision></collisions>")
		+ box("wall", 160, 100, "<collisions><enabled>true</enabled></collisions>"), "<set variable=\"ball.altitude\">0</set><reset object=\"wall\" />");
	Played play(file.path);

	play.frames(30);
	CHECK(play.get("ball").position.x > 170);

	play.game.resetObject("ball");
	play.tap(KeyCode::Space);
	play.frames(30);
	CHECK(play.get("ball").velocity.x == Approx(-4));
}

TEST_CASE("an <altitude> is not below the ground, and only what has one has one to set", "[altitude]")
{
	const auto load = [](const std::string& objects, const std::string& onSpace = "<pop />")
	{
		const Scratch file(objects, onSpace);
		Game game(file.path);
	};
	CHECK_THROWS_WITH(load(box("ball", 0, 0, "<altitude><start>-1</start></altitude>")), ContainsSubstring("cannot start below the ground"));
	CHECK_THROWS_WITH(load(box("ball", 0, 0, "<altitude><bounce>2</bounce></altitude>")), ContainsSubstring("<bounce> is 2"));
	CHECK_THROWS_WITH(load(box("ball", 0, 0), "<set variable=\"ball.rise\">3</set>"), ContainsSubstring("has no <altitude> to set"));
	CHECK_THROWS_WITH(load(box("ball", 0, 0, "<shadow object=\"bal\" />")), ContainsSubstring("there is no object named 'bal'"));
}

TEST_CASE("tennis.xml: the player hits a low ball back over the net, and the computer returns it", "[altitude][tennis]")
{
	Played play("descriptions/tennis.xml");
	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "playing");

	// The serve comes down toward the player; stand where it will be.
	Object& ball = play.get("ball");
	Object& player = play.get("player");
	ball.velocity.x = 0;
	player.position.x = ball.position.x - 9;
	bool hit = false;
	for (int i = 0; i < 300 && !hit; ++i)
	{
		play.frames(1);
		hit = ball.velocity.y < 0;
	}
	REQUIRE(hit);
	CHECK(ball.altitude < 50);
	CHECK(ball.rise > 0);

	// Over the net and back: the computer walks under it and returns it.
	bool returned = false;
	for (int i = 0; i < 400 && !returned; ++i)
	{
		play.frames(1);
		returned = ball.velocity.y > 0;
	}
	CHECK(returned);
	CHECK(play.get("player").variable.at("points") == Approx(0));
	CHECK(play.get("cpu").variable.at("points") == Approx(0));
}

TEST_CASE("tennis.xml: a ball let past is a point, said in words, and four take the game", "[altitude][tennis][choose]")
{
	Played play("descriptions/tennis.xml");
	play.tap(KeyCode::Space);
	Object& player = play.get("player");
	CHECK(play.get("playerscore").spriteParams.at(1) == "YOU 0 LOVE");

	for (int point = 1; point <= 4; ++point)
	{
		player.position.x = 50; // nowhere near
		for (int i = 0; i < 400 && play.get("cpu").variable.at("points") + play.get("cpu").variable.at("games") * 4 < static_cast<float>(point); ++i)
		{
			play.frames(1);
		}
		if (point == 2) { CHECK(play.get("cpuscore").spriteParams.at(1) == "CPU 0 30"); }
	}
	CHECK(play.get("cpu").variable.at("games") == Approx(1));
	CHECK(play.get("cpu").variable.at("points") == Approx(0));
	CHECK(play.get("cpuscore").spriteParams.at(1) == "CPU 1 LOVE");
}
