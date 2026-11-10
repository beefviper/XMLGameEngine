// test_slopes_and_keys.cpp
// XML Game Engine
// author: beefviper
// date: Oct 31, 2026
//
// Catch2 tests for the first stage of tables and tracks: a <bounce> off a
// drawn slope goes off the way the slope faces, a bounce keeps the share
// of its speed it is given (below 1 a dead cushion, above 1 a bumper), and
// a key's three moments, on="press", on="hold" and on="release", the last
// two answering only a press made in the state current now.

#include "played.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <stdexcept>
#include <string>

using namespace xge;
using Catch::Approx;
using played::Played;

namespace
{
	// A game written out to a scratch file, loaded and played, and taken
	// away again: a window, then the states given.
	struct Written
	{
		explicit Written(const std::string& states)
		{
			std::ofstream out(path);
			out << "<game xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"../xgedef.xsd\">"
				"<window name=\"scratch\"><width>800</width><height>600</height><background>color.black</background>"
				"<fullscreen>false</fullscreen><framerate>60</framerate></window>"
				"<variables><variable name=\"unused\">0</variable></variables>"
				"<states>" << states << "</states></game>";
		}
		~Written()
		{
			std::error_code ignored;
			std::filesystem::remove(path, ignored);
		}
		Written(const Written&) = delete;
		Written& operator=(const Written&) = delete;

		const std::string path = scratchName("descriptions/slopes_scratch.xml");
	};

	// A ball of radius 5 with its top left at x, y, going by vx, vy, that
	// bounces off walls with `bounce` (a whole <bounce> tag) and off the
	// screen's edges the same.
	std::string ball(float x, float y, float vx, float vy, const std::string& bounce)
	{
		return "<object name=\"ball\"><sprite><circle><radius>5</radius><color>color.white</color></circle></sprite>"
			"<position><x>" + std::to_string(x) + "</x><y>" + std::to_string(y) + "</y></position>"
			"<velocity><x>" + std::to_string(vx) + "</x><y>" + std::to_string(vy) + "</y></velocity>"
			"<collisions><collision class=\"wall\">" + bounce + "</collision><collision edge=\"all\">" + bounce + "</collision></collisions></object>";
	}

	std::string playing(const std::string& objects)
	{
		return "<state name=\"playing\"><objects>" + objects + "</objects><inputs><input button=\"escape\"><pop /></input></inputs></state>";
	}
}

TEST_CASE("a ball falling onto a slope drawn as a line goes off along the way it faces", "[slopes][bounce]")
{
	// A line falling to the right at 45 degrees, tested by its pixels; the
	// ball comes straight down onto it from above.
	const std::string slope = "<object name=\"slope\" class=\"wall\"><sprite><line><from><x>0</x><y>0</y></from><to><x>100</x><y>100</y></to>"
		"<color>color.white</color><thickness>3</thickness></line></sprite><position><x>300</x><y>300</y></position>"
		"<collisions><type>pixel</type></collisions></object>";
	Written file(playing(slope + ball(345.0f, 290.0f, 0.0f, 4.0f, "<bounce />")));
	Played play(file.path);
	Object& ball = play.get("ball");

	for (int i = 0; i < 30 && ball.velocity.y > 0.0f; ++i) { play.frames(1); }

	// Off a slope facing up and to the right: straight down becomes
	// straight right, at the same speed.
	CHECK(ball.velocity.x == Approx(4.0f).margin(0.7f));
	CHECK(ball.velocity.y == Approx(0.0f).margin(0.7f));
}

TEST_CASE("off a box a bounce is the plain flip, with or without a share of the speed kept", "[slopes][bounce]")
{
	const std::string floor = "<object name=\"floor\" class=\"wall\"><sprite><rectangle><width>200</width><height>20</height></rectangle></sprite>"
		"<position><x>300</x><y>400</y></position><collisions /></object>";

	SECTION("kept whole")
	{
		Written file(playing(floor + ball(380.0f, 300.0f, 1.0f, 4.0f, "<bounce />")));
		Played play(file.path);
		play.frames(40);
		CHECK(play.get("ball").velocity.x == Approx(1.0f));
		CHECK(play.get("ball").velocity.y == Approx(-4.0f));
	}
	SECTION("half kept: a dead cushion")
	{
		Written file(playing(floor + ball(380.0f, 300.0f, 1.0f, 4.0f, "<bounce>0.5</bounce>")));
		Played play(file.path);
		play.frames(40);
		CHECK(play.get("ball").velocity.x == Approx(1.0f));
		CHECK(play.get("ball").velocity.y == Approx(-2.0f));
	}
	SECTION("more than came in: a bumper")
	{
		Written file(playing(floor + ball(380.0f, 300.0f, 1.0f, 4.0f, "<bounce>1.5</bounce>")));
		Played play(file.path);
		play.frames(40);
		CHECK(play.get("ball").velocity.y == Approx(-6.0f));
	}
}

TEST_CASE("off a screen edge a bounce keeps its share of the speed too", "[slopes][bounce]")
{
	Written file(playing(ball(20.0f, 300.0f, -4.0f, 0.0f, "<bounce>0.25</bounce>")));
	Played play(file.path);
	play.frames(20);
	CHECK(play.get("ball").velocity.x == Approx(1.0f));
}

TEST_CASE("a bounce off a ball meets it round: off at an angle when it is hit off its middle", "[slopes][bounce]")
{
	// A still round post below and to the left of a falling ball's line:
	// the ball comes off to the right, not straight back up.
	const std::string post = "<object name=\"post\" class=\"wall\"><sprite><circle><radius>10</radius><color>color.white</color></circle></sprite>"
		"<position><x>340</x><y>400</y></position><collisions /></object>";
	Written file(playing(post + ball(352.0f, 300.0f, 0.0f, 4.0f, "<bounce />")));
	Played play(file.path);
	Object& ball = play.get("ball");
	for (int i = 0; i < 40 && ball.velocity.y > 0.0f; ++i) { play.frames(1); }
	CHECK(ball.velocity.x > 1.0f);
	CHECK(ball.velocity.y < 0.0f);
}

namespace
{
	// A title whose Space starts the game, and a game whose Space counts
	// its presses, the frames it is held and its letting go; P pauses.
	const std::string keyGame =
		"<state name=\"title\"><inputs><input button=\"space\"><push state=\"playing\" /></input></inputs></state>"
		"<state name=\"playing\"><objects><object name=\"counts\"><variables>"
		"<variable name=\"pressed\">0</variable><variable name=\"held\">0</variable><variable name=\"letgo\">0</variable>"
		"</variables></object></objects><inputs>"
		"<input button=\"space\"><inc variable=\"counts.pressed\" /></input>"
		"<input button=\"space\" on=\"hold\"><inc variable=\"counts.held\" /></input>"
		"<input button=\"space\" on=\"release\"><inc variable=\"counts.letgo\" /></input>"
		"<input button=\"p\"><push state=\"paused\" /></input>"
		"</inputs></state>"
		"<state name=\"paused\"><inputs><input button=\"p\"><pop /></input></inputs></state>";
}

TEST_CASE("a key answers its press, every frame it is held, and its letting go", "[keys][moments]")
{
	Written file(keyGame);
	Played play(file.path);
	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "playing");
	play.frames(1);

	const auto count = [&](const char* name) { return play.get("counts").variable.at(name); };
	play.engine.handleKeyPressed(KeyCode::Space);
	CHECK(count("pressed") == 1.0f);
	play.frames(5);
	CHECK(count("held") == 5.0f);
	CHECK(count("letgo") == 0.0f);
	play.engine.handleKeyReleased(KeyCode::Space);
	CHECK(count("letgo") == 1.0f);
	play.frames(3);
	CHECK(count("held") == 5.0f);
	CHECK(count("pressed") == 1.0f);
}

TEST_CASE("held through a state change, a key's hold and release do not answer", "[keys][moments]")
{
	Written file(keyGame);
	Played play(file.path);
	const auto count = [&](const char* name) { return play.get("counts").variable.at(name); };

	// Space held down from the title into the game: it started the game, and
	// neither counts its hold nor its letting go there.
	play.engine.handleKeyPressed(KeyCode::Space);
	REQUIRE(play.state() == "playing");
	play.frames(4);
	play.engine.handleKeyReleased(KeyCode::Space);
	CHECK(count("pressed") == 0.0f);
	CHECK(count("held") == 0.0f);
	CHECK(count("letgo") == 0.0f);

	// Pressed in the game, then paused and back: that press no longer counts.
	play.engine.handleKeyPressed(KeyCode::Space);
	play.frames(2);
	play.tap(KeyCode::P);
	play.tap(KeyCode::P);
	REQUIRE(play.state() == "playing");
	play.frames(2);
	play.engine.handleKeyReleased(KeyCode::Space);
	CHECK(count("held") == 2.0f);
	CHECK(count("letgo") == 0.0f);
}

TEST_CASE("on= names press, hold or release, and is for a button only", "[keys][moments]")
{
	SECTION("another word")
	{
		Written file("<state name=\"playing\"><inputs><input button=\"space\" on=\"later\"><pop /></input></inputs></state>");
		CHECK_THROWS(Game(file.path));
	}
	SECTION("on a control that points")
	{
		Written file("<state name=\"playing\"><objects><object name=\"pad\"><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite>"
			"<actions><action name=\"steer\"><track /></action></actions></object></objects>"
			"<inputs><input axis=\"mouse.x\" on=\"hold\"><trigger object=\"pad\" action=\"steer\" /></input></inputs></state>");
		CHECK_THROWS(Game(file.path));
	}
}
