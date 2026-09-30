// states.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "states.h"

namespace xge
{
	std::ostream& operator<<(std::ostream& o, const RawState& f)
	{
		o << "state: ";
		o << "name=" << f.name << ", show=";
		for (auto& show : f.show)
		{
			o << show << (show != f.show.back() ? ", " : "\n");
		}
		for (auto& input : f.input)
		{
			o << "       button=" << input.first << ", action=" << input.second << '\n';
		}

		for (auto& condition : f.conditions)
		{
			o << "       condition: ";
			if (condition.test == RawCondition::Test::Remaining)
			{
				o << "remaining=" << condition.threshold;
			}
			else if (condition.test == RawCondition::Test::AtMost)
			{
				o << "variable=" << condition.variableName << ", atmost=" << condition.threshold;
			}
			else
			{
				o << "variable=" << condition.variableName << ", atleast=" << condition.threshold;
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
		for (auto& show : f.show)
		{
			o << show << (show != f.show.back() ? ", " : "\n");
		}
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
			if (condition.remaining)
			{
				o << "remaining=" << *condition.remaining;
			}
			else if (condition.atMost)
			{
				o << "variable=" << condition.variableName << ", atmost=" << *condition.atMost;
			}
			else
			{
				o << "variable=" << condition.variableName << ", value=" << condition.value;
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

		return o;
	}
}
