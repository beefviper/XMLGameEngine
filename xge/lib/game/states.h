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
	// from= too, and the top-level <objects> pieces it takes, all as load/merge made them),
	// and what runs while it is its state's current scene.
	struct WrittenScene
	{
		std::string name;
		std::string from;
		std::vector<std::pair<std::string, std::string>> ways;
		std::vector<std::pair<std::string, WrittenValue>> variables;
		std::vector<WrittenObject> objects;
		std::vector<WrittenCondition> conditions;
		std::vector<WrittenTimer> timers;
	};

	// The moments of a key an <input> can answer, as on= names them:
	// "press" (left out: the same), "hold", "release". In the order of
	// WrittenState::keyMaps and State::keyMaps.
	inline constexpr std::array<const char*, 3> keyMoments{ "press", "hold", "release" };

	// A step of a state's <starts>, as written: <send from into> (N, or
	// <all />), <order group> (<random />), <distribute from into>N<more>.
	struct WrittenStart
	{
		enum class Step { Send, Distribute, Order };
		Step step{ Step::Send };
		const char* tag() const noexcept { return step == Step::Send ? "send" : step == Step::Distribute ? "distribute" : "order"; }
		std::string from;      // send, distribute; order: the group ordered
		std::string into;
		int count{};
		int more{};
		bool all{ false };
	};

	class WrittenState
	{
	public:
		std::string name;
		int line{ 0 }; // of its tag in the game file; 0 when not known (a generated program)
		std::vector<std::string> show;
		// <input button="...">: what a key runs when pressed (and, for the
		// verbs that are held, for as long as it stays down); with
		// on="hold", every frame while it is down; with on="release", once
		// when it is let go.
		std::map<std::string, std::vector<WrittenCommand>> input;
		std::map<std::string, std::vector<WrittenCommand>> holding;
		std::map<std::string, std::vector<WrittenCommand>> letGo;
		std::map<std::string, std::vector<WrittenCommand>> axes;  // <input axis="mouse.x">
		// <seat number="2">: the seats a button or a pointing control is in
		// (none: everyone's).
		std::map<std::string, std::vector<int>> seats;
		std::map<std::string, std::vector<int>> axisSeats;

		// The three, in the order of keyMoments.
		std::array<std::map<std::string, std::vector<WrittenCommand>>*, 3> keyMaps() { return { &input, &holding, &letGo }; }
		std::array<const std::map<std::string, std::vector<WrittenCommand>>*, 3> keyMaps() const { return { &input, &holding, &letGo }; }
		std::vector<WrittenCondition> conditions;
		std::vector<WrittenTimer> timers;

		// <computer>: conditions whose commands are only <hold>s, every one
		// tested every frame; see State::computer.
		std::vector<WrittenCondition> computer;
		int computerSeat{ 0 }; // <computer seat="2">; 0: it plays always

		// <starts>: what is done when it begins and on a full reset, where
		// each held member starts (placed by load/expr.cpp).
		std::vector<WrittenStart> starts;

		// The states it draws still underneath its own (<show state="..." />).
		std::vector<std::string> showStates;

		// <music sound="..." />: its own sound ("" for none).
		std::string music;

		// Its own <objects>, in play in every scene of it, as read (names
		// as written; with those of the state it is from=, and the top-level <objects> pieces
		// it takes, all as load/merge made them); then its scenes.
		// game/scenes.h lays them out among the game's objects.
		std::vector<WrittenObject> objects;
		std::vector<WrittenScene> scenes;

		// from="...": an earlier state it is a copy of (see takeStates); its
		// objects are copied already. Empty when it is a state of its own.
		std::string from;

		friend std::ostream& operator<<(std::ostream& o, WrittenState const& f);

	private:

	};

	// A scene, made ready to run: its path (playing.room1), and the
	// conditions and timers that run while it is its state's current scene,
	// after the state's own.
	struct Scene
	{
		Name name;
		std::map<Name, Name> ways; // a way's name: the path of the scene it leads to
		std::vector<Condition> conditions;
		std::vector<Timer> timers;
	};

	class State
	{
	public:
		Name name;
		int line{ 0 }; // WrittenState::line, for the messages of the checks made once all is loaded
		std::vector<Name> show;
		std::map<KeyCode, std::vector<Command>> input;    // pressed (WrittenState::input)
		std::map<KeyCode, std::vector<Command>> holding;  // on="hold": every frame while down
		std::map<KeyCode, std::vector<Command>> letGo;    // on="release": once, let go
		std::map<Axis, std::vector<Command>> axes;  // <input axis="mouse.x">: run as the control moves
		// The seats a key or a pointing control is in (WrittenState::seats):
		// only that seat's player, a person or the computer, presses it.
		std::map<KeyCode, std::vector<int>> seats;
		std::map<Axis, std::vector<int>> axisSeats;

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
		// The seat it plays (<computer seat="2">), only while no person has
		// it; 0 for one that always plays.
		int computerSeat{ 0 };

		// The states drawn still underneath it (WrittenState::showStates).
		std::vector<Name> showStates;

		// Its music (<music sound="..." />, "" for none): a loop plays while
		// this is the current state, a sound that does not loop plays as it
		// begins (Game::pushState).
		Name music;

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
