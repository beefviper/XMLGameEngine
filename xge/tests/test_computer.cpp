// test_computer.cpp
// XML Game Engine
// author: beefviper
// date: Oct 17, 2026
//
// Catch2 tests for a state's <computer>: conditions tested every frame, each
// holding down the buttons of its <hold>s while it holds, as a player would;
// what a condition compares can be where things are and how they go. Then
// the far player of games/descriptions/tennis.xml.

#include "played.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>

using namespace xge;
using namespace played;
using Catch::Approx;
using Catch::Matchers::ContainsSubstring;

namespace
{
	const std::string bat = box("bat", 300, 500, "<actions><action name=\"left\"><move direction=\"left\">2</move></action>"
		"<action name=\"right\"><move direction=\"right\">2</move></action></actions>");
	const std::string keys = "<input button=\"j\"><trigger object=\"bat\" action=\"left\" /></input>"
		"<input button=\"l\"><trigger object=\"bat\" action=\"right\" /></input>";
	const std::string follows = "<computer>"
		"<condition object=\"bat\" variable=\"x\"><below object=\"ball\" variable=\"x\" /><hold button=\"l\" /></condition>"
		"<condition object=\"bat\" variable=\"x\"><above object=\"ball\" variable=\"x\" /><hold button=\"j\" /></condition>"
		"</computer>";

	// A game with the bat, a ball and the keys, and what else the state has.
	std::string gameWith(const std::string& ball, const std::string& more)
	{
		return "<game xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"../xgedef.xsd\">"
			"<window name=\"computer\"><width>800</width><height>600</height><background>color.black</background>"
			"<fullscreen>false</fullscreen><framerate>60</framerate></window>"
			"<variables><variable name=\"unused\">0</variable></variables><states><state name=\"playing\"><objects>" + bat + ball + "</objects>"
			"<inputs>" + keys + "</inputs>" + more + "</state></states></game>";
	}

	struct Written
	{
		const std::string path = "descriptions/computer_scratch.xml";
		explicit Written(const std::string& text) { std::ofstream(path) << text; }
		~Written()
		{
			std::error_code ignored;
			std::filesystem::remove(path, ignored);
		}
		Written(const Written&) = delete;
		Written& operator=(const Written&) = delete;
	};
}

TEST_CASE("a <computer> holds a button down while its condition holds, and lets it go after", "[computer]")
{
	const Written file(gameWith(box("ball", 100, 100, "<velocity><x>1</x><y>0</y></velocity>"), follows));
	Played play(file.path);

	// The ball is to the left: the bat goes left, two a frame, held.
	play.frames(1);
	CHECK(play.get("bat").velocity.x == Approx(-2));

	// It catches the ball up, then keeps with it, never more than a step off.
	play.frames(200);
	CHECK(std::abs(play.get("bat").position.x - play.get("ball").position.x) <= 2.0f);
}

TEST_CASE("every condition of a <computer> is tested, not only the first that holds", "[computer]")
{
	const Written file(gameWith(box("ball", 100, 100),
		"<computer><condition object=\"bat\" variable=\"x\"><above>0</above><hold button=\"j\" /></condition>"
		"<condition object=\"bat\" variable=\"y\"><above>0</above><hold button=\"l\" /></condition></computer>"));
	Played play(file.path);
	// Left and right held together: going neither way.
	play.frames(1);
	CHECK(play.get("bat").velocity.x == Approx(0));
	CHECK(play.get("bat").position.x == Approx(300));
}

TEST_CASE("a <hold> is only for a <computer>, and holds a key there is", "[computer]")
{
	const auto load = [](const std::string& more)
	{
		const Written file(gameWith(box("ball", 100, 100), more));
		Game game(file.path);
	};
	// The schema turns these away first; written naming no schema, the loader does.
	const auto loose = [](const std::string& more)
	{
		std::string text = gameWith(box("ball", 100, 100), more);
		const std::string named = " xsi:noNamespaceSchemaLocation=\"../xgedef.xsd\"";
		text.erase(text.find(named), named.size());
		const Written file(text);
		Game game(file.path);
	};
	CHECK_THROWS_WITH(load("<conditions><condition object=\"bat\" variable=\"x\"><above>0</above><hold button=\"j\" /></condition></conditions>"),
		ContainsSubstring("'hold'"));
	CHECK_THROWS_WITH(loose("<conditions><condition object=\"bat\" variable=\"x\"><above>0</above><hold button=\"j\" /></condition></conditions>"),
		ContainsSubstring("<hold> does nothing in a <condition>"));
	CHECK_THROWS_WITH(load("<computer><condition object=\"bat\" variable=\"x\"><above>0</above><hold button=\"joystick\" /></condition></computer>"),
		ContainsSubstring("no key named 'joystick'"));
	CHECK_THROWS_WITH(load("<computer><condition object=\"bat\" variable=\"x\"><above>0</above><pop /></condition></computer>"),
		ContainsSubstring("'pop'"));
	CHECK_THROWS_WITH(loose("<computer><condition object=\"bat\" variable=\"x\"><above>0</above><pop /></condition></computer>"),
		ContainsSubstring("<pop> does nothing in a <computer>'s <condition>"));
}

TEST_CASE("a condition can test where an object is and how it goes, against a number or another's", "[computer][conditions]")
{
	const Written file(gameWith(box("ball", 100, 100, "<velocity><x>3</x><y>0</y></velocity>"),
		"<conditions><condition object=\"ball\" variable=\"x\"><above object=\"bat\" variable=\"x\" /><set variable=\"bat.y\">0</set></condition>"
		"<condition object=\"ball\" variable=\"velocity.x\"><atleast>3</atleast><set variable=\"ball.y\">ball.y + 1</set></condition></conditions>"));
	Played play(file.path);

	// The first that holds runs: the ball going at 3 moves down a pixel a
	// frame until it passes the bat, and then the bat jumps to the top.
	play.frames(60);
	CHECK(play.get("bat").position.y == Approx(500));
	CHECK(play.get("ball").position.y == Approx(160));
	play.frames(10);
	CHECK(play.get("bat").position.y == Approx(0));
}

TEST_CASE("tennis.xml: the computer walks its racket under the ball", "[computer][tennis]")
{
	Played play("descriptions/tennis.xml");
	play.tap(KeyCode::Space);
	Object& ball = play.get("ball");
	Object& cpu = play.get("cpu");
	ball.position.x = 200;
	ball.velocity = { 0, 0 };
	play.frames(120);
	CHECK(std::abs(cpu.position.x - ball.position.x) <= 3.0f);
}
