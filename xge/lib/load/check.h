// check.h
// XML Game Engine
// author: beefviper
// date: Oct 29, 2026

#pragma once

#include "audio/sound.h"
#include "game/command.h"
#include "game/object.h"
#include "game/states.h"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace xge
{
	// The load-time checks of every name a game file uses, once its objects,
	// states, sounds and paths are built: what the schema cannot see. Each
	// stops the load with what to write instead (see load/spelling.h).

	// The object a command names: its exact name (aliens.3.2), the name
	// from the file (aliens), or the name of its <group> - the same lookup
	// Game::tryGetObject does while the game runs.
	const Object* findObject(const std::vector<Object>& objects, const std::string& name);

	// Every name an object can be called by (its own and its group's),
	// for a message about one the game does not have.
	std::vector<std::string> objectNames(const std::vector<Object>& objects);

	// A color is one of the color. names; anything else would draw nothing.
	void checkColor(const std::string& name, const std::string& where);

	// Which objects can fly each path: those a <follow> anywhere sends
	// (itself, in an object's own commands, or every one of the name or
	// group it names), and those of a path whose step sends its flier on
	// along another.
	std::map<std::string, std::set<std::string>> followersOf(const std::vector<State>& states, const std::vector<Object>& objects,
		const std::map<std::string, Path>& paths);

	// The palettes the <become style>s can give each object, by its name,
	// each style once (see game_expr::makePalettes).
	using WantedPalettes = std::map<std::string, std::vector<std::string>>;

	// Every command of every state, scene, object and path checked for the
	// names it uses and for being in a place where it does something
	// (ruleFor); hands back the palettes the <become style>s ask for.
	WantedPalettes checkReferences(const std::vector<State>& states, const std::vector<Object>& objects, const std::vector<SoundDesc>& sounds,
		const std::map<std::string, Path>& paths, const std::vector<RawStyle>& styles);
}
