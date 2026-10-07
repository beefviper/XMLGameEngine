// test_gameplay_verbs.cpp
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026
//
// Catch2 tests for timers (an object's and a state's, <every> and <after>),
// <facing> and firing along it, <jump>, <reverse />, <reset object> in a
// collision, and named <keys> sets with several keys to one <input>.
//
// Each game is written beside the shipped ones so that its schema path
// resolves, and loaded with Xerces (full validation) or TinyXML2 (the built-in
// validator), so every test also checks that the schema takes the new tags.
// Frames are played with Game::updateObjects(); nothing needs a window.

#include "command_executor.h"
#include "game.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace xge;
using Catch::Approx;
using Catch::Matchers::ContainsSubstring;

namespace
{
	struct ScratchFile
	{
		std::filesystem::path path;

		explicit ScratchFile(std::filesystem::path where) : path(std::move(where)) {}
		ScratchFile(const ScratchFile&) = delete;
		ScratchFile& operator=(const ScratchFile&) = delete;
		~ScratchFile()
		{
			std::error_code ignored;
			std::filesystem::remove(path, ignored);
		}
	};

	// A whole game file: 800 by 600 at 60 frames a second, the objects and
	// states given, and the <keys> sets given before the states.
	std::string gameXml(const std::string& objects, const std::string& states, const std::string& keys = {})
	{
		return "<game xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"../xgedef.xsd\">"
			"<window name=\"verbs\"><width>800</width><height>600</height><background>color.black</background>"
			"<fullscreen>false</fullscreen><framerate>60</framerate></window>"
			"<variables><variable name=\"unused\">0</variable></variables>"
			"<objects>" + objects + "</objects>"
			"<states>" + keys + states + "</states>"
			"</game>";
	}

	// A rectangle object; `extra` goes after <velocity> (facing, hidden), and
	// `after` after <collisions> (actions, variables, timers).
	std::string box(const std::string& name, float x, float y, float w, float h, const std::string& collisions = "<enabled>false</enabled>",
		const std::string& extra = {}, const std::string& after = {}, const std::string& velocity = "<x>0</x><y>0</y>", const std::string& attributes = {})
	{
		return "<object name=\"" + name + "\"" + attributes + "><sprite><rectangle><width>" + std::to_string(w) + "</width><height>" + std::to_string(h)
			+ "</height></rectangle></sprite><position><x>" + std::to_string(x) + "</x><y>" + std::to_string(y) + "</y></position>"
			"<velocity>" + velocity + "</velocity>" + extra + "<collisions>" + collisions + "</collisions>" + after + "</object>";
	}

	// One state showing the objects named, with Space popping and the extra
	// parts (conditions, timers) after the inputs.
	std::string state(const std::string& name, const std::vector<std::string>& shows, const std::string& inputs = "<input button=\"space\"><pop /></input>",
		const std::string& extra = {}, const std::string& inputsAttributes = {})
	{
		std::string xml = "<state name=\"" + name + "\"><shows>";
		for (const auto& show : shows) { xml += "<show object=\"" + show + "\" />"; }
		return xml + "</shows><inputs" + inputsAttributes + ">" + inputs + "</inputs>" + extra + "</state>";
	}

	struct Loaded
	{
		ScratchFile file;
		Game game;

		explicit Loaded(const std::string& xml, XmlBackend backend = XmlBackend::Xerces) :
			file(write(xml)),
			game(file.path.string(), backend)
		{
			// What Engine's constructor does: the first state is the one shown,
			// and every shape measured (as a window backend would).
			game.setCurrentState(0);
			for (auto& object : game.getCurrentObjects())
			{
				object.size = measureShapeSize(object.spriteParams, object.shapeKind);
			}
		}

		static std::filesystem::path write(const std::string& xml)
		{
			const std::filesystem::path path = "games/verbs_scratch.xml";
			std::ofstream out(path);
			out << xml;
			return path;
		}

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { game.updateObjects(); }
		}

		float variable(const std::string& object, const std::string& name)
		{
			return game.getObject(object).variable.at(name);
		}
	};

	const std::string kCounter = "<variables><variable name=\"n\">0</variable><variable name=\"gap\">1</variable></variables>";
}

TEST_CASE("an object's <every> timer goes off again and again, a frame count of seconds apart", "[timers]")
{
	Loaded loaded(gameXml(
		box("clock", 0, 0, 1, 1, "<enabled>false</enabled>", {}, kCounter + "<timers><timer><every>0.5</every><inc variable=\"clock.n\" /></timer></timers>"),
		state("playing", { "clock" })));

	loaded.frames(29);
	CHECK(loaded.variable("clock", "n") == 0);
	loaded.frames(1);
	CHECK(loaded.variable("clock", "n") == 1);
	loaded.frames(60);
	CHECK(loaded.variable("clock", "n") == 3);
}

TEST_CASE("an <after> timer goes off once", "[timers]")
{
	Loaded loaded(gameXml(
		box("clock", 0, 0, 1, 1, "<enabled>false</enabled>", {}, kCounter + "<timers><timer><after>1</after><inc variable=\"clock.n\">5</inc></timer></timers>"),
		state("playing", { "clock" })));

	loaded.frames(59);
	CHECK(loaded.variable("clock", "n") == 0);
	loaded.frames(300);
	CHECK(loaded.variable("clock", "n") == 5);
}

TEST_CASE("an object's timer counts only while it is shown, and a reset starts it over", "[timers]")
{
	Loaded loaded(gameXml(
		box("clock", 0, 0, 1, 1, "<enabled>false</enabled>", {}, kCounter + "<timers><timer><every>1</every><inc variable=\"clock.n\" /></timer></timers>")
		+ box("other", 0, 0, 1, 1),
		state("menu", { "other" }) + state("playing", { "clock" })));

	loaded.frames(200);
	CHECK(loaded.variable("clock", "n") == 0);

	loaded.game.pushState("playing");
	loaded.frames(40);
	loaded.game.resetObject("clock");
	loaded.frames(40);
	CHECK(loaded.variable("clock", "n") == 0); // started over at the reset
	loaded.frames(20);
	CHECK(loaded.variable("clock", "n") == 1);
}

TEST_CASE("a state's timer counts while it is the current state and carries on after a pause", "[timers]")
{
	Loaded loaded(gameXml(
		box("score", 0, 0, 1, 1, "<enabled>false</enabled>", {}, kCounter),
		state("playing", { "score" }, "<input button=\"space\"><push state=\"paused\" /></input>",
			"<timers><timer><every>1</every><inc variable=\"score.n\" /></timer><timer><after>3</after><push state=\"over\" /></timer></timers>")
		+ state("paused", { "score" }) + state("over", { "score" })));

	loaded.frames(50);
	loaded.game.pushState("paused");
	loaded.frames(100);
	CHECK(loaded.variable("score", "n") == 0);
	loaded.game.popState();
	loaded.frames(10);
	CHECK(loaded.variable("score", "n") == 1);

	// Three seconds of play in all: the <after> pushes the end.
	loaded.frames(119);
	CHECK(loaded.game.getCurrentState().name == "playing");
	loaded.frames(1);
	CHECK(loaded.game.getCurrentState().name == "over");

	// A whole-game reset starts the state's timers over too.
	loaded.game.resetAll();
	loaded.frames(179);
	CHECK(loaded.game.getCurrentState().name == "playing");
}

TEST_CASE("an interval is worked out again every round, so it can follow a variable or be random", "[timers]")
{
	SECTION("a variable")
	{
		Loaded loaded(gameXml(
			box("clock", 0, 0, 1, 1, "<enabled>false</enabled>", {}, kCounter
				+ "<timers><timer><every>clock.gap</every><inc variable=\"clock.n\" /><inc variable=\"clock.gap\" /></timer></timers>"),
			state("playing", { "clock" })));

		loaded.frames(60);
		CHECK(loaded.variable("clock", "n") == 1);
		loaded.frames(119); // the next one waits two seconds
		CHECK(loaded.variable("clock", "n") == 1);
		loaded.frames(1);
		CHECK(loaded.variable("clock", "n") == 2);
	}

	SECTION("a random number")
	{
		Loaded loaded(gameXml(
			box("clock", 0, 0, 1, 1, "<enabled>false</enabled>", {}, kCounter
				+ "<timers><timer><every><random min=\"0.1\" max=\"1\" /></every><inc variable=\"clock.n\" /></timer></timers>"),
			state("playing", { "clock" })));

		std::set<int> gaps;
		int last = 0;
		float seen = 0;
		for (int frame = 1; frame <= 1200; ++frame)
		{
			loaded.game.updateObjects();
			if (loaded.variable("clock", "n") != seen)
			{
				seen = loaded.variable("clock", "n");
				CHECK(frame - last >= 6);
				CHECK(frame - last <= 60);
				gaps.insert(frame - last);
				last = frame;
			}
		}
		CHECK(gaps.size() > 3);
	}
}

TEST_CASE("a timer on an enemy fires its shots the way it faces", "[timers][facing]")
{
	Loaded loaded(gameXml(
		box("enemy", 100, 100, 20, 10, "<enabled>true</enabled>", "<facing>down</facing>", "<timers><timer><every>0.5</every><fire object=\"bomb\" /></timer></timers>")
		+ box("bomb", 0, 0, 4, 6, "<enabled>false</enabled><collision edge=\"all\"><die /></collision>", "<hidden>true</hidden>", {}, "<x>0</x><y>-3</y>"),
		state("playing", { "enemy", "bomb" })));

	Object& bomb = loaded.game.getObject("bomb");
	loaded.frames(29);
	CHECK_FALSE(bomb.isVisible);
	loaded.frames(1);
	REQUIRE(bomb.isVisible);
	CHECK(bomb.collisionData.enabled);

	// Below the middle of the enemy, falling at the bomb's own speed.
	CHECK(bomb.velocity.x == Approx(0));
	CHECK(bomb.velocity.y == Approx(3));
	CHECK(bomb.position.x == Approx(108));
	CHECK(bomb.position.y == Approx(110 + 3)); // and it has made its first move
}

TEST_CASE("an object with a <facing> faces the way it last moved, and fires that way", "[facing]")
{
	const std::string actions = "<actions><action name=\"left\"><move direction=\"left\">2</move></action>"
		"<action name=\"right\"><hop direction=\"right\">10</hop></action>"
		"<action name=\"gun\"><fire object=\"shot\" /></action></actions>";

	Loaded loaded(gameXml(
		box("man", 100, 100, 20, 20, "<enabled>true</enabled>", "<facing>up</facing>", actions)
		+ box("shot", 0, 0, 4, 4, "<enabled>false</enabled>", "<hidden>true</hidden>", {}, "<x>0</x><y>-6</y>"),
		state("playing", { "man", "shot" })));

	CommandExecutor executor(loaded.game);
	Object& man = loaded.game.getObject("man");
	Object& shot = loaded.game.getObject("shot");

	executor.executeInput(CmdTriggerAction{ "man", "gun" }, true);
	CHECK(shot.position.x == Approx(108));
	CHECK(shot.position.y == Approx(96));
	CHECK(shot.velocity.y == Approx(-6));

	loaded.game.resetObject("shot");
	executor.executeInput(CmdTriggerAction{ "man", "left" }, true);
	executor.executeInput(CmdTriggerAction{ "man", "left" }, false);
	CHECK(man.facing == Direction::Left);
	executor.executeInput(CmdTriggerAction{ "man", "gun" }, true);
	CHECK(shot.position.x == Approx(96));
	CHECK(shot.position.y == Approx(108));
	CHECK(shot.velocity.x == Approx(-6));
	CHECK(shot.velocity.y == Approx(0));

	loaded.game.resetObject("shot");
	executor.executeInput(CmdTriggerAction{ "man", "right" }, true);
	CHECK(man.facing == Direction::Right);
	executor.executeInput(CmdTriggerAction{ "man", "gun" }, true);
	CHECK(shot.position.x == Approx(120));
	CHECK(shot.velocity.x == Approx(6));

	// A reset faces it the way it started.
	loaded.game.resetObject("man");
	CHECK(man.facing == Direction::Up);
}

TEST_CASE("a jump takes its time, passes over what is in the way, and meets what it lands on", "[jump]")
{
	const std::string actions = "<actions><action name=\"down\"><jump direction=\"down\"><distance>60</distance><seconds>0.5</seconds></jump></action></actions>"
		"<variables><variable name=\"hits\">0</variable></variables>";
	const std::string rules = "<enabled>true</enabled><collision class=\"hazard\"><inc variable=\"man.hits\" /></collision>";

	Loaded loaded(gameXml(
		box("man", 100, 100, 20, 20, rules, {}, actions)
		+ box("bar", 0, 130, 800, 10, "<enabled>true</enabled>", {}, {}, "<x>0</x><y>0</y>", " class=\"hazard\"")
		+ box("pit", 0, 170, 800, 30, "<enabled>true</enabled>", {}, {}, "<x>0</x><y>0</y>", " class=\"hazard\""),
		state("playing", { "man", "bar", "pit" })));

	CommandExecutor executor(loaded.game);
	Object& man = loaded.game.getObject("man");

	executor.executeInput(CmdTriggerAction{ "man", "down" }, true);
	REQUIRE(man.isAirborne());
	loaded.frames(15);
	CHECK(man.position.y == Approx(130));
	CHECK(man.isAirborne());

	// A second press in the air does nothing.
	executor.executeInput(CmdTriggerAction{ "man", "down" }, true);
	loaded.frames(15);
	CHECK(man.position.y == Approx(160));
	CHECK_FALSE(man.isAirborne());

	// It went over the bar without touching it, and landed in the pit.
	CHECK(loaded.variable("man", "hits") >= 1);
	const float onLanding = loaded.variable("man", "hits");
	CHECK(onLanding <= 2);
}

TEST_CASE("nothing is touched in the air: a jump over a hazard and clear of it costs nothing", "[jump]")
{
	const std::string actions = "<actions><action name=\"down\"><jump direction=\"down\"><distance>80</distance></jump></action></actions>"
		"<variables><variable name=\"hits\">0</variable></variables>";

	Loaded loaded(gameXml(
		box("man", 100, 100, 20, 20, "<enabled>true</enabled><collision class=\"hazard\"><inc variable=\"man.hits\" /></collision>", {}, actions)
		+ box("bar", 0, 140, 800, 20, "<enabled>true</enabled>", {}, {}, "<x>0</x><y>0</y>", " class=\"hazard\""),
		state("playing", { "man", "bar" })));

	CommandExecutor executor(loaded.game);
	executor.executeInput(CmdTriggerAction{ "man", "down" }, true);
	loaded.frames(18); // 0.3 seconds, the default
	CHECK(loaded.game.getObject("man").position.y == Approx(180));
	loaded.frames(5);
	CHECK(loaded.variable("man", "hits") == 0);
}

TEST_CASE("<reverse /> turns an object round, and a collision can reset another object", "[reverse]")
{
	Loaded loaded(gameXml(
		box("walker", 400, 100, 10, 10, "<enabled>true</enabled><collision edge=\"right\"><reset object=\"marker\" /><reverse /></collision>",
			{}, "<timers><timer><every>0.5</every><reverse /></timer></timers>", "<x>3</x><y>1</y>")
		+ box("marker", 50, 50, 10, 10, "<enabled>false</enabled>", {}, {}, "<x>1</x><y>0</y>"),
		state("playing", { "walker", "marker" })));

	Object& walker = loaded.game.getObject("walker");
	loaded.frames(29);
	CHECK(walker.velocity.x == Approx(3));
	loaded.frames(1);
	CHECK(walker.velocity.x == Approx(-3));
	CHECK(walker.velocity.y == Approx(-1));

	// Off the right edge: the marker goes back to where it started.
	walker.position = { 795, 100 };
	walker.velocity = { 3, 0 };
	Object& marker = loaded.game.getObject("marker");
	CHECK(marker.position.x > 50);
	loaded.frames(1);
	CHECK(marker.position.x == Approx(51)); // back at 50, and moved on a pixel
	CHECK(walker.velocity.x == Approx(-3));
}

TEST_CASE("a <keys> set is shared by states, one <input> can name several keys, and a state's own binding wins", "[keys]")
{
	const std::string keys = "<keys name=\"move\">"
		"<input button=\"a left\"><trigger object=\"man\" action=\"left\" /></input>"
		"<input button=\"space\"><push state=\"paused\" /></input>"
		"</keys>";
	const std::string actions = "<actions><action name=\"left\"><move direction=\"left\">2</move></action></actions>";

	Loaded loaded(gameXml(
		box("man", 100, 100, 20, 20, "<enabled>false</enabled>", {}, actions),
		state("playing", { "man" }, {}, {}, " keys=\"move\"")
		+ state("special", { "man" }, "<input button=\"a\"><push state=\"paused\" /></input>", {}, " keys=\"move\"")
		+ state("paused", { "man" })
		, keys), XmlBackend::TinyXml2);

	const auto& states = loaded.game.getStates();
	const State& playing = states.at(0);
	const State& special = states.at(1);

	REQUIRE(playing.input.count(KeyCode::A));
	REQUIRE(playing.input.count(KeyCode::Left));
	CHECK(std::holds_alternative<CmdTriggerAction>(playing.input.at(KeyCode::A).at(0)));
	CHECK(std::holds_alternative<CmdTriggerAction>(playing.input.at(KeyCode::Left).at(0)));
	CHECK(std::holds_alternative<CmdPushState>(playing.input.at(KeyCode::Space).at(0)));

	// The special state keeps the set's Left, and A is its own.
	CHECK(std::holds_alternative<CmdTriggerAction>(special.input.at(KeyCode::Left).at(0)));
	CHECK(std::holds_alternative<CmdPushState>(special.input.at(KeyCode::A).at(0)));
}

TEST_CASE("mistakes in the new tags are turned away when the game loads, saying where", "[timers][facing][jump][keys][errors]")
{
	const auto fails = [](const std::string& objects, const std::string& states, const std::string& expected)
	{
		INFO(objects << states);
		const std::string xml = "<game><window name=\"verbs\"><width>800</width><height>600</height><background>color.black</background>"
			"<fullscreen>false</fullscreen><framerate>60</framerate></window><variables><variable name=\"unused\">0</variable></variables>"
			"<objects>" + objects + "</objects><states>" + states + "</states></game>";
		ScratchFile scratch{ std::filesystem::temp_directory_path() / "xge_verbs_errors.xml" };
		{
			std::ofstream out(scratch.path);
			out << xml;
		}
		CHECK_THROWS_WITH(Game{ scratch.path.string() }, ContainsSubstring(expected));
	};

	const std::string plain = state("playing", { "o" });

	fails(box("o", 0, 0, 1, 1, "<enabled>false</enabled>", "<facing>north</facing>"), plain, "<facing> is \"north\"");
	fails(box("o", 0, 0, 1, 1, "<enabled>false</enabled>", {}, "<timers><timer><pop /></timer></timers>"), plain, "needs an <every> or an <after>");
	fails(box("o", 0, 0, 1, 1, "<enabled>false</enabled>", {}, "<timers><timer><every>0</every></timer></timers>"), plain, "<every> is 0");
	fails(box("o", 0, 0, 1, 1, "<enabled>false</enabled>", {}, "<timers><timer><after>1</after><push state=\"nowhere\" /></timer></timers>"), plain,
		"<push state=\"nowhere\" /> names no state");
	fails(box("o", 0, 0, 1, 1, "<enabled>false</enabled>", {}, "<actions><action name=\"j\"><jump direction=\"up\"><seconds>1</seconds></jump></action></actions>"),
		plain, "missing <distance>");
	fails(box("o", 0, 0, 1, 1, "<enabled>false</enabled>", {}, "<actions><action name=\"j\"><jump direction=\"up\"><distance>5</distance><seconds>0</seconds></jump></action></actions>"),
		plain, "<jump> has <seconds> of 0");
	fails(box("o", 0, 0, 1, 1), state("playing", { "o" }, {}, {}, " keys=\"nothing\""), "keys=\"nothing\" names no <keys> set");
}

TEST_CASE("an object with several named sprites can <become> each of them, and a reset brings back the first", "[looks]")
{
	const std::string lamp = "<object name=\"lamp\" class=\"lamp\">"
		"<sprite name=\"off\"><rectangle><width>10</width><height>10</height><color>color.grey</color></rectangle></sprite>"
		"<sprite name=\"on\"><rectangle><width>10</width><height>10</height><color>color.yellow</color></rectangle></sprite>"
		"<position><x>100</x><y>100</y></position><velocity><x>0</x><y>0</y></velocity>"
		"<collisions><enabled>true</enabled>"
		"<collision class=\"ball\" sprite=\"off\"><become sprite=\"on\" /><inc variable=\"lamp.lit\" /></collision>"
		"</collisions><variables><variable name=\"lit\">0</variable></variables></object>";
	const std::string ball = box("ball", 60, 100, 10, 10, "<enabled>true</enabled>", {}, {}, "<x>2</x><y>0</y>", " class=\"ball\"");

	Loaded loaded(gameXml(lamp + ball,
		state("playing", { "lamp", "ball" }, "<input button=\"space\"><become object=\"lamp\" sprite=\"off\" /></input>")));

	Object& lamp1 = loaded.game.getObject("lamp");
	CHECK(lamp1.lookName() == "off");
	CHECK(lamp1.spriteParams.at(3) == "color.grey");

	// The ball runs into it: the rule runs while it is off, and turns it on.
	loaded.frames(40);
	CHECK(lamp1.lookName() == "on");
	CHECK(lamp1.spriteParams.at(3) == "color.yellow");
	CHECK(loaded.variable("lamp", "lit") == 1); // once: the rule is for an unlit lamp

	// A key in a state can name it; a reset brings back the first look.
	CommandExecutor executor(loaded.game);
	executor.executeInput(CmdBecome{ "off", "lamp" }, true);
	CHECK(lamp1.lookName() == "off");
	executor.executeInput(CmdBecome{ "on", "lamp" }, true);
	loaded.game.resetAll();
	CHECK(lamp1.lookName() == "off");
}

TEST_CASE("an edge rule with sprite= runs only while the object shows that look, looked at as the rule comes", "[looks]")
{
	// Two hits on the left side: the first cracks it and sends it back, the
	// second takes it out. The cracked rule is first, so the hit that cracks
	// it is not also the one that takes it out.
	const std::string puck = "<object name=\"puck\">"
		"<sprite name=\"whole\"><rectangle><width>10</width><height>10</height><color>color.grey</color></rectangle></sprite>"
		"<sprite name=\"cracked\"><rectangle><width>10</width><height>10</height><color>color.darkgrey</color></rectangle></sprite>"
		"<position><x>20</x><y>100</y></position><velocity><x>-4</x><y>0</y></velocity>"
		"<collisions><enabled>true</enabled>"
		"<collision edge=\"left\" sprite=\"cracked\"><die /></collision>"
		"<collision edge=\"left\" sprite=\"whole\"><become sprite=\"cracked\" /><bounce /></collision>"
		"<collision edge=\"horizontal\"><inc variable=\"puck.hits\" /></collision>"
		"</collisions><variables><variable name=\"hits\">0</variable></variables></object>";

	Loaded loaded(gameXml(puck, state("playing", { "puck" })));
	Object& one = loaded.game.getObject("puck");
	REQUIRE(one.lookName() == "whole");

	loaded.frames(10);
	CHECK(one.lookName() == "cracked");
	CHECK(one.velocity.x > 0.0f);
	CHECK(one.isVisible);
	CHECK(loaded.variable("puck", "hits") == 1); // a rule with no sprite= runs whatever it shows

	one.velocity.x = -4.0f;
	loaded.frames(10);
	CHECK_FALSE(one.isVisible);
	CHECK(loaded.variable("puck", "hits") == 2);
}

TEST_CASE("<reveal> brings hidden members of a pool back where they started, as many as asked", "[reveal]")
{
	const std::string pool = "<group name=\"blocks\"><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite>"
		"<velocity><x>0</x><y>0</y></velocity><hidden>true</hidden><collisions><enabled>false</enabled></collisions>"
		"<member><position><x>10</x><y>10</y></position></member>"
		"<member><position><x>30</x><y>10</y></position></member>"
		"<member><position><x>50</x><y>10</y></position></member></group>";
	const std::string builder = box("builder", 0, 0, 1, 1, "<enabled>false</enabled>", {},
		"<variables><variable name=\"n\">0</variable></variables><timers><timer><every>1</every><reveal object=\"blocks\" /></timer></timers>");

	Loaded loaded(gameXml(pool + builder,
		state("playing", { "blocks", "builder" }, "<input button=\"space\"><reveal object=\"blocks\">2</reveal></input>")));

	const auto shown = [&]
	{
		int n = 0;
		for (const auto& object : loaded.game.getCurrentObjects()) { if (object.groupName == "blocks" && object.isVisible) { ++n; } }
		return n;
	};

	CHECK(shown() == 0);
	loaded.frames(60);
	CHECK(shown() == 1);
	CHECK(loaded.game.getObject("blocks.1").isVisible);
	CHECK(loaded.game.getObject("blocks.1").position.x == Approx(10));

	CommandExecutor executor(loaded.game);
	executor.executeInput(CmdReveal{ "blocks", 2 }, true);
	CHECK(shown() == 3);
	CHECK(loaded.game.getObject("blocks.3").position.x == Approx(50));

	// None left to bring back: nothing happens.
	loaded.frames(60);
	CHECK(shown() == 3);
}

TEST_CASE("a condition can change a variable, so it can count and start over", "[conditions]")
{
	Loaded loaded(gameXml(
		box("counter", 0, 0, 1, 1, "<enabled>false</enabled>", {}, kCounter + "<timers><timer><every>0.1</every><inc variable=\"counter.n\" /></timer></timers>"),
		state("playing", { "counter" }, "<input button=\"space\"><pop /></input>",
			"<conditions><condition object=\"counter\" variable=\"n\"><atleast>3</atleast><dec variable=\"counter.n\">3</dec><inc variable=\"counter.gap\" /></condition></conditions>")));

	loaded.frames(18);
	CHECK(loaded.variable("counter", "n") == 0);
	CHECK(loaded.variable("counter", "gap") == 2);
	loaded.frames(18);
	CHECK(loaded.variable("counter", "gap") == 3);
}

TEST_CASE("looks and reveals that name nothing are turned away when the game loads", "[looks][reveal][errors]")
{
	const auto fails = [](const std::string& objects, const std::string& states, const std::string& expected)
	{
		INFO(objects << states);
		const std::string xml = "<game><window name=\"verbs\"><width>800</width><height>600</height><background>color.black</background>"
			"<fullscreen>false</fullscreen><framerate>60</framerate></window><variables><variable name=\"unused\">0</variable></variables>"
			"<objects>" + objects + "</objects><states>" + states + "</states></game>";
		ScratchFile scratch{ std::filesystem::temp_directory_path() / "xge_verbs_errors.xml" };
		{
			std::ofstream out(scratch.path);
			out << xml;
		}
		CHECK_THROWS_WITH(Game{ scratch.path.string() }, ContainsSubstring(expected));
	};

	const std::string twoLooks = "<object name=\"o\"><sprite name=\"a\"><rectangle><width>5</width><height>5</height></rectangle></sprite>"
		"<sprite name=\"b\"><rectangle><width>5</width><height>5</height></rectangle></sprite>"
		"<position><x>0</x><y>0</y></position><velocity><x>0</x><y>0</y></velocity><collisions><enabled>false</enabled></collisions></object>";

	fails(twoLooks, state("playing", { "o" }, "<input button=\"space\"><become sprite=\"a\" /></input>"), "needs object=");
	fails(twoLooks, state("playing", { "o" }, "<input button=\"space\"><become object=\"o\" sprite=\"c\" /></input>"), "has no look of that name");
	fails(twoLooks, state("playing", { "o" }, "<input button=\"space\"><become object=\"nobody\" sprite=\"a\" /></input>"), "no object of that name");
	fails(box("o", 0, 0, 1, 1, "<enabled>false</enabled>", {}, "<actions><action name=\"x\"><become sprite=\"a\" /></action></actions>"),
		state("playing", { "o" }), "it has no look of that name");
	fails(box("o", 0, 0, 1, 1, "<enabled>true</enabled><collision class=\"x\" sprite=\"lit\"><die /></collision>"), state("playing", { "o" }),
		"names no look of the object");
	fails(box("o", 0, 0, 1, 1, "<enabled>true</enabled><collision edge=\"left\" sprite=\"lit\"><die /></collision>"), state("playing", { "o" }),
		"<collision edge sprite=\"lit\"> names no look of the object");
	fails(box("o", 0, 0, 1, 1), state("playing", { "o" }, "<input button=\"space\"><reveal object=\"ghost\" /></input>"), "<reveal> names 'ghost'");
}

TEST_CASE("<chase> heads straight for the nearest one in play, and stops when near enough", "[chase]")
{
	const std::string chaser = box("hunter", 100, 100, 20, 20, "<enabled>true</enabled>", "<facing>up</facing>",
		"<timers><timer><every>0.1</every><chase object=\"prey\"><speed>2</speed><near>50</near></chase></timer></timers>");
	Loaded loaded(gameXml(chaser + box("prey", 400, 500, 20, 20) + box("prey2", 0, 0, 20, 20, "<enabled>false</enabled>", "<hidden>true</hidden>"),
		state("playing", { "hunter", "prey", "prey2" })));

	Object& hunter = loaded.game.getObject("hunter");
	loaded.frames(6);
	// 300 across and 400 down: three fifths and four fifths of the speed.
	CHECK(hunter.velocity.x == Approx(1.2f));
	CHECK(hunter.velocity.y == Approx(1.6f));
	CHECK(hunter.facing == Direction::Down);

	// It closes in, and stops within 50 of the prey's middle.
	loaded.frames(400);
	const Object& prey = loaded.game.getObject("prey");
	const float apart = std::hypot(prey.position.x - hunter.position.x, prey.position.y - hunter.position.y);
	CHECK(apart <= 50.0f);
	CHECK(apart > 35.0f); // a timer every 6 frames at 2 a frame: within 12 past it
	CHECK(hunter.velocity.x == 0.0f);
	CHECK(hunter.velocity.y == 0.0f);
}

TEST_CASE("<aim> sends the next shots straight at the target, at any angle, until a key moves the shooter", "[aim]")
{
	const std::string gun = box("gun", 100, 100, 20, 20, "<enabled>true</enabled>", {},
		"<actions><action name=\"left\"><move direction=\"left\">1</move></action></actions>"
		"<timers><timer><every>0.5</every><aim object=\"target\" /><fire object=\"shot\" /></timer></timers>");
	Loaded loaded(gameXml(gun + box("target", 400, 500, 20, 20)
		+ box("shot", 0, 0, 4, 4, "<enabled>false</enabled><collision edge=\"all\"><die /></collision>", "<hidden>true</hidden>", {}, "<x>0</x><y>-5</y>"),
		state("playing", { "gun", "target", "shot" })));

	Object& shot = loaded.game.getObject("shot");
	loaded.frames(30);
	REQUIRE(shot.isVisible);
	CHECK(shot.velocity.x == Approx(3.0f));
	CHECK(shot.velocity.y == Approx(4.0f));
	CHECK(loaded.game.getObject("gun").facing == Direction::Down);

	// It leaves from the middle of the gun, clear of it, along the aim.
	const Vector2f middle = shot.position - shot.velocity + shot.size * 0.5f;
	CHECK((middle.y - 110.0f) / (middle.x - 110.0f) == Approx(4.0f / 3.0f));

	// A key that moves the gun drops the aim: from its top again.
	CommandExecutor executor(loaded.game);
	executor.executeInput(CmdTriggerAction{ "gun", "left" }, true);
	CHECK_FALSE(loaded.game.getObject("gun").hasAim);
}

TEST_CASE("<chase> and <aim> that name nothing are turned away when the game loads", "[chase][aim][errors]")
{
	CHECK_THROWS_WITH(Loaded(gameXml(box("o", 0, 0, 1, 1, "<enabled>false</enabled>", {}, "<timers><timer><every>1</every><chase object=\"playr\"><speed>1</speed></chase></timer></timers>")
		+ box("player", 0, 0, 1, 1), state("playing", { "o" }))), ContainsSubstring("<chase> names 'playr', and there is no object of that name (did you mean 'player'?)"));
	CHECK_THROWS_WITH(Loaded(gameXml(box("o", 0, 0, 1, 1, "<enabled>false</enabled>", {}, "<timers><timer><every>1</every><aim object=\"nobody\" /></timer></timers>"),
		state("playing", { "o" }))), ContainsSubstring("<aim> names 'nobody'"));
	CHECK_THROWS_WITH(Loaded(gameXml(box("o", 0, 0, 1, 1), "<state name=\"playing\"><shows><show object=\"o\" /></shows><inputs><input button=\"a\"><aim object=\"o\" /></input></inputs></state>")),
		ContainsSubstring("<aim> does nothing in an <input>"));
}
