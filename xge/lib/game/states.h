// states.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include "game/command.h"
#include "engine/keys.h"
#include "game/object.h"

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <ostream>

namespace xge
{
	// What a scene changes in a piece it takes (from its state's kit, or from
	// the scene it is from=): each part given replaces that part in every
	// object the piece is made of. A sprite replaces the look of its name (an
	// unnamed one, an object's only sprite).
	struct RawSceneChange
	{
		std::optional<RawValue> positionX;
		std::optional<RawValue> positionY;
		std::optional<RawValue> velocityX;
		std::optional<RawValue> velocityY;
		std::optional<bool> hidden;
		std::optional<bool> drawn;
		std::map<std::string, RawValue> variables;
		std::vector<RawSprite> sprites;
	};

	// One <object> or <group> in a scene's <objects>, in the order written:
	// a piece taken (`take` its name, and what it changes), or the scene's
	// own (`objects`, what it is made of: one object, or a group's members).
	struct RawSceneEntry
	{
		std::string take;
		RawSceneChange change;
		std::vector<RawObject> objects;
	};

	// A <scene> as written: its name inside its state, the earlier scene it
	// is from= (or empty), its <ways> (each a name and the scene it leads to,
	// as written), its own <variables>, its objects, and what runs while it
	// is its state's current scene.
	struct RawScene
	{
		std::string name;
		std::string from;
		std::vector<std::pair<std::string, std::string>> ways;
		std::vector<std::pair<std::string, RawValue>> variables;
		std::vector<RawSceneEntry> entries;
		std::vector<RawCondition> conditions;
		std::vector<RawTimer> timers;
	};

	class RawState
	{
	public:
		std::string name;
		std::vector<std::string> show;
		std::map<std::string, std::vector<RawCommand>> input;
		std::map<std::string, std::vector<RawCommand>> axes;  // <input axis="mouse.x">
		std::vector<RawCondition> conditions;
		std::vector<RawTimer> timers;

		// The states it draws still underneath its own (<show state="..." />).
		std::vector<std::string> showStates;

		// <music sound="..." />: its own sound ("" for none).
		std::string music;

		// Its own <objects>, in play in every scene of it, as read (names
		// as written); then the kit at the top of its <scenes>, and the scenes.
		// game/scenes.h lays them out among the game's objects.
		std::vector<RawObject> objects;
		std::vector<RawObject> kit;
		std::vector<RawScene> scenes;

		// from="...": an earlier state it is a copy of (see takeStates), and
		// what it changes in the objects it copies (each a take and its
		// change, as in a scene). Empty when it is a state of its own.
		std::string from;
		std::vector<RawSceneEntry> changes;

		friend std::ostream& operator<<(std::ostream& o, RawState const& f);

	private:

	};

	// A scene, made ready to run: its path (playing.room1), and the
	// conditions and timers that run while it is its state's current scene,
	// after the state's own.
	struct Scene
	{
		std::string name;
		std::map<std::string, std::string> ways; // a way's name: the path of the scene it leads to
		std::vector<Condition> conditions;
		std::vector<Timer> timers;
	};

	class State
	{
	public:
		std::string name;
		std::vector<std::string> show;
		std::map<KeyCode, std::vector<Command>> input;
		std::map<Axis, std::vector<Command>> axes;  // <input axis="mouse.x">: run as the control moves
		std::vector<Condition> conditions;

		// The states drawn still underneath it (RawState::showStates).
		std::vector<std::string> showStates;

		// Its music (<music sound="..." />, "" for none): a loop plays while
		// this is the current state, a sound that does not loop plays as it
		// begins (Game::pushState).
		std::string music;

		// Its scenes, in the order written; the first is current at the start.
		std::vector<Scene> scenes;

		// Counted while this is the current state (Game::updateTimers); the
		// counts themselves are kept by Game, per state, so a state pushed
		// again after a pause carries on where it was.
		std::vector<Timer> timers;

		friend std::ostream& operator<<(std::ostream& o, State const& f);

	private:

	};
}
