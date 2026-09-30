// test_object_variables.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026
//
// Catch2 tests for parseVariableReference()/formatDisplayNumber() against
// the actual production command.cpp (not a reimplementation), using the
// <number> expressions games/*.xml gives a text sprite. Neither function
// touches SFML.

#include "command.h"

#include <catch2/catch_test_macros.hpp>

using namespace xge;

TEST_CASE("parseVariableReference recognizes an owner.variable reference", "[object_variables]")
{
	// pong.xml: score1/score2's <number> - a single "owner.variable".
	SECTION("paddle1.score")
	{
		auto binding = parseVariableReference("paddle1.score");
		REQUIRE(binding.has_value());
		CHECK(binding->first == "paddle1");
		CHECK(binding->second == "score");
	}

	SECTION("paddle2.score")
	{
		auto binding = parseVariableReference("paddle2.score");
		REQUIRE(binding.has_value());
		CHECK(binding->first == "paddle2");
		CHECK(binding->second == "score");
	}

	SECTION("whitespace around the reference is ignored")
	{
		auto binding = parseVariableReference("\n   frog.lives  \n");
		REQUIRE(binding.has_value());
		CHECK(binding->first == "frog");
		CHECK(binding->second == "lives");
	}
}

TEST_CASE("parseVariableReference finds nothing in a number or a sum", "[object_variables]")
{
	// A number typed by hand, and arithmetic - worked out once, not bound.
	CHECK_FALSE(parseVariableReference("0").has_value());
	CHECK_FALSE(parseVariableReference("paddle1.score + 1").has_value());
	CHECK_FALSE(parseVariableReference("window.width.center / 2").has_value());
	CHECK_FALSE(parseVariableReference("").has_value());
}

TEST_CASE("parseVariableReference rejects malformed owner.variable shapes rather than guessing", "[object_variables]")
{
	CHECK_FALSE(parseVariableReference("hp.max.current").has_value()); // more than one '.'
	CHECK_FALSE(parseVariableReference(".score").has_value());          // no owner before '.'
	CHECK_FALSE(parseVariableReference("score.").has_value());          // no variable after '.'
	CHECK_FALSE(parseVariableReference("score").has_value());           // no owner at all
}

TEST_CASE("formatDisplayNumber matches what someone hand-typing a <text> content of 0 would write", "[object_variables]")
{
	CHECK(formatDisplayNumber(0.0f) == "0");
	CHECK(formatDisplayNumber(15.0f) == "15");  // no trailing .0
	CHECK(formatDisplayNumber(-3.0f) == "-3");
}
