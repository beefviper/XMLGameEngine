// test_calc.cpp
// XML Game Engine
// author: beefviper
// date: Oct 29, 2026
//
// Catch2 tests for load/calc, what a value's text holds: one number or one
// name, read as it is now; what it says about a text that is more (arithmetic
// hidden in it, a function), and the name a text reads.

#include "load/calc.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <stdexcept>
#include <string>
#include <vector>

using namespace xge;
using Catch::Matchers::ContainsSubstring;

namespace
{
	float workOut(const Calc& calc, const std::string& text)
	{
		return Calc::run(calc.compile(text));
	}
}

TEST_CASE("calc: a text is one number, with its sign", "[calc]")
{
	const Calc calc;
	CHECK(workOut(calc, "14") == 14.0f);
	CHECK(workOut(calc, "-130") == -130.0f);
	CHECK(workOut(calc, "+4") == 4.0f);
	CHECK(workOut(calc, ".25") == 0.25f);
	CHECK(workOut(calc, "1e2") == 100.0f);
	CHECK(workOut(calc, "  7  ") == 7.0f);
}

TEST_CASE("calc: a constant reads its number, a variable the number as it is now", "[calc]")
{
	Calc calc;
	float speed = 2.0f;
	calc.constant("window.right", 800.0f);
	calc.variable("ball.speed", speed);

	const Calc::Program program = calc.compile("BALL.speed");
	CHECK(Calc::run(program) == 2.0f);
	speed = 5.0f;
	CHECK(Calc::run(program) == 5.0f);

	// The first given under a name keeps it.
	calc.constant("window.right", 1.0f);
	CHECK(workOut(calc, "Window.Right") == 800.0f);
	CHECK(calc.has("WINDOW.RIGHT"));
	CHECK_FALSE(calc.has("window.left"));
}

TEST_CASE("calc: a text that is more than a number or a name says what to write", "[calc]")
{
	Calc calc;
	calc.constant("window.right", 800.0f);
	CHECK_THROWS_WITH(calc.compile(""), ContainsSubstring("no number or name"));
	CHECK_THROWS_WITH(calc.compile("speed"), ContainsSubstring("'speed' names nothing"));
	CHECK_THROWS_WITH(calc.compile("max(1, 2)"), ContainsSubstring("has no functions"));
	CHECK_THROWS_WITH(calc.compile("window.right - 4"), ContainsSubstring("<subtract><value>window.right</value><value>4</value></subtract>"));
	CHECK_THROWS_WITH(calc.compile("cell * 8"), ContainsSubstring("<multiply><value>cell</value><value>8</value></multiply>"));
	CHECK_THROWS_WITH(calc.compile("(1 + 2) * 3"), ContainsSubstring("as operations (<add>, <subtract>"));
	CHECK_THROWS_WITH(calc.compile("-speed"), ContainsSubstring("one number or one name"));
	CHECK_THROWS_WITH(calc.compile("2x"), ContainsSubstring("one number or one name"));
}

TEST_CASE("calc: the name a text reads, lowercased", "[calc]")
{
	CHECK(Calc::namesIn("Paddle1.score") == std::vector<std::string>{ "paddle1.score" });
	CHECK(Calc::namesIn(" window.width.center ") == std::vector<std::string>{ "window.width.center" });
	CHECK(Calc::namesIn("12").empty());
}
