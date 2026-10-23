// test_live.cpp
// XML Game Engine
// author: beefviper
// date: Oct 17, 2026
//
// Catch2 tests for values read when they are used: a command's numbers
// worked out as it runs (where things are, how they go, every variable as
// it is now, the touch a rule answers), <set>, <spawn> and a <tether>
// (more of <spawn> in test_spawn.cpp);
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

TEST_CASE("<spawn> makes a new object where and going as it says, read as it runs", "[live]")
{
	const Scratch file(box("ship", 300, 400, "<velocity><x>3</x><y>0</y></velocity>")
		+ "<object name=\"shot\"><sprite><rectangle><width>4</width><height>8</height></rectangle></sprite>"
		"<velocity><x>0</x><y>-5</y></velocity><exists>false</exists></object>",
		"<spawn object=\"shot\"><position><x><centered>ship.x + 5</centered></x><y>ship.y - 8</y></position>"
		"<velocity><x>ship.velocity.x</x></velocity></spawn>");
	Played play(file.path);

	play.tap(KeyCode::Space);
	play.game.settleSpawns();
	const Object& first = play.get("shot.1");
	CHECK(first.isVisible);
	CHECK(first.position.x == Approx(303));
	CHECK(first.position.y == Approx(392));
	CHECK(first.velocity.x == Approx(3));
	CHECK(first.velocity.y == Approx(-5)); // left out: as the object starts
	CHECK_FALSE(play.get("shot").exists);
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

TEST_CASE("a <tether> is in play only while both its ends are", "[live]")
{
	const Scratch file(box("rod", 100, 100) + box("hook", 300, 200, "<hidden>true</hidden>")
		+ "<object name=\"line\"><sprite><tether from=\"rod\" to=\"hook\" /></sprite>"
		"<position><x>0</x><y>0</y></position></object>", "<reveal object=\"hook\" />");
	Played play(file.path);

	play.frames(1);
	CHECK_FALSE(play.get("line").isVisible);
	play.tap(KeyCode::Space);
	play.frames(1);
	CHECK(play.get("line").isVisible);
	CHECK(play.get("line").size.x == Approx(201).margin(1));
}

TEST_CASE("an object brought back into play starts its timers over", "[live]")
{
	// A shot that goes out half a second after it comes in.
	const Scratch file(box("shot", 100, 100, "<hidden>true</hidden><timers><timer><every>0.5</every><die /></timer></timers>"), "<reveal object=\"shot\" />");
	Played play(file.path);

	play.tap(KeyCode::Space);
	play.frames(20);
	REQUIRE(play.get("shot").isVisible);

	// Put out early, and brought in again: it has its whole half second.
	play.get("shot").isVisible = false;
	play.tap(KeyCode::Space);
	play.frames(25);
	CHECK(play.get("shot").isVisible);
	play.frames(6);
	CHECK_FALSE(play.get("shot").isVisible);
}

TEST_CASE("a <chase> turns the heading of what chases to the way it goes", "[live]")
{
	const Scratch file(box("target", 500, 100)
		+ "<object name=\"missile\"><sprite><line><from><x>0</x><y>0</y></from><to><x>0</x><y>8</y></to><thickness>2</thickness></line></sprite>"
		"<position><x>100</x><y>100</y></position><heading>0</heading>"
		"<timers><timer><every>0.1</every><chase object=\"target\"><speed>2</speed></chase></timer></timers></object>", "<pop />");
	Played play(file.path);

	play.frames(10);
	CHECK(play.get("missile").heading == Approx(90).margin(1));
	CHECK(play.get("missile").velocity.x == Approx(2).margin(0.01));
}

TEST_CASE("a group gives its members a <heading>, each turned on its own", "[live]")
{
	const Scratch file(box("target", 500, 100)
		+ "<group name=\"missiles\"><sprite><line><from><x>0</x><y>0</y></from><to><x>0</x><y>8</y></to><thickness>2</thickness></line></sprite>"
		"<position><y>100</y></position><heading>180</heading>"
		"<timers><timer><every>0.1</every><chase object=\"target\"><speed>2</speed></chase></timer></timers>"
		"<member><position><x>100</x></position></member><member><position><x>900</x></position></member></group>", "<pop />");
	Played play(file.path);

	CHECK(play.get("missiles.1").hasHeading);
	CHECK(play.get("missiles.2").heading == Approx(180));
	play.frames(10);
	CHECK(play.get("missiles.1").heading == Approx(90).margin(1));
	CHECK(play.get("missiles.2").heading == Approx(270).margin(1));
}

TEST_CASE("an object out of play does not act on a key", "[live]")
{
	const Scratch file(box("gun", 100, 300, "<hidden>true</hidden><actions><action name=\"fire\"><fire object=\"shot\" /></action></actions>")
		+ box("shot", 0, 0, "<velocity><x>0</x><y>-4</y></velocity><hidden>true</hidden>"), "<trigger object=\"gun\" action=\"fire\" />");
	Played play(file.path);

	play.tap(KeyCode::Space);
	play.frames(1);
	CHECK_FALSE(play.get("shot").isVisible);

	play.get("gun").isVisible = true;
	play.tap(KeyCode::Space);
	play.frames(1);
	CHECK(play.get("shot").isVisible);
}

TEST_CASE("fishingderby.xml: a fish that bites is claimed by that hook, landed for its worth, and back in its row", "[live][fishingderby]")
{
	Played play("descriptions/fishingderby.xml");
	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "playing");

	// Keep the shark well away, and put the hook just over the first cod.
	play.get("shark").position = { 700, 195 };
	play.get("shark").velocity = { 0, 0 };
	Object& fish = play.get("cod.1");
	REQUIRE(fish.variable.at("worth") == Approx(6));
	const Vector2f start = fish.position;
	Object& hook = play.get("hook1");
	hook.position = { fish.position.x + 10, fish.position.y - 6 };
	play.frames(2);
	CHECK(play.get("hook1").carrying == fish.name);
	CHECK(play.get("hook1").variable.at("line") == Approx(2));
	CHECK(fish.variable.at("by") == Approx(1));
	CHECK(fish.velocity.x == Approx(0));

	// Reel it up.
	play.engine.handleKeyPressed(KeyCode::W);
	for (int i = 0; i < 400 && play.get("left").variable.at("score") == 0.0f; ++i) { play.frames(1); }
	play.engine.handleKeyReleased(KeyCode::W);

	CHECK(play.get("left").variable.at("score") == Approx(6));
	CHECK(play.get("right").variable.at("score") == Approx(0));
	CHECK(play.get("hook1").carrying.empty());
	CHECK(play.get("hook1").variable.at("line") == Approx(0));
	const Object& again = play.get("cod.1");
	CHECK(again.isVisible);
	CHECK(again.variable.at("by") == Approx(0));
	CHECK(again.position.y == Approx(start.y).margin(2));
}

TEST_CASE("fishingderby.xml: a hook on its way up takes no second fish, and the shark eats the one on it", "[live][fishingderby]")
{
	Played play("descriptions/fishingderby.xml");
	play.tap(KeyCode::Space);
	play.get("shark").position = { 700, 195 };
	play.get("shark").velocity = { 0, 0 };

	Object& cod = play.get("cod.1");
	Object& hook = play.get("hook1");
	hook.position = { cod.position.x + 10, cod.position.y - 6 };
	play.frames(2);
	REQUIRE(hook.carrying == cod.name);

	// Reel up through a perch: the hook keeps the cod.
	Object& perch = play.get("perch.2");
	perch.position = { hook.position.x - 10, hook.position.y - 30 };
	perch.velocity = { 0, 0 };
	play.engine.handleKeyPressed(KeyCode::W);
	play.frames(20);
	play.engine.handleKeyReleased(KeyCode::W);
	CHECK(play.get("hook1").carrying == cod.name);
	CHECK(play.get("perch.2").variable.at("by") == Approx(0));

	// The shark swims into the cod: gone, back in its row, the hook put back.
	Object& shark = play.get("shark");
	shark.position = { play.get("cod.1").position.x - 100, play.get("cod.1").position.y - 4 };
	shark.velocity = { 4, 0 };
	play.frames(30);
	CHECK(play.get("hook1").carrying.empty());
	CHECK(play.get("hook1").variable.at("line") == Approx(0));
	CHECK(play.get("cod.1").variable.at("by") == Approx(0));
	CHECK(play.get("cod.1").position.y == Approx(470).margin(2));
	CHECK(play.get("left").variable.at("score") == Approx(0));
}

TEST_CASE("fishingderby.xml: the computer takes its hook down and lands what bites", "[live][fishingderby]")
{
	Played play("descriptions/fishingderby.xml");
	play.tap(KeyCode::Space);
	play.get("shark").velocity = { 0, 0 };
	play.get("shark").position = { 0, 195 };
	const float startY = play.get("hook2").position.y;
	play.frames(30);
	CHECK(play.get("hook2").position.y > startY);
	for (int i = 0; i < 3000 && play.get("right").variable.at("score") == 0.0f; ++i) { play.frames(1); }
	CHECK(play.get("right").variable.at("score") > 0.0f);
	CHECK(play.get("left").variable.at("score") == Approx(0));
}
