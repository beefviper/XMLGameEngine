// test_poses.cpp
// XML Game Engine
// author: beefviper
// date: Oct 23, 2026
//
// Catch2 tests for what an object's looks do as it moves: named
// <animation>s (looks that move), <poses> picked from how it moves, a
// <mirror> that flips its pictures going one way, a <grip> that makes it
// slide into a walk and out of one, and owner.blink, seconds of blinking.

#include "played.h"
#include "play/executor.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <string>

using namespace xge;
using namespace played;
using Catch::Matchers::ContainsSubstring;

namespace
{
	// A picture four pixels wide with one pixel lit, at `column` of its top row.
	std::string dot(const std::string& name, int column)
	{
		std::string row = "....";
		row[static_cast<std::size_t>(column)] = '*';
		return "<sprite name=\"" + name + "\"><bitmap><row>" + row + "</row><row>....</row><row>....</row><row>****</row></bitmap></sprite>";
	}

	// A walker on a floor: a look for each pose, the walk one that moves,
	// pulled down, sliding with a grip of a quarter, and drawn the other
	// way round going left.
	std::string walker(const std::string& extra = "<grip>0.25</grip>")
	{
		return "<object name=\"floor\" class=\"floor\"><sprite><rectangle><width>600</width><height>10</height></rectangle></sprite>"
			"<position><x>0</x><y>300</y></position><collisions><enabled>true</enabled></collisions></object>"
			"<object name=\"guy\">" + dot("stand", 0) + dot("stepa", 1) + dot("stepb", 2) + dot("skid", 3) + dot("up", 0) + dot("down", 1) + dot("hurt", 2)
			+ "<animation name=\"walk\"><interval>0.1</interval><frame sprite=\"stepa\" /><frame sprite=\"stepb\" /></animation>"
			"<position><x>300</x><y>290</y></position>"
			"<acceleration><x>0</x><y>0.5</y></acceleration>"
			"<poses><standing sprite=\"stand\" /><walking sprite=\"walk\" /><skidding sprite=\"skid\" /><rising sprite=\"up\" /><falling sprite=\"down\" /></poses>"
			"<mirror>left</mirror>" + extra +
			"<collisions><enabled>true</enabled><collision class=\"floor\"><land /></collision></collisions>"
			"<actions>"
			"<action name=\"left\"><move direction=\"left\">2</move></action>"
			"<action name=\"right\"><move direction=\"right\">2</move></action>"
			"<action name=\"jump\"><leap>40</leap></action>"
			"<action name=\"hurt\"><become sprite=\"hurt\" /></action>"
			"<action name=\"well\"><become sprite=\"stand\" /></action>"
			"</actions></object>";
	}

	bool litAt(const Object& object, int column) { return object.bitmap && object.bitmap->solidAt(column, 0); }
}

TEST_CASE("a named <animation> is a look that moves, and an object starts as its first", "[poses][animation]")
{
	const Scratch file("<object name=\"lamp\">" + dot("off", 0) + dot("a", 1) + dot("b", 2)
		+ "<animation name=\"blinking\"><interval>0.25</interval><frame sprite=\"a\" /><frame sprite=\"b\" /></animation>"
		"<position><x>10</x><y>10</y></position>"
		"<actions><action name=\"stop\"><become sprite=\"off\" /></action><action name=\"go\"><become sprite=\"blinking\" /></action></actions></object>",
		"<trigger object=\"lamp\" action=\"stop\" />");
	Played play(file.path);
	Object& lamp = play.get("lamp");
	CommandExecutor executor(play.game);

	REQUIRE(lamp.looks.size() == 4);
	CHECK(lamp.lookName() == "blinking");
	CHECK(litAt(lamp, 1));
	play.frames(15);
	CHECK(litAt(lamp, 2));
	play.frames(15);
	CHECK(litAt(lamp, 1));

	// A still look holds still, and back to the moving one starts it from its first picture.
	play.tap(KeyCode::Space);
	CHECK(lamp.lookName() == "off");
	play.frames(40);
	CHECK(litAt(lamp, 0));
	executor.executeInput(CmdTriggerAction{ "lamp", "go" }, true);
	CHECK(litAt(lamp, 1));
	play.frames(15);
	CHECK(litAt(lamp, 2));
}

TEST_CASE("<poses> pick the look from how it moves: standing, walking, skidding, rising and falling", "[poses]")
{
	const Scratch file(walker(), "<pop />");
	Played play(file.path);
	Object& guy = play.get("guy");
	CommandExecutor executor(play.game);

	play.frames(10);
	CHECK(guy.grounded);
	CHECK(guy.lookName() == "stand");

	// A grip of a quarter: a quarter of the way to the walking speed each frame.
	executor.executeInput(CmdTriggerAction{ "guy", "right" }, true);
	play.frames(1);
	CHECK(guy.velocity.x == 0.5f);
	CHECK(guy.lookName() == "walk");
	play.frames(30);
	CHECK(guy.velocity.x == 2.0f);
	CHECK(litAt(guy, 1) != litAt(guy, 2)); // one of the walk's two pictures

	// Letting go, it slides on, skidding, and stands where the slide ends.
	const float letGo = guy.position.x;
	executor.executeInput(CmdTriggerAction{ "guy", "right" }, false);
	play.frames(1);
	CHECK(guy.lookName() == "skid");
	CHECK(guy.velocity.x == 1.5f);
	play.frames(30);
	CHECK(guy.velocity.x == 0.0f);
	CHECK(guy.lookName() == "stand");
	CHECK(guy.position.x > letGo + 4.0f);

	// Up, then down, then on its feet again.
	executor.executeInput(CmdTriggerAction{ "guy", "jump" }, true);
	play.frames(2);
	CHECK(guy.lookName() == "up");
	play.frames(14);
	CHECK(guy.lookName() == "down");
	play.frames(30);
	CHECK(guy.lookName() == "stand");

	// A look no pose has takes it out of its poses until one brings it back.
	executor.executeInput(CmdTriggerAction{ "guy", "hurt" }, true);
	executor.executeInput(CmdTriggerAction{ "guy", "right" }, true);
	play.frames(5);
	CHECK(guy.lookName() == "hurt");
	executor.executeInput(CmdTriggerAction{ "guy", "well" }, true);
	play.frames(1);
	CHECK(guy.lookName() == "walk");
}

TEST_CASE("with no grip it is at its speed at once, and stops at once, so it never skids", "[poses][grip]")
{
	const Scratch file(walker(""), "<pop />");
	Played play(file.path);
	Object& guy = play.get("guy");
	CommandExecutor executor(play.game);

	play.frames(5);
	executor.executeInput(CmdTriggerAction{ "guy", "right" }, true);
	play.frames(1);
	CHECK(guy.velocity.x == 2.0f);
	executor.executeInput(CmdTriggerAction{ "guy", "right" }, false);
	play.frames(1);
	CHECK(guy.velocity.x == 0.0f);
	CHECK(guy.lookName() == "stand");
}

TEST_CASE("a <mirror> draws its pictures flipped going that way, and a stop leaves it facing as it was", "[poses][mirror]")
{
	const Scratch file(walker(), "<reset object=\"guy\" />");
	Played play(file.path);
	Object& guy = play.get("guy");
	CommandExecutor executor(play.game);

	play.frames(5);
	CHECK(litAt(guy, 0));
	executor.executeInput(CmdTriggerAction{ "guy", "left" }, true);
	play.frames(40);
	executor.executeInput(CmdTriggerAction{ "guy", "left" }, false);
	play.frames(40);
	CHECK(guy.lookName() == "stand");
	CHECK(guy.mirrored);
	CHECK(litAt(guy, 3)); // the standing picture, flipped
	CHECK_FALSE(litAt(guy, 0));

	// Pressing the other way turns it at once, while it still slides left.
	executor.executeInput(CmdTriggerAction{ "guy", "left" }, true);
	play.frames(30);
	executor.executeInput(CmdTriggerAction{ "guy", "left" }, false);
	executor.executeInput(CmdTriggerAction{ "guy", "right" }, true);
	play.frames(1);
	CHECK(guy.velocity.x < 0.0f);
	CHECK(guy.lookName() == "skid");
	CHECK_FALSE(guy.mirrored);
	CHECK(litAt(guy, 3)); // the skid picture as drawn
	executor.executeInput(CmdTriggerAction{ "guy", "right" }, false);

	// A reset faces it as it started.
	executor.executeInput(CmdTriggerAction{ "guy", "left" }, true);
	play.frames(5);
	executor.executeInput(CmdTriggerAction{ "guy", "left" }, false);
	CHECK(guy.mirrored);
	play.tap(KeyCode::Space);
	CHECK_FALSE(guy.mirrored);
	CHECK(guy.lookName() == "stand");
	CHECK(litAt(guy, 0));
}

TEST_CASE("a diagonal hop going left mirrors it too", "[poses][mirror]")
{
	const Scratch file("<object name=\"hopper\">" + dot("front", 0) + "<position><x>100</x><y>100</y></position><mirror>left</mirror>"
		"<actions><action name=\"go\"><hop direction=\"down-left\"><x>8</x><y>8</y></hop></action></actions></object>",
		"<trigger object=\"hopper\" action=\"go\" />");
	Played play(file.path);
	Object& hopper = play.get("hopper");

	play.tap(KeyCode::Space);
	play.frames(1);
	CHECK(hopper.mirrored);
	CHECK(litAt(hopper, 3));
}

TEST_CASE("owner.blink is seconds of blinking: drawn every other fifteenth of a second, and in play all the while", "[poses][blink]")
{
	const Scratch file(walker(), "<set variable=\"guy.blink\">1</set>");
	Played play(file.path);
	Object& guy = play.get("guy");

	CHECK_FALSE(guy.blinkedOut());
	play.tap(KeyCode::Space);
	CHECK(guy.blink == 1.0f);
	int hidden = 0;
	for (int frame = 0; frame < 60; ++frame)
	{
		play.frames(1);
		if (guy.blinkedOut()) { ++hidden; }
		CHECK(guy.collisionData.enabled);
	}
	CHECK(hidden > 20);
	CHECK(hidden < 40);
	play.frames(2);
	CHECK(guy.blink == 0.0f);
	CHECK_FALSE(guy.blinkedOut());

	play.tap(KeyCode::Space);
	play.game.resetObject("guy");
	CHECK(guy.blink == 0.0f);
}

TEST_CASE("mistakes in poses, mirrors, grips and named animations stop the load", "[poses][errors]")
{
	const auto load = [](const std::string& objects)
	{
		const Scratch bad(objects, "<pop />");
		Game game(bad.path);
	};
	const std::string two = dot("a", 0) + dot("b", 1);
	const std::string at = "<position><x>0</x><y>0</y></position>";

	CHECK_THROWS_WITH(load("<object name=\"o\">" + two + at + "<poses><standing sprite=\"c\" /></poses></object>"),
		ContainsSubstring("<standing sprite=\"c\">: the object has no look of that name"));
	CHECK_THROWS_WITH(load("<object name=\"o\">" + two + at + "<grip>0</grip></object>"), ContainsSubstring("<grip> is 0"));
	CHECK_THROWS_WITH(load("<object name=\"o\">" + two + at + "<grip>1.5</grip></object>"), ContainsSubstring("<grip> is 1.5"));
	CHECK_THROWS_WITH(load("<object name=\"o\">" + dot("a", 0) + at + "<heading>0</heading><mirror>left</mirror></object>"),
		ContainsSubstring("<mirror> flips a picture that does not turn"));
	CHECK_THROWS_WITH(load("<object name=\"o\">" + two + "<animation name=\"x\"><interval>1</interval><frame sprite=\"a\" /><frame sprite=\"b\" /></animation>"
		"<animation><interval>1</interval><frame sprite=\"a\" /><frame sprite=\"b\" /></animation>" + at + "</object>"),
		ContainsSubstring("an <animation> with no name beside named ones"));
	CHECK_THROWS_WITH(load("<object name=\"o\">" + two + "<animation name=\"a\"><interval>1</interval><frame sprite=\"a\" /><frame sprite=\"b\" /></animation>" + at + "</object>"),
		ContainsSubstring("two looks are called \"a\""));
	CHECK_THROWS_WITH(load("<object name=\"o\">" + two + "<animation name=\"x\"><interval>1</interval><frame sprite=\"a\" /><frame sprite=\"c\" /></animation>" + at + "</object>"),
		ContainsSubstring("<frame sprite=\"c\">: no <sprite name=\"c\">"));
}

TEST_CASE("a group's members share its named animations, and a member can give its own of a name", "[poses][animation]")
{
	const Scratch file("<group name=\"birds\">" + dot("a", 0) + dot("b", 1) + dot("c", 2)
		+ "<animation name=\"flap\"><interval>0.5</interval><frame sprite=\"a\" /><frame sprite=\"b\" /></animation>"
		"<member><position><x>0</x><y>0</y></position></member>"
		"<member><animation name=\"flap\"><interval>0.5</interval><frame sprite=\"c\" /><frame sprite=\"a\" /></animation><position><x>20</x><y>0</y></position></member>"
		"</group>", "<pop />");
	Played play(file.path);

	CHECK(litAt(play.get("birds.1"), 0));
	CHECK(litAt(play.get("birds.2"), 2));
	play.frames(30);
	CHECK(litAt(play.get("birds.1"), 1));
	CHECK(litAt(play.get("birds.2"), 0));
}
