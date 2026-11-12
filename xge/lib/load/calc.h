// calc.h
// XML Game Engine
// author: beefviper
// date: Oct 29, 2026

#pragma once

#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

namespace xge
{
	// What a value's text holds (load/expr): one number (2, -130, 0.5, 1e3)
	// or one name (window.right, ship.score; case does not matter). Nothing
	// more: arithmetic is operation tags (<add>, <subtract>, ...), never
	// hidden in a string.
	//
	// A text is compiled once into where its number lives, so working it out
	// again only reads the number as it is now.
	class Calc
	{
	public:
		// A name that always reads `value`. The first given under a name
		// (constant or variable) keeps it.
		void constant(const std::string& name, float value);

		// A name that reads `value` as it is whenever the text is worked
		// out; `value` must stay where it is.
		void variable(const std::string& name, const float& value);

		// Whether the name reads anything (a constant or a variable).
		bool has(const std::string& name) const;

		// The name a text is, lowercased, whether it names anything or not;
		// none for a number.
		static std::vector<std::string> namesIn(const std::string& text);

		// What a text compiles to: its number, or where the number it names lives.
		struct Program
		{
			float number{};
			const float* read{ nullptr };
		};

		// The text compiled. Throws std::invalid_argument, saying what is
		// wrong with it, for a text that is more than one number or one name
		// (and how to write it as an operation), or names nothing.
		Program compile(const std::string& text) const;

		// The program's number, read as it is now.
		static float run(const Program& program);

	private:
		std::unordered_map<std::string, const float*> names;

		// The constants' numbers, where they stay put as more come.
		std::deque<float> constants;
	};
}
