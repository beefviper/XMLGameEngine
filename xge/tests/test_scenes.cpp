// test_scenes.cpp
// XML Game Engine
// author: beefviper
// date: Oct 15, 2026
//
// Catch2 tests for objects a state owns and for <scenes>: the game's <kit>,
// scenes that take its pieces and change them, from=, names looked for from the
// inside out, <enter>, <reset scene>, what is carried going along, <drawn>
// and <show state>. Each game is written beside the shipped ones so that
// its schema path resolves, and loaded with Xerces (full validation) and
// TinyXml2 (the built-in validator). Frames are played with
// Game::updateObjects(); nothing needs a window.

#include "scratch.h"
#include "play/executor.h"
#include "game/game.h"
#include "load/xml.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

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

	// A small rectangle object; `velocity` its <velocity>, `after` what comes
	// after <velocity> (active, drawn, collisions, variables, timers).
	std::string box(const std::string& name, float x, float y, const std::string& velocity = {}, const std::string& after = {}, const std::string& attributes = {})
	{
		return "<object name=\"" + name + "\"" + attributes + "><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite>"
			"<position><x>" + std::to_string(x) + "</x><y>" + std::to_string(y) + "</y></position>"
			+ (velocity.empty() ? std::string{} : "<velocity>" + velocity + "</velocity>") + after + "</object>";
	}

	// A whole game file: 800 by 600 at 60 frames a second, the pieces of its
	// <kit> given, the game's own objects (one, `marker`, when none are
	// given) and the states given.
	std::string gameXml(const std::string& states, const std::string& objects = {}, const std::string& variables = {}, const std::string& kit = {})
	{
		return "<game xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"../xgedef.xsd\">"
			"<window name=\"scenes\"><width>800</width><height>600</height><background>color.black</background>"
			"<fullscreen>false</fullscreen><framerate>60</framerate></window>"
			"<variables><variable name=\"unused\">0</variable>" + variables + "</variables>"
			+ (kit.empty() ? std::string{} : "<kit>" + kit + "</kit>") +
			"<objects>" + (objects.empty() ? box("marker", 0, 0) : objects) + "</objects>"
			"<states>" + states + "</states>"
			"</game>";
	}

	struct Loaded
	{
		ScratchFile file;
		Game game;

		explicit Loaded(const std::string& xml, XmlBackend backend = XmlBackend::Xerces) :
			file(write(xml)),
			game(readGame(file.path.string(), backend))
		{
			// What Engine's constructor does: the first state is the one shown,
			// and every shape measured (as a window backend would).
			game.setCurrentState(0);
			for (auto& object : game.getCurrentObjects())
			{
				object.size = measureShapeSize(object.picture);
			}
		}

		static std::filesystem::path write(const std::string& xml)
		{
			const std::filesystem::path path = scratchName("descriptions/scenes_scratch.xml");
			std::ofstream out(path);
			out << xml;
			return path;
		}

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { game.updateObjects(); }
		}

		bool has(const std::string& name)
		{
			for (const Object& object : game.getCurrentObjects())
			{
				if (object.name == name) { return true; }
			}
			return false;
		}
	};

	void fails(const std::string& xml, const std::string& message)
	{
		REQUIRE_THROWS_WITH(Loaded(xml), ContainsSubstring(message));
	}

	const std::string kSpace = "<inputs><input button=\"space\"><pop /></input></inputs>";

	// A kit of one rock that drifts right, and a state of three rooms: the
	// first takes it as it is, the second changes its speed, the third is
	// the second with a sign of its own.
	const std::string kRock = box("rock", 100, 100, "<x>1</x>");
	const std::string kRooms =
		"<state name=\"playing\">"
		"<objects>" + box("hero", 400, 500) + "</objects>"
		"<scenes>"
		"<scene name=\"room1\"><objects><object name=\"rock\" /></objects></scene>"
		"<scene name=\"room2\"><objects><object name=\"rock\"><velocity><x>3</x></velocity></object></objects></scene>"
		"<scene name=\"room3\" from=\"room2\"><objects>" + box("sign", 700, 50) + "</objects></scene>"
		"</scenes>"
		+ kSpace +
		"</state>";
}

TEST_CASE("a state's own objects are in play while it is the current state, with no <show> for them", "[scenes]")
{
	auto backend = GENERATE(XmlBackend::Xerces, XmlBackend::TinyXml2);
	Loaded loaded(gameXml(
		"<state name=\"title\"><inputs><input button=\"space\"><push state=\"playing\" /></input></inputs></state>"
		"<state name=\"playing\"><objects>" + box("ball", 100, 100, "<x>2</x>") + "</objects>" + kSpace + "</state>"), backend);

	REQUIRE(loaded.has("playing.ball"));
	Object& ball = loaded.game.getObject("playing.ball");
	CHECK_FALSE(loaded.game.isShown(ball));
	loaded.frames(10);
	CHECK(ball.position.x == Approx(100));

	loaded.game.pushState("playing");
	CHECK(loaded.game.isShown(ball));
	loaded.frames(10);
	CHECK(ball.position.x == Approx(120));

	// A short name finds it too, as the end of its path.
	CHECK(&loaded.game.getObject("ball") == &ball);
}

TEST_CASE("an object with no <sprite> only holds numbers: it is never drawn and needs no <position>", "[scenes]")
{
	auto backend = GENERATE(XmlBackend::Xerces, XmlBackend::TinyXml2);
	Loaded loaded(gameXml(
		"<state name=\"playing\"><objects><object name=\"tally\"><variables><variable name=\"kills\">4</variable></variables></object></objects>" + kSpace + "</state>"), backend);

	Object& tally = loaded.game.getObject("playing.tally");
	CHECK_FALSE(tally.isDrawn);
	CHECK(tally.position.x == Approx(0));
	CHECK(tally.variable.at("kills") == Approx(4));
}

TEST_CASE("each scene that takes a piece gets its own copy, named by its path, and only the current scene runs", "[scenes]")
{
	auto backend = GENERATE(XmlBackend::Xerces, XmlBackend::TinyXml2);
	Loaded loaded(gameXml(kRooms, {}, {}, kRock), backend);

	REQUIRE(loaded.has("playing.hero"));
	REQUIRE(loaded.has("playing.room1.rock"));
	REQUIRE(loaded.has("playing.room2.rock"));
	REQUIRE(loaded.has("playing.room3.rock"));
	REQUIRE(loaded.has("playing.room3.sign"));
	CHECK_FALSE(loaded.has("rock"));          // the kit itself is never in play
	CHECK_FALSE(loaded.has("playing.room2.sign"));

	CHECK(loaded.game.currentScene("playing") == "playing.room1");
	Object& one = loaded.game.getObject("playing.room1.rock");
	Object& two = loaded.game.getObject("playing.room2.rock");
	CHECK(two.velocity.x == Approx(3));
	CHECK(loaded.game.getObject("playing.room3.rock").velocity.x == Approx(3)); // from room2, changed there

	loaded.frames(10);
	CHECK(one.position.x == Approx(110));
	CHECK(two.position.x == Approx(100));
	CHECK(loaded.game.isShown(loaded.game.getObject("playing.hero")));

	// Entering room2: room1 stops where it is, and is found as it was when
	// it is entered again.
	loaded.game.enterScene("playing.room2");
	loaded.frames(10);
	CHECK(one.position.x == Approx(110));
	CHECK(two.position.x == Approx(130));
	loaded.game.enterScene("playing.room1");
	loaded.frames(1);
	CHECK(one.position.x == Approx(111));
}

TEST_CASE("names inside a scene mean its own first, then its state's, then the game's", "[scenes]")
{
	// Each room's rock counts its hits on its own tally; the hero's score is
	// the state's, and a game-level label reads a room's rock by its path.
	const std::string tally = "<variables><variable name=\"hits\">0</variable></variables>";
	const std::string rock = "<object name=\"rock\"><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite>"
		"<position><x>100</x><y>100</y></position><collisions><enabled>true</enabled>"
		"<collision class=\"shot\"><inc variable=\"tally.hits\" /><inc variable=\"hero.score\" /></collision></collisions></object>";
	const std::string shot = box("shot", 100, 300, "<y>-20</y>", "<collisions><enabled>true</enabled><collision object=\"rock\"><die /></collision></collisions>", " class=\"shot\"");
	const std::string states =
		"<state name=\"playing\">"
		"<objects>" + box("hero", 400, 500, {}, "<variables><variable name=\"score\">0</variable></variables>") + shot + "</objects>"
		"<scenes>"
		"<scene name=\"room1\"><objects><object name=\"rock\" /><object name=\"tally\" /></objects></scene>"
		"<scene name=\"room2\" from=\"room1\" />"
		"</scenes>" + kSpace + "</state>";
	const std::string label = "<object name=\"label\"><sprite><text><number>room2.tally.hits</number><size>10</size></text></sprite>"
		"<position><x>playing.room1.rock.width</x><y>0</y></position></object>";
	Loaded loaded(gameXml("<state name=\"top\"><shows><show object=\"label\" /></shows><inputs><input button=\"space\"><push state=\"playing\" /></input></inputs></state>" + states, label,
		{}, rock + box("tally", 0, 0, {}, tally)));

	loaded.game.pushState("playing");
	loaded.frames(30);
	CHECK(loaded.game.getObject("playing.room1.tally").variable.at("hits") == 1);
	CHECK(loaded.game.getObject("playing.room2.tally").variable.at("hits") == 0);
	CHECK(loaded.game.getObject("playing.hero").variable.at("score") == 1);

	const Object& shown = loaded.game.getObject("label");
	CHECK(shown.showsVariable("playing.room2.tally", "hits"));
	CHECK(shown.position.x == Approx(10));

	// room2's own rock and tally, from its own rules
	CommandExecutor executor(loaded.game);
	executor.executeInput(CmdResetObject{ "shot" }, true);
	loaded.game.enterScene("playing.room2");
	loaded.frames(30);
	CHECK(loaded.game.getObject("playing.room2.tally").variable.at("hits") == 1);
	CHECK(loaded.game.getObject("playing.room1.tally").variable.at("hits") == 1);
	CHECK(loaded.game.getObject("playing.hero").variable.at("score") == 2);
}

TEST_CASE("a scene's variables are read inside it by their plain names, and a piece can change its own", "[scenes]")
{
	const std::string states =
		"<state name=\"playing\"><scenes>"
		"<scene name=\"wave1\"><variables><variable name=\"pace\">1</variable></variables><objects><object name=\"bomb\" /></objects></scene>"
		"<scene name=\"wave2\" from=\"wave1\"><variables><variable name=\"pace\">3</variable></variables></scene>"
		"<scene name=\"wave3\" from=\"wave1\"><objects><object name=\"bomb\"><variables><variable name=\"drop\">7</variable></variables></object></objects></scene>"
		"</scenes>" + kSpace + "</state>";
	Loaded loaded(gameXml(states, {}, {}, box("bomb", 100, 0, "<y>pace</y>", "<variables><variable name=\"drop\">pace * 2</variable></variables>")));

	CHECK(loaded.game.getVariable("playing.wave2.pace") == Approx(3));
	CHECK(loaded.game.getObject("playing.wave1.bomb").velocity.y == Approx(1));
	CHECK(loaded.game.getObject("playing.wave2.bomb").velocity.y == Approx(3));
	CHECK(loaded.game.getObject("playing.wave2.bomb").variable.at("drop") == Approx(6));
	CHECK(loaded.game.getObject("playing.wave3.bomb").variable.at("drop") == Approx(7));
}

TEST_CASE("a condition that names no object looks only at the current scene, and <enter> moves on", "[scenes][conditions]")
{
	// A wave is over when its aliens are gone; the next wave's are not counted.
	const std::string aliens = "<group name=\"aliens\" class=\"alien\"><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite>"
		"<member><position><x>10</x><y>10</y></position></member><member><position><x>30</x><y>10</y></position></member></group>";
	const std::string states =
		"<state name=\"playing\"><scenes>"
		"<scene name=\"wave1\"><objects><group name=\"aliens\" /></objects>"
		"<conditions><condition name=\"cleared\" class=\"alien\"><remaining>0</remaining><enter scene=\"wave2\" /></condition></conditions></scene>"
		"<scene name=\"wave2\" from=\"wave1\"><conditions><condition name=\"cleared\" class=\"alien\"><remaining>0</remaining><pop state=\"won\" /></condition></conditions></scene>"
		"</scenes>" + kSpace + "</state>"
		"<state name=\"won\"><inputs><input button=\"space\"><reset /></input></inputs></state>";
	Loaded loaded(gameXml(states, {}, {}, aliens));

	loaded.frames(2);
	CHECK(loaded.game.currentScene("playing") == "playing.wave1");
	for (const char* name : { "playing.wave1.aliens.1", "playing.wave1.aliens.2" }) { loaded.game.getObject(name).isVisible = false; }
	loaded.frames(1);
	CHECK(loaded.game.currentScene("playing") == "playing.wave2");
	CHECK(loaded.game.getCurrentState().name == "playing");

	for (const char* name : { "playing.wave2.aliens.1", "playing.wave2.aliens.2" }) { loaded.game.getObject(name).isVisible = false; }
	loaded.frames(1);
	CHECK(loaded.game.getCurrentState().name == "won");

	// A new game: every scene as it started, and the first one current.
	CommandExecutor executor(loaded.game);
	executor.executeInput(CmdReset{}, true);
	CHECK(loaded.game.currentScene("playing") == "playing.wave1");
	CHECK(loaded.game.getObject("playing.wave2.aliens.1").isVisible);
}

TEST_CASE("from= copies the conditions, timers and ways of the scene it is from; its own are added, or take the place of one of the same name", "[scenes][conditions]")
{
	auto backend = GENERATE(XmlBackend::Xerces, XmlBackend::TinyXml2);
	// Each wave is over when its aliens are gone and goes on by its way
	// "next"; a timer named "tick" counts the time spent in waves. The second
	// wave says only where its "next" leads; the third has a "cleared" of
	// its own that wins, a "tick" that counts ten at a time, and a timer
	// more, which is added.
	const std::string aliens = "<group name=\"aliens\" class=\"alien\"><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite>"
		"<member><position><x>10</x><y>10</y></position></member><member><position><x>30</x><y>10</y></position></member></group>";
	const std::string states =
		"<state name=\"playing\"><objects>" + box("clock", 0, 500, {}, "<variables><variable name=\"ticks\">0</variable></variables>") + "</objects>"
		"<scenes>"
		"<scene name=\"wave1\"><ways><way name=\"next\" scene=\"wave2\" /><way name=\"back\" scene=\"wave1\" /></ways>"
		"<objects><group name=\"aliens\" /></objects>"
		"<conditions><condition name=\"cleared\" class=\"alien\"><remaining>0</remaining><enter way=\"next\" /></condition></conditions>"
		"<timers><timer name=\"tick\"><every>0.5</every><inc variable=\"clock.ticks\" /></timer></timers></scene>"
		"<scene name=\"wave2\" from=\"wave1\"><ways><way name=\"next\" scene=\"wave3\" /></ways></scene>"
		"<scene name=\"wave3\" from=\"wave2\"><conditions><condition name=\"cleared\" class=\"alien\"><remaining>0</remaining><pop state=\"won\" /></condition></conditions>"
		"<timers><timer><after>100</after><inc variable=\"clock.ticks\" /></timer><timer name=\"tick\"><every>0.5</every><inc variable=\"clock.ticks\">10</inc></timer></timers></scene>"
		"</scenes>" + kSpace + "</state>"
		"<state name=\"won\"><inputs><input button=\"space\"><reset /></input></inputs></state>";
	Loaded loaded(gameXml(states, {}, {}, aliens), backend);

	const State& playing = loaded.game.getStates().front();
	REQUIRE(playing.scenes.size() == 3);
	CHECK(playing.scenes[1].conditions.size() == 1);
	CHECK(playing.scenes[1].timers.size() == 1);
	REQUIRE(playing.scenes[2].timers.size() == 2);
	CHECK(playing.scenes[2].conditions.size() == 1);
	CHECK_FALSE(playing.scenes[2].timers[1].repeat); // the added one, after the one it took the place of
	CHECK(playing.scenes[1].ways.at("next") == "playing.wave3");
	CHECK(playing.scenes[1].ways.at("back") == "playing.wave1");
	CHECK(playing.scenes[2].ways.at("next") == "playing.wave3");

	// Wave 2's copy of the condition counts wave 2's aliens and goes by wave 2's way.
	loaded.game.enterScene("playing.wave2");
	for (const char* name : { "playing.wave2.aliens.1", "playing.wave2.aliens.2" }) { loaded.game.getObject(name).isVisible = false; }
	loaded.frames(1);
	CHECK(loaded.game.currentScene("playing") == "playing.wave3");
	CHECK(loaded.game.getCurrentState().name == "playing");

	// Wave 3's own "tick", in place of the copy: ten at a time.
	const float ticks = loaded.game.getObject("playing.clock").variable.at("ticks");
	loaded.frames(31);
	CHECK(loaded.game.getObject("playing.clock").variable.at("ticks") == Approx(ticks + 10));

	// Wave 3's own condition, in place of the copy: it wins.
	for (const char* name : { "playing.wave3.aliens.1", "playing.wave3.aliens.2" }) { loaded.game.getObject(name).isVisible = false; }
	loaded.frames(1);
	CHECK(loaded.game.getCurrentState().name == "won");
}

TEST_CASE("a class, or a name every scene has, counts the scene in play; a name by its path counts that scene wherever it is", "[scenes][conditions]")
{
	const std::string robots = "<group name=\"robots\" class=\"robot\"><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite>"
		"<member><position><x>10</x><y>10</y></position></member><member><position><x>30</x><y>10</y></position></member></group>";
	const auto game = [&](const std::string& condition)
	{
		return gameXml("<state name=\"playing\"><scenes>"
			"<scene name=\"room1\"><objects><group name=\"robots\" /></objects></scene>"
			"<scene name=\"room2\"><objects><group name=\"robots\" /></objects></scene>"
			"</scenes>" + kSpace + "<conditions>" + condition + "</conditions></state>"
			"<state name=\"won\">" + kSpace + "</state>", {}, {}, robots);
	};
	const auto clearRoom2 = [](Loaded& loaded)
	{
		for (const char* name : { "playing.room2.robots.1", "playing.room2.robots.2" }) { loaded.game.getObject(name).isVisible = false; }
		loaded.frames(1);
	};

	SECTION("a class: room 2 cleared while in room 1 is not room 1 cleared")
	{
		Loaded loaded(game("<condition class=\"robot\"><remaining>0</remaining><push state=\"won\" /></condition>"));
		clearRoom2(loaded);
		CHECK(loaded.game.getCurrentState().name == "playing");
	}
	SECTION("the name every room's robots have is the robots in play")
	{
		Loaded loaded(game("<condition object=\"robots\"><remaining>0</remaining><push state=\"won\" /></condition>"));
		clearRoom2(loaded);
		CHECK(loaded.game.getCurrentState().name == "playing");
	}
	SECTION("room 2's robots by their path, counted from room 1")
	{
		Loaded loaded(game("<condition object=\"room2.robots\"><remaining>0</remaining><push state=\"won\" /></condition>"));
		loaded.frames(1);
		CHECK(loaded.game.getCurrentState().name == "playing");
		clearRoom2(loaded);
		CHECK(loaded.game.currentScene("playing") == "playing.room1");
		CHECK(loaded.game.getCurrentState().name == "won");
	}
	SECTION("one of them by its path")
	{
		Loaded loaded(game("<condition object=\"playing.room2.robots.2\"><remaining>0</remaining><push state=\"won\" /></condition>"));
		loaded.game.getObject("playing.room2.robots.2").isVisible = false;
		loaded.frames(1);
		CHECK(loaded.game.getCurrentState().name == "won");
	}
}

TEST_CASE("<reset scene> puts back what started in it, and leaves the other scenes alone", "[scenes]")
{
	Loaded loaded(gameXml(kRooms, {}, {}, kRock));
	loaded.frames(10);
	loaded.game.enterScene("playing.room2");
	loaded.frames(10);

	CommandExecutor executor(loaded.game);
	executor.executeInput(CmdResetScene{ "playing.room1", {} }, true);
	CHECK(loaded.game.getObject("playing.room1.rock").position.x == Approx(100));
	CHECK(loaded.game.getObject("playing.room2.rock").position.x == Approx(130));
	CHECK(loaded.game.currentScene("playing") == "playing.room2");
}

TEST_CASE("what a state's own object carries goes along into the next scene, and stays where it is dropped", "[scenes][carry]")
{
	const std::string hero = box("hero", 100, 100, "<x>1</x>", "<collisions><enabled>true</enabled><collision class=\"item\"><carry /></collision></collisions>"
		"<actions><action name=\"drop\"><drop /></action></actions>");
	const std::string states =
		"<state name=\"playing\"><objects>" + hero + "</objects><scenes>"
		"<scene name=\"room1\"><objects>" + box("sword", 105, 100, {}, "<collisions><enabled>true</enabled></collisions>", " class=\"item\"") + "</objects></scene>"
		"<scene name=\"room2\"><objects>" + box("key", 600, 100, {}, "<collisions><enabled>true</enabled></collisions>", " class=\"item\"") + "</objects></scene>"
		"</scenes><inputs><input button=\"space\"><trigger object=\"hero\" action=\"drop\" /></input></inputs></state>";
	Loaded loaded(gameXml(states));

	loaded.frames(2);
	Object& hero_ = loaded.game.getObject("playing.hero");
	Object& sword = loaded.game.getObject("playing.room1.sword");
	REQUIRE(hero_.carrying == "playing.room1.sword");

	const float held = sword.position.x - hero_.position.x;

	loaded.game.enterScene("playing.room2");
	CHECK(sword.ownerScene == "playing.room2");
	CHECK(loaded.game.isShown(sword));
	hero_.position.x = 300;
	loaded.frames(2);
	CHECK(sword.position.x - hero_.position.x == Approx(held));

	// Dropped in room2, it stays there when the hero goes back.
	CommandExecutor executor(loaded.game);
	executor.executeInput(CmdTriggerAction{ "playing.hero", "drop" }, true);
	hero_.position.x = 500;
	loaded.frames(2);
	loaded.game.enterScene("playing.room1");
	CHECK(sword.ownerScene == "playing.room2");
	CHECK_FALSE(loaded.game.isShown(sword));

	// A reset of the game puts it back where it started.
	executor.executeInput(CmdReset{}, true);
	CHECK(sword.ownerScene == "playing.room1");
}

TEST_CASE("an object that is not <drawn> still runs and collides: an exit", "[scenes][drawn]")
{
	const std::string hero = box("hero", 100, 100, "<x>5</x>", "<collisions><enabled>true</enabled></collisions>", " class=\"hero\"");
	const std::string exit = box("exit", 300, 100, {}, "<drawn>false</drawn><collisions><enabled>true</enabled>"
		"<collision class=\"hero\"><enter scene=\"room2\" /></collision></collisions>");
	const std::string states =
		"<state name=\"playing\"><objects>" + hero + "</objects><scenes>"
		"<scene name=\"room1\"><objects>" + exit + "</objects></scene>"
		"<scene name=\"room2\" />"
		"</scenes>" + kSpace + "</state>";
	Loaded loaded(gameXml(states));

	const Object& door = loaded.game.getObject("playing.room1.exit");
	CHECK_FALSE(door.isDrawn);
	CHECK(loaded.game.isShown(door));
	loaded.frames(60);
	CHECK(loaded.game.currentScene("playing") == "playing.room2");
}

TEST_CASE("<show state> draws that state's objects still, underneath, without running them", "[scenes][shows]")
{
	const std::string states =
		"<state name=\"playing\"><objects>" + box("ball", 100, 100, "<x>2</x>") + "</objects>"
		"<inputs><input button=\"p\"><push state=\"paused\" /></input></inputs></state>"
		"<state name=\"paused\"><objects>" + box("sign", 300, 300) + "</objects><shows><show state=\"playing\" /></shows>" + kSpace + "</state>";
	Loaded loaded(gameXml(states));

	Object& ball = loaded.game.getObject("playing.ball");
	loaded.frames(5);
	CHECK_FALSE(loaded.game.isStill(ball));
	loaded.game.pushState("paused");
	loaded.frames(5);
	CHECK(ball.position.x == Approx(110));
	CHECK_FALSE(loaded.game.isShown(ball));
	CHECK(loaded.game.isStill(ball));
	CHECK(loaded.game.isShown(loaded.game.getObject("paused.sign")));
	CHECK_FALSE(loaded.game.isStill(loaded.game.getObject("marker")));
}

TEST_CASE("names that could mean two things, and takes of nothing, stop the load", "[scenes][errors]")
{
	SECTION("a scene object and a state object of one name")
	{
		fails(gameXml("<state name=\"playing\"><objects>" + box("rock", 0, 0) + "</objects><scenes>"
			"<scene name=\"room1\"><objects>" + box("rock", 0, 0) + "</objects></scene></scenes>" + kSpace + "</state>"),
			"would both be 'rock'");
	}
	SECTION("a name that is the end of two paths")
	{
		fails(gameXml(kRooms + "<state name=\"practice\"><objects>" + box("rock", 0, 0) + "</objects>" + kSpace + "</state>",
			box("label", 0, 0, "<x>rock.width</x>"), {}, kRock), "'rock' could be");
	}
	SECTION("a scene from= one there is not")
	{
		fails(gameXml("<state name=\"playing\"><scenes>"
			"<scene name=\"room1\" /><scene name=\"room2\" from=\"room9\" /></scenes>" + kSpace + "</state>"),
			"from=\"room9\" names no scene before it");
	}
	SECTION("a take of a piece the kit does not have")
	{
		fails(gameXml("<state name=\"playing\"><scenes><scene name=\"room1\"><objects><object name=\"stone\" from=\"rocks\" /></objects></scene></scenes>" + kSpace + "</state>",
			{}, {}, kRock), "from=\"rocks\" names no piece of the <kit>");
	}
	SECTION("an own object that is not complete")
	{
		fails(gameXml("<state name=\"playing\"><scenes><scene name=\"room1\"><objects><object name=\"rock\" /></objects></scene></scenes>" + kSpace + "</state>"),
			"rock");
	}
	SECTION("an object with no picture that asks to be drawn")
	{
		fails(gameXml("<state name=\"playing\"><objects><object name=\"tally\"><drawn>true</drawn><variables><variable name=\"kills\">0</variable></variables></object></objects>" + kSpace + "</state>"),
			"it has no <drawn>");
	}
	SECTION("a take of a numbered member of a group, which only the group's name or a member's own name can take")
	{
		const std::string rocks = "<group name=\"rocks\"><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite>"
			"<member><position><x>0</x><y>0</y></position></member><member><position><x>20</x><y>0</y></position></member></group>";
		fails(gameXml("<state name=\"playing\"><scenes>"
			"<scene name=\"room1\"><objects><object name=\"rocks.2\"><active>false</active></object></objects></scene></scenes>" + kSpace + "</state>",
			{}, {}, rocks), "a numbered member of a group");
	}
	SECTION("a scene no state has")
	{
		fails(gameXml("<state name=\"playing\"><scenes><scene name=\"room1\" /></scenes>"
			"<inputs><input button=\"space\"><enter scene=\"room7\" /></input></inputs></state>"),
			"names no scene of the game");
	}
	SECTION("a state to draw that there is not")
	{
		fails(gameXml("<state name=\"playing\"><shows><show state=\"plying\" /></shows>" + kSpace + "</state>"), "there is no state of that name");
	}
}

TEST_CASE("a game variable written with a dot stays that variable beside a state's object of its first part", "[scenes]")
{
	// ball.radius is the game's; the playing state's ball is another thing.
	Loaded loaded(gameXml("<state name=\"playing\"><objects>" + box("ball", 0, 0, "<x>ball.radius</x>") + "</objects>" + kSpace + "</state>",
		{}, "<variable name=\"ball.radius\">4</variable>"));
	CHECK(loaded.game.getObject("playing.ball").velocity.x == Approx(4));
}

TEST_CASE("a game whose states own every object has no <objects> of its own", "[scenes]")
{
	auto backend = GENERATE(XmlBackend::Xerces, XmlBackend::TinyXml2);
	std::string xml = gameXml("<state name=\"playing\"><objects>" + box("ball", 0, 0, "<x>1</x>") + "</objects>" + kSpace + "</state>");
	const std::string marker = "<objects>" + box("marker", 0, 0) + "</objects>";
	xml.replace(xml.find(marker), marker.size(), "");
	Loaded loaded(xml, backend);
	REQUIRE(loaded.game.getCurrentObjects().size() == 1);
	loaded.frames(5);
	CHECK(loaded.game.getObject("ball").position.x == Approx(5));
}

TEST_CASE("a scene's <ways> say where its <enter way> and <reset way> lead, so one shared door serves every room", "[scenes][ways]")
{
	auto backend = GENERATE(XmlBackend::Xerces, XmlBackend::TinyXml2);
	// One door of the kit, taken by every room; walking into it enters the
	// room its way leads to, and that room starts again first.
	const std::string door = box("door", 200, 100, {}, "<drawn>false</drawn><collisions><enabled>true</enabled>"
		"<collision object=\"hero\"><reset way=\"on\" /><enter way=\"on\" /></collision></collisions>");
	const std::string rock = box("rock", 500, 500, "<x>1</x>");
	Loaded loaded(gameXml(
		"<state name=\"playing\"><objects>" + box("hero", 100, 100, "<x>4</x>",
			"<collisions><enabled>true</enabled><collision object=\"door\"><move direction=\"left\">150</move></collision></collisions>") + "</objects>"
		"<scenes>"
		"<scene name=\"room1\"><ways><way name=\"on\" scene=\"room2\" /></ways><objects><object name=\"door\" /><object name=\"rock\" /></objects></scene>"
		"<scene name=\"room2\"><ways><way name=\"on\" scene=\"playing.room1\" /></ways><objects><object name=\"door\" /><object name=\"rock\" /></objects></scene>"
		"</scenes>" + kSpace + "</state>", {}, {}, door + rock), backend);
	loaded.game.pushState("playing");

	REQUIRE(loaded.game.currentScene("playing") == "playing.room1");
	CHECK(loaded.game.wayLeadsTo("on") == "playing.room2");

	// Into room1's door: on to room2.
	loaded.frames(30);
	CHECK(loaded.game.currentScene("playing") == "playing.room2");
	CHECK(loaded.game.wayLeadsTo("on") == "playing.room1");
	const float rockLeft = loaded.game.getObject("playing.room1.rock").position.x;
	CHECK(rockLeft > 500.0f);

	// Into room2's door: back to room1, which starts again.
	loaded.frames(40);
	CHECK(loaded.game.currentScene("playing") == "playing.room1");
	CHECK(loaded.game.getObject("playing.room1.rock").position.x < rockLeft);

	// From C++ too, and a way the current scene does not have goes nowhere.
	CommandExecutor executor(loaded.game);
	executor.executeInput(CmdEnterScene{ {}, "on" }, true);
	CHECK(loaded.game.currentScene("playing") == "playing.room2");
	executor.executeInput(CmdEnterScene{ {}, "nowhere" }, true);
	CHECK(loaded.game.currentScene("playing") == "playing.room2");
}

TEST_CASE("mistakes with ways stop the load", "[scenes][ways][errors]")
{
	SECTION("a way to a scene the state does not have")
	{
		fails(gameXml("<state name=\"playing\"><scenes><scene name=\"room1\"><ways><way name=\"on\" scene=\"room9\" /></ways></scene></scenes>" + kSpace + "</state>"),
			"names no scene of state 'playing'");
	}
	SECTION("a way its scene does not have, inside the scene")
	{
		fails(gameXml("<state name=\"playing\"><scenes><scene name=\"room1\"><ways><way name=\"on\" scene=\"room2\" /></ways>"
			"<objects>" + box("door", 0, 0, {}, "<collisions><enabled>true</enabled><collision><enter way=\"off\" /></collision></collisions>") + "</objects></scene>"
			"<scene name=\"room2\" /></scenes>" + kSpace + "</state>"),
			"is no way of scene 'playing.room1'");
	}
	SECTION("a way no scene has")
	{
		fails(gameXml("<state name=\"playing\"><scenes><scene name=\"room1\" /></scenes>"
			"<inputs><input button=\"space\"><enter way=\"on\" /></input></inputs></state>"),
			"names no way of any scene");
	}
	SECTION("an <enter> with both, or neither")
	{
		fails(gameXml("<state name=\"playing\"><scenes><scene name=\"room1\" /></scenes>"
			"<inputs><input button=\"space\"><enter scene=\"room1\" way=\"on\" /></input></inputs></state>"),
			"names a scene=\"...\" or a way=\"...\", one of them");
	}
	SECTION("two conditions, or two timers, of one name in one list")
	{
		fails(gameXml("<state name=\"playing\"><scenes><scene name=\"room1\"><conditions>"
			"<condition name=\"done\" object=\"marker\" variable=\"x\"><atleast>1</atleast><pop /></condition>"
			"<condition name=\"done\" object=\"marker\" variable=\"x\"><atleast>2</atleast><pop /></condition>"
			"</conditions></scene></scenes>" + kSpace + "</state>"),
			"has two conditions called 'done'");
		fails(gameXml("<state name=\"playing\"><scenes><scene name=\"room1\"><timers>"
			"<timer name=\"tick\"><every>1</every><pop /></timer><timer name=\"tick\"><every>2</every><pop /></timer>"
			"</timers></scene></scenes>" + kSpace + "</state>"),
			"has two timers called 'tick'");
	}
	SECTION("two ways of one name")
	{
		fails(gameXml("<state name=\"playing\"><scenes><scene name=\"room1\"><ways><way name=\"on\" scene=\"room1\" /><way name=\"on\" scene=\"room1\" /></ways></scene></scenes>" + kSpace + "</state>"),
			"has two ways called 'on'");
	}
}

TEST_CASE("a state from= an earlier one is a copy of it with its own parts over it", "[scenes][states]")
{
	auto backend = GENERATE(XmlBackend::Xerces, XmlBackend::TinyXml2);
	const std::string text = "<sprite><text><content>YOU WIN</content><size>40</size><color>color.green</color></text></sprite>";
	Loaded loaded(gameXml(
		"<state name=\"youwin\"><objects><object name=\"message\">" + text + "<position><x>100</x><y>50</y></position></object></objects>"
		"<shows><show object=\"marker\" /></shows><inputs><input button=\"space\"><reset /></input></inputs></state>"
		"<state name=\"gameover\" from=\"youwin\"><objects>"
		"<object name=\"message\"><sprite><text><content>GAME OVER</content><size>40</size><color>color.red</color></text></sprite></object>"
		+ box("skull", 10, 10) + "</objects></state>"
		"<state name=\"theend\" from=\"gameover\"><objects><object name=\"message\"><position><y>80</y></position></object></objects>"
		"<inputs><input button=\"escape\"><pop /></input></inputs></state>"), backend);

	Object& over = loaded.game.getObject("gameover.message");
	CHECK(over.picture.color == "color.red");
	CHECK(over.position.x == Approx(100));          // copied
	CHECK(loaded.has("gameover.skull"));            // its own
	Object& end = loaded.game.getObject("theend.message");
	CHECK(end.picture.color == "color.red");   // a copy of a copy
	CHECK(end.position.y == Approx(80));
	CHECK(loaded.has("theend.skull"));

	const auto state = [&](const std::string& name) -> const State&
	{
		const std::vector<State>& states = loaded.game.getStates();
		const auto found = std::find_if(states.begin(), states.end(), [&](const State& each) { return each.name == name; });
		REQUIRE(found != states.end());
		return *found;
	};
	CHECK(state("gameover").input.count(KeyCode::Space) == 1);   // its keys come with it
	CHECK(state("theend").input.count(KeyCode::Space) == 1);
	CHECK(state("theend").input.count(KeyCode::Escape) == 1);
	CHECK(std::find(state("gameover").show.begin(), state("gameover").show.end(), "marker") != state("gameover").show.end());
}

TEST_CASE("a state from= that cannot be made stops the load", "[scenes][states]")
{
	SECTION("a state that is not before it")
	{
		fails(gameXml("<state name=\"youwin\" from=\"gameover\" />"
			"<state name=\"gameover\">" + kSpace + "</state>"), "from=\"gameover\" names no state before it");
	}
	SECTION("a state of its own with no <inputs>")
	{
		fails(gameXml("<state name=\"youwin\" />"), "missing <inputs>");
	}
	SECTION("half a position on an object of its own")
	{
		fails(gameXml("<state name=\"youwin\"><objects><object name=\"rock\"><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite>"
			"<position><x>5</x></position></object></objects>" + kSpace + "</state>"), "missing <y>");
	}
	SECTION("a copy of a state with scenes")
	{
		fails(gameXml("<state name=\"playing\"><scenes><scene name=\"room1\" /></scenes>" + kSpace + "</state>"
			"<state name=\"again\" from=\"playing\" />"), "a copy of one is not built yet");
	}
}
