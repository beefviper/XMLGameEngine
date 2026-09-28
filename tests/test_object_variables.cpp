// test_object_variables.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026
//
// Catch2 tests for parseTextVariableBinding()/formatDisplayNumber() against
// the actual production command.cpp (not a reimplementation), using real
// sprite src strings taken straight out of games/*.xml. Neither function
// touches SFML.

#include "command.h"

#include <catch2/catch_test_macros.hpp>

using namespace xge;

TEST_CASE("parseTextVariableBinding recognizes an owner.variable reference", "[object_variables]")
{
	// pong.xml: score1/score2's sprites - an unquoted "owner.variable" reference.
	SECTION("paddle1.score")
	{
		auto binding = parseTextVariableBinding("text(paddle1.score,128,'color.white')");
		REQUIRE(binding.has_value());
		CHECK(binding->first == "paddle1");
		CHECK(binding->second == "score");
	}

	SECTION("paddle2.score")
	{
		auto binding = parseTextVariableBinding("text(paddle2.score,128,'color.white')");
		REQUIRE(binding.has_value());
		CHECK(binding->first == "paddle2");
		CHECK(binding->second == "score");
	}
}

TEST_CASE("parseTextVariableBinding treats a quoted first argument as a literal label", "[object_variables]")
{
	// A plain hand-typed number, pong.xml's title, and both games' paused
	// screens - none of these are bindings.
	CHECK_FALSE(parseTextVariableBinding("text('0',128,'color.blue')").has_value());
	CHECK_FALSE(parseTextVariableBinding("text('PONG', text.size, 'color.red')").has_value());
	CHECK_FALSE(parseTextVariableBinding("text('paused',128,'color.white')").has_value());
	CHECK_FALSE(parseTextVariableBinding("text('PAUSED',128,'color.white')").has_value());
}

TEST_CASE("parseTextVariableBinding finds nothing outside a text() call", "[object_variables]")
{
	CHECK_FALSE(parseTextVariableBinding("shape.circle(ball.radius,'color.green')").has_value());
	CHECK_FALSE(parseTextVariableBinding("image('assets/paddle.jpg', 'flip.horizontal')").has_value());
}

TEST_CASE("parseTextVariableBinding rejects malformed owner.variable shapes rather than guessing", "[object_variables]")
{
	CHECK_FALSE(parseTextVariableBinding("text(hp.max.current,128,'color.white')").has_value()); // more than one '.'
	CHECK_FALSE(parseTextVariableBinding("text(.score,128,'color.white')").has_value());          // no owner before '.'
}

TEST_CASE("formatDisplayNumber matches what someone hand-typing text('0', ...) would write", "[object_variables]")
{
	CHECK(formatDisplayNumber(0.0f) == "0");
	CHECK(formatDisplayNumber(15.0f) == "15");  // no trailing .0
	CHECK(formatDisplayNumber(-3.0f) == "-3");
}
