// states.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include "command.h"
#include "keycode.h"

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <ostream>

namespace xge
{
	// A <condition> checked every frame its state is current, in one of three
	// forms, told apart by which test tag it holds. <atleast>: if any object
	// matching filterClass/filterObject (same optional-either-or-both
	// filtering as a collision rule) has a variable named `variableName` that
	// has reached that value, the commands fire. <atmost>: the same, when the
	// variable has fallen to that or below - lives 0 is a game over.
	// <remaining>: if no more than that many of the matching objects are still
	// in play (visible - die() hides an object), the commands fire;
	// remaining 0 is "they are all gone". Leaving both filters empty matches
	// any object, same as an unfiltered collision rule.
	struct RawCondition
	{
		enum class Test { AtLeast, AtMost, Remaining };

		std::string filterClass;
		std::string filterObject;
		std::string variableName;
		Test test{ Test::AtLeast };
		RawValue threshold;
		std::vector<RawCommand> commands;
	};

	struct Condition
	{
		std::string filterClass;
		std::string filterObject;
		std::string variableName;
		float value{};
		std::optional<float> remaining;
		std::optional<float> atMost;
		std::vector<Command> commands;
	};

	class RawState
	{
	public:
		std::string name;
		std::vector<std::string> show;
		std::map<std::string, std::vector<RawCommand>> input;
		std::vector<RawCondition> conditions;
		std::vector<RawTimer> timers;

		friend std::ostream& operator<<(std::ostream& o, RawState const& f);

	private:

	};

	class State
	{
	public:
		std::string name;
		std::vector<std::string> show;
		std::map<KeyCode, std::vector<Command>> input;
		std::vector<Condition> conditions;

		// Counted while this is the current state (Game::updateTimers); the
		// counts themselves are kept by Game, per state, so a state pushed
		// again after a pause carries on where it was.
		std::vector<Timer> timers;

		friend std::ostream& operator<<(std::ostream& o, State const& f);

	private:

	};
}
