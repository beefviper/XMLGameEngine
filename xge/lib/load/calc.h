// calc.h
// XML Game Engine
// author: beefviper
// date: Oct 29, 2026

#pragma once

#include <cstddef>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

namespace xge
{
	// The arithmetic in a value's text (load/expr): numbers (2, 0.5, 1e3),
	// names (window.right, ship.score; case does not matter), + - * /, a
	// minus or plus in front, brackets, pi, and the functions abs, ceil,
	// floor, round, sgn, sqrt, sin, cos and tan (of one number) and min and
	// max (of two or more), and one comparison (< <= > >= == !=), 1 when it
	// holds and 0 when not (Pac-Man's 20 + 100000 * (mode.waves >= 4)).
	// Worked out left to right, * and / before + and -, those before a
	// comparison, in float, exactly as written: nothing is regrouped.
	//
	// A text is compiled once into steps that read each name's number where
	// it lives, so working it out again only reads the numbers as they are
	// now.
	class Calc
	{
	public:
		Calc();

		// A name that always reads `value`. The first given under a name
		// (constant or variable) keeps it.
		void constant(const std::string& name, float value);

		// A name that reads `value` as it is whenever the text is worked
		// out; `value` must stay where it is.
		void variable(const std::string& name, const float& value);

		// Whether the name reads anything (a constant or a variable).
		bool has(const std::string& name) const;

		// Whether the name is one of the functions, or pi: a game cannot
		// name a variable so.
		static bool isOwnName(const std::string& name);

		// One step of a compiled text.
		struct Step
		{
			enum class Kind { Number, Read, Negate, Add, Subtract, Multiply, Divide, Call,
				Less, AtMost, More, AtLeast, Same, NotSame };
			Kind kind{ Kind::Number };
			float number{};
			const float* read{ nullptr };
			std::size_t function{};
			std::size_t count{};
		};

		// What a text compiles to: its steps, in the order they run.
		struct Program
		{
			std::vector<Step> steps;
		};

		// The text compiled. Throws std::invalid_argument, saying what is
		// wrong with it, for a text that is not arithmetic or names a name
		// that reads nothing.
		Program compile(const std::string& text) const;

		// The program's answer, from its names' numbers as they are now.
		static float run(const Program& program);

	private:
		std::unordered_map<std::string, const float*> names;

		// The constants' numbers, where they stay put as more come.
		std::deque<float> constants;
	};
}
