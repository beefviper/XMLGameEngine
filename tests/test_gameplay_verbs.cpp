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
		return "<game xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"../assets/xmlgameengine.xsd\">"
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
