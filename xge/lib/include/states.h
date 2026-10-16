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
	//
	// What is tested can also be a total over the matching objects in play
	// (total="count", "sum", "highest", "lowest") or, per matching object,
	// how far it is from the nearest of another (distance="name"). And a
	// condition can be <all> or <any> of several <check>s (each a test of
	// its own, with no commands), which can hold <all>s and <any>s in turn:
	// then `join` says which, the checks are `parts`, and the condition's own
	// test is not used.
	struct RawCondition
	{
		enum class Test { AtLeast, AtMost, Remaining, Above, Below, Equal };
		enum class Join { None, All, Any };

		std::string filterClass;
		std::string filterObject;
		std::string variableName;
		std::string total;    // total="...": "" (each object), count, sum, highest, lowest
		std::string distance; // distance="...": the object or group measured to, or ""
		Test test{ Test::AtLeast };
		RawValue threshold;      // when otherVariable is empty
		std::string otherObject;  // the test tag's object=, or empty
		std::string otherVariable; // the test tag's variable=, or empty
		Join join{ Join::None };
		std::vector<RawCondition> parts; // <all> or <any>: its <check>s, <all>s and <any>s
		std::vector<RawCommand> commands;
		std::string name; // name="...": a scene from= this one's can put another in its place
	};

	// How a condition's variable is compared, and whether `variable` passes
	// against `against` that way.
	enum class Compare { AtLeast, AtMost, Above, Below, Equal };
	bool passes(Compare compare, float variable, float against) noexcept;


	// What a condition tests: each matching object's variable, a total over
	// the matching objects in play, or each one's distance to another.
	enum class Total { None, Count, Sum, Highest, Lowest };

	struct Condition
	{
		std::string filterClass;
		std::string filterObject;
		std::string variableName;
		Total total{ Total::None };
		std::string distanceTo; // measured to the nearest one in play of this name or group
		RawCondition::Join join{ RawCondition::Join::None };
		std::vector<Condition> parts;
		Compare compare{ Compare::AtLeast };
		float value{};
		// Compared with this variable instead of value, read every frame: of
		// otherObject, or, when that is empty, of the object being tested.
		std::string otherObject;
		std::string otherVariable;
		std::optional<float> remaining;
		std::vector<Command> commands;
	};

	// "status.lives <= 0", "count of alien <= 2", "all of (...)": a
	// condition's test in a line, for printing and the inspector.
	std::string describe(const Condition& condition);

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
