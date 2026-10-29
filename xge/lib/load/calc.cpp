// calc.cpp
// XML Game Engine
// author: beefviper
// date: Oct 29, 2026

#include "load/calc.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <utility>

namespace xge
{
	namespace
	{
		// How deep a text's working may go: numbers waiting for an operator.
		constexpr std::size_t kDeepest = 64;

		std::string lowered(std::string text)
		{
			std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return text;
		}

		bool isLetter(char c) { return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_'; }
		bool isDigit(char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; }

		// The name that starts at `at` (a letter), lowercased; `at` moves past it.
		std::string readName(const std::string& text, std::size_t& at)
		{
			std::string word;
			while (at < text.size() && (isLetter(text[at]) || isDigit(text[at]) || text[at] == '.'))
			{
				word += text[at];
				++at;
			}
			return lowered(word);
		}

		// The number that starts at `at` (a digit or a dot), its exponent too
		// (2e-3); `at` moves past it. False when there is no number there.
		bool readNumber(const std::string& text, std::size_t& at, float& value)
		{
			const char* begin = text.c_str() + at;
			char* end = nullptr;
			value = std::strtof(begin, &end);
			if (end == begin) { return false; }
			at += static_cast<std::size_t>(end - begin);
			return true;
		}

		// Reads a text into steps: an expression is terms added and taken
		// away, a term is factors multiplied and divided, a factor a number,
		// a name, a bracketed expression, or one of those with a minus or
		// plus in front.
		class Reader
		{
		public:
			Reader(const std::string& written, const std::unordered_map<std::string, const float*>& names) : text(written), known(names) {}

			Calc::Program read()
			{
				skipSpace();
				if (at == text.size()) { fail("there is nothing to work out"); }
				expression();
				skipSpace();
				if (at < text.size()) { fail(std::string("'") + text[at] + "' where an operator or the end was expected"); }
				return std::move(program);
			}

		private:
			const std::string& text;
			const std::unordered_map<std::string, const float*>& known;
			std::size_t at{ 0 };
			std::size_t depth{ 0 };
			Calc::Program program;

			[[noreturn]] void fail(const std::string& why) const
			{
				throw std::invalid_argument(why);
			}

			void skipSpace()
			{
				while (at < text.size() && std::isspace(static_cast<unsigned char>(text[at])) != 0) { ++at; }
			}

			bool take(char c)
			{
				skipSpace();
				if (at < text.size() && text[at] == c)
				{
					++at;
					return true;
				}
				return false;
			}

			void emit(Calc::Step step, std::size_t popped, std::size_t pushed)
			{
				depth = depth - popped + pushed;
				if (depth > kDeepest) { fail("it is nested too deeply"); }
				program.steps.push_back(step);
			}

			void binary(Calc::Step::Kind kind)
			{
				Calc::Step step;
				step.kind = kind;
				emit(step, 2, 1);
			}

			void expression()
			{
				term();
				for (;;)
				{
					if (take('+')) { term(); binary(Calc::Step::Kind::Add); }
					else if (take('-')) { term(); binary(Calc::Step::Kind::Subtract); }
					else { return; }
				}
			}

			void term()
			{
				factor();
				for (;;)
				{
					if (take('*')) { factor(); binary(Calc::Step::Kind::Multiply); }
					else if (take('/')) { factor(); binary(Calc::Step::Kind::Divide); }
					else { return; }
				}
			}

			void factor()
			{
				if (take('-'))
				{
					factor();
					Calc::Step step;
					step.kind = Calc::Step::Kind::Negate;
					emit(step, 1, 1);
					return;
				}
				if (take('+'))
				{
					factor();
					return;
				}
				if (take('('))
				{
					expression();
					if (!take(')')) { fail("a '(' that is never closed"); }
					return;
				}

				skipSpace();
				if (at == text.size()) { fail("it ends where a number or a name was expected"); }
				if (isDigit(text[at]) || text[at] == '.') { number(); return; }
				if (isLetter(text[at])) { name(); return; }
				fail(std::string("'") + text[at] + "' where a number or a name was expected");
			}

			void number()
			{
				Calc::Step step;
				step.kind = Calc::Step::Kind::Number;
				if (!readNumber(text, at, step.number)) { fail("a number that cannot be read"); }
				if (at < text.size() && (isLetter(text[at]) || text[at] == '.'))
				{
					fail("a number with letters after it; write the * (2 * x, not 2x)");
				}
				emit(step, 0, 1);
			}

			void name()
			{
				const std::string word = readName(text, at);
				skipSpace();
				if (at < text.size() && text[at] == '(')
				{
					fail("'" + word + "(' is a function, and a value's text has none; write it as a <formula> (<smallest>, <largest>, <sign>, ...)");
				}

				const auto found = known.find(word);
				if (found == known.end()) { fail("'" + word + "' names nothing"); }
				Calc::Step step;
				step.kind = Calc::Step::Kind::Read;
				step.read = found->second;
				emit(step, 0, 1);
			}
		};
	}

	void Calc::constant(const std::string& name, float value)
	{
		const std::string key = lowered(name);
		if (names.count(key)) { return; }
		constants.push_back(value);
		names.emplace(key, &constants.back());
	}

	void Calc::variable(const std::string& name, const float& value)
	{
		names.emplace(lowered(name), &value);
	}

	bool Calc::has(const std::string& name) const
	{
		return names.count(lowered(name)) != 0;
	}

	std::vector<std::string> Calc::namesIn(const std::string& text)
	{
		std::vector<std::string> found;
		for (std::size_t at = 0; at < text.size();)
		{
			float number = 0.0f;
			if (isLetter(text[at]))
			{
				std::string word = readName(text, at);
				if (std::find(found.begin(), found.end(), word) == found.end()) { found.push_back(std::move(word)); }
			}
			else if (!((isDigit(text[at]) || text[at] == '.') && readNumber(text, at, number)))
			{
				++at;
			}
		}
		return found;
	}

	Calc::Program Calc::compile(const std::string& text) const
	{
		return Reader(text, names).read();
	}

	float Calc::run(const Program& program)
	{
		std::array<float, kDeepest + 1> stack{};
		std::size_t top = 0;
		for (const Step& step : program.steps)
		{
			switch (step.kind)
			{
			case Step::Kind::Number:   stack[top++] = step.number; break;
			case Step::Kind::Read:     stack[top++] = *step.read; break;
			case Step::Kind::Negate:   stack[top - 1] = -stack[top - 1]; break;
			case Step::Kind::Add:      --top; stack[top - 1] += stack[top]; break;
			case Step::Kind::Subtract: --top; stack[top - 1] -= stack[top]; break;
			case Step::Kind::Multiply: --top; stack[top - 1] *= stack[top]; break;
			case Step::Kind::Divide:   --top; stack[top - 1] /= stack[top]; break;
			}
		}
		return top == 0 ? 0.0f : stack[top - 1];
	}
}
