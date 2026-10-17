// test_live.cpp
// XML Game Engine
// author: beefviper
// date: Oct 17, 2026
//
// Catch2 tests for values read when they are used: a command's numbers
// worked out as it runs (where things are, how they go, every variable as
// it is now, the touch a rule answers), <set>, <spawn> and a <tether>;
// then played in games/descriptions/fishingderby.xml.

#include "engine/engine.h"
#include "game/game.h"
#include "play/collision.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

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

	// A game of one state, "playing", with these objects and these
	// commands on the space bar, written out to be loaded and played.
	class Scratch
	{
	public:
		Scratch(const std::string& objects, const std::string& onSpace)
		{
			std::ofstream out(path);
			out << "<game xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"../../xge/xgedef.xsd\">"
				"<window name=\"live\"><width>800</width><height>600</height><background>color.black</background>"
				"<fullscreen>false</fullscreen><framerate>60</framerate></window>"
				"<variables><variable name=\"unused\">0</variable></variables>"
				"<states><state name=\"playing\"><objects>" << objects << "</objects>"
				"<inputs><input button=\"space\">" << onSpace << "</input></inputs></state></states></game>";
		}
		~Scratch() { std::filesystem::remove(path); }
		Scratch(const Scratch&) = delete;
		Scratch& operator=(const Scratch&) = delete;

		const std::string path = "descriptions/live_scratch.xml";
	};

	std::string box(const std::string& name, float x, float y, const std::string& rest = "")
	{
		return "<object name=\"" + name + "\"><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite>"
			"<position><x>" + std::to_string(x) + "</x><y>" + std::to_string(y) + "</y></position>" + rest + "</object>";
	}
}

TEST_CASE("a command's value is read when it runs: a variable as it is now, where things are and how they go", "[live]")
{
	const Scratch file(box("ball", 100, 50, "<velocity><x>2</x><y>0</y></velocity><variables><variable name=\"step\">5</variable><variable name=\"count\">0</variable></variables>"),
		"<inc variable=\"ball.count\">ball.step</inc><set variable=\"ball.velocity.y\">ball.velocity.x * 3</set><set variable=\"ball.step\">ball.x</set>");
	Played play(file.path);

	play.tap(KeyCode::Space);
	CHECK(play.get("ball").variable.at("count") == Approx(5));
	CHECK(play.get("ball").velocity.y == Approx(6));
	CHECK(play.get("ball").variable.at("step") == Approx(100));

	// The step it adds is the one it was just set to.
	play.tap(KeyCode::Space);
	CHECK(play.get("ball").variable.at("count") == Approx(105));
}

TEST_CASE("<set> puts a value into where an object is, or into every member of a group", "[live]")
{
	const Scratch file(box("ball", 100, 50)
		+ "<group name=\"row\"><sprite><rectangle><width>4</width><height>4</height></rectangle></sprite>"
		"<member><position><x>0</x><y>300</y></position></member><member><position><x>20</x><y>300</y></position></member></group>",
		"<set variable=\"ball.x\">400</set><set variable=\"row.y\">ball.x / 2</set><set variable=\"row.velocity.x\">-1</set>");
	Played play(file.path);

	play.tap(KeyCode::Space);
	CHECK(play.get("ball").position.x == Approx(400));
	CHECK(play.get("row.1").position.y == Approx(200));
	CHECK(play.get("row.2").position.y == Approx(200));
	CHECK(play.get("row.2").velocity.x == Approx(-1));
}

TEST_CASE("where things are is only known while the game runs, and <set> names something there is", "[live]")
{
	const auto load = [](const std::string& objects, const std::string& onSpace)
	{
		const Scratch file(objects, onSpace);
		Game game(file.path);
	};

	CHECK_THROWS_WITH(load(box("ball", 100, 50) + box("other", 0, 0) + "<object name=\"late\"><sprite><rectangle><width>ball.x</width><height>4</height></rectangle></sprite>"
		"<position><x>0</x><y>0</y></position></object>", "<pop />"),
		ContainsSubstring("ball.x, which is only known while the game runs"));
	CHECK_THROWS_WITH(load(box("ball", 100, 50), "<set variable=\"bat.x\">1</set>"), ContainsSubstring("there is no object named 'bat'"));
	CHECK_THROWS_WITH(load(box("ball", 100, 50), "<set variable=\"ball.heading\">90</set>"), ContainsSubstring("has no <heading> to set"));
	CHECK_THROWS_WITH(load(box("ball", 100, 50), "<spawn object=\"shots\" />"), ContainsSubstring("shots"));
	CHECK_NOTHROW(load(box("ball", 100, 50), "<set variable=\"ball.velocity.x\">ball.x + 1</set>"));
}

TEST_CASE("<spawn> brings in the first of a group out of play, where and going as it says", "[live]")
{
	const Scratch file(box("ship", 300, 400, "<velocity><x>3</x><y>0</y></velocity>")
		+ "<group name=\"shots\"><sprite><rectangle><width>4</width><height>8</height></rectangle></sprite>"
		"<velocity><x>0</x><y>-5</y></velocity><hidden>true</hidden>"
		"<member><position><x>0</x><y>0</y></position></member><member><position><x>0</x><y>0</y></position></member></group>",
		"<spawn object=\"shots\"><position><x><centered>ship.x + 5</centered></x><y>ship.y - 8</y></position>"
		"<velocity><x>ship.velocity.x</x></velocity></spawn>");
	Played play(file.path);

	play.tap(KeyCode::Space);
	const Object& first = play.get("shots.1");
	CHECK(first.isVisible);
	CHECK(first.position.x == Approx(303));
	CHECK(first.position.y == Approx(392));
	CHECK(first.velocity.x == Approx(3));
	CHECK(first.velocity.y == Approx(-5)); // left out: its own
	CHECK_FALSE(play.get("shots.2").isVisible);

	play.tap(KeyCode::Space);
	CHECK(play.get("shots.2").isVisible);

	// None left out of play: nothing.
	const Vector2f was = play.get("shots.1").position;
	play.tap(KeyCode::Space);
	CHECK(play.get("shots.1").position.x == Approx(was.x));
}

TEST_CASE("a rule's value can read the touch it answers", "[live]")
{
	const Scratch file(box("ball", 100, 100, "<velocity><x>20</x><y>0</y></velocity><collisions><enabled>true</enabled>"
		"<collision object=\"wall\"><set variable=\"marker.x\">touch.x</set><set variable=\"marker.y\">touch.normal.x</set><stop /></collision></collisions>")
		+ box("wall", 200, 95, "<collisions><enabled>true</enabled></collisions>") + box("marker", 0, 0), "<pop />");
	Played play(file.path);

	play.frames(10);
	CHECK(play.get("marker").position.x == Approx(200));
	CHECK(play.get("marker").position.y == Approx(-1)); // out of the wall, toward the ball
}

TEST_CASE("a <tether> is drawn from one object's middle to another's, wherever they go", "[live]")
{
	const Scratch file(box("rod", 100, 100) + box("hook", 300, 200, "<velocity><x>0</x><y>2</y></velocity>")
		+ "<object name=\"line\"><sprite><tether from=\"rod\" to=\"hook\"><thickness>2</thickness><color>color.red</color></tether></sprite>"
		"<position><x>0</x><y>0</y></position></object>", "<pop />");
	Played play(file.path);

	play.frames(1);
	const Object& line = play.get("line");
	REQUIRE(line.bitmap);
	CHECK(line.position.x == Approx(104));
	CHECK(line.position.y == Approx(104));
	CHECK(line.size.x == Approx(202).margin(1));
	CHECK(line.size.y == Approx(102 + 2).margin(1));
	CHECK(line.bitmap->solidAt(1, 1));

	play.frames(10);
	CHECK(play.get("line").size.y == Approx(124).margin(1));

	const auto load = [](const std::string& objects)
	{
		const Scratch bad(objects, "<pop />");
		Game game(bad.path);
	};
	CHECK_THROWS_WITH(load(box("rod", 0, 0) + "<object name=\"line\"><sprite><tether from=\"rod\" to=\"hook\" /></sprite><position><x>0</x><y>0</y></position></object>"),
		ContainsSubstring("there is no object named 'hook'"));
}

TEST_CASE("fishingderby.xml: a fish reeled up to the surface is landed for its worth, and comes back faster", "[live][fishingderby]")
{
	Played play("descriptions/fishingderby.xml");
	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "playing");

	// Put the hook just over a deep fish and let it bite.
	// The first of the deep row.
	Object& fish = play.get("deep.1");
	REQUIRE(fish.variable.at("worth") == Approx(6));
	Object& hook = play.get("hook");
	hook.position = { fish.position.x + 10, fish.position.y - 6 };
	play.frames(2);
	CHECK(play.get("hook").carrying == fish.name);
	CHECK(fish.velocity.x == Approx(0));

	// Keep the shark out of the way, and reel up.
	play.get("shark").position = { 700, 220 };
	play.get("shark").velocity = { 0, 0 };
	const float sharkWas = 2.0f;
	play.get("shark").velocity.x = sharkWas;
	play.engine.handleKeyPressed(KeyCode::W);
	for (int i = 0; i < 200 && play.get("player").variable.at("score") == 0.0f; ++i) { play.frames(1); }
	play.engine.handleKeyReleased(KeyCode::W);

	CHECK(play.get("player").variable.at("score") == Approx(6));
	CHECK(std::abs(play.get("shark").velocity.x) == Approx(sharkWas * 1.1f));

	// Back in at the side, at its own depth, going as fast as the score says.
	const Object& again = play.get("deep.1");
	CHECK(again.isVisible);
	CHECK(again.position.y == Approx(200 + 60 + 6 * 30));
	CHECK(std::abs(again.velocity.x) == Approx(1 + 6.0f / 30));
	CHECK(play.get("hook").carrying.empty());
}
