// test_steering.cpp
// XML Game Engine
// author: beefviper
// date: Oct 17, 2026
//
// Catch2 tests for being pulled toward something (<pull>), turning toward
// it (<seek>, a homing missile), hopping on a diagonal (Q*bert) and a
// control moving an object along a path (<track path>).

#include "engine/engine.h"
#include "game/game.h"
#include "play/collision.h"
#include "play/executor.h"
#include "play/geometry.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace xge;
using Catch::Approx;
using Catch::Matchers::ContainsSubstring;

namespace
{
	class FakeWindow : public Window
	{
	public:
		bool isOpen() const override { return true; }
		void close() override {}
		void init(std::vector<Object>&) override {}
		std::vector<std::pair<KeyCode, bool>> pollEvents() override { return {}; }
		void clear(const std::string&) override {}
		void draw(Object&) override {}
		void display() override {}
	};

	// A game played with no window, each shape the size its sprite implies.
	struct Played
	{
		Game game;
		Engine engine;

		explicit Played(const std::string& file) : game(file), engine(game, std::make_unique<FakeWindow>())
		{
			for (auto& object : game.getCurrentObjects())
			{
				object.size = measureShapeSize(object.spriteParams, object.shapeKind);
			}
		}

		void tap(KeyCode key)
		{
			engine.handleKeyPressed(key);
			engine.handleKeyReleased(key);
		}

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { engine.step(); }
		}

		std::string state() { return game.getCurrentState().name; }
		Object& get(const std::string& name) { return game.getObject(name); }
	};

	// A game of one state, "playing", with these objects and these commands
	// on the space bar (and `paths` before them), written out to be played.
	class Scratch
	{
	public:
		Scratch(const std::string& objects, const std::string& onSpace, const std::string& paths = "")
		{
			std::ofstream out(path);
			out << "<game xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"../../xge/xgedef.xsd\">"
				"<window name=\"steering\"><width>800</width><height>600</height><background>color.black</background>"
				"<fullscreen>false</fullscreen><framerate>60</framerate></window>"
				"<variables><variable name=\"unused\">0</variable></variables>" << paths <<
				"<states><state name=\"playing\"><objects>" << objects << "</objects>"
				"<inputs><input button=\"space\">" << onSpace << "</input></inputs></state></states></game>";
		}
		~Scratch() { std::filesystem::remove(path); }
		Scratch(const Scratch&) = delete;
		Scratch& operator=(const Scratch&) = delete;

		const std::string path = "descriptions/steering_scratch.xml";
	};

	std::string box(const std::string& name, float x, float y, const std::string& rest = "")
	{
		return "<object name=\"" + name + "\"><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite>"
			"<position><x>" + std::to_string(x) + "</x><y>" + std::to_string(y) + "</y></position>" + rest + "</object>";
	}

	float length(const Vector2f& v) { return std::hypot(v.x, v.y); }
}

TEST_CASE("a turn toward a way goes no further than it is let, and keeps the speed", "[steering]")
{
	const Vector2f east{ 3, 0 };
	const Vector2f turned = turnedToward(east, { 0, 10 }, 10.0f);
	CHECK(length(turned) == Approx(3));
	CHECK(std::atan2(turned.y, turned.x) * 180.0f / 3.14159265f == Approx(10).margin(0.01));

	// Close enough, it goes straight there.
	const Vector2f straight = turnedToward(east, { 10, 1 }, 45.0f);
	CHECK(straight.y / straight.x == Approx(0.1f).margin(0.0001));

	// The short way round, and still stays still.
	CHECK(turnedToward(east, { -1, -0.1f }, 5.0f).y < 0.0f);
	CHECK(turnedToward({ 0, 0 }, { 1, 1 }, 5.0f).x == 0.0f);
}

TEST_CASE("<pull> draws an object toward the nearest of what pulls it, a little more every frame", "[steering]")
{
	const Scratch file(box("ship", 100, 295, "<pull object=\"sun\">0.5</pull>") + box("sun", 400, 295), "<pop />");
	Played play(file.path);

	play.frames(1);
	CHECK(play.get("ship").velocity.x == Approx(0.5));
	play.frames(9);
	CHECK(play.get("ship").velocity.x == Approx(5));
	CHECK(play.get("ship").velocity.y == Approx(0));
	CHECK(play.get("ship").position.x > 100.0f);

	const auto load = [](const std::string& objects)
	{
		const Scratch bad(objects, "<pop />");
		Game game(bad.path);
	};
	CHECK_THROWS_WITH(load(box("ship", 0, 0, "<pull object=\"moon\">1</pull>")), ContainsSubstring("<pull>: there is no object named 'moon'"));
}

TEST_CASE("<seek> turns a missile toward its target a few degrees a frame, heading and all", "[steering]")
{
	const Scratch file(box("ship", 400, 500) + "<object name=\"missile\"><sprite><line><from><x>0</x><y>0</y></from><to><x>0</x><y>10</y></to></line></sprite>"
		"<position><x>100</x><y>100</y></position><velocity><x>4</x><y>0</y></velocity><heading>90</heading><seek object=\"ship\">3</seek></object>",
		"<pop />");
	Played play(file.path);

	play.frames(1);
	const Object& missile = play.get("missile");
	CHECK(length(missile.velocity) == Approx(4));
	CHECK(missile.heading == Approx(93).margin(0.01));

	// In time it comes round and goes over the ship.
	float nearest = 1000.0f;
	for (int i = 0; i < 300; ++i)
	{
		play.frames(1);
		nearest = std::min(nearest, length(play.get("ship").position - play.get("missile").position));
	}
	CHECK(nearest < 10.0f);

	const auto load = [](const std::string& objects)
	{
		const Scratch bad(objects, "<pop />");
		Game game(bad.path);
	};
	CHECK_THROWS_WITH(load(box("missile", 0, 0, "<seek object=\"ship\">-1</seek>") + box("ship", 50, 50)), ContainsSubstring("expected 0 or more"));
}

TEST_CASE("a diagonal <hop> goes one distance along both, or its own across and down", "[steering]")
{
	const Scratch file(box("qbert", 300, 300, "<actions><action name=\"downright\"><hop direction=\"down-right\"><x>32</x><y>48</y></hop></action>"
		"<action name=\"upleft\"><hop direction=\"up-left\">20</hop></action></actions>"),
		"<trigger object=\"qbert\" action=\"downright\" />");
	Played play(file.path);

	play.tap(KeyCode::Space);
	play.frames(1);
	CHECK(play.get("qbert").position.x == Approx(332));
	CHECK(play.get("qbert").position.y == Approx(348));
	CHECK(play.get("qbert").facing == Direction::Down);

	CommandExecutor executor(play.game);
	executor.executeInput(CmdTriggerAction{ "qbert", "upleft" }, true);
	play.frames(1);
	CHECK(play.get("qbert").position.x == Approx(312));
	CHECK(play.get("qbert").position.y == Approx(328));

	const auto load = [](const std::string& objects)
	{
		const Scratch bad(objects, "<pop />");
		Game game(bad.path);
	};
	CHECK_THROWS_WITH(load(box("qbert", 0, 0, "<actions><action name=\"down\"><hop direction=\"down\"><x>1</x><y>2</y></hop></action></actions>")),
		ContainsSubstring("is not diagonal"));
}

TEST_CASE("<track path> puts the object that far along the path a control moves it on", "[steering]")
{
	const std::string paths = "<paths><path name=\"rail\"><speed>1</speed><start><x>100</x><y>100</y></start>"
		"<step><x>300</x><y>0</y></step><step><x>0</x><y>100</y></step></path></paths>";
	const Scratch file(box("gun", 0, 0, "<actions><action name=\"steer\"><track path=\"rail\" /></action></actions>"), "<pop />", paths);
	Played play(file.path);

	CommandExecutor executor(play.game);
	const auto at = [&](float fraction)
	{
		executor.executeAxis(CmdTriggerAction{ "gun", "steer" }, Axis::MouseX, fraction);
		return play.get("gun").position + Vector2f{ 5, 5 };
	};

	CHECK(at(0.0f).x == Approx(100));
	CHECK(at(0.5f).x == Approx(300));   // 200 of the 400 along it
	CHECK(at(0.5f).y == Approx(100));
	CHECK(at(0.875f).x == Approx(400)); // 350: round the corner
	CHECK(at(0.875f).y == Approx(150));
	CHECK(at(2.0f).y == Approx(200));   // held to the end

	const auto load = [&](const std::string& path)
	{
		const Scratch bad(box("gun", 0, 0, "<actions><action name=\"steer\"><track path=\"" + path + "\" /></action></actions>"), "<pop />",
			"<paths><path name=\"loose\"><speed>1</speed><step><x>10</x><y>0</y></step></path></paths>");
		Game game(bad.path);
	};
	CHECK_THROWS_WITH(load("rail"), ContainsSubstring("names no path of the game"));
	CHECK_THROWS_WITH(load("loose"), ContainsSubstring("the path needs a <start>"));
}

TEST_CASE("qbert.xml: a hop down a diagonal lights the cube landed on, and one off the pyramid costs a life", "[steering][qbert]")
{
	Played play("descriptions/qbert.xml");
	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "playing");

	// The red ball out of the way, where its hops go nowhere.
	play.get("ball").position = { 760, 560 };
	play.get("ball").positionOriginal = { 760, 560 };

	play.tap(KeyCode::D);
	play.frames(1);
	CHECK(play.get("qbert").position.x == Approx(422));
	CHECK(play.get("qbert").position.y == Approx(168));
	CHECK(play.get("row2.2.1").lookName() == "lit");
	CHECK(play.get("row2.1.1").lookName() == "unlit");
	CHECK(play.get("player").variable.at("score") == Approx(25));

	// Back up onto the top, and a second landing on a lit cube scores nothing.
	play.tap(KeyCode::A);
	play.frames(1);
	CHECK(play.get("row1.1.1").lookName() == "lit");
	play.tap(KeyCode::D);
	play.frames(1);
	play.tap(KeyCode::A);
	play.frames(1);
	CHECK(play.get("player").variable.at("score") == Approx(50));

	// Up and left off the top: into the void.
	play.tap(KeyCode::A);
	play.frames(2);
	CHECK(play.get("player").variable.at("lives") == Approx(2));
	CHECK(play.get("qbert").position.x == Approx(390));
	CHECK(play.get("qbert").position.y == Approx(120));
}

TEST_CASE("yarsrevenge.xml: the missile homes on Yar, the zone keeps him safe, and the cannon leaves level with him", "[steering][yarsrevenge]")
{
	Played play("descriptions/yarsrevenge.xml");
	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "playing");

	// The missile turns toward Yar, wherever he is.
	play.get("yar").position = { 100, 500 };
	play.frames(30);
	CHECK(play.get("missile").velocity.y > 0.5f);

	// In the neutral zone it goes over him.
	play.get("yar").position = { 320, 300 };
	play.get("missile").position = { 322, 302 };
	play.frames(2);
	CHECK(play.get("player").variable.at("lives") == Approx(3));

	// Out of it, it costs a Yar and goes back to the Qotile.
	play.get("yar").position = { 100, 300 };
	play.get("missile").position = { 104, 304 };
	play.frames(2);
	CHECK(play.get("player").variable.at("lives") == Approx(2));
	CHECK(play.get("yar").position.x == Approx(60));
	CHECK(play.get("missile").position.x > 600.0f);

	// The cannon comes in at the left edge, its middle level with Yar's.
	play.get("yar").position = { 200, 400 };
	play.tap(KeyCode::Space);
	CHECK(play.get("cannon").isVisible);
	CHECK(play.get("cannon").position.y + 6 == Approx(412));
	CHECK(play.get("cannon").position.x == Approx(0));

	// Yar eats a cell of the shield.
	play.get("missile").position = { 10, 10 };
	Object& cell = play.get("shield.1.1");
	play.get("yar").position = cell.position - Vector2f{ 26, 0 };
	play.engine.handleKeyPressed(KeyCode::D);
	play.frames(3);
	play.engine.handleKeyReleased(KeyCode::D);
	CHECK_FALSE(play.get("shield.1.1").isVisible);
	CHECK(play.get("player").variable.at("score") >= 69.0f);
}
