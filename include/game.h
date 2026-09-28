// game.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include "collision_detector.h"
#include "command.h"
#include "game_xml.h"
#include "game_expr.h"
#include "game_sfml.h"
#include "object.h"
#include "states.h"

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
		Game(const std::string& game);

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

		void setObjectParam(const std::string& name, const std::string& param, const float& value);

		void updateGroupOfObjects(const Object& object, std::string side) noexcept;

		// Thin forwarders so CommandExecutor (which only sees Game through a few
		// public entry points) can reach the sfml/xml subsystems without those
		// being made public wholesale.
		void incrementText(const std::string& objectName);

		// reset('objectName') from a state's <input>/<condition> action (see
		// CmdResetObject): restores that object's position, velocity, and every
		// <variable> to their starting values, then refreshes any text display
		// bound to one of those variables. Used e.g. to zero the paddles' scores
		// before a new game starts, so a stale <condition> doesn't instantly
		// re-fire.
		void resetObject(const std::string& name);

	private:
		std::string filename;
		WindowDesc windowDesc;

		game_xml xml;
		game_expr expr;
		game_sfml sfml;

		std::map<std::string, float> variables;
		std::vector<RawState> rawStates;
		std::vector<State> states;
		std::vector<RawObject> rawObjects;
		std::vector<Object> objects;
		std::stack<State> currentState;

		void checkEdge(Object& object, Edge edge);

		void checkObjectCollision(Object& a, Object& b);

		// Checked once per frame against the current state's <conditions>; the
		// first one whose target variable has reached its threshold fires its
		// action and stops (the state may have just changed).
		void checkConditions();
	};
}
