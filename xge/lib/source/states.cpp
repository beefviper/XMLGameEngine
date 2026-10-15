// states.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "states.h"

namespace xge
{
	bool passes(Compare compare, float variable, float against) noexcept
	{
		switch (compare)
		{
		case Compare::AtLeast: return variable >= against;
		case Compare::AtMost:  return variable <= against;
		case Compare::Above:   return variable > against;
		case Compare::Below:   return variable < against;
		case Compare::Equal:   return variable == against;
		}
		return false;
	}

	std::ostream& operator<<(std::ostream& o, const RawState& f)
	{
		o << "state: ";
		o << "name=" << f.name << ", show=";
		for (std::size_t i = 0; i < f.show.size(); ++i)
		{
			o << (i > 0 ? ", " : "") << f.show[i];
		}
		for (const auto& still : f.showStates)
		{
			o << (f.show.empty() && &still == &f.showStates.front() ? "" : ", ") << "state " << still;
		}
		o << '\n';
		for (auto& input : f.input)
		{
			o << "       button=" << input.first << ", action=" << input.second << '\n';
		}

		for (auto& condition : f.conditions)
		{
			o << "       condition: ";
			static const char* const tests[] = { "atleast", "atmost", "remaining", "above", "below", "equal" };
			const char* test = tests[static_cast<std::size_t>(condition.test)];
			if (condition.test == RawCondition::Test::Remaining)
			{
				o << "remaining=" << condition.threshold;
			}
			else if (!condition.otherVariable.empty())
			{
				o << "variable=" << condition.variableName << ", " << test << "=" << (condition.otherObject.empty() ? "" : condition.otherObject + ".") << condition.otherVariable;
			}
			else
			{
				o << "variable=" << condition.variableName << ", " << test << "=" << condition.threshold;
			}
			o << (condition.filterClass.empty() ? "" : ", class=" + condition.filterClass)
				<< (condition.filterObject.empty() ? "" : ", object=" + condition.filterObject)
				<< ", action=" << condition.commands << '\n';
		}

		return o;
	}

	std::ostream& operator<<(std::ostream& o, const State& f)
	{
		o << "state: ";
		o << "name=" << f.name << ", show=";
		for (std::size_t i = 0; i < f.show.size(); ++i)
		{
			o << (i > 0 ? ", " : "") << f.show[i];
		}
		for (const auto& still : f.showStates)
		{
			o << (f.show.empty() && &still == &f.showStates.front() ? "" : ", ") << "state " << still;
		}
		o << '\n';
		for (auto& input : f.input)
		{
			o << "       button=" << keyCodeToString(input.first) << ", action=";
			for (auto& command : input.second)
			{
				o << command << (&command != &input.second.back() ? ";" : "");
			}
			o << '\n';
		}

		for (auto& condition : f.conditions)
		{
			o << "       condition: ";
			static const char* const compares[] = { "atleast", "atmost", "above", "below", "equal" };
			const char* compare = compares[static_cast<std::size_t>(condition.compare)];
			if (condition.remaining)
			{
				o << "remaining=" << *condition.remaining;
			}
			else if (!condition.otherVariable.empty())
			{
				o << "variable=" << condition.variableName << ", " << compare << "=" << (condition.otherObject.empty() ? "" : condition.otherObject + ".") << condition.otherVariable;
			}
			else
			{
				o << "variable=" << condition.variableName << ", " << compare << "=" << condition.value;
			}
			o << (condition.filterClass.empty() ? "" : ", class=" + condition.filterClass)
				<< (condition.filterObject.empty() ? "" : ", object=" + condition.filterObject)
				<< ", action=";
			for (auto& command : condition.commands)
			{
				o << command << (&command != &condition.commands.back() ? ";" : "");
			}
			o << '\n';
		}

		for (auto& timer : f.timers)
		{
			o << "       timer: " << (timer.repeat ? "every=" : "after=") << timer.interval << ", action=";
			for (auto& command : timer.commands)
			{
				o << command << (&command != &timer.commands.back() ? ";" : "");
			}
			o << '\n';
		}

		return o;
	}
}
