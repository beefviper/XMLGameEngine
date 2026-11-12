// calc.cpp
// XML Game Engine
// author: beefviper
// date: Oct 29, 2026

#include "load/calc.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace xge
{
	namespace
	{
		std::string lowered(std::string text)
		{
			std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return text;
		}

		bool isLetter(char c) { return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_'; }
		bool isDigit(char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; }

		std::string trimmed(const std::string& text)
		{
			const std::size_t first = text.find_first_not_of(" \t\r\n");
			if (first == std::string::npos) { return {}; }
			const std::size_t last = text.find_last_not_of(" \t\r\n");
			return text.substr(first, last - first + 1);
		}

		// Whether the text is a name: a letter or underscore, then letters,
		// digits, underscores and dots (window.width.center).
		bool isName(const std::string& text)
		{
			return !text.empty() && isLetter(text[0])
				&& std::all_of(text.begin(), text.end(), [](char c) { return isLetter(c) || isDigit(c) || c == '.'; });
		}

		// The text as one number (2, -130, 0.5, 1e3), or false.
		bool readNumber(const std::string& text, float& value)
		{
			if (text.empty() || !(isDigit(text[0]) || text[0] == '.' || text[0] == '-' || text[0] == '+')) { return false; }
			const char* begin = text.c_str();
			char* end = nullptr;
			value = std::strtof(begin, &end);
			return end != begin && static_cast<std::size_t>(end - begin) == text.size();
		}

		// What to write instead of a text that is more than a number or a
		// name: its sum as an operation when it is one, else the operations.
		std::string asOperation(const std::string& text)
		{
			static const char* const kOperations[][2] = { { "+", "add" }, { "-", "subtract" }, { "*", "multiply" }, { "/", "divide" } };
			for (const auto& [sign, tag] : kOperations)
			{
				const std::size_t at = text.find(std::string(" ") + sign + " ");
				if (at == std::string::npos) { continue; }
				const std::string left = trimmed(text.substr(0, at));
				const std::string right = trimmed(text.substr(at + 3));
				float number = 0.0f;
				const auto term = [&](const std::string& part) { return isName(part) || readNumber(part, number); };
				if (term(left) && term(right))
				{
					return std::string("write it as <") + tag + "><value>" + left + "</value><value>" + right + "</value></" + tag + ">";
				}
			}
			return "write the arithmetic as operations (<add>, <subtract>, <multiply>, <divide>, <negate>, ...), each of <value>s";
		}
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
		const std::string written = trimmed(text);
		if (isName(written)) { return { lowered(written) }; }
		return {};
	}

	Calc::Program Calc::compile(const std::string& text) const
	{
		const std::string written = trimmed(text);
		if (written.empty()) { throw std::invalid_argument("there is no number or name in it"); }

		Program program;
		if (readNumber(written, program.number)) { return program; }
		if (isName(written))
		{
			const auto found = names.find(lowered(written));
			if (found == names.end()) { throw std::invalid_argument("'" + lowered(written) + "' names nothing"); }
			program.read = found->second;
			return program;
		}
		if (written.find('(') != std::string::npos && isLetter(written[0]))
		{
			throw std::invalid_argument("a value's text has no functions; write it as an operation (<smallest>, <largest>, <sign>, ...)");
		}
		throw std::invalid_argument("a value's text is one number or one name, with no arithmetic hidden in it; " + asOperation(written));
	}

	float Calc::run(const Program& program)
	{
		return program.read ? *program.read : program.number;
	}
}
