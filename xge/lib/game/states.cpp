// states.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "game/states.h"

#include <sstream>

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
		case Compare::Unequal: return variable != against;
		case Compare::Shows:   return true;
		case Compare::Lowest:  return false; // one of many: Game::holds
		case Compare::Highest: return false;
		}
		return false;
	}

	std::vector<RawCondition> flattened(const std::vector<RawCondition>& conditions)
	{
		std::vector<RawCondition> out;
		for (const RawCondition& condition : conditions)
		{
			if (condition.inner.empty())
			{
				out.push_back(condition);
				continue;
			}
			RawCondition test = condition;
			test.inner.clear();
			test.first.clear();
			test.commands.clear();
			test.name.clear();
			for (RawCondition& each : flattened(condition.inner))
			{
				RawCondition both;
				both.join = RawCondition::Join::All;
				both.commands = condition.first;
				both.commands.insert(both.commands.end(), each.commands.begin(), each.commands.end());
				each.commands.clear();
				each.name.clear();
				both.parts = { test, std::move(each) };
				out.push_back(std::move(both));
			}
			if (!condition.first.empty() || !condition.commands.empty())
			{
				RawCondition own = condition;
				own.inner.clear();
				own.first.clear();
				own.commands = condition.first;
				own.commands.insert(own.commands.end(), condition.commands.begin(), condition.commands.end());
				out.push_back(std::move(own));
			}
		}
		return out;
	}

	std::string describe(const Condition& condition)
	{
		std::ostringstream text;
		if (condition.join != RawCondition::Join::None)
		{
			text << (condition.join == RawCondition::Join::All ? "all of (" : "any of (");
			for (std::size_t i = 0; i < condition.parts.size(); ++i)
			{
				text << (i ? "; " : "") << describe(condition.parts[i]);
			}
			text << ")";
			return text.str();
		}

		std::string what = condition.filterObject.empty() ? condition.filterClass : condition.filterObject;
		if (condition.compare == Compare::Shows)
		{
			text << (what.empty() ? "anything" : what) << " shows " << condition.sprite;
			return text.str();
		}
		if (!condition.sprite.empty()) { what += " showing " + condition.sprite; }
		if (condition.remaining)
		{
			text << "remaining of " << (what.empty() ? "anything" : what) << " <= " << *condition.remaining;
			return text.str();
		}

		static const char* const signs[] = { " >= ", " <= ", " > ", " < ", " = ", "", " lowest", " highest", " != " };
		static const char* const totals[] = { "", "count of ", "sum of ", "highest of ", "lowest of " };
		text << totals[static_cast<std::size_t>(condition.total)];
		if (!condition.distanceTo.empty())
		{
			text << "distance from " << (what.empty() ? "anything" : what) << " to " << condition.distanceTo;
		}
		else if (condition.total == Total::Count)
		{
			text << (what.empty() ? "anything" : what);
		}
		else
		{
			text << (what.empty() ? "" : what + ".") << condition.variableName;
		}
		text << signs[static_cast<std::size_t>(condition.compare)];
		if (condition.compare == Compare::Lowest || condition.compare == Compare::Highest) { return text.str(); }
		if (condition.live) { text << valueText(*condition.live); }
		else { text << condition.value; }
		if (!condition.filterClass.empty() && !condition.filterObject.empty()) { text << " (class " << condition.filterClass << ")"; }
		return text.str();
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
		for (std::size_t moment = 0; moment < keyMoments.size(); ++moment)
		{
			for (auto& input : *f.keyMaps()[moment])
			{
				o << "       button=" << input.first << (moment == 0 ? "" : std::string(" on ") + keyMoments[moment]) << ", action=" << input.second << '\n';
			}
		}
		for (auto& input : f.axes)
		{
			o << "       axis=" << input.first << ", action=" << input.second << '\n';
		}

		for (auto& condition : f.conditions)
		{
			o << "       condition: ";
			static const char* const tests[] = { "atleast", "atmost", "remaining", "above", "below", "equal", "shows", "lowest", "highest", "unequal" };
			const char* test = tests[static_cast<std::size_t>(condition.test)];
			if (condition.join != RawCondition::Join::None)
			{
				o << (condition.join == RawCondition::Join::All ? "all" : "any") << " of " << condition.parts.size() << " checks";
			}
			else if (condition.test == RawCondition::Test::Remaining)
			{
				o << "remaining=" << condition.threshold;
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
		for (std::size_t moment = 0; moment < keyMoments.size(); ++moment)
		{
			for (auto& input : *f.keyMaps()[moment])
			{
				o << "       button=" << keyCodeToString(input.first) << (moment == 0 ? "" : std::string(" on ") + keyMoments[moment]) << ", action=";
				for (auto& command : input.second)
				{
					o << command << (&command != &input.second.back() ? ";" : "");
				}
				o << '\n';
			}
		}
		for (auto& input : f.axes)
		{
			o << "       axis=" << axisToString(input.first) << ", action=";
			for (auto& command : input.second)
			{
				o << command << (&command != &input.second.back() ? ";" : "");
			}
			o << '\n';
		}

		for (auto& condition : f.conditions)
		{
			o << "       condition: " << describe(condition) << ", action=";
			for (auto& command : condition.commands)
			{
				o << command << (&command != &condition.commands.back() ? ";" : "");
			}
			o << '\n';
		}

		for (auto& condition : f.computer)
		{
			o << "       computer: " << describe(condition) << ", holds=";
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
