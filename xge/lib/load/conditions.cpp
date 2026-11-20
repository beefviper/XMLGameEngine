// conditions.cpp
// XML Game Engine
// author: beefviper
// date: Nov 15, 2026
//
// A <condition> made ready when the game loads (Evaluator::makeTest,
// makeCondition): its test, checked against the objects it names, and its
// commands, made for each object it matches when they name self or others.

#include "load/expr.h"
#include "load/calc.h"
#include "load/check.h"
#include "load/reading.h"
#include "picture/color.h"
#include "engine/keys.h"
#include "load/spelling.h"
#include "picture/svg.h"
#include "play/numbers.h"
#include "audio/wave.h"
#include <algorithm>
#include <functional>
#include <initializer_list>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>

namespace xge
{
	namespace
	{
		// Whether a value says self.
		bool saysSelf(const WrittenValue& value)
		{
			WrittenValue probe = value;
			return renameInValue(probe, "self", "self_");
		}

		// Whether a condition's class= and object= pick the object.
		bool matchesFilter(const WrittenCondition& writtenCondition, const Object& object)
		{
			return (writtenCondition.filterClass.empty() || writtenCondition.filterClass == object.objClass)
				&& (writtenCondition.filterObject.empty() || object.named(writtenCondition.filterObject));
		}

		// A command's `from` (self, others) made the object `to`: as the
		// object it names, at the start of the variables and values it names,
		// and, as a look, its own name (sprite="self": the look named after
		// it). Whether it named `from` at all.
		bool renameIn(WrittenCommand& command, const std::string& from, const std::string& to)
		{
			bool changed = false;
			if (command.object == from) { command.object = to; changed = true; }
			if (command.sprite == from)
			{
				command.sprite = to.substr(to.rfind('.') + 1);
				changed = true;
			}
			for (std::string* text : { &command.variable, &command.into })
			{
				changed = renameStart(*text, from, to) || changed;
			}
			command.forEachValue([&](WrittenValue& each) { changed = renameInValue(each, from, to) || changed; });
			return changed;
		}

		// A state's condition's commands for the one it matched (self) among
		// what it looks at: self made that one, and a command naming the
		// others made once for each of the rest. Whether they name either.
		bool forMatched(std::vector<WrittenCommand>& list, const std::string& self, const std::vector<std::string>& others)
		{
			bool named = false;
			std::vector<WrittenCommand> out;
			for (const WrittenCommand& command : list)
			{
				WrittenCommand probe = command;
				if (renameIn(probe, "others", "others_"))
				{
					named = true;
					for (const std::string& other : others)
					{
						WrittenCommand each = command;
						renameIn(each, "others", other);
						renameIn(each, "self", self);
						out.push_back(std::move(each));
					}
					continue;
				}
				WrittenCommand each = command;
				named = renameIn(each, "self", self) || named;
				out.push_back(std::move(each));
			}
			list = std::move(out);
			return named;
		}
	}

	// What a <condition> tests, made ready (with its <all> or
	// <any>'s checks, each made the same way).
	Condition Evaluator::makeTest(const WrittenCondition& writtenCondition, const std::string& where, const std::string& owner, const std::vector<Object>& objects)
	{
		return onLine(writtenCondition.line, [&] { return makeTestHere(writtenCondition, where, owner, objects); });
	}

	Condition Evaluator::makeTestHere(const WrittenCondition& writtenCondition, const std::string& where, const std::string& owner, const std::vector<Object>& objects)
	{
		Condition condition;
		condition.line = writtenCondition.line;
		condition.join = writtenCondition.join;
		for (std::size_t i = 0; i < writtenCondition.parts.size(); ++i)
		{
			const WrittenCondition& part = writtenCondition.parts[i];
			const char* kind = part.join == WrittenCondition::Join::All ? "<all>" : part.join == WrittenCondition::Join::Any ? "<any>" : "<condition>";
			condition.parts.push_back(makeTest(part, where + " > " + kind + " " + std::to_string(i + 1), owner, objects));
		}
		if (condition.join != WrittenCondition::Join::None)
		{
			return condition;
		}

		// One about self is made, and checked, for the object being made;
		// as it is tested, it is about the object testing it.
		// The same for other, in a rule about touching: the one touched.
		const bool own = !selfName.empty() && (writtenCondition.filterObject == "self" || writtenCondition.distance == "self" || writtenCondition.sees == "self"
			|| saysSelf(writtenCondition.threshold));
		const bool theirs = !otherName.empty() && namesOther(writtenCondition);
		if (own || theirs)
		{
			WrittenCondition named = writtenCondition;
			if (own) { nameSelfInTest(named, selfName); }
			if (theirs) { nameOtherInTest(named, otherName); }
			Condition made = makeTestHere(named, where, owner, objects);
			made.saysSelf = true;
			if (own) { made.madeFor = selfName; }
			if (theirs) { made.madeAgainst = otherName; }
			for (const char* name : { "self", "other" })
			{
				if (writtenCondition.filterObject == name) { made.filterObject = name; }
				if (writtenCondition.distance == name) { made.distanceTo = name; }
				if (writtenCondition.sees == name) { made.sees = name; }
			}
			if (theirs && writtenCondition.variableName.compare(0, 6, "other.") == 0)
			{
				throw std::runtime_error(where + ": variable=\"" + writtenCondition.variableName + "\": say whose with object=\"other\" and variable=\"" + writtenCondition.variableName.substr(6) + "\"");
			}
			if (saysSelf(writtenCondition.threshold) || (theirs && namesLive(writtenCondition.threshold)))
			{
				made.live = std::make_shared<const WrittenValue>(writtenCondition.threshold);
				made.where = where;
				// What it reads of itself, by the names its variables have.
				const auto sameName = [](const std::string& a, const std::string& b)
				{
					return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(),
						[](char x, char y) { return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y)); });
				};
				made.reads.erase(std::remove_if(made.reads.begin(), made.reads.end(), [&](const Condition::Read& read)
					{
						return (own && sameName(read.owner, selfName)) || (theirs && sameName(read.owner, otherName));
					}), made.reads.end());
				const auto reads = [&](const std::string& prefix, const std::string& whose, std::vector<Name>& into)
				{
					if (whose.empty()) { return; }
					for (const std::string* text : writtenCondition.threshold.expressions())
					{
						for (const std::string& name : namesIn(*text))
						{
							if (name.compare(0, prefix.size(), prefix) != 0) { continue; }
							const std::string wanted = whose + "." + name.substr(prefix.size());
							for (const auto& [key, slot] : objectVariables)
							{
								if (sameName(key, wanted)) { into.push_back(key.substr(whose.size() + 1)); break; }
							}
						}
					}
				};
				reads("self.", own ? selfName : std::string(), made.selfReads);
				reads("other.", theirs ? otherName : std::string(), made.otherReads);
			}
			return made;
		}

		condition.filterClass = writtenCondition.filterClass;
		condition.filterObject = writtenCondition.filterObject;

		// What it looks at is something the game has: a misspelt class or
		// object would match nothing, and the condition would quietly
		// never hold (or always, for an <atmost>).
		if (!condition.filterClass.empty() && std::none_of(objects.begin(), objects.end(), [&](const Object& object) { return object.objClass == condition.filterClass; }))
		{
			std::set<std::string> classes;
			for (const Object& object : objects) { if (!object.objClass.empty()) { classes.insert(object.objClass); } }
			const std::vector<std::string> names(classes.begin(), classes.end());
			throw std::runtime_error(where + ": class=\"" + condition.filterClass + "\": no object has that class" + didYouMean(condition.filterClass, names)
				+ (names.empty() ? std::string("; no object has a class") : ". The classes are: " + listOf(names)));
		}
		if (!condition.filterObject.empty() && !findObject(objects, condition.filterObject))
		{
			throw std::runtime_error(where + ": object=\"" + condition.filterObject + "\" names no object or group" + didYouMean(condition.filterObject, objectNames(objects)));
		}
		condition.variableName = writtenCondition.variableName;
		condition.distanceTo = writtenCondition.distance;
		condition.sprite = writtenCondition.sprite;
		condition.sees = writtenCondition.sees;
		condition.past = NameStore::program().of(writtenCondition.past);
		if (writtenCondition.total == "count") { condition.total = Total::Count; }
		else if (writtenCondition.total == "sum") { condition.total = Total::Sum; }
		else if (writtenCondition.total == "highest") { condition.total = Total::Highest; }
		else if (writtenCondition.total == "lowest") { condition.total = Total::Lowest; }

		// A number that names something that changes is worked out again
		// each time the condition is tested (Game::testCondition); now, it
		// may read where things are as 0, as a command's values may.
		const bool shows = writtenCondition.test == WrittenCondition::Test::Shows || writtenCondition.test == WrittenCondition::Test::Lowest || writtenCondition.test == WrittenCondition::Test::Highest
			|| writtenCondition.test == WrittenCondition::Test::Sees;
		const bool live = !shows && namesLive(writtenCondition.threshold);
		commandValues = live;
		const float threshold = shows ? 0.0f : evaluate(writtenCondition.threshold, where);
		commandValues = false;
		if (live)
		{
			condition.live = std::make_shared<const WrittenValue>(writtenCondition.threshold);
			condition.where = where;
			const auto sameName = [](const std::string& a, const std::string& b)
			{
				return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(),
					[](char x, char y) { return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y)); });
			};
			for (const std::string* text : writtenCondition.threshold.expressions())
			{
				for (const std::string& name : namesIn(*text))
				{
					for (auto& [key, slot] : objectVariables)
					{
						if (!sameName(key, name)) { continue; }
						const std::size_t dot = key.rfind('.');
						condition.reads.push_back({ key.substr(0, dot), key.substr(dot + 1), &slot });
						break;
					}
				}
			}
		}
		switch (writtenCondition.test)
		{
		case WrittenCondition::Test::AtLeast:   condition.compare = Compare::AtLeast; break;
		case WrittenCondition::Test::AtMost:    condition.compare = Compare::AtMost; break;
		case WrittenCondition::Test::Above:     condition.compare = Compare::Above; break;
		case WrittenCondition::Test::Below:     condition.compare = Compare::Below; break;
		case WrittenCondition::Test::Equal:     condition.compare = Compare::Equal; break;
		case WrittenCondition::Test::Unequal:   condition.compare = Compare::Unequal; break;
		case WrittenCondition::Test::Remaining: condition.remaining = threshold; break;
		case WrittenCondition::Test::Shows:     condition.compare = Compare::Shows; break;
		case WrittenCondition::Test::Lowest:    condition.compare = Compare::Lowest; break;
		case WrittenCondition::Test::Highest:   condition.compare = Compare::Highest; break;
		case WrittenCondition::Test::Sees:      condition.compare = Compare::Equal; break;
		}
		condition.value = writtenCondition.test == WrittenCondition::Test::Sees ? 1.0f : threshold;

		checkTestNames(writtenCondition, condition, where, owner, objects);
		return condition;
	}

	// makeTestHere's checks: what a test names (what it sees and what is in
	// the way, a look, a distance, a variable) is something the game has.
	void Evaluator::checkTestNames(const WrittenCondition& writtenCondition, const Condition& condition, const std::string& where, const std::string& owner, const std::vector<Object>& objects)
	{
		// <sees>: what is looked for is there, and so is each class in the way.
		if (!condition.sees.empty() && !findObject(objects, condition.sees))
		{
			throw std::runtime_error(where + ": <sees object=\"" + condition.sees + "\"> names no object or group" + didYouMean(condition.sees, objectNames(objects)));
		}
		for (const std::string& past : condition.past)
		{
			if (std::none_of(objects.begin(), objects.end(), [&](const Object& object) { return object.objClass == past; }))
			{
				std::set<std::string> classes;
				for (const Object& object : objects) { if (!object.objClass.empty()) { classes.insert(object.objClass); } }
				throw std::runtime_error(where + ": <past class=\"" + past + "\">: no object has that class" + didYouMean(past, std::vector<std::string>(classes.begin(), classes.end())));
			}
		}

		// A look by name: something it looks at has a look of that name.
		if (!condition.sprite.empty())
		{
			std::vector<std::string> looks;
			for (const Object& object : objects)
			{
				if (!matchesFilter(writtenCondition, object)) { continue; }
				for (const auto& look : object.looks.all()) { looks.push_back(look.name); }
			}
			if (std::find(looks.begin(), looks.end(), condition.sprite) == looks.end())
			{
				throw std::runtime_error(where + ": sprite=\"" + condition.sprite + "\" is no look of what the condition looks at" + didYouMean(condition.sprite, looks));
			}
		}

		if (!condition.distanceTo.empty() && !findObject(objects, condition.distanceTo))
		{
			throw std::runtime_error(where + ": distance=\"" + condition.distanceTo + "\" names no object or group" + didYouMean(condition.distanceTo, objectNames(objects)));
		}

		// A total of a variable: some object it can count has it.
		if ((condition.total == Total::Sum || condition.total == Total::Highest || condition.total == Total::Lowest)
			&& condition.distanceTo.empty()
			&& std::none_of(objects.begin(), objects.end(), [&](const Object& object) { return matchesFilter(writtenCondition, object) && numberOf(*parts, object, condition.variableName); }))
		{
			throw std::runtime_error(where + ": total=\"" + writtenCondition.total + "\" of variable=\"" + condition.variableName + "\", and no object it looks at has a variable of that name");
		}

		// One object's variable: something it looks at has it, as a
		// variable of its own or as a number the engine keeps (x,
		// moves, ...). A misspelt one was never there to compare.
		if (condition.total == Total::None && !condition.remaining && condition.distanceTo.empty() && !condition.variableName.empty()
			&& std::none_of(objects.begin(), objects.end(), [&](const Object& object) { return matchesFilter(writtenCondition, object) && numberOf(*parts, object, condition.variableName); }))
		{
			std::set<std::string> own;
			for (const Object& object : objects)
			{
				if (!matchesFilter(writtenCondition, object)) { continue; }
				for (const auto& [name, value] : object.variable) { own.insert(name); }
			}
			std::vector<std::string> names(own.begin(), own.end());
			std::vector<std::string> all = names;
			all.insert(all.end(), livePartNames().begin(), livePartNames().end());
			throw std::runtime_error(where + ": variable=\"" + condition.variableName + "\": nothing the condition looks at has a variable or number of that name"
				+ didYouMean(condition.variableName, all) + (names.empty() ? std::string("; it has no <variables>") : "; its variables are: " + listOf(names)));
		}

		if ((condition.remaining || condition.total == Total::Count) && std::none_of(objects.begin(), objects.end(), [&](const Object& object) { return matchesFilter(writtenCondition, object); }))
		{
			std::cout << "warning: " << owner << (condition.remaining ? ": a condition with remaining= matches no object at all, so it would fire at once\n"
				: ": a condition with total=\"count\" matches no object at all, so it counts 0\n");
		}
	}

	// A state's condition: its test, and its commands; or, when they name
	// self or others, its commands for each object it may match (the
	// last test of an <all>'s, as a condition in a condition is), and
	// they run for each one that holds.
	Condition Evaluator::makeConditionHere(const WrittenCondition& writtenCondition, const std::string& where, const std::string& owner, const std::string& stateName, const std::vector<Object>& objects)
	{
		Condition condition = makeTest(writtenCondition, where, owner, objects);
		std::vector<WrittenCommand> probe = writtenCondition.commands;
		if (!forMatched(probe, "self_", { "others_" }))
		{
			condition.commands = processCommands(writtenCondition.commands, where);
			return condition;
		}
		const WrittenCondition* test = &writtenCondition;
		while (test->join == WrittenCondition::Join::All && !test->parts.empty()) { test = &test->parts.back(); }
		if (test->join != WrittenCondition::Join::None || (test->filterClass.empty() && test->filterObject.empty()))
		{
			throw std::runtime_error(where + ": its commands name self or others, so its test (or the last of its <all>) needs class= or object= to say what they are among");
		}
		if (test->total != "" || test->test == WrittenCondition::Test::Remaining)
		{
			throw std::runtime_error(where + ": its commands name self or others, but its test is a total of everything matched, so no one object holds it");
		}
		std::vector<std::string> matched;
		for (const Object& object : objects)
		{
			if (matchesFilter(*test, object) && (object.ownerState.empty() || object.ownerState == stateName)) { matched.push_back(object.name); }
		}
		// What names self or others runs for each one it holds for; the rest once, after.
		std::vector<WrittenCommand> eachOne;
		std::vector<WrittenCommand> once;
		for (const WrittenCommand& command : writtenCondition.commands)
		{
			std::vector<WrittenCommand> one{ command };
			(forMatched(one, "self_", { "others_" }) ? eachOne : once).push_back(command);
		}
		for (const std::string& self : matched)
		{
			std::vector<std::string> others;
			for (const std::string& other : matched) { if (other != self) { others.push_back(other); } }
			std::vector<WrittenCommand> commands = eachOne;
			forMatched(commands, self, others);
			condition.each.push_back({ self, processCommands(commands, where) });
		}
		condition.commands = processCommands(once, where);
		return condition;
	}
	Condition Evaluator::makeCondition(const WrittenCondition& writtenCondition, const std::string& where, const std::string& owner, const std::string& stateName, const std::vector<Object>& objects)
	{
		return onLine(writtenCondition.line, [&] { return makeConditionHere(writtenCondition, where, owner, stateName, objects); });
	}
}
