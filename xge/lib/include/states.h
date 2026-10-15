// states.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include "command.h"
#include "keycode.h"
#include "object.h"

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <ostream>

namespace xge
{
	// A <condition> checked every frame its state is current, told apart by
	// which test tag it holds. <atleast>: if any object matching
	// filterClass/filterObject (same optional-either-or-both filtering as a
	// collision rule) has a variable named `variableName` that has reached
	// that value, the commands fire. <atmost>: the same, when the variable has
	// fallen to that or below - lives 0 is a game over. <above>, <below> and
	// <equal>: more than, less than, the same as. Each of those five compares
	// with a number (the tag's content), or, with variable= on the tag, with
	// another variable, read every frame: object= on the tag names whose, and
	// without it it is the tested object's own (<above object="gun2"
	// variable="score" />). <remaining>: if no more than that many of the
	// matching objects are still in play (visible - die() hides an object),
	// the commands fire; remaining 0 is "they are all gone". Leaving both
	// filters empty matches any object, same as an unfiltered collision rule.
	struct RawCondition
	{
		enum class Test { AtLeast, AtMost, Remaining, Above, Below, Equal };

		std::string filterClass;
		std::string filterObject;
		std::string variableName;
		Test test{ Test::AtLeast };
		RawValue threshold;      // when otherVariable is empty
		std::string otherObject;  // the test tag's object=, or empty
		std::string otherVariable; // the test tag's variable=, or empty
		std::vector<RawCommand> commands;
	};

	// How a condition's variable is compared, and whether `variable` passes
	// against `against` that way.
	enum class Compare { AtLeast, AtMost, Above, Below, Equal };
	bool passes(Compare compare, float variable, float against) noexcept;

	struct Condition
	{
		std::string filterClass;
		std::string filterObject;
		std::string variableName;
		Compare compare{ Compare::AtLeast };
		float value{};
		// Compared with this variable instead of value, read every frame: of
		// otherObject, or, when that is empty, of the object being tested.
		std::string otherObject;
		std::string otherVariable;
		std::optional<float> remaining;
		std::vector<Command> commands;
	};

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
	// is from= (or empty), its own <variables>, its objects, and what runs
	// while it is its state's current scene.
	struct RawScene
	{
		std::string name;
		std::string from;
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
		std::vector<RawCondition> conditions;
		std::vector<RawTimer> timers;

		// The states it draws still underneath its own (<show state="..." />).
		std::vector<std::string> showStates;

		// Its own <objects>, in play in every scene of it, as read (names
		// as written); then the kit at the top of its <scenes>, and the scenes.
		// scenes.h lays them out among the game's objects.
		std::vector<RawObject> objects;
		std::vector<RawObject> kit;
		std::vector<RawScene> scenes;

		friend std::ostream& operator<<(std::ostream& o, RawState const& f);

	private:

	};

	// A scene, made ready to run: its path (playing.room1), and the
	// conditions and timers that run while it is its state's current scene,
	// after the state's own.
	struct Scene
	{
		std::string name;
		std::vector<Condition> conditions;
		std::vector<Timer> timers;
	};

	class State
	{
	public:
		std::string name;
		std::vector<std::string> show;
		std::map<KeyCode, std::vector<Command>> input;
		std::vector<Condition> conditions;

		// The states drawn still underneath it (RawState::showStates).
		std::vector<std::string> showStates;

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
