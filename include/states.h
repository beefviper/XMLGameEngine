// states.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include "command.h"

#include <string>
#include <vector>
#include <map>
#include <ostream>

namespace xge
{
	// A <condition> checked every frame its state is current: if any object
	// matching filterClass/filterObject (same optional-either-or-both
	// filtering as a collision's basic rule) has a variable named
	// `variableName` that has reached `value`, `action` fires. Leaving both
	// filters empty matches any object, same as an unfiltered collision rule.
	struct RawCondition
	{
		std::string filterClass;
		std::string filterObject;
		std::string variableName;
		float value{};
		std::string action;
	};

	struct Condition
	{
		std::string filterClass;
		std::string filterObject;
		std::string variableName;
		float value{};
		std::vector<Command> commands;
	};

	class RawState
	{
	public:
		std::string name;
		std::vector<std::string> show;
		std::map<std::string, std::string> input;
		std::vector<RawCondition> conditions;

		friend std::ostream& operator<<(std::ostream& o, RawState const& f);

	private:

	};

	class State
	{
	public:
		std::string name;
		std::vector<std::string> show;
		std::map<std::string, std::vector<Command>> input;
		std::vector<Condition> conditions;

		friend std::ostream& operator<<(std::ostream& o, State const& f);

	private:

	};
}
