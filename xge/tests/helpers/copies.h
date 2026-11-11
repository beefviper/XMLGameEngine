// copies.h
// XML Game Engine
// author: beefviper
// date: Nov 11, 2026
//
// Copies of a piece (an object of the top-level <objects>), for tests that
// put a shot in by hand rather than through a key: one made and joined to the
// game at once, as a <fire> or <spawn> would make it and the frame's end
// would join it, and how many there are.

#pragma once

#include "game/game.h"

#include <stdexcept>
#include <string>

namespace copies
{
	// Whether `name` is `piece`, or ends with it after a dot (playing.bullet).
	inline bool calls(const std::string& name, const std::string& piece)
	{
		return name == piece || (name.size() > piece.size() && name.compare(name.size() - piece.size(), piece.size(), piece) == 0 && name[name.size() - piece.size() - 1] == '.');
	}

	// A new copy of `piece`, in the game now: of the one the current state
	// has (a piece a state uses is put in it, as playing.bullet), as a
	// <fire> there makes. Throws when its <most> are out.
	inline xge::Object& spawned(xge::Game& game, const std::string& piece)
	{
		std::string from = piece;
		for (const xge::Object& object : game.getCurrentObjects())
		{
			if (!object.start.exists && !object.isCopy() && calls(object.name, piece) && object.ownerState == game.getCurrentState().name) { from = object.name; break; }
		}
		const xge::Object* made = game.spawn(from);
		if (!made) { throw std::runtime_error("no more of '" + piece + "' may be spawned"); }
		const std::string name = made->name;
		game.settleSpawns();
		return game.getObject(name);
	}

	// How many copies of `piece` are in the game (its name, or the end of
	// its path: bullet for playing.bullet).
	inline int count(xge::Game& game, const std::string& piece)
	{
		int found = 0;
		for (const xge::Object& object : game.getCurrentObjects())
		{
			if (object.exists && object.isCopy() && calls(object.copyOf, piece)) { ++found; }
		}
		return found;
	}
}
