// condition.h
// XML Game Engine
// author: beefviper
// date: Oct 16, 2026

#pragma once

#include "game/command.h"

#include <optional>
#include <string>
#include <vector>

namespace xge
{
	// A <condition> checked every frame its state is current, told apart by
	// which test tag it holds. <atleast>: if any object matching
	// filterClass/filterObject (same optional-either-or-both filtering as a
	// collision rule) has a variable named `variableName` that has reached
	// that value, the commands fire. <atmost>: the same, when the variable has
	// fallen to that or below - lives 0 is a game over. <above>, <below> and
	// <equal>: more than, less than, the same as. Each of those five compares
	// with a number (the tag's content), or, with variable= on the tag, with
	// another variable, read every frame: object= on the tag names whose, and
	// without it it is the tested object's own (<above object="gun2"
	// variable="score" />). <remaining>: if no more than that many of the
	// matching objects are still in play (visible - die() hides an object),
	// the commands fire; remaining 0 is "they are all gone". Leaving both
	// filters empty matches any object, same as an unfiltered collision rule.
	//
	// What is tested can also be a total over the matching objects in play
	// (total="count", "sum", "highest", "lowest") or, per matching object,
	// how far it is from the nearest of another (distance="name"). And a
	// condition can be <all> or <any> of several <check>s (each a test of
	// its own, with no commands), which can hold <all>s and <any>s in turn:
	// then `join` says which, the checks are `parts`, and the condition's own
	// test is not used.
	struct RawCondition
	{
		enum class Test { AtLeast, AtMost, Remaining, Above, Below, Equal };
		enum class Join { None, All, Any };

		std::string filterClass;
		std::string filterObject;
		std::string variableName;
		std::string total;    // total="...": "" (each object), count, sum, highest, lowest
		std::string distance; // distance="...": the object or group measured to, or ""
		Test test{ Test::AtLeast };
		RawValue threshold;      // when otherVariable is empty
		std::string otherObject;  // the test tag's object=, or empty
		std::string otherVariable; // the test tag's variable=, or empty
		Join join{ Join::None };
		std::vector<RawCondition> parts; // <all> or <any>: its <check>s, <all>s and <any>s
		std::vector<RawCommand> commands;
		std::string name; // name="...": a scene from= this one's can put another in its place
	};

	// How a condition's variable is compared, and whether `variable` passes
	// against `against` that way.
	enum class Compare { AtLeast, AtMost, Above, Below, Equal };
	bool passes(Compare compare, float variable, float against) noexcept;


	// What a condition tests: each matching object's variable, a total over
	// the matching objects in play, or each one's distance to another.
	enum class Total { None, Count, Sum, Highest, Lowest };

	struct Condition
	{
		std::string filterClass;
		std::string filterObject;
		std::string variableName;
		Total total{ Total::None };
		std::string distanceTo; // measured to the nearest one in play of this name or group
		RawCondition::Join join{ RawCondition::Join::None };
		std::vector<Condition> parts;
		Compare compare{ Compare::AtLeast };
		float value{};
		// Compared with this variable instead of value, read every frame: of
		// otherObject, or, when that is empty, of the object being tested.
		std::string otherObject;
		std::string otherVariable;
		std::optional<float> remaining;
		std::vector<Command> commands;
	};

	// "status.lives <= 0", "count of alien <= 2", "all of (...)": a
	// condition's test in a line, for printing and the inspector.
	std::string describe(const Condition& condition);
}
