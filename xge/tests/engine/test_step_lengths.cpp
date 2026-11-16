// test_step_lengths.cpp
// XML Game Engine
// author: beefviper
// date: Nov 16, 2026
//
// A game plays the same whatever the length of its steps: each case plays a
// small game for some seconds in steps of a tenth, a sixtieth and a 240th of
// a second and in uneven ones (helpers/step_lengths.h), and what it measures
// agrees across all four.

#include "step_lengths.h"
#include "play/executor.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <string>

using namespace played;
using Catch::Approx;

TEST_CASE("a still object is in the same place however the time is cut into steps", "[steps]")
{
	const std::string game = secondsGame(box("still", 100, 200));

	const auto answers = atEveryStepLengthOf(game, 2.0, [](Played& play) { return static_cast<double>(play.get("still").position.x); });
	agree(answers, 0.0);
	CHECK(answers.front().value == 100.0);
}

TEST_CASE("a timer goes off as often as the time passed owes it, however the time is cut into steps", "[steps][timers]")
{
	const std::string clock = box("clock", 0, 0,
		"<variables><variable name=\"n\">0</variable><variable name=\"ticks\">0</variable></variables>"
		"<timers><timer><every>0.05</every><inc variable=\"clock.n\" /></timer>"
		"<timer name=\"left\"><after>3</after><inc variable=\"clock.n\">1000</inc></timer></timers>");
	const std::string game = secondsGame(clock,
		"<timers><timer><every>0.25</every><inc variable=\"clock.ticks\" /></timer></timers>");

	// The object's every twentieth of a second: 40 in 2 seconds.
	const auto fired = atEveryStepLengthOf(game, 2.0, [](Played& play) { return static_cast<double>(play.get("clock").variable.at("n")); });
	agree(fired, 0.0);
	CHECK(fired.front().value == 40.0);

	// The time left of a named one is whole seconds, rounded up.
	const auto left = atEveryStepLengthOf(game, 2.0, [](Played& play) { return static_cast<double>(play.get("clock").variable.at("left")); });
	agree(left, 0.0);
	CHECK(left.front().value == 1.0);

	// The state's every quarter second: 8 in 2 seconds.
	const auto ticks = atEveryStepLengthOf(game, 2.0, [](Played& play) { return static_cast<double>(play.get("clock").variable.at("ticks")); });
	agree(ticks, 0.0);
	CHECK(ticks.front().value == 8.0);
}

TEST_CASE("an animation passes the same pictures, and runs their commands as often, however the time is cut into steps", "[steps][animation]")
{
	const auto sprite = [](const std::string& name) { return "<sprite name=\"" + name + "\"><bitmap><row>**</row><row>**</row></bitmap></sprite>"; };
	const std::string sprites = sprite("a") + sprite("b") + sprite("c");
	// Round and round, a twentieth of a second a picture: c comes up at 0.1,
	// 0.25, 0.4 ... seconds, 13 times in 2 seconds.
	const std::string spinner = "<object name=\"spinner\">" + sprites +
		"<animation><seconds>0.05</seconds><frame sprite=\"a\" /><frame sprite=\"b\" />"
		"<frame sprite=\"c\"><inc variable=\"self.n\" /></frame></animation>"
		"<position><x>0</x><y>0</y></position><collisions><off /></collisions>"
		"<variables><variable name=\"n\">0</variable></variables></object>";
	// Once through, its <then> once.
	const std::string flash = "<object name=\"flash\">" + sprites +
		"<animation><seconds>0.1</seconds><once /><frame sprite=\"a\" /><frame sprite=\"b\" /><frame sprite=\"c\" />"
		"<then><inc variable=\"self.ended\" /></then></animation>"
		"<position><x>20</x><y>0</y></position><collisions><off /></collisions>"
		"<variables><variable name=\"ended\">0</variable></variables></object>";
	const std::string game = secondsGame(spinner + flash);

	const auto came = atEveryStepLengthOf(game, 2.0, [](Played& play) { return static_cast<double>(play.get("spinner").variable.at("n")); });
	agree(came, 0.0);
	CHECK(came.front().value == 13.0);

	const auto picture = atEveryStepLengthOf(game, 2.0, [](Played& play) { return static_cast<double>(play.get("spinner").animation.index); });
	agree(picture, 0.0);

	const auto ended = atEveryStepLengthOf(game, 2.0, [](Played& play) { return static_cast<double>(play.get("flash").variable.at("ended")); });
	agree(ended, 0.0);
	CHECK(ended.front().value == 1.0);
}

TEST_CASE("a hop of half a second lands in the same place at the same time, however the time is cut into steps", "[steps][hop]")
{
	const std::string man = box("man", 100, 100,
		"<collisions><off /></collisions>"
		"<actions><action name=\"down\"><hop direction=\"down\">60<seconds>0.5</seconds></hop></action></actions>");
	const std::string game = secondsGame(man);
	const auto jump = [](Played& play) { xge::CommandExecutor(play.game).executeInput(xge::CmdTriggerAction{ "man", "down" }, true); };
	const auto down = [](Played& play) { return static_cast<double>(play.get("man").position.y); };
	const auto flying = [](Played& play) { return play.get("man").isAirborne() ? 1.0 : 0.0; };

	// Half way at a quarter of a second, then just in the air, then down.
	const auto half = atEveryStepLengthOf(game, 0.25, down, jump);
	agree(half, 2.0);
	CHECK(half.front().value == Approx(130.0).margin(2.0));

	const auto before = atEveryStepLengthOf(game, 0.49, flying, jump);
	agree(before, 0.0);
	CHECK(before.front().value == 1.0);

	const auto landed = atEveryStepLengthOf(game, 0.5, flying, jump);
	agree(landed, 0.0);
	CHECK(landed.front().value == 0.0);

	const auto where = atEveryStepLengthOf(game, 1.0, down, jump);
	agree(where, 0.0);
	CHECK(where.front().value == Approx(160.0));
}

TEST_CASE("a <go> of 8 cells a second has gone 16 cells after 2 seconds, however the time is cut into steps", "[steps][cells]")
{
	const std::string square = "<sprite><rectangle><width>8</width><height>8</height></rectangle></sprite>";
	const std::string map = "<map name=\"field\"><tile><width>10</width><height>10</height></tile><position><x>0</x><y>0</y></position><cells />"
		"<row>@...................</row>"
		"<object name=\"head\" char=\"@\">" + square +
		"<actions><action name=\"right\"><go direction=\"right\">8</go></action></actions></object></map>";
	const std::string game = secondsGame(map);
	const auto go = [](Played& play) { xge::CommandExecutor(play.game).executeInput(xge::CmdTriggerAction{ "head", "right" }, true); };

	const auto column = atEveryStepLengthOf(game, 2.0, [](Played& play) { return static_cast<double>(play.get("playing.head").cellColumn); }, go);
	agree(column, 0.0);
	CHECK(column.front().value == 17.0);
}

TEST_CASE("Space Invaders' stepped block is in the same place after 5 seconds, however the time is cut into steps", "[steps][stepped][spaceinvaders]")
{
	const auto invade = [](Played& play)
	{
		play.game.pushState("playing");
		play.get("player").collisionsOn = false;
	};
	const auto across = atEveryStepLength("descriptions/spaceinvaders.xml", 5.0,
		[](Played& play) { return static_cast<double>(play.get("aliens.1.1").position.x); }, invade);
	// Within one of its own steps: 15 pixels a note.
	agree(across, 15.0);
	const auto down = atEveryStepLength("descriptions/spaceinvaders.xml", 5.0,
		[](Played& play) { return static_cast<double>(play.get("aliens.1.1").position.y); }, invade);
	agree(down, 0.0);
}

TEST_CASE("a mover at a steady velocity is in the same place, however the time is cut into steps", "[steps][velocity]")
{
	// 100 pixels a second, for 2 seconds.
	const std::string moved = secondsGame(box("mover", 100, 100, "<velocity><x>100</x><y>50</y></velocity>"));
	const auto x = atEveryStepLengthOf(moved, 2.0, [](Played& play) { return static_cast<double>(play.get("mover").position.x); });
	agree(x, 2.0);
	CHECK(x.front().value == Approx(300.0).margin(2.0));

	// A game still in pixels a frame: 2 a frame is 120 a second.
	std::string old = moved;
	old.replace(old.find(" time=\"seconds\""), std::string(" time=\"seconds\"").size(), "");
	old.replace(old.find("<x>100</x><y>50</y></velocity>"), std::string("<x>100</x><y>50</y></velocity>").size(), "<x>2</x><y>1</y></velocity>");
	const auto frames = atEveryStepLengthOf(old, 2.0, [](Played& play) { return static_cast<double>(play.get("mover").position.x); });
	agree(frames, 2.0);
	CHECK(frames.front().value == Approx(340.0).margin(2.0));
}

TEST_CASE("a thing dropped from rest falls as far, however the time is cut into steps", "[steps][acceleration]")
{
	// 100 pixels a second, each second: 200 pixels in 2 seconds.
	const std::string game = secondsGame(box("stone", 100, 10, "<acceleration><x>0</x><y>100</y></acceleration>"));
	const auto fallen = atEveryStepLengthOf(game, 2.0, [](Played& play) { return static_cast<double>(play.get("stone").position.y); });
	agree(fallen, 2.0);
	CHECK(fallen.front().value == Approx(210.0).margin(1.0));
	const auto speed = atEveryStepLengthOf(game, 2.0, [](Played& play) { return static_cast<double>(play.get("stone").velocity.y); });
	agree(speed, 4.0);
	CHECK(speed.front().value == Approx(200.0).margin(1.0));
}

TEST_CASE("a <jump> reaches the same height and lands at the same time, however the time is cut into steps", "[steps][jump]")
{
	// A pull of 1,000 pixels a second each second, a jump of 50 pixels, off
	// a floor at 300.
	const std::string jumper = "<object name=\"jumper\"><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite>"
		"<position><x>100</x><y>290</y></position><acceleration><x>0</x><y>1000</y></acceleration>"
		"<collisions><collision object=\"floor\"><land /></collision></collisions>"
		"<actions><action name=\"up\"><jump>50</jump></action></actions></object>";
	const std::string floor = "<object name=\"floor\"><sprite><rectangle><width>400</width><height>20</height></rectangle></sprite>"
		"<position><x>0</x><y>300</y></position><collisions></collisions></object>";
	const std::string game = secondsGame(jumper + floor);
	// Stood on the floor a moment, then up.
	const auto jump = [](Played& play)
	{
		play.engine.step(0.05f);
		xge::CommandExecutor(play.game).executeInput(xge::CmdTriggerAction{ "jumper", "up" }, true);
	};
	// Its height when the top is due: v / a = sqrt(2 h / a), 0.316 seconds.
	const auto top = atEveryStepLengthOf(game, 0.316, [](Played& play) { return static_cast<double>(play.get("jumper").position.y); }, jump);
	agree(top, 3.0);
	CHECK(top.front().value == Approx(240.0).margin(3.0));
	// Down again after twice that.
	const auto landed = atEveryStepLengthOf(game, 0.8, [](Played& play) { return static_cast<double>(play.get("jumper").position.y); }, jump);
	agree(landed, 1.0);
	CHECK(landed.front().value == Approx(290.0).margin(1.0));
}

TEST_CASE("a turn held faces the same way, however the time is cut into steps", "[steps][turn]")
{
	// 45 degrees a second, held for 2 seconds.
	const std::string ship = "<object name=\"ship\"><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite>"
		"<position><x>100</x><y>100</y></position><heading>0</heading><collisions><off /></collisions>"
		"<actions><action name=\"right\"><turn direction=\"clockwise\">45</turn></action></actions></object>";
	const std::string game = secondsGame(ship);
	const auto hold = [](Played& play) { xge::CommandExecutor(play.game).executeInput(xge::CmdTriggerAction{ "ship", "right" }, true); };
	const auto facing = atEveryStepLengthOf(game, 2.0, [](Played& play) { return static_cast<double>(play.get("ship").heading); }, hold);
	agree(facing, 2.0);
	CHECK(facing.front().value == Approx(90.0).margin(2.0));
}

TEST_CASE("a thing coasting under drag goes as far, however the time is cut into steps", "[steps][drag]")
{
	// 300 pixels a second, half of it lost each second.
	const std::string game = secondsGame(box("puck", 10, 100, "<velocity><x>300</x><y>0</y></velocity><drag>0.5</drag>"));
	const auto x = atEveryStepLengthOf(game, 2.0, [](Played& play) { return static_cast<double>(play.get("puck").position.x); });
	// 300 (1 - 0.25) / ln 2 on from 10.
	agree(x, 2.0);
	CHECK(x.front().value == Approx(334.6).margin(2.0));
	const auto speed = atEveryStepLengthOf(game, 2.0, [](Played& play) { return static_cast<double>(play.get("puck").velocity.x); });
	agree(speed, 0.5);
	CHECK(speed.front().value == Approx(75.0).margin(0.5));
}
