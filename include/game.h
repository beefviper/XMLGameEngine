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
		void printGame(void);

		WindowDesc& getWindowDesc(void) noexcept;
		bool isShown(const Object& object) noexcept;
		Object& getObject(const std::string& name);
		Object* tryGetObject(const std::string& name) noexcept;
		float getVariable(const std::string& name);
		State getCurrentState(void);
		std::vector<Object>& getCurrentObjects(void) noexcept;

		void setCurrentState(const int& index);
		void setCurrentState(const std::string& name);
		void pushState(std::string name);
		void popState(void) noexcept;

		// Goes up by one every time the state stack changes (any push or pop,
		// including resets and condition-driven changes), so Engine can tell
		// that held keys need re-evaluating without comparing state names.
		unsigned long stateChangeCount(void) const noexcept;

		void setObjectParam(const std::string& name, const std::string& param, const float& value);

		void updateGroupOfObjects(const Object& object, std::string side) noexcept;

		// Thin forwarders so CommandExecutor (which only sees Game through a few
		// public entry points) can reach the xml subsystem without it being
		// made public wholesale.
		void incrementText(const std::string& objectName);

		// reset('objectName') from a state's <input>/<condition> action (see
		// CmdResetObject): restores that object's position, velocity, and every
		// <variable> to their starting values, then refreshes any text display
		// bound to one of those variables. Used e.g. to zero the paddles' scores
		// before a new game starts, so a stale <condition> doesn't instantly
		// re-fire.
		void resetObject(const std::string& name);

		// Bare reset() from a state's <input>/<condition> action (see CmdReset,
		// handled this way only in CommandExecutor::executeInput - a collision's
		// own bare reset() still just resets that one object's position).
		// Resets every object back to how the game loaded (position, velocity,
		// every <variable>), refreshes every bound text display, and collapses
		// the whole state stack back down to the very first state - the same
		// state Engine's constructor pushes via setCurrentState(0) - so
		// mainmenu -> playing -> gameover -> mainmenu -> ... doesn't grow the
		// stack forever.
		void resetAll();

	private:
		std::string filename;
		WindowDesc windowDesc;

		game_xml xml;
		game_expr expr;

		// How xml above checked this game's file against
		// assets/xmlgameengine.xsd - set once, by xml.init() inside the
		// constructor, and only ever read back by printGame() (see
		// xml_document.h for what each value means).
		SchemaValidation xmlValidation = SchemaValidation::None;

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

		void checkObjectCollision(Object& a, Object& b);

		// Checked once per frame against the current state's <conditions>; the
		// first one whose target variable has reached its threshold fires its
		// action and stops (the state may have just changed).
		void checkConditions();
	};
}
