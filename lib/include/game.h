// game.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include "collision_detector.h"
#include "command.h"
#include "game_xml.h"
#include "game_expr.h"
#include "object.h"
#include "states.h"
#include "xml_document.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stack>
#include <algorithm>
#include <cmath>
#include <assert.h>

namespace xge
{
	class Game
	{
	public:
		// xmlBackend picks which XmlDocument implementation actually parses
		// the file (Xerces, TinyXML2, PugiXML, or RapidXML - see
		// xml_document.h); defaults to Xerces so existing callers (main.cpp)
		// don't have to name one - same pattern as Engine's own
		// WindowBackend parameter.
		explicit Game(const std::string& game, XmlBackend xmlBackend = XmlBackend::Xerces);

		void updateObjects(void);

		// Works out the <position> of every object that uses the size of a text
		// or image (name.width / name.height, its own or another's) from the
		// now-measured sizes, and again whenever one of them has changed since.
		// Engine calls it once its Window has measured sizes, and each frame
		// after that. See Object::positionUsesSize.
		void resolveSizeDependentPositions(void);
		void printGame(void);

		WindowDesc& getWindowDesc(void) noexcept;
		bool isShown(const Object& object) noexcept;
		Object& getObject(const std::string& name);
		Object* tryGetObject(const std::string& name) noexcept;
		float getVariable(const std::string& name);
		State getCurrentState(void);

		// Every state the game file defines, in file order (the current one is
		// a copy of one of these, pushed on the state stack).
		const std::vector<State>& getStates(void) const noexcept;
		std::vector<Object>& getCurrentObjects(void) noexcept;

		void setCurrentState(const int& index);
		void setCurrentState(const std::string& name);

		// Throws std::out_of_range for a name that is not one of the game's
		// states (a game file is checked for that when it loads).
		void pushState(const std::string& name);

		// Does nothing when only the state the game started in is left.
		void popState(void) noexcept;

		// Goes up by one every time the state stack changes (any push or pop,
		// including resets and condition-driven changes), so Engine can tell
		// that held keys need re-evaluating without comparing state names.
		unsigned long stateChangeCount(void) const noexcept;

		void setObjectParam(const std::string& name, const std::string& param, const float& value);

		// Sets an object's own <variable> to a value (for a front end that lets
		// the user edit one), and refreshes every text display bound to it, the
		// same way <inc /> does. Returns false when there is no such object or
		// variable.
		bool setVariable(const std::string& objectName, const std::string& variableName, float value);

		// A member of a lockstep block (see <lockstep> in <collisions>) hit the left or
		// right screen edge: the whole block turns around and steps once in
		// its new direction, every member the same amount, so the formation
		// keeps its spacing.
		void updateLockstepObjects(const Object& object) noexcept;

		// Thin forwarders so CommandExecutor (which only sees Game through a few
		// public entry points) can reach the xml subsystem without it being
		// made public wholesale.
		void incrementText(const std::string& objectName, float amount = 1.0f);

		// <dec variable="owner.variable" />: takes 1 (or the amount) off, the same way, and refreshes the
		// same bound texts. A variable may go below zero; a <condition> with
		// atmost= is what notices it has run out.
		void decrementText(const std::string& objectName, float amount = 1.0f);

		// <reset object="objectName" /> from a state's <input>/<condition> (see
		// CmdResetObject): restores that object's position, velocity, and every
		// <variable> to their starting values, then refreshes any text display
		// bound to one of those variables. Used e.g. to zero the paddles' scores
		// before a new game starts, so a stale <condition> doesn't instantly
		// re-fire.
		void resetObject(const std::string& name);

		// Bare <reset /> from a state's <input>/<condition> (see CmdReset,
		// handled this way only in CommandExecutor::executeInput - a collision's
		// own bare <reset /> still just resets that one object's position).
		// Resets every object back to how the game loaded (position, velocity,
		// every <variable>), refreshes every bound text display, and collapses
		// the whole state stack back down to the very first state - the same
		// state Engine's constructor pushes via setCurrentState(0) - so
		// mainmenu -> playing -> gameover -> mainmenu -> ... doesn't grow the
		// stack forever.
		void resetAll();

	private:
		// The one implementation behind incrementText and decrementText;
		// `verb` is only for the warning printed when the target is not found.
		void changeVariable(const std::string& target, float delta, const char* verb);

		// Shows `value` in every text object bound to ownerName.variableName.
		void refreshBoundTexts(const std::string& ownerName, const std::string& variableName, float value);

		std::string filename;
		WindowDesc windowDesc;

		game_xml xml;
		game_expr expr;

		// How xml above checked this game's file against
		// assets/xmlgameengine.xsd - set once, by xml.init() inside the
		// constructor, and only ever read back by printGame() (see
		// xml_document.h for what each value means).
		SchemaValidation xmlValidation = SchemaValidation::None;

		std::vector<std::pair<std::string, RawValue>> rawVariables;
		std::map<std::string, float> variables;
		std::vector<RawState> rawStates;
		std::vector<State> states;
		std::vector<RawObject> rawObjects;
		std::vector<Object> objects;
		std::stack<State> currentState;
		unsigned long stateChanges = 0;

		void checkEdge(Object& object, Edge edge);

		// After the frame's move: pushes an object back inside any screen edge
		// it has a stick() rule for, even if it has stopped moving.
		void keepStuckObjectInBounds(Object& object);

		// Whether a and b have anything to do about touching each other.
		static bool canCollide(const Object& a, const Object& b) noexcept;

		// How far an object moves in one frame: its own velocity, plus the
		// velocity of whatever it is riding (see Object::carry).
		static Vector2f motionOf(const Object& object) noexcept;

		// Whether it is in motion at all this frame: moving, being carried, or
		// having just made a hop.
		static bool isMoving(const Object& object) noexcept;

		// Changes every shown object's velocity by its <acceleration> and by
		// whatever <accelerate> thrust is being held (burning its fuel), once a
		// frame, before anything moves: a constant pull, or a thruster.
		void applyAcceleration(void);

		// The start of a frame's move: makes every hop queued by a hop.*()
		// action, unless it would leave the window, and forgets last frame's
		// carrying (this frame's collisions work it out again).
		void applyHops(void);

		// Whether `object` is right now touching any other object in play of
		// this class - what a collision rule's unless= asks.
		bool isTouchingClass(const Object& object, const Object& excluding, const std::string& objClass);

		// Moves everything shown by its velocity for one frame, stopping at each
		// touch that has a rule on the way to run it - see the definition.
		void moveObjects(void);

		// Runs both objects' rules for touching each other; edgeOfB is the edge
		// of b that was hit.
		void applyObjectCollision(Object& a, Object& b, Edge edgeOfB);

		// Checked once per frame against the current state's <conditions>; the
		// first one whose target variable has reached its threshold fires its
		// action and stops (the state may have just changed).
		void checkConditions();

		void runConditionCommands(std::vector<Command> commands);
	};
}
