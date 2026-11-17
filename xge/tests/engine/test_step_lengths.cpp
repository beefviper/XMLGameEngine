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
	const std::string game = smallGame(box("still", 100, 200));

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
	const std::string game = smallGame(clock,
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
	const std::string game = smallGame(spinner + flash);

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
	const std::string game = smallGame(man);
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
	const std::string game = smallGame(map);
	const auto go = [](Played& play) { xge::CommandExecutor(play.game).executeInput(xge::CmdTriggerAction{ "head", "right" }, true); };

	const auto column = atEveryStepLengthOf(game, 2.0, [](Played& play) { return static_cast<double>(play.game.place(play.get("playing.head")).column); }, go);
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
	const std::string moved = smallGame(box("mover", 100, 100, "<velocity><x>100</x><y>50</y></velocity>"));
	const auto x = atEveryStepLengthOf(moved, 2.0, [](Played& play) { return static_cast<double>(play.get("mover").position.x); });
	agree(x, 2.0);
	CHECK(x.front().value == Approx(300.0).margin(2.0));
}

TEST_CASE("a thing dropped from rest falls as far, however the time is cut into steps", "[steps][acceleration]")
{
	// 100 pixels a second, each second: 200 pixels in 2 seconds.
	const std::string game = smallGame(box("stone", 100, 10, "<acceleration><x>0</x><y>100</y></acceleration>"));
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
	const std::string game = smallGame(jumper + floor);
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
	const std::string game = smallGame(ship);
	const auto hold = [](Played& play) { xge::CommandExecutor(play.game).executeInput(xge::CmdTriggerAction{ "ship", "right" }, true); };
	const auto facing = atEveryStepLengthOf(game, 2.0, [](Played& play) { return static_cast<double>(play.get("ship").heading); }, hold);
	agree(facing, 2.0);
	CHECK(facing.front().value == Approx(90.0).margin(2.0));
}

TEST_CASE("a thing coasting under drag goes as far, however the time is cut into steps", "[steps][drag]")
{
	// 300 pixels a second, losing half of it each second (a half-life of 1).
	const std::string game = smallGame(box("puck", 10, 100, "<velocity><x>300</x><y>0</y></velocity><drag>1</drag>"));
	const auto x = atEveryStepLengthOf(game, 2.0, [](Played& play) { return static_cast<double>(play.get("puck").position.x); });
	// 300 (1 - 0.25) / ln 2 on from 10.
	agree(x, 2.0);
	CHECK(x.front().value == Approx(334.6).margin(2.0));
	const auto speed = atEveryStepLengthOf(game, 2.0, [](Played& play) { return static_cast<double>(play.get("puck").velocity.x); });
	agree(speed, 0.5);
	CHECK(speed.front().value == Approx(75.0).margin(0.5));
}

TEST_CASE("a paddle held for a second goes as far, however the time is cut into steps", "[steps][held]")
{
	// 200 pixels a second down while S is held.
	const std::string paddle = box("paddle", 100, 100,
		"<collisions><off /></collisions><actions><action name=\"down\"><move direction=\"down\">200</move></action></actions>");
	const std::string game = smallGame(paddle, "", "<input button=\"s\"><trigger object=\"paddle\" action=\"down\" /></input>");
	const auto hold = [](Played& play) { play.engine.handleKeyPressed(xge::KeyCode::S); };
	const auto y = atEveryStepLengthOf(game, 1.0, [](Played& play) { return static_cast<double>(play.get("paddle").position.y); }, hold);
	agree(y, 2.0);
	CHECK(y.front().value == Approx(300.0).margin(2.0));
}

TEST_CASE("a climb held goes as far up the ladder, however the time is cut into steps", "[steps][held][climb]")
{
	// 50 pixels a second up a ladder 200 high, from the floor at 400.
	const std::string climber = box("climber", 100, 390,
		"<acceleration><x>0</x><y>1000</y></acceleration>"
		"<collisions><collision object=\"floor\"><land /></collision></collisions>"
		"<actions><action name=\"up\"><climb direction=\"up\" class=\"ladder\">50</climb></action></actions>");
	const std::string ladder = "<object name=\"ladder\" class=\"ladder\"><sprite><rectangle><width>20</width><height>200</height></rectangle></sprite>"
		"<position><x>95</x><y>200</y></position><collisions><off /></collisions></object>";
	const std::string floor = "<object name=\"floor\"><sprite><rectangle><width>400</width><height>20</height></rectangle></sprite>"
		"<position><x>0</x><y>400</y></position><collisions></collisions></object>";
	const std::string game = smallGame(ladder + floor + climber);
	const auto up = [](Played& play)
	{
		play.engine.step(0.05f);
		xge::CommandExecutor(play.game).executeInput(xge::CmdTriggerAction{ "climber", "up" }, true);
	};
	const auto y = atEveryStepLengthOf(game, 2.0, [](Played& play) { return static_cast<double>(play.get("climber").position.y); }, up);
	agree(y, 2.0);
	CHECK(y.front().value == Approx(290.0).margin(2.0));
}

TEST_CASE("Cannon Hill's charge after a second of Space held is the same, however the time is cut into steps", "[steps][held][cannonhill]")
{
	// Three quarters a sixtieth of a second: 45 in a second.
	const auto hold = [](Played& play)
	{
		play.tap(xge::KeyCode::Space);
		play.frames(3);
		play.engine.handleKeyPressed(xge::KeyCode::Space);
	};
	const auto power = atEveryStepLength("descriptions/cannonhill.xml", 1.0, [](Played& play) { return static_cast<double>(play.get("barrel1").variable.at("power")); }, hold);
	agree(power, 0.9);
	CHECK(power.front().value == Approx(45.0).margin(0.1));
}

TEST_CASE("a path of four corners is walked in the same time and never cuts a corner, however the time is cut into steps", "[steps][paths]")
{
	// 150 pixels a second round a square of 100 from (100, 100).
	std::string game = smallGame(box("walker", 100, 100,
		"<collisions><off /></collisions><actions><action name=\"go\"><follow path=\"square\" /></action></actions>"));
	game.replace(game.find("<states>"), 0, "<paths><path name=\"square\"><speed>150</speed>"
		"<step><x>100</x><y>0</y></step><step><x>0</x><y>100</y></step><step><x>-100</x><y>0</y></step><step><x>0</x><y>-100</y></step></path></paths>");
	const auto go = [](Played& play) { xge::CommandExecutor(play.game).executeInput(xge::CmdTriggerAction{ "walker", "go" }, true); };
	const auto x = [](Played& play) { return static_cast<double>(play.get("walker").position.x); };
	const auto y = [](Played& play) { return static_cast<double>(play.get("walker").position.y); };

	// 105 along: round the first corner, 5 down the second side.
	const auto x1 = atEveryStepLengthOf(game, 0.7, x, go);
	const auto y1 = atEveryStepLengthOf(game, 0.7, y, go);
	agree(x1, 0.5);
	agree(y1, 0.5);
	CHECK(x1.front().value == Approx(200.0).margin(0.5));
	CHECK(y1.front().value == Approx(105.0).margin(0.5));

	// 285 along: past the second corner, 85 along the third side.
	const auto x3 = atEveryStepLengthOf(game, 1.9, x, go);
	const auto y3 = atEveryStepLengthOf(game, 1.9, y, go);
	agree(x3, 0.5);
	agree(y3, 0.5);
	CHECK(x3.front().value == Approx(115.0).margin(0.5));
	CHECK(y3.front().value == Approx(200.0).margin(0.5));
}

TEST_CASE("a <chase> stops at its near distance inside a step, however the time is cut into steps", "[steps][chase]")
{
	// 400 pixels a second at a still target 300 off, stopping 20 short. It
	// stops inside the step that the chase is run in; run less often, it
	// goes on at its velocity between runs, as any mover does.
	const std::string game = smallGame(
		box("chaser", 100, 100, "<collisions><off /></collisions>"
			"<timers><timer><every>0.004</every><chase object=\"target\">400<near>20</near></chase></timer></timers>")
		+ box("target", 400, 100, "<collisions><off /></collisions>"));
	const auto x = atEveryStepLengthOf(game, 2.0, [](Played& play) { return static_cast<double>(play.get("chaser").position.x); });
	agree(x, 2.0);
	CHECK(x.front().value == Approx(380.0).margin(2.0));
}

TEST_CASE("a <hunt> through a maze keeps to its lanes and goes as far, however the time is cut into steps", "[steps][maze]")
{
	// 300 pixels a second, 20-pixel tiles: a step of a tenth passes the
	// middles of one or two tiles.
	const std::string map = "<map name=\"maze\"><tile><width>20</width><height>20</height></tile><position><x>0</x><y>0</y></position>"
		"<row>###############</row>"
		"<row>#H..........###</row>"
		"<row>#.#########...#</row>"
		"<row>#.#########.#.#</row>"
		"<row>#...........#.#</row>"
		"<row>#############.#</row>"
		"<row>#T............#</row>"
		"<row>###############</row>"
		"<group name=\"walls\" char=\"#\" class=\"wall\"><sprite><rectangle><width>20</width><height>20</height></rectangle></sprite></group>"
		"<object name=\"prey\" char=\"T\"><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite><collisions><off /></collisions></object>"
		"<object name=\"hunter\" char=\"H\"><sprite><rectangle><width>16</width><height>16</height></rectangle></sprite>"
		"<maze map=\"maze\"><wall class=\"wall\" /></maze><collisions><off /></collisions>"
		"<actions><action name=\"go\"><hunt object=\"prey\">300<search /></hunt></action></actions></object></map>";
	const std::string game = smallGame(map);
	const auto go = [](Played& play) { xge::CommandExecutor(play.game).executeInput(xge::CmdTriggerAction{ "playing.hunter", "go" }, true); };
	const auto middleX = [](Played& play) { const auto& h = play.get("playing.hunter"); return static_cast<double>(h.position.x + h.size.x / 2.0f); };
	const auto middleY = [](Played& play) { const auto& h = play.get("playing.hunter"); return static_cast<double>(h.position.y + h.size.y / 2.0f); };
	// On a lane: its middle across the way it goes is a tile's middle.
	const auto offLane = [&](Played& play)
	{
		const double mx = std::fmod(middleX(play), 20.0) - 10.0;
		const double my = std::fmod(middleY(play), 20.0) - 10.0;
		return std::min(std::abs(mx), std::abs(my));
	};
	for (const double seconds : { 0.5, 1.0, 1.5 })
	{
		INFO("after " << seconds << " seconds");
		const auto x = atEveryStepLengthOf(game, seconds, middleX, go);
		const auto y = atEveryStepLengthOf(game, seconds, middleY, go);
		agree(x, 2.0);
		agree(y, 2.0);
		// Gone from its start at (30, 30).
		CHECK(std::abs(x.front().value - 30.0) + std::abs(y.front().value - 30.0) > 100.0);
		const auto lane = atEveryStepLengthOf(game, seconds, offLane, go);
		for (const Paced& each : lane)
		{
			INFO("steps of " << each.pacing);
			CHECK(each.value < 0.01);
		}
	}
}

TEST_CASE("a fast ball bounces off all four sides at the moment it meets them, however the time is cut into steps", "[steps][edges]")
{
	// 2,000 and 1,500 pixels a second in a window of 800 by 600: a step of a
	// tenth goes 200 and 150, and every side turns it back where it meets it.
	const std::string game = smallGame(box("ball", 400, 300, "<velocity><x>2000</x><y>1500</y></velocity>"
		"<collisions><collision edge=\"left\"><bounce /></collision><collision edge=\"right\"><bounce /></collision>"
		"<collision edge=\"top\"><bounce /></collision><collision edge=\"bottom\"><bounce /></collision></collisions>"));
	const std::string path = scratchName("descriptions/steps_edges.xml");
	{
		std::ofstream out(path);
		out << game;
	}
	{
		Played play{ path };
		int turnsX = 0;
		int turnsY = 0;
		xge::Vector2f was = play.get("ball").velocity;
		for (int step = 0; step < 30; ++step)
		{
			play.engine.step(0.1f);
			const xge::Object& ball = play.get("ball");
			INFO("step " << step << ": " << ball.position.x << ", " << ball.position.y);
			CHECK(ball.position.x >= -0.01f);
			CHECK(ball.position.x <= 789.01f);
			CHECK(ball.position.y >= -0.01f);
			CHECK(ball.position.y <= 590.01f);
			if ((ball.velocity.x > 0.0f) != (was.x > 0.0f)) { ++turnsX; }
			if ((ball.velocity.y > 0.0f) != (was.y > 0.0f)) { ++turnsY; }
			was = ball.velocity;
		}
		// 6,000 across 779 and 4,500 down 590 in 3 seconds.
		CHECK(turnsX >= 6);
		CHECK(turnsY >= 6);
	}
	std::error_code ignored;
	std::filesystem::remove(path, ignored);

	const auto x = atEveryStepLengthOf(game, 2.0, [](Played& play) { return static_cast<double>(play.get("ball").position.x); });
	const auto y = atEveryStepLengthOf(game, 2.0, [](Played& play) { return static_cast<double>(play.get("ball").position.y); });
	agree(x, 2.0);
	agree(y, 2.0);
}
