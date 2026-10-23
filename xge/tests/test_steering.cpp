// test_steering.cpp
// XML Game Engine
// author: beefviper
// date: Oct 17, 2026
//
// Catch2 tests for being pulled toward something (<pull>), turning toward
// it (<seek>, a homing missile), hopping on a diagonal (Q*bert), hops that
// take time and hunting by hops (Coily), and a control moving an object
// along a path (<track path>).

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

TEST_CASE("a <hop> with <seconds> goes through the air that long, and a second waits for the landing", "[steering]")
{
	const Scratch file(box("frog", 100, 300, "<actions><action name=\"right\"><hop direction=\"right\"><distance>40</distance><seconds>0.5</seconds></hop></action>"
		"<action name=\"up\"><hop direction=\"up-right\"><x>10</x><y>20</y><seconds>0.25</seconds></hop></action></actions>"),
		"<trigger object=\"frog\" action=\"right\" />");
	Played play(file.path);
	Object& frog = play.get("frog");
	CommandExecutor executor(play.game);

	play.tap(KeyCode::Space);
	play.frames(15);
	CHECK(frog.isAirborne());
	CHECK(frog.position.x == Approx(120));
	executor.executeInput(CmdTriggerAction{ "frog", "up" }, true); // in the air: nothing
	play.frames(15);
	CHECK_FALSE(frog.isAirborne());
	CHECK(frog.position.x == Approx(140));
	CHECK(frog.position.y == Approx(300));

	executor.executeInput(CmdTriggerAction{ "frog", "up" }, true);
	play.frames(15);
	CHECK(frog.position.x == Approx(150));
	CHECK(frog.position.y == Approx(280));
	CHECK(frog.facing == Direction::Up);

	const auto load = [](const std::string& hop)
	{
		const Scratch bad(box("frog", 0, 0, "<actions><action name=\"go\">" + hop + "</action></actions>"), "<pop />");
		Game game(bad.path);
	};
	CHECK_THROWS_WITH(load("<hop direction=\"up\"><distance>4</distance><seconds>0</seconds></hop>"), ContainsSubstring("expected more than 0"));
	CHECK_THROWS_WITH(load("<hop direction=\"up\"><x>4</x><y>4</y></hop>"), ContainsSubstring("is not diagonal"));
}

TEST_CASE("a <hunt> by <hop>s makes the one hop that lands nearest what it hunts", "[steering]")
{
	const std::string hunts = "<actions>"
		"<action name=\"diagonal\"><hunt object=\"prey\"><hop><x>20</x><y>30</y></hop></hunt></action>"
		"<action name=\"straight\"><hunt object=\"prey\"><hop><distance>20</distance><seconds>0.1</seconds></hop></hunt></action></actions>";
	const Scratch file(box("hunter", 100, 100, hunts) + box("prey", 300, 160), "<pop />");
	Played play(file.path);
	Object& hunter = play.get("hunter");
	CommandExecutor executor(play.game);

	// Down and right is nearest.
	executor.executeInput(CmdTriggerAction{ "hunter", "diagonal" }, true);
	play.frames(1);
	CHECK(hunter.position.x == Approx(120));
	CHECK(hunter.position.y == Approx(130));

	// Level with it now, up and down tie: up goes first.
	play.get("prey").position = { 300, 130 };
	executor.executeInput(CmdTriggerAction{ "hunter", "diagonal" }, true);
	play.frames(1);
	CHECK(hunter.position.x == Approx(140));
	CHECK(hunter.position.y == Approx(100));

	// By straight hops, which take their time.
	executor.executeInput(CmdTriggerAction{ "hunter", "straight" }, true);
	play.frames(7);
	CHECK(hunter.position.x == Approx(160));
	CHECK(hunter.position.y == Approx(100));

	const auto load = [](const std::string& hunt)
	{
		const Scratch bad(box("hunter", 0, 0, "<actions><action name=\"go\">" + hunt + "</action></actions>") + box("prey", 50, 50), "<pop />");
		Game game(bad.path);
	};
	CHECK_THROWS_WITH(load("<hunt object=\"prey\"><hop direction=\"up\">20</hop></hunt>"), ContainsSubstring("direction"));
	CHECK_THROWS_WITH(load("<hunt object=\"ghost\"><hop>20</hop></hunt>"), ContainsSubstring("names 'ghost'"));
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

	// The red ball and Coily out of the way, where their hops go nowhere.
	for (const char* enemy : { "ball", "coily" })
	{
		play.get(enemy).position = { 760, 560 };
		play.get(enemy).positionOriginal = { 760, 560 };
	}

	// A hop takes a quarter of a second, 15 frames, through the air.
	const auto hop = [&](KeyCode key)
	{
		play.tap(key);
		play.frames(16);
	};

	// The cubes above the bottom row are numbered in reading order: 1 the top,
	// 2 and 3 the row below.
	play.tap(KeyCode::D);
	play.frames(8);
	CHECK(play.get("qbert").isAirborne());
	CHECK(play.get("upper.3").lookName() == "unlit"); // not until he lands
	play.frames(8);
	CHECK(play.get("qbert").position.x == Approx(420));
	CHECK(play.get("qbert").position.y == Approx(157));
	CHECK(play.get("qbert").lookName() == "downright");
	CHECK(play.get("upper.3").lookName() == "lit");
	CHECK(play.get("upper.2").lookName() == "unlit");
	CHECK(play.get("upper.1").lookName() == "unlit");
	CHECK(play.get("player").variable.at("score") == Approx(25));

	// Down and left: only the top he lands on is touched.
	hop(KeyCode::S);
	CHECK(play.get("upper.5").lookName() == "lit");
	CHECK(play.get("upper.2").lookName() == "unlit");
	CHECK(play.get("upper.1").lookName() == "unlit");
	CHECK(play.get("player").variable.at("score") == Approx(50));

	// Back up onto the top, and a second landing on a lit cube scores nothing.
	hop(KeyCode::W);
	hop(KeyCode::A);
	CHECK(play.get("qbert").lookName() == "upleft");
	CHECK(play.get("upper.1").lookName() == "lit");
	CHECK(play.get("player").variable.at("score") == Approx(75));
	hop(KeyCode::D);
	hop(KeyCode::A);
	CHECK(play.get("player").variable.at("score") == Approx(75));

	// Up and left off the top: into the void, and he curses.
	hop(KeyCode::A);
	CHECK(play.get("player").variable.at("lives") == Approx(2));
	CHECK(play.get("curse").lookName() == "said");
	CHECK(play.get("qbert").position.x == Approx(388));
	CHECK(play.get("qbert").position.y == Approx(109));
}

TEST_CASE("qbert.xml: Coily hatches on the bottom row and hunts Q*bert a hop at a time", "[steering][qbert]")
{
	Played play("descriptions/qbert.xml");
	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "playing");
	play.get("ball").position = { 760, 560 };
	play.get("ball").positionOriginal = { 760, 560 };

	// The egg, on the left of the sixth row, hops down and left onto the
	// bottom row (that cube's top centred at 208, 418) and hatches there.
	Object& coily = play.get("coily");
	coily.position = { 232, 354 };
	CHECK(coily.lookName() == "egg");
	CommandExecutor executor(play.game);
	executor.executeInput(CmdTriggerAction{ "coily", "left" }, true);
	play.frames(20);
	CHECK(coily.position.x == Approx(200));
	CHECK(coily.position.y == Approx(402));
	CHECK(coily.lookName() == "snake");
	CHECK(coily.variable.at("snake") == Approx(1));

	// The snake's hops go the diagonal that lands nearest Q*bert, up on the
	// top cube: up and right each time, a row nearer.
	play.frames(70);
	CHECK(coily.position.y < 402.0f);
	CHECK(coily.position.x > 200.0f);
	CHECK(play.get("player").variable.at("lives") == Approx(3));

	// Until it lands on him: a life, and Coily back to an egg at the start.
	play.frames(400);
	CHECK(play.get("player").variable.at("lives") < 3.0f);
	CHECK(coily.lookName() == "egg");
}

TEST_CASE("qbert.xml: a disc carries Q*bert back to the top once, and Coily chasing him falls off", "[steering][qbert]")
{
	Played play("descriptions/qbert.xml");
	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "playing");
	for (const char* enemy : { "ball", "coily" })
	{
		play.get(enemy).position = { 760, 560 };
		play.get(enemy).positionOriginal = { 760, 560 };
	}
	Object& qbert = play.get("qbert");
	Object& coily = play.get("coily");

	// On the left cube of the sixth row, up and left onto the disc beside
	// the fifth: back on the top, no life lost, and Coily, a snake by now,
	// after him.
	coily.variable["snake"] = 1.0f;
	qbert.position = { 228, 349 };
	play.frames(2);
	const float before = play.get("player").variable.at("score");
	play.tap(KeyCode::A);
	play.frames(20);
	CHECK(qbert.position.x == Approx(388));
	CHECK(qbert.position.y == Approx(109));
	CHECK(play.get("player").variable.at("lives") == Approx(3));
	CHECK(play.get("discs.1").lookName() == "used");
	CHECK(play.get("discs.2").lookName() == "ready");
	CHECK(play.get("player").variable.at("score") - before == Approx(500));
	CHECK(coily.variable.at("snake") == Approx(0));

	// The disc is gone: the same hop again falls.
	qbert.position = { 228, 349 };
	play.frames(2);
	play.tap(KeyCode::A);
	play.frames(20);
	CHECK(play.get("player").variable.at("lives") == Approx(2));
}

TEST_CASE("yarsrevenge.xml: the missile homes on Yar, and the neutral zone keeps him safe", "[steering][yarsrevenge]")
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
	CHECK(play.get("player").variable.at("lives") == Approx(4));

	// Out of it, it costs a Yar and goes back to the Qotile.
	play.get("yar").position = { 100, 300 };
	play.get("missile").position = { 104, 304 };
	play.frames(2);
	CHECK(play.get("player").variable.at("lives") == Approx(3));
	CHECK(play.get("yar").position.x == Approx(60));
	CHECK(play.get("missile").position.x > 600.0f);

	// Off the top, Yar comes in at the bottom.
	play.get("missile").position = { 700, 500 };
	play.get("yar").position = { 100, 4 };
	play.engine.handleKeyPressed(KeyCode::W);
	play.frames(12);
	play.engine.handleKeyReleased(KeyCode::W);
	CHECK(play.get("yar").position.y > 500.0f);
}

TEST_CASE("yarsrevenge.xml: the shield and the Qotile go up and down together", "[steering][yarsrevenge]")
{
	Played play("descriptions/yarsrevenge.xml");
	play.tap(KeyCode::Space);
	play.get("missile").position = { 10, 10 };
	const Object& qotile = play.get("qotile");
	const Object& cell = play.get("cells.1");
	const float gap = qotile.position.y - cell.position.y;
	const float top = cell.position.y;

	play.frames(180);
	CHECK(cell.position.y > top + 100.0f);
	CHECK(qotile.position.y - cell.position.y == Approx(gap));

	// Past the turn: on the way back up, still together.
	play.frames(300);
	CHECK(cell.velocity.y < 0.0f);
	CHECK(qotile.velocity.y < 0.0f);
	CHECK(qotile.position.y - cell.position.y == Approx(gap));
}

TEST_CASE("yarsrevenge.xml: a bite brings the cannon level with Yar, and its blast takes the Qotile", "[steering][yarsrevenge]")
{
	Played play("descriptions/yarsrevenge.xml");
	play.tap(KeyCode::Space);
	play.get("missile").position = { 10, 10 };

	// With no cannon, Space fires only Yar's energy missile.
	play.tap(KeyCode::Space);
	play.frames(1);
	CHECK(play.get("energy").isVisible);
	CHECK_FALSE(play.get("cannon").isVisible);
	CHECK_FALSE(play.get("blast").isVisible);

	// Yar eats a cell of the shield.
	const Object& cell = play.get("cells.1");
	play.get("yar").position = cell.position - Vector2f{ 26, 0 };
	play.engine.handleKeyPressed(KeyCode::D);
	play.frames(3);
	play.engine.handleKeyReleased(KeyCode::D);
	CHECK_FALSE(cell.isVisible);
	CHECK(play.get("player").variable.at("score") >= 69.0f);

	// The cannon is in at the left edge, following him up and down.
	const Object& cannon = play.get("cannon");
	REQUIRE(cannon.isVisible);
	CHECK(cannon.position.x == Approx(0));
	play.get("yar").position = { 120, 400 };
	play.frames(2);
	CHECK(cannon.position.y == Approx(404));

	// Space fires it, and it is gone until the next bite.
	play.tap(KeyCode::Space);
	play.frames(1);
	CHECK(play.get("blast").isVisible);
	CHECK_FALSE(cannon.isVisible);

	// Into the Qotile: 1000, and a moment later the shield is back whole.
	Object& qotile = play.get("qotile");
	play.get("blast").position = { qotile.position.x - 25, qotile.position.y + 4 };
	play.frames(4);
	CHECK_FALSE(qotile.isVisible);
	CHECK(play.get("player").variable.at("score") >= 1069.0f);
	play.frames(1);
	CHECK(play.state() == "destroyed");
	play.frames(130);
	CHECK(play.state() == "playing");
	CHECK(qotile.isVisible);
	CHECK(cell.isVisible);
}

TEST_CASE("yarsrevenge.xml: the Swirl leaves the Qotile and flies at where Yar is", "[steering][yarsrevenge]")
{
	Played play("descriptions/yarsrevenge.xml");
	play.tap(KeyCode::Space);
	// The Swirls there are: none until the Qotile spawns one.
	const auto swirls = [&]()
	{
		std::vector<const Object*> found;
		for (const Object& object : play.game.getCurrentObjects())
		{
			if (object.exists && object.copyOf == "playing.swirl") { found.push_back(&object); }
		}
		return found;
	};
	CHECK(swirls().empty());

	int waited = 0;
	for (; waited < 600 && swirls().empty(); ++waited)
	{
		play.get("missile").position = { 10, 10 };
		play.frames(1);
	}
	REQUIRE(swirls().size() == 1);
	CHECK(waited >= 5 * 60 - 1);
	CHECK(swirls().front()->position.x > 600.0f);
	CHECK(swirls().front()->velocity.x == 0.0f);

	play.get("missile").position = { 10, 10 };
	play.frames(61);
	REQUIRE(swirls().size() == 1); // one at a time
	CHECK(swirls().front()->velocity.x < -3.0f);
}
