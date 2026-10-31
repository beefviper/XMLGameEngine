// test_flippers.cpp
// XML Game Engine
// author: beefviper
// date: Oct 31, 2026
//
// Catch2 tests for the third stage of tables and tracks: a picture turned
// about its <pivot>, which stays put; a <swing> that turns it to an angle
// while its key is held and back when it is let go; the frame's sweep
// following the turn, so a ball in the way is struck however fast the
// flipper swings; the speed a turning thing lends what it knocks, faster
// the further from the pivot it is struck; a swing whose turn grows by its
// <gain>; and <friction>, which brings a ball to rest on a shallow slope
// or wedged in a corner, but lets it roll down a steep one.

#include "played.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <fstream>
#include <string>

using namespace xge;
using Catch::Approx;
using played::Played;

namespace
{
	struct Written
	{
		explicit Written(const std::string& objects)
		{
			std::ofstream out(path);
			out << "<game xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"../xgedef.xsd\">"
				"<window name=\"scratch\"><width>800</width><height>600</height><background>color.black</background>"
				"<fullscreen>false</fullscreen><framerate>60</framerate></window>"
				"<variables><variable name=\"unused\">0</variable></variables>"
				"<states><state name=\"playing\"><objects>" << objects << "</objects><inputs>"
				<< (objects.find("name=\"flipper\"") != std::string::npos ? "<input button=\"d\"><trigger object=\"flipper\" action=\"flip\" /></input>" : "")
				<< "<input button=\"escape\"><pop /></input></inputs></state></states></game>";
		}
		~Written()
		{
			std::error_code ignored;
			std::filesystem::remove(path, ignored);
		}
		Written(const Written&) = delete;
		Written& operator=(const Written&) = delete;

		const std::string path = scratchName("descriptions/flippers_scratch.xml");
	};

	// A flipper 80 long, hinged at its left end, which sits at 300, 400; it
	// rests tilted 30 degrees down to the right and swings `rate` degrees a
	// frame up to 30 above level.
	std::string flipper(float rate, const std::string& gain = "")
	{
		return "<object name=\"flipper\" class=\"flipper\"><sprite><line><from><x>0</x><y>0</y></from><to><x>80</x><y>0</y></to>"
			"<color>color.white</color><thickness>8</thickness></line></sprite>"
			"<position><x>300</x><y>400</y></position><heading>30</heading><pivot><x>0</x><y>0</y></pivot>"
			"<collisions><type>pixel</type><collision class=\"ball\"><knock>0.5</knock></collision></collisions>"
			"<actions><action name=\"flip\"><swing>" + std::to_string(rate) + "<to>330</to>" + gain + "</swing></action></actions></object>";
	}

	// A ball of radius 5 with its middle at x, y, still, with a mass.
	std::string ball(float x, float y)
	{
		return "<object name=\"ball\" class=\"ball\"><sprite><circle><radius>5</radius><color>color.white</color></circle></sprite>"
			"<position><x>" + std::to_string(x - 5.0f) + "</x><y>" + std::to_string(y - 5.0f) + "</y></position>"
			"<mass>1</mass><collisions /></object>";
	}

	// Where along the flipper, `along` from its hinge, the top of it is
	// turned to `degrees` (heading 0 is level, to the right).
	Vector2f onFlipper(float along, float degrees, float above)
	{
		const float radians = degrees * 3.14159265f / 180.0f;
		return Vector2f{ 300.0f + along * std::cos(radians) + above * std::sin(radians), 400.0f + along * std::sin(radians) - above * std::cos(radians) };
	}
}

TEST_CASE("a picture turned about its pivot keeps the pivot where it is", "[flippers][pivot]")
{
	Written file(flipper(15.0f));
	Played play(file.path);
	Object& flip = play.get("flipper");

	const Vector2f hinge = flip.pivotOnScreen();
	CHECK(hinge.x == Approx(300.0f).margin(1.0f));
	CHECK(hinge.y == Approx(400.0f).margin(1.0f));

	play.engine.handleKeyPressed(KeyCode::D);
	play.frames(2);
	CHECK(flip.heading == Approx(0.0f).margin(0.01f));
	CHECK(flip.pivotOnScreen().x == Approx(hinge.x).margin(0.01f));
	CHECK(flip.pivotOnScreen().y == Approx(hinge.y).margin(0.01f));
}

TEST_CASE("a swing turns while held, never past where it swings to, and back when let go", "[flippers][swing]")
{
	Written file(flipper(15.0f));
	Played play(file.path);
	Object& flip = play.get("flipper");

	play.engine.handleKeyPressed(KeyCode::D);
	play.frames(1);
	CHECK(flip.heading == Approx(15.0f));
	CHECK(flip.spin == Approx(-15.0f));
	play.frames(10);
	CHECK(flip.heading == Approx(330.0f));
	CHECK(flip.spin == 0.0f);

	play.engine.handleKeyReleased(KeyCode::D);
	play.frames(10);
	CHECK(flip.heading == Approx(30.0f));
	CHECK(flip.spin == 0.0f);
}

TEST_CASE("a ball in a flipper's way is struck as it swings, however fast", "[flippers][sweep]")
{
	// The ball sits where the flipper's top passes halfway through its swing,
	// level; a 60 degree swing in one frame goes right past it.
	const Vector2f at = onFlipper(60.0f, 0.0f, 10.0f);
	Written file(flipper(60.0f) + ball(at.x, at.y));
	Played play(file.path);
	Object& ball = play.get("ball");

	play.engine.handleKeyPressed(KeyCode::D);
	play.frames(1);
	CHECK(play.get("flipper").heading == Approx(330.0f));

	// Knocked up, and off along the way the flipper faced when it struck.
	CHECK(ball.velocity.y < -10.0f);
	play.frames(1);
	CHECK(ball.position.y < at.y - 5.0f);
}

TEST_CASE("a flipper lends more speed the further from its pivot it strikes", "[flippers][knock]")
{
	const auto struck = [](float along)
	{
		const Vector2f at = onFlipper(along, 0.0f, 10.0f);
		Written file(flipper(15.0f) + ball(at.x, at.y));
		Played play(file.path);
		Object& ball = play.get("ball");
		play.engine.handleKeyPressed(KeyCode::D);
		for (int i = 0; i < 6 && ball.velocity.y >= 0.0f; ++i) { play.frames(1); }
		return std::hypot(ball.velocity.x, ball.velocity.y);
	};

	const float nearHinge = struck(20.0f);
	const float nearTip = struck(70.0f);
	CHECK(nearHinge > 0.0f);
	// Lent speed is the turn times the distance from the pivot.
	CHECK(nearTip > nearHinge * 2.0f);
}

TEST_CASE("a swing with a gain turns faster every frame, from still, up to its rate", "[flippers][swing]")
{
	Written file(flipper(20.0f, "<gain>5</gain>"));
	Played play(file.path);
	Object& flip = play.get("flipper");

	play.engine.handleKeyPressed(KeyCode::D);
	float was = flip.heading;
	for (const float turn : { 5.0f, 10.0f, 15.0f, 20.0f, 10.0f })
	{
		play.frames(1);
		CHECK(turnBetween(was, flip.heading) == Approx(-turn));
		was = flip.heading;
	}
	CHECK(flip.heading == Approx(330.0f));

	// Back down from still too.
	play.engine.handleKeyReleased(KeyCode::D);
	play.frames(1);
	CHECK(turnBetween(was, flip.heading) == Approx(5.0f));
}

TEST_CASE("a later strike in a swing with a gain sends the ball off faster", "[flippers][knock]")
{
	// The same ball, met near the start of the stroke and near its end: how
	// fast the first strike sends it (a ball left lying on a flipper is
	// struck again and again as it speeds up, and goes off at the end of the
	// stroke).
	const auto struck = [](float degrees)
	{
		const Vector2f at = onFlipper(60.0f, degrees, 11.0f);
		Written file(flipper(20.0f, "<gain>5</gain>") + ball(at.x, at.y));
		Played play(file.path);
		Object& ball = play.get("ball");
		play.engine.handleKeyPressed(KeyCode::D);
		for (int i = 0; i < 8 && ball.velocity.x == 0.0f && ball.velocity.y == 0.0f; ++i) { play.frames(1); }
		return std::hypot(ball.velocity.x, ball.velocity.y);
	};
	// Met as it turns 10 degrees a frame, then 20.
	CHECK(struck(350.0f) > struck(20.0f) * 1.4f);
}

namespace
{
	// A still slope of `degrees` (down to the right), drawn as a line, and a
	// ball with `friction` under a pull straight down, let go just above it.
	std::string slope(float degrees)
	{
		const float radians = degrees * 3.14159265f / 180.0f;
		return "<object name=\"slope\" class=\"wall\"><sprite><line><from><x>0</x><y>0</y></from><to><x>200</x><y>"
			+ std::to_string(200.0f * std::tan(radians)) + "</y></to><color>color.white</color><thickness>4</thickness></line></sprite>"
			"<position><x>200</x><y>300</y></position><collisions><type>pixel</type></collisions></object>";
	}

	std::string rubbing(float x, float y, const std::string& friction)
	{
		return "<object name=\"ball\" class=\"ball\"><sprite><circle><radius>5</radius><color>color.white</color></circle></sprite>"
			"<position><x>" + std::to_string(x - 5.0f) + "</x><y>" + std::to_string(y - 5.0f) + "</y></position>"
			"<acceleration><x>0</x><y>0.2</y></acceleration>" + friction + "<mass>1</mass>"
			"<collisions><collision class=\"wall\"><bounce>0.5</bounce></collision></collisions></object>";
	}
}

TEST_CASE("friction brings a ball to rest on a slope its pull cannot slide it down", "[friction]")
{
	// 10 degrees: friction 0.3 holds it (the pull along it, tan 10 = 0.18,
	// is less than 0.3 of the pull into it).
	Written file(slope(10.0f) + rubbing(250.0f, 290.0f, "<friction>0.3</friction>"));
	Played play(file.path);
	Object& ball = play.get("ball");
	play.frames(90);
	REQUIRE(ball.resting);
	const Vector2f still = ball.position;
	play.frames(60);
	CHECK(ball.position == still);
	CHECK(ball.velocity.x == 0.0f);
	CHECK(ball.velocity.y == 0.0f);
}

TEST_CASE("friction too small for the slope lets a ball roll down it", "[friction]")
{
	Written file(slope(30.0f) + rubbing(250.0f, 315.0f, "<friction>0.3</friction>"));
	Played play(file.path);
	Object& ball = play.get("ball");
	play.frames(60);
	CHECK_FALSE(ball.resting);
	CHECK(ball.velocity.x > 0.5f);
}

TEST_CASE("without friction a ball never comes to rest on a slope", "[friction]")
{
	Written file(slope(10.0f) + rubbing(250.0f, 290.0f, ""));
	Played play(file.path);
	Object& ball = play.get("ball");
	play.frames(90);
	CHECK_FALSE(ball.resting);
	CHECK(ball.velocity.x > 0.1f);
}

TEST_CASE("a ball at rest on a flipper wakes when the flipper moves", "[friction][flippers]")
{
	// Lying on the flipper raised and still, a little friction does not hold
	// it on 30 degrees; on the flipper turned level (heading 0), it does.
	std::string level = flipper(15.0f);
	level.replace(level.find("<heading>30</heading>"), 21, "<heading>0</heading>");
	level.replace(level.find("<to>330</to>"), 12, "<to>30</to>");
	const Vector2f at = onFlipper(40.0f, 0.0f, 11.0f);
	Written file(level + rubbing(at.x, at.y, "<friction>0.2</friction>"));
	Played play(file.path);
	Object& ball = play.get("ball");
	play.frames(30);
	REQUIRE(ball.resting);

	// Swung down to the right, away under it: the ball falls and rolls.
	play.engine.handleKeyPressed(KeyCode::D);
	play.frames(10);
	CHECK_FALSE(ball.resting);
	CHECK(ball.velocity.y > 0.2f);
}
