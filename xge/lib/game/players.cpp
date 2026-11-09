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

#include <algorithm>
#include <set>
#include <stdexcept>

namespace xge
{
	void settlePlayers(GameData& data)
	{
		std::set<std::string> started;
		for (RawObject& object : data.objects)
		{
			if (!object.takesTurns)
			{
				if (!object.think.empty()) { throw std::runtime_error("object '" + object.name + "': has a <think>, which plays a member of a group that takes <turns>"); }
				continue;
			}
			const bool first = started.insert(object.groupName).second;
			object.variable["turn"] = RawValue::expression(first ? "1" : "0");
			if (!object.turnsSprite.empty() && std::none_of(object.looks.begin(), object.looks.end(), [&](const RawSprite& look) { return look.name == object.turnsSprite; }))
			{
				throw std::runtime_error("object '" + object.name + "': its group's <turns sprite=\"" + object.turnsSprite + "\">, a look it has not");
			}
			for (RawCommand& think : object.think) { think.object = object.name; }
		}
	}

	void Game::startPlayers()
	{
		std::set<std::string> started;
		for (Object& object : objects)
		{
			if (!object.takesTurns) { continue; }
			const bool first = started.insert(object.groupName).second;
			object.variable["turn"] = first ? 1.0f : 0.0f;
			refreshBoundTexts(object.name, "turn", object.variable["turn"]);
			if (first) { object.showLook(0); }
			else if (!object.turnsSprite.empty()) { showLookNamed(object, object.turnsSprite); }
		}
	}

	Object* Game::onTurn(const std::string& name)
	{
		const std::string full = fullName(name);
		for (Object& object : objects)
		{
			if (object.takesTurns && (object.inGroup(name) || object.inGroup(full)) && object.variable["turn"] != 0.0f) { return &object; }
		}
		return nullptr;
	}

	bool Game::takesTurns(const std::string& name) const
	{
		const std::string full = fullName(name);
		return std::any_of(objects.begin(), objects.end(), [&](const Object& object) { return object.takesTurns && (object.inGroup(name) || object.inGroup(full)); });
	}

	bool Game::answers(const Object& member) const
	{
		if (!member.takesTurns) { return true; }
		const auto onIt = member.variable.find("turn");
		if (onIt == member.variable.end() || onIt->second == 0.0f) { return false; }
		return memberThinks.count(member.name) > 0 ? computerPresses : !computerPresses;
	}

	bool Game::passPlayers(Object& member)
	{
		if (!member.takesTurns || member.variable["turn"] == 0.0f) { return false; }
		// Not while a holder waits for the call its wild card asks of the
		// one who laid it.
		for (Object* pile : shownPiles())
		{
			if (pile->calling > 0.0f) { return false; }
		}
		// The next member of the group, round to the first.
		std::vector<Object*> members;
		for (Object& object : objects)
		{
			if (object.takesTurns && object.groupName == member.groupName && object.exists) { members.push_back(&object); }
		}
		const auto at = std::find(members.begin(), members.end(), &member);
		Object* next = members[static_cast<std::size_t>(at - members.begin() + 1) % members.size()];
		if (next == &member) { return true; }
		setVariable(member.name, "turn", 0.0f);
		if (!member.turnsSprite.empty()) { showLookNamed(member, member.turnsSprite); }
		setVariable(next->name, "turn", 1.0f);
		next->showLook(0);
		// What else the press would do is not the next one's.
		turnHandedOver = true;
		layOutCards();
		return true;
	}
}
