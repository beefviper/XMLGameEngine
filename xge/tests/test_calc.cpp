// test_calc.cpp
// XML Game Engine
// author: beefviper
// date: Oct 29, 2026
//
// Catch2 tests for load/calc, the arithmetic in a value's text: numbers,
// names, + - * /, a sign in front and brackets, worked out as written; what
// it says about a text it cannot read; and the names a text reads.

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

TEST_CASE("calc: * and / before + and -, left to right, brackets first", "[calc]")
{
	const Calc calc;
	CHECK(workOut(calc, "2 + 3 * 4") == 14.0f);
	CHECK(workOut(calc, "(2 + 3) * 4") == 20.0f);
	CHECK(workOut(calc, "100 - 10 - 20") == 70.0f);
	CHECK(workOut(calc, "100 / 10 / 5") == 2.0f);
	CHECK(workOut(calc, "-3 * -2") == 6.0f);
	CHECK(workOut(calc, "+4 - -1") == 5.0f);
	CHECK(workOut(calc, "0.5 + .25 + 1e2") == 100.75f);
	CHECK(workOut(calc, "  7  ") == 7.0f);
}

TEST_CASE("calc: a constant reads its number, a variable the number as it is now", "[calc]")
{
	Calc calc;
	float speed = 2.0f;
	calc.constant("window.right", 800.0f);
	calc.variable("ball.speed", speed);

	const Calc::Program program = calc.compile("Window.Right - BALL.speed * 10");
	CHECK(Calc::run(program) == 780.0f);
	speed = 5.0f;
	CHECK(Calc::run(program) == 750.0f);

	// The first given under a name keeps it.
	calc.constant("window.right", 1.0f);
	CHECK(workOut(calc, "window.right") == 800.0f);
	CHECK(calc.has("WINDOW.RIGHT"));
	CHECK_FALSE(calc.has("window.left"));
}

TEST_CASE("calc: a text it cannot read says what is wrong", "[calc]")
{
	const Calc calc;
	CHECK_THROWS_WITH(calc.compile(""), ContainsSubstring("nothing to work out"));
	CHECK_THROWS_WITH(calc.compile("speed"), ContainsSubstring("'speed' names nothing"));
	CHECK_THROWS_WITH(calc.compile("max(1, 2)"), ContainsSubstring("'max(' is a function"));
	CHECK_THROWS_WITH(calc.compile("2x"), ContainsSubstring("write the * (2 * x, not 2x)"));
	CHECK_THROWS_WITH(calc.compile("(1 + 2"), ContainsSubstring("never closed"));
	CHECK_THROWS_WITH(calc.compile("1 +"), ContainsSubstring("ends where a number or a name was expected"));
	CHECK_THROWS_WITH(calc.compile("1 2"), ContainsSubstring("'2' where an operator or the end was expected"));
	CHECK_THROWS_WITH(calc.compile("a >= 2"), ContainsSubstring("'a' names nothing"));

	// 1 + (1 + (1 + ...)): each 1 waits for the bracket after it.
	std::string deep = "1";
	for (int i = 0; i < 100; ++i) { deep = "1 + (" + deep + ")"; }
	CHECK_THROWS_WITH(calc.compile(deep), ContainsSubstring("nested too deeply"));
}

TEST_CASE("calc: the names a text reads, each once, lowercased", "[calc]")
{
	CHECK(Calc::namesIn("Paddle1.score + 2e-3 * paddle1.SCORE - window.width.center / 2") == std::vector<std::string>{ "paddle1.score", "window.width.center" });
	CHECK(Calc::namesIn("12 * (3 - 1)").empty());
}
