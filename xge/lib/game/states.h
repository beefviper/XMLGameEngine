// states.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include "game/command.h"
#include "engine/keys.h"
#include "game/object.h"

#include <array>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <ostream>

namespace xge
{
	// A <scene> as written: its name inside its state, the earlier scene it
	// is from= (or empty), its <ways> (each a name and the scene it leads to,
	// as written), its own <variables>, its objects (those of the scene it is
	// from= too, and the <kit> pieces it takes, all as load/merge made them),
	// and what runs while it is its state's current scene.
	struct RawScene
	{
		std::string name;
		std::string from;
		std::vector<std::pair<std::string, std::string>> ways;
		std::vector<std::pair<std::string, RawValue>> variables;
		std::vector<RawObject> objects;
		std::vector<RawCondition> conditions;
		std::vector<RawTimer> timers;
	};

	// The moments of a key an <input> can answer, as on= names them:
	// "press" (left out: the same), "hold", "release". In the order of
	// RawState::keyMaps and State::keyMaps.
	inline constexpr std::array<const char*, 3> keyMoments{ "press", "hold", "release" };

	class RawState
	{
	public:
		std::string name;
		std::vector<std::string> show;
		// <input button="...">: what a key runs when pressed (and, for the
		// verbs that are held, for as long as it stays down); with
		// on="hold", every frame while it is down; with on="release", once
		// when it is let go.
		std::map<std::string, std::vector<RawCommand>> input;
		std::map<std::string, std::vector<RawCommand>> holding;
		std::map<std::string, std::vector<RawCommand>> letGo;
		std::map<std::string, std::vector<RawCommand>> axes;  // <input axis="mouse.x">

		// The three, in the order of keyMoments.
		std::array<std::map<std::string, std::vector<RawCommand>>*, 3> keyMaps() { return { &input, &holding, &letGo }; }
		std::array<const std::map<std::string, std::vector<RawCommand>>*, 3> keyMaps() const { return { &input, &holding, &letGo }; }
		std::vector<RawCondition> conditions;
		std::vector<RawTimer> timers;

		// <computer>: conditions whose commands are only <hold>s, every one
		// tested every frame; see State::computer.
		std::vector<RawCondition> computer;

		// The states it draws still underneath its own (<show state="..." />).
		std::vector<std::string> showStates;

		// <music sound="..." />: its own sound ("" for none).
		std::string music;

		// Its own <objects>, in play in every scene of it, as read (names
		// as written; with those of the state it is from=, and the <kit> pieces
		// it takes, all as load/merge made them); then its scenes.
		// game/scenes.h lays them out among the game's objects.
		std::vector<RawObject> objects;
		std::vector<RawScene> scenes;

		// from="...": an earlier state it is a copy of (see takeStates); its
		// objects are copied already. Empty when it is a state of its own.
		std::string from;

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
		std::map<KeyCode, std::vector<Command>> input;    // pressed (RawState::input)
		std::map<KeyCode, std::vector<Command>> holding;  // on="hold": every frame while down
		std::map<KeyCode, std::vector<Command>> letGo;    // on="release": once, let go
		std::map<Axis, std::vector<Command>> axes;  // <input axis="mouse.x">: run as the control moves

		// The three, in the order of keyMoments.
		std::array<std::map<KeyCode, std::vector<Command>>*, 3> keyMaps() { return { &input, &holding, &letGo }; }
		std::array<const std::map<KeyCode, std::vector<Command>>*, 3> keyMaps() const { return { &input, &holding, &letGo }; }
		std::vector<Condition> conditions;

		// <computer>: a player the game plays itself. Every frame each of
		// these conditions is tested (all of them, not only the first that
		// holds), and the buttons the <hold>s of those that hold name are
		// held down, the rest let go, as keys a player presses: see
		// Game::computerButtons and Engine::step.
		std::vector<Condition> computer;

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
