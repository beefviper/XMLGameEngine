// players.cpp
// XML Game Engine
// author: beefviper
// date: Nov 9, 2026
//
// Players as a group: the members of a group with <turns /> take turns in
// the order written. Keys reach only the member on its turn, and only when
// a person plays it; a member with a <think> is played by the computer,
// which presses the same keys for it. <pass /> hands the turn on.

#include "game/players.h"
#include "game/data.h"
#include "game/game.h"
#include "game/words.h"

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>

namespace xge
{
	void settlePlayers(GameData& data)
	{
		std::map<std::string, int> count;
		for (WrittenObject& object : data.objects)
		{
			if (!object.takesTurns)
			{
				if (!object.think.empty()) { throw std::runtime_error("object '" + object.name + "': has a <think>, which plays a member of a group that takes <turns>"); }
				continue;
			}
			const int number = ++count[object.groupName];
			const bool first = number == 1;
			// Its seat at a trick is its number.
			if (object.handSeat == 0) { object.handSeat = number; }
			object.variable["turn"] = WrittenValue::expression(first ? "1" : "0");
			if (!object.turnsSprite.empty() && std::none_of(object.looks.begin(), object.looks.end(), [&](const WrittenSprite& look) { return look.name == object.turnsSprite; }))
			{
				throw std::runtime_error("object '" + object.name + "': its group's <turns sprite=\"" + object.turnsSprite + "\">, a look it has not");
			}
			for (WrittenCommand& think : object.think) { think.object = object.name; }
		}
	}

	void Players::startPlayers()
	{
		std::set<std::string> started;
		for (Object& object : game.getCurrentObjects())
		{
			if (!game.turnsOf(object).takes) { continue; }
			anyTurns = true;
			const bool first = started.insert(object.groupName).second;
			object.variable[words::turn] = first ? 1.0f : 0.0f;
			game.variableChanged(object.name, words::turn, object.variable[words::turn]);
			if (first) { object.showLook(0); }
			else if (!game.turnsOf(object).sprite.empty()) { game.showLook(object, game.turnsOf(object).sprite); }
		}
	}

	Object* Players::onTurn(Name name)
	{
		const Name full = game.fullName(name);
		for (Object& object : game.getCurrentObjects())
		{
			if (game.turnsOf(object).takes && (object.inGroup(name) || object.inGroup(full)) && object.variable[words::turn] != 0.0f) { return &object; }
		}
		return nullptr;
	}

	bool Players::takesTurns(Name name) const
	{
		const Name full = game.fullName(name);
		return std::any_of(game.getCurrentObjects().begin(), game.getCurrentObjects().end(), [&](const Object& object) { return game.turnsOf(object).takes && (object.inGroup(name) || object.inGroup(full)); });
	}

	bool Players::answers(const Object& member) const
	{
		if (!game.turnsOf(member).takes) { return true; }
		const auto onIt = member.variable.find(words::turn);
		if (onIt == member.variable.end() || onIt->second == 0.0f) { return false; }
		// A member the computer thinks for answers only its presses; any other
		// answers whoever presses (a member played by <computer> conditions).
		return memberThinks.count(member.name) == 0 || computerPresses;
	}

	bool Players::passPlayers(Object& member)
	{
		if (!game.turnsOf(member).takes || member.variable[words::turn] == 0.0f) { return false; }
		// Not while a holder waits for the call its wild card asks of the
		// one who laid it.
		for (Object* holder : game.holders().shownHolders())
		{
			if (game.holderOf(*holder).calling > 0.0f) { return false; }
		}
		// The next member of the group, round to the first.
		std::vector<Object*> members;
		for (Object& object : game.getCurrentObjects())
		{
			if (game.turnsOf(object).takes && object.groupName == member.groupName && object.exists) { members.push_back(&object); }
		}
		const auto at = std::find(members.begin(), members.end(), &member);
		Object* next = members[static_cast<std::size_t>(at - members.begin() + 1) % members.size()];
		if (next == &member) { return true; }
		handTurn(members, next);
		// What else the press would do is not the next one's.
		game.stopCommands();
		return true;
	}

	bool Players::passPlayersTo(Name group, int number)
	{
		const Name full = game.fullName(group);
		std::vector<Object*> members;
		for (Object& object : game.getCurrentObjects())
		{
			if (game.turnsOf(object).takes && (object.inGroup(group) || object.inGroup(full)) && object.exists) { members.push_back(&object); }
		}
		if (members.empty() || number > static_cast<int>(members.size())) { return false; }
		handTurn(members, number == 0 ? nullptr : members[static_cast<std::size_t>(number - 1)]);
		return true;
	}

	// The turn to `next` (none: nobody's), the others off it.
	void Players::handTurn(const std::vector<Object*>& members, Object* next)
	{
		for (Object* each : members)
		{
			const bool on = each == next;
			if (on == (each->variable[words::turn] != 0.0f)) { continue; }
			game.setVariable(each->name, words::turn, on ? 1.0f : 0.0f);
			// Onto its turn it leaves its off-turn look, for its first; a look
			// given it while it waited (a pass's "again") stays.
			if (on && (game.turnsOf(*each).sprite.empty() || each->lookName() == game.turnsOf(*each).sprite)) { each->showLook(0); }
			else if (!on && !game.turnsOf(*each).sprite.empty()) { game.showLook(*each, game.turnsOf(*each).sprite); }
		}
		// A moment before the computer plays the one whose turn came, and the
		// game's conditions see the turn change first.
		game.holders().itemWait = Holders::kItemPace;
		game.holders().layOutItems();
	}

	int Players::seatOnTurn() const
	{
		int seat = -1;
		for (const Object& object : game.getCurrentObjects())
		{
			if (!game.turnsOf(object).takes || !object.exists) { continue; }
			seat = 0;
			const auto onIt = object.variable.find(words::turn);
			if (onIt != object.variable.end() && onIt->second != 0.0f) { return game.turnsOf(object).seat; }
		}
		return seat;
	}

	Name Players::turnClass()
	{
		for (const Object& object : game.getCurrentObjects())
		{
			if (!game.turnsOf(object).takes || game.turnsOf(object).ownsClass.empty() || !object.exists || !game.isShown(object)) { continue; }
			const auto onIt = object.variable.find(words::turn);
			if (onIt != object.variable.end() && onIt->second != 0.0f) { return game.turnsOf(object).ownsClass; }
		}
		return {};
	}

	Name Players::turnOwner(Name owner)
	{
		if (!anyTurns) { return owner; }
		static const std::string ending = "." + words::current.str();
		const std::string& written = owner.str();
		if (written.size() <= ending.size() || written.compare(written.size() - ending.size(), ending.size(), ending) != 0) { return owner; }
		const Object* on = onTurn(Name::of(std::string_view(written).substr(0, written.size() - ending.size())));
		return on ? on->name : owner;
	}
}
