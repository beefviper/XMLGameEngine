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

	// What a verb's check is given (Verb::check, verbs/verb.h): the game as
	// it was built, and where the command is, for a message.
	struct Checking
	{
		const std::vector<State>& states;
		const std::vector<Object>& objects;
		const std::vector<SoundDesc>& sounds;
		const std::map<std::string, Path>& paths;
		const std::vector<RawStyle>& styles;
		WantedPalettes& wanted;
		const std::string& where;
	};

	// What the verbs' checks share. Each throws std::runtime_error saying
	// where, what was written and what there is instead.
	// `what` (a tag, as "<chase>") names an object, or an object or group.
	void checkObject(const Checking& checking, const std::string& name, const std::string& what, const char* kind = "object");
	// `what` (as "<push state=\"menu\" />") names a state.
	void checkState(const Checking& checking, const std::string& name, const std::string& what);
	// <play> or <silence> names a sound.
	void checkSound(const Checking& checking, const char* verb, const std::string& name);
	// <enter> or <reset> names a scene, or a way some scene has.
	void checkScene(const Checking& checking, const char* verb, const std::string& scene, const std::string& way);
	// A map of cells a command names (a spawn's or a send's into=).
	void checkCellMap(const Checking& checking, const std::string& map, const std::string& tag);
	// The variable a command names: owner.variable, the variable after the
	// last dot (the owner may be a path).
	void checkVariable(const Checking& checking, const VariableName& target, const std::string& tag);
	// A command writing what its object follows (update="live"): it would
	// be undone at the end of the frame, so it is a mistake.
	void refuseFollowed(const Checking& checking, const VariableName& target, const std::string& tag);
	// A <become style>: the style exists, and the object is to be given its
	// palette.
	void wantPalette(const Checking& checking, const Object& object, const std::string& style);

	// Every command of every state, scene, object and path checked for the
	// names it uses and for being in a place where it does something
	// (the places of its verb, verbs/verb.h); hands back the palettes the <become style>s ask for.
	WantedPalettes checkReferences(const std::vector<State>& states, const std::vector<Object>& objects, const std::vector<SoundDesc>& sounds,
		const std::map<std::string, Path>& paths, const std::vector<RawStyle>& styles);
}
