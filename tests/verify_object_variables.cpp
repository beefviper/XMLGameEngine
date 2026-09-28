// verify_object_variables.cpp
// XML Game Engine
// author: beefviper
// date: Sept 27, 2026
//
// Not part of the build (see CMakeLists.txt - it isn't listed there).
// A standalone check of parseTextVariableBinding()/formatDisplayNumber()
// against the *actual* production command.cpp (not a reimplementation),
// using real sprite src strings taken straight out of games/*.xml. Neither
// function touches SFML, so this builds and runs with no dependencies at
// all - not even the header-only SFML types the collision-geometry test
// next to this one needs. Build and run it with:
//   g++ -std=c++20 -Iinclude tests/verify_object_variables.cpp source/command.cpp -o verify_vars && ./verify_vars

#include "command.h"

#include <cstdio>
#include <string>

using namespace xge;

namespace
{
	int failures = 0;

	void expect(bool condition, const std::string& description)
	{
		std::printf("%s: %s\n", condition ? "PASS" : "FAIL", description.c_str());
		if (!condition) { ++failures; }
	}
}

int main()
{
	// --- parseTextVariableBinding ---

	// pong.xml: score1's sprite - an unquoted "owner.variable" reference.
	{
		auto binding = parseTextVariableBinding("text(paddle1.score,128,'color.white')");
		expect(binding.has_value() && binding->first == "paddle1" && binding->second == "score",
			"text(paddle1.score,...) binds to owner=paddle1, variable=score");
	}
	{
		auto binding = parseTextVariableBinding("text(paddle2.score,128,'color.white')");
		expect(binding.has_value() && binding->first == "paddle2" && binding->second == "score",
			"text(paddle2.score,...) binds to owner=paddle2, variable=score");
	}

	// pong.xml's title, and a plain hand-typed number like '0' - literal
	// string labels, not bindings (score1/score2 themselves are bindings now -
	// see the paddle1.score/paddle2.score cases above).
	{
		auto binding = parseTextVariableBinding("text('0',128,'color.blue')");
		expect(!binding.has_value(), "text('0',...) is a literal label, not a binding");
	}
	{
		auto binding = parseTextVariableBinding("text('PONG', text.size, 'color.red')");
		expect(!binding.has_value(), "text('PONG', text.size, ...) is a literal label, not a binding");
	}

	// spaceinvaders.xml / pong.xml's paused screen - also literal labels.
	{
		auto binding = parseTextVariableBinding("text('paused',128,'color.white')");
		expect(!binding.has_value(), "text('paused',...) is a literal label, not a binding");
	}
	{
		auto binding = parseTextVariableBinding("text('PAUSED',128,'color.white')");
		expect(!binding.has_value(), "text('PAUSED',...) is a literal label, not a binding");
	}

	// Not a text() call at all (a shape, an image) - no binding, no crash.
	{
		auto binding = parseTextVariableBinding("shape.circle(ball.radius,'color.green')");
		expect(!binding.has_value(), "a non-text() src has no binding");
	}
	{
		auto binding = parseTextVariableBinding("image('assets/paddle.jpg', 'flip.horizontal')");
		expect(!binding.has_value(), "image(...) has no binding");
	}

	// Malformed/ambiguous owner.variable shapes are rejected rather than guessed at.
	{
		auto binding = parseTextVariableBinding("text(hp.max.current,128,'color.white')");
		expect(!binding.has_value(), "more than one '.' is rejected, not silently truncated");
	}
	{
		auto binding = parseTextVariableBinding("text(.score,128,'color.white')");
		expect(!binding.has_value(), "a leading '.' with no owner is rejected");
	}

	// --- formatDisplayNumber ---

	expect(formatDisplayNumber(0.0f) == "0", "formatDisplayNumber(0) == \"0\"");
	expect(formatDisplayNumber(15.0f) == "15", "formatDisplayNumber(15) == \"15\" (no trailing .0)");
	expect(formatDisplayNumber(-3.0f) == "-3", "formatDisplayNumber(-3) == \"-3\"");

	std::printf("\n%d failure(s)\n", failures);
	return failures == 0 ? 0 : 1;
}
