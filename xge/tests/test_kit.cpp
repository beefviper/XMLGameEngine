// test_kit.cpp
// XML Game Engine
// author: beefviper
// date: Oct 25, 2026
//
// Catch2 tests for the game's <kit> and from=: pieces written once, in play
// only where an <objects> takes them (by from= or by a piece's name), each
// take changing the piece by the one rule every start-from follows; a piece
// from= an earlier one; a named member of a group piece taken on its own;
// what a <spawn>, <fire> or <release> names coming in beside what runs it;
// and the mistakes that stop the load. Small games are written out and
// played with no window (played.h).

#include "played.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <filesystem>
#include <fstream>
#include <string>

using namespace xge;
using played::box;
using played::Played;
using played::Scratch;
using Catch::Approx;
using Catch::Matchers::ContainsSubstring;

namespace
{
	// A paddle: a 10 by 40 rectangle at 10, 100, going down, with a score.
	const std::string kPaddle = "<object name=\"paddle\" class=\"bat\"><sprite><rectangle><width>10</width><height>40</height></rectangle></sprite>"
		"<position><x>10</x><y>100</y></position><velocity><x>0</x><y>1</y></velocity>"
		"<variables><variable name=\"score\">0</variable><variable name=\"lives\">3</variable></variables></object>";

	void fails(const std::string& objects, const std::string& kit, const std::string& message, const std::string& more = {})
	{
		const Scratch file(objects, "<pop />", more, kit);
		REQUIRE_THROWS_WITH(Game(file.path), ContainsSubstring(message));
	}
}

TEST_CASE("a piece of the kit is in play only where an <objects> takes it, by from= or by its name", "[kit]")
{
	const Scratch file("<object name=\"paddle\" />"
		"<object name=\"paddle2\" from=\"paddle\"><position><x>780</x></position><velocity><y>-2</y></velocity></object>",
		"<pop />", {}, kPaddle);
	Played play(file.path);

	const Object& one = play.get("playing.paddle");
	CHECK(one.position.x == Approx(10));
	CHECK(one.objClass == "bat");

	// The x of its position and the y of its velocity its own; the rest the piece's.
	const Object& two = play.get("playing.paddle2");
	CHECK(two.position.x == Approx(780));
	CHECK(two.position.y == Approx(100));
	CHECK(two.velocity.x == Approx(0));
	CHECK(two.velocity.y == Approx(-2));
	CHECK(two.objClass == "bat");
	CHECK(two.variable.at("lives") == Approx(3));
	CHECK(play.game.getCurrentObjects().size() == 2); // the piece itself is never in play
}

TEST_CASE("a take changes its piece: sprites by name, whole; variables one by one; collision rules after the piece's", "[kit]")
{
	const std::string piece = "<object name=\"brick\" class=\"wall\">"
		"<sprite name=\"whole\"><rectangle><width>20</width><height>10</height></rectangle></sprite>"
		"<sprite name=\"cracked\"><rectangle><width>20</width><height>8</height></rectangle></sprite>"
		"<position><x>100</x><y>100</y></position>"
		"<collisions><enabled>true</enabled><collision class=\"ball\"><inc variable=\"self.hits\" /></collision></collisions>"
		"<variables><variable name=\"hits\">0</variable><variable name=\"worth\">10</variable></variables></object>";
	const Scratch file("<object name=\"gold\" from=\"brick\" class=\"treasure\">"
		"<sprite name=\"cracked\"><rectangle><width>20</width><height>4</height></rectangle></sprite>"
		"<sprite name=\"shining\"><rectangle><width>20</width><height>12</height></rectangle></sprite>"
		"<collisions><enabled>true</enabled><collision class=\"shot\"><inc variable=\"self.worth\" /></collision></collisions>"
		"<variables><variable name=\"worth\">50</variable><variable name=\"shine\">1</variable></variables></object>",
		"<pop />", {}, piece);
	Played play(file.path);

	const Object& gold = play.get("gold");
	CHECK(gold.objClass == "treasure");
	REQUIRE(gold.looks.size() == 3);
	CHECK(gold.looks[0].name == "whole");
	CHECK(gold.looks[1].name == "cracked");
	CHECK(gold.looks[2].name == "shining");
	CHECK(gold.looks[1].spriteParams != gold.looks[0].spriteParams);
	CHECK(gold.variable.at("hits") == Approx(0));
	CHECK(gold.variable.at("worth") == Approx(50));
	CHECK(gold.variable.at("shine") == Approx(1));
	CHECK(gold.collisionData.enabled);
	CHECK(gold.collisionData.others.size() == 2); // its rule after the piece's
}

TEST_CASE("a piece can be from= an earlier one, and a named member of a group piece is taken on its own", "[kit]")
{
	const std::string kit = box("rock", 0, 0, "<velocity><x>1</x><y>0</y></velocity>")
		+ "<object name=\"boulder\" from=\"rock\"><sprite><rectangle><width>30</width><height>30</height></rectangle></sprite></object>"
		"<group name=\"robots\" class=\"robot\"><sprite><rectangle><width>8</width><height>8</height></rectangle></sprite>"
		"<member name=\"red\"><position><x>10</x><y>10</y></position></member>"
		"<member name=\"blue\"><position><x>50</x><y>10</y></position></member></group>";
	const Scratch file("<object name=\"boulder\" /><object name=\"red\"><position><x>200</x></position></object>", "<pop />", {}, kit);
	Played play(file.path);

	const Object& boulder = play.get("boulder");
	CHECK(boulder.velocity.x == Approx(1));
	CHECK(boulder.size.x == Approx(30));

	const Object& red = play.get("red");
	CHECK(red.objClass == "robot");
	CHECK(red.position.x == Approx(200));
	CHECK(red.position.y == Approx(10));
	CHECK_FALSE(play.has("blue"));
	CHECK(play.game.getCurrentObjects().size() == 2);
}

TEST_CASE("what a <fire> names comes in beside what fires it, so a copy reads its scene's variables", "[kit][spawn]")
{
	const std::string kit = "<object name=\"bomb\"><sprite><rectangle><width>4</width><height>4</height></rectangle></sprite>"
		"<velocity><x>0</x><y>pace</y></velocity><most>3</most></object>";
	const std::string path = "descriptions/kit_scratch.xml";
	{
		std::ofstream out(path);
		out << "<game xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"../../xge/xgedef.xsd\">"
			"<window name=\"kit\"><width>800</width><height>600</height><background>color.black</background>"
			"<fullscreen>false</fullscreen><framerate>60</framerate></window>"
			"<variables><variable name=\"unused\">0</variable></variables><kit>" << kit << "</kit>"
			"<states><state name=\"playing\"><objects>" << box("ship", 300, 500) << "</objects><scenes>"
			"<scene name=\"wave1\"><variables><variable name=\"pace\">2</variable></variables><objects>"
			<< box("dropper", 300, 100, "<timers><timer><every>0.05</every><fire object=\"bomb\" /></timer></timers>") << "</objects></scene>"
			"<scene name=\"wave2\" from=\"wave1\"><variables><variable name=\"pace\">5</variable></variables></scene>"
			"</scenes><inputs><input button=\"space\"><enter scene=\"wave2\" /></input></inputs></state></states></game>";
	}
	struct Gone
	{
		std::string file;
		~Gone() { std::error_code ignored; std::filesystem::remove(file, ignored); }
	} gone{ path };
	Played play(path);

	CHECK_FALSE(play.get("playing.wave1.bomb").exists);
	play.frames(5);
	REQUIRE(play.has("wave1.bomb.1"));
	const Object& first = play.get("wave1.bomb.1");
	CHECK(first.velocity.y == Approx(2));
	CHECK(first.ownerScene == "playing.wave1");

	play.tap(KeyCode::Space);
	play.frames(5);
	REQUIRE(play.has("wave2.bomb.1"));
	CHECK(play.get("wave2.bomb.1").velocity.y == Approx(5));
}

TEST_CASE("mistakes with the kit and from= stop the load", "[kit][errors]")
{
	SECTION("from= a piece there is not")
	{
		fails("<object name=\"bat\" from=\"padle\" />", kPaddle, "from=\"padle\" names no piece of the <kit>");
	}
	SECTION("a take by a name already in the list")
	{
		fails(box("bat", 0, 0) + "<object name=\"bat\" from=\"paddle\" />", kPaddle, "there is already an object or group called 'bat' here");
	}
	SECTION("a piece from= one after it")
	{
		fails(box("ball", 0, 0), "<object name=\"wide\" from=\"paddle\" />" + kPaddle, "names no piece of the <kit> before it");
	}
	SECTION("two pieces of one name")
	{
		fails(box("ball", 0, 0), kPaddle + kPaddle, "the <kit> has another piece (or a member of one) called 'paddle'");
	}
	SECTION("a piece with a dot in its name")
	{
		fails(box("ball", 0, 0), box("left.paddle", 0, 0), "a piece's name has no dots");
	}
	SECTION("a member changed in more than where it is and how it goes")
	{
		const std::string robots = "<group name=\"robots\"><sprite><rectangle><width>8</width><height>8</height></rectangle></sprite>"
			"<member name=\"red\"><position><x>10</x><y>10</y></position></member><member name=\"blue\"><position><x>50</x><y>10</y></position></member></group>";
		fails(robots + "<object name=\"red\"><variables><variable name=\"hp\">2</variable></variables></object>", {},
			"a member changes only its <sprite>s, <animation>s, <position> and <velocity>");
	}
}
