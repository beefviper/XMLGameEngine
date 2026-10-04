// test_deflect.cpp
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026
//
// Catch2 tests for <deflect>: a bounce off another object whose angle comes
// from where it hit. How it parses and prints, the angle it gives along a
// Pong paddle (straight from the middle, the full angle at either end, in
// proportion between), that the speed is kept, and Pong's ball doing it in a
// real frame.

#include "command.h"
#include "command_executor.h"
#include "game.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <cmath>
#include <sstream>
#include <variant>

using namespace xge;
using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::WithinAbs;

namespace
{
	float evaluatePlainNumber(const RawValue& value)
	{
		return std::stof(value.text);
	}

	RawCommand deflectTag(const std::string& angle)
	{
		RawCommand command;
		command.verb = "deflect";
		command.amount = RawValue::expression(angle);
		return command;
	}

	float speedOf(const Object& object)
	{
		return std::hypot(object.velocity.x, object.velocity.y);
	}

	// The angle the object is heading, in degrees away from straight along x:
	// positive is down the screen, negative up.
	float angleOf(const Object& object)
	{
		return std::atan2(object.velocity.y, std::abs(object.velocity.x)) * 180.0f / 3.14159265358979323846f;
	}

	// pong.xml's ball and paddles, sized as a backend would (the paddle is a
	// 30 by 150 image), with paddle1's side at x = 130 and its middle at y = 375.
	struct Pong
	{
		Game game{ "games/pong.xml" };
		CommandExecutor executor{ game };
		Object& ball;
		Object& paddle1;
		Object& paddle2;

		Pong() : ball(game.getObject("ball")), paddle1(game.getObject("paddle1")), paddle2(game.getObject("paddle2"))
		{
			game.setCurrentState("playing");
			ball.size = { 20.0f, 20.0f };
			paddle1.size = { 30.0f, 150.0f };
			paddle2.size = { 30.0f, 150.0f };
			paddle1.position = { 100.0f, 300.0f };
			paddle2.position = { 1150.0f, 300.0f };
			paddle1.velocity = {};
			paddle2.velocity = {};
		}

		// Puts the ball's middle `offset` pixels below paddle1's middle, against
		// its right side, heading in at (-6, 2), and deflects it.
		void hitPaddle1(float offset, float maxAngle = 45.0f)
		{
			ball.position = { 130.0f, 375.0f + offset - 10.0f };
			ball.velocity = { -6.0f, 2.0f };
			executor.executeObjectCollision(Command{ CmdDeflect{ maxAngle } }, ball, paddle1, Edge::Left);
		}
	};
}

TEST_CASE("<deflect>angle</deflect> makes a CmdDeflect, and prints as it is written", "[deflect][command_parsing]")
{
	const auto commands = makeCommands({ deflectTag("45") }, evaluatePlainNumber);

	REQUIRE(commands.size() == 1);
	REQUIRE(std::holds_alternative<CmdDeflect>(commands[0]));
	CHECK(std::get<CmdDeflect>(commands[0]).maxAngle == 45.0f);

	std::ostringstream printed;
	printed << commands[0] << " " << deflectTag("45");
	CHECK(printed.str() == "deflect(45) deflect(45)");
}

TEST_CASE("a <deflect> angle is from 0 up to, not including, 90 degrees", "[deflect][command_parsing][errors]")
{
	CHECK_NOTHROW(makeCommands({ deflectTag("0") }, evaluatePlainNumber));
	CHECK_NOTHROW(makeCommands({ deflectTag("89") }, evaluatePlainNumber));
	REQUIRE_THROWS_WITH(makeCommands({ deflectTag("90") }, evaluatePlainNumber), ContainsSubstring("<deflect> has an angle of 90"));
	REQUIRE_THROWS_WITH(makeCommands({ deflectTag("-10") }, evaluatePlainNumber), ContainsSubstring("<deflect> has an angle of -10"));
}

TEST_CASE("Pong's ball deflects off the paddles at up to 45 degrees", "[deflect][xml]")
{
	Pong pong;

	const auto& rules = pong.ball.collisionData.basic;
	REQUIRE(rules.size() == 1);
	REQUIRE(std::holds_alternative<CmdDeflect>(rules[0].commands.at(0)));
	CHECK(std::get<CmdDeflect>(rules[0].commands.at(0)).maxAngle == 45.0f);
}

TEST_CASE("off the middle of a paddle the ball goes straight back, at the speed it came in", "[deflect]")
{
	Pong pong;
	pong.hitPaddle1(0.0f);

	CHECK_THAT(pong.ball.velocity.x, WithinAbs(std::hypot(6.0f, 2.0f), 1e-4f));
	CHECK_THAT(pong.ball.velocity.y, WithinAbs(0.0f, 1e-4f));
}

TEST_CASE("off the top end the ball leaves 45 degrees up, off the bottom end 45 degrees down", "[deflect]")
{
	Pong pong;
	const float speed = std::hypot(6.0f, 2.0f);

	pong.hitPaddle1(-75.0f);
	CHECK(pong.ball.velocity.x > 0.0f);
	CHECK_THAT(angleOf(pong.ball), WithinAbs(-45.0f, 1e-3f));
	CHECK_THAT(speedOf(pong.ball), WithinAbs(speed, 1e-4f));

	pong.hitPaddle1(75.0f);
	CHECK(pong.ball.velocity.x > 0.0f);
	CHECK_THAT(angleOf(pong.ball), WithinAbs(45.0f, 1e-3f));
	CHECK_THAT(speedOf(pong.ball), WithinAbs(speed, 1e-4f));
}

TEST_CASE("between the middle and an end the angle is in proportion, and past an end it is the full angle", "[deflect]")
{
	Pong pong;

	pong.hitPaddle1(-37.5f); // half way up
	CHECK_THAT(angleOf(pong.ball), WithinAbs(-22.5f, 1e-3f));

	pong.hitPaddle1(15.0f); // a fifth of the way down
	CHECK_THAT(angleOf(pong.ball), WithinAbs(9.0f, 1e-3f));

	pong.hitPaddle1(-84.0f); // clipping the corner, the ball's middle above the paddle
	CHECK_THAT(angleOf(pong.ball), WithinAbs(-45.0f, 1e-3f));

	pong.hitPaddle1(-37.5f, 60.0f); // a wider angle is the game's to choose
	CHECK_THAT(angleOf(pong.ball), WithinAbs(-30.0f, 1e-3f));
}

TEST_CASE("which way the ball goes depends on the side it hit, not on how it came in", "[deflect]")
{
	Pong pong;

	// Coming in steeply downwards onto the top half of paddle1: back up.
	pong.ball.position = { 130.0f, 330.0f };
	pong.ball.velocity = { -1.0f, 7.0f };
	pong.executor.executeObjectCollision(Command{ CmdDeflect{ 45.0f } }, pong.ball, pong.paddle1, Edge::Left);
	CHECK(pong.ball.velocity.x > 0.0f);
	CHECK(pong.ball.velocity.y < 0.0f);

	// Off paddle2's left side the ball goes left.
	pong.ball.position = { 1130.0f, 410.0f };
	pong.ball.velocity = { 6.0f, 0.0f };
	pong.executor.executeObjectCollision(Command{ CmdDeflect{ 45.0f } }, pong.ball, pong.paddle2, Edge::Right);
	CHECK(pong.ball.velocity.x < 0.0f);
	CHECK(pong.ball.velocity.y > 0.0f);
	CHECK_THAT(speedOf(pong.ball), WithinAbs(6.0f, 1e-4f));
}

TEST_CASE("a <deflect> in a screen-edge rule does nothing", "[deflect]")
{
	Pong pong;
	pong.ball.velocity = { -6.0f, 2.0f };
	pong.executor.executeScreenEdgeCollision(Command{ CmdDeflect{ 45.0f } }, pong.ball, Edge::Left);
	CHECK(pong.ball.velocity.x == -6.0f);
	CHECK(pong.ball.velocity.y == 2.0f);
}

TEST_CASE("in a real frame, Pong's ball comes off the top of a paddle heading up", "[deflect][swept_collision]")
{
	Pong pong;

	// Heading straight left at paddle1, its middle 60 above the paddle's middle.
	pong.ball.position = { 134.0f, 375.0f - 60.0f - 10.0f };
	pong.ball.velocity = { -6.0f, 0.0f };

	pong.game.updateObjects();

	CHECK(pong.ball.velocity.x > 0.0f);
	CHECK(pong.ball.velocity.y < 0.0f);
	CHECK_THAT(angleOf(pong.ball), WithinAbs(-36.0f, 0.5f));
	CHECK_THAT(speedOf(pong.ball), WithinAbs(6.0f, 1e-4f));
}
