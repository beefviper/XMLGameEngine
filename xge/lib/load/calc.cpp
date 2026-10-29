// calc.cpp
// XML Game Engine
// author: beefviper
// date: Oct 29, 2026

#include "load/calc.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <utility>

namespace xge
{
	namespace
	{
		constexpr float kPi = 3.14159265358979323846f;

		// How deep a text's working may go: numbers waiting for an operator.
		constexpr std::size_t kDeepest = 64;

		struct Function
		{
			const char* name;
			std::size_t fewest;
			std::size_t most; // 0: any number from fewest
		};

		const std::array<Function, 11> kFunctions{ {
			{ "abs", 1, 1 }, { "ceil", 1, 1 }, { "floor", 1, 1 }, { "round", 1, 1 }, { "sgn", 1, 1 }, { "sqrt", 1, 1 },
			{ "sin", 1, 1 }, { "cos", 1, 1 }, { "tan", 1, 1 }, { "min", 2, 0 }, { "max", 2, 0 },
		} };

		const Function* functionNamed(const std::string& name, std::size_t* index = nullptr)
		{
			for (std::size_t i = 0; i < kFunctions.size(); ++i)
			{
				if (name == kFunctions[i].name)
				{
					if (index) { *index = i; }
					return &kFunctions[i];
				}
			}
			return nullptr;
		}

		std::string lowered(std::string text)
		{
			std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return text;
		}

		bool isLetter(char c) { return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_'; }
		bool isDigit(char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; }

		// Reads a text into steps: an expression is terms added and taken
		// away, a term is factors multiplied and divided, a factor a number,
		// a name, a function of expressions, a bracketed expression, or one
		// of those with a minus or plus in front.
		class Reader
		{
		public:
			Reader(const std::string& written, const std::unordered_map<std::string, const float*>& names) : text(written), known(names) {}

			Calc::Program read()
			{
				skipSpace();
				if (at == text.size()) { fail("there is nothing to work out"); }
				comparison();
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

			// An expression, or two compared: 1 when it holds, 0 when not.
			void comparison()
			{
				expression();
				skipSpace();
				using Kind = Calc::Step::Kind;
				const auto ahead = [&](const char* op) { return text.compare(at, std::char_traits<char>::length(op), op) == 0; };
				for (const auto& [op, kind] : { std::pair{ "<=", Kind::AtMost }, std::pair{ ">=", Kind::AtLeast }, std::pair{ "==", Kind::Same },
					std::pair{ "!=", Kind::NotSame }, std::pair{ "<", Kind::Less }, std::pair{ ">", Kind::More } })
				{
					if (!ahead(op)) { continue; }
					at += std::char_traits<char>::length(op);
					expression();
					binary(kind);
					return;
				}
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
					comparison();
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
				const char* begin = text.c_str() + at;
				char* end = nullptr;
				const float value = std::strtof(begin, &end);
				if (end == begin) { fail("a number that cannot be read"); }
				at += static_cast<std::size_t>(end - begin);
				if (at < text.size() && (isLetter(text[at]) || text[at] == '.'))
				{
					fail("a number with letters after it; write the * (2 * x, not 2x)");
				}
				Calc::Step step;
				step.kind = Calc::Step::Kind::Number;
				step.number = value;
				emit(step, 0, 1);
			}

			void name()
			{
				std::string word;
				while (at < text.size() && (isLetter(text[at]) || isDigit(text[at]) || text[at] == '.'))
				{
					word += text[at];
					++at;
				}
				word = lowered(word);

				std::size_t index = 0;
				if (const Function* function = functionNamed(word, &index))
				{
					if (!take('(')) { fail(word + " is a function; give it its numbers in brackets: " + word + "(...)"); }
					std::size_t count = 0;
					if (!take(')'))
					{
						do
						{
							comparison();
							++count;
						} while (take(','));
						if (!take(')')) { fail(word + "( that is never closed"); }
					}
					if (count < function->fewest || (function->most > 0 && count > function->most))
					{
						fail(word + " takes " + (function->most == 1 ? std::string("one number") : "two numbers or more")
							+ ", not " + std::to_string(count));
					}
					Calc::Step step;
					step.kind = Calc::Step::Kind::Call;
					step.function = index;
					step.count = count;
					emit(step, count, 1);
					return;
				}

				const auto found = known.find(word);
				if (found == known.end()) { fail("'" + word + "' names nothing"); }
				Calc::Step step;
				step.kind = Calc::Step::Kind::Read;
				step.read = found->second;
				emit(step, 0, 1);
			}
		};

		float call(std::size_t function, const float* first, std::size_t count)
		{
			const float x = first[0];
			switch (function)
			{
			case 0: return std::fabs(x);
			case 1: return std::ceil(x);
			case 2: return std::floor(x);
			case 3: return std::round(x);
			case 4: return x > 0.0f ? 1.0f : x < 0.0f ? -1.0f : 0.0f;
			case 5: return std::sqrt(x);
			case 6: return std::sin(x);
			case 7: return std::cos(x);
			case 8: return std::tan(x);
			case 9: return *std::min_element(first, first + count);
			default: return *std::max_element(first, first + count);
			}
		}
	}

	Calc::Calc()
	{
		constant("pi", kPi);
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

	bool Calc::isOwnName(const std::string& name)
	{
		const std::string word = lowered(name);
		return word == "pi" || functionNamed(word) != nullptr;
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
			case Step::Kind::Less:     --top; stack[top - 1] = stack[top - 1] < stack[top] ? 1.0f : 0.0f; break;
			case Step::Kind::AtMost:   --top; stack[top - 1] = stack[top - 1] <= stack[top] ? 1.0f : 0.0f; break;
			case Step::Kind::More:     --top; stack[top - 1] = stack[top - 1] > stack[top] ? 1.0f : 0.0f; break;
			case Step::Kind::AtLeast:  --top; stack[top - 1] = stack[top - 1] >= stack[top] ? 1.0f : 0.0f; break;
			case Step::Kind::Same:     --top; stack[top - 1] = stack[top - 1] == stack[top] ? 1.0f : 0.0f; break;
			case Step::Kind::NotSame:  --top; stack[top - 1] = stack[top - 1] != stack[top] ? 1.0f : 0.0f; break;
			case Step::Kind::Call:
				top -= step.count;
				stack[top] = call(step.function, &stack[top], step.count);
				++top;
				break;
			}
		}
		return top == 0 ? 0.0f : stack[top - 1];
	}
}
