// game.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

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
		Object& getObject(const std::string& name);
		float& getVariable(const std::string& name);
		State getCurrentState(void);

		void setCurrentState(const int& index);
		void setCurrentState(const std::string& name);
		void setObjectParam(const std::string& name, const std::string& param, const float& value);

	private:
		std::string filename;
		WindowDesc windowDesc;

		game_xml xml;
		game_expr expr;
		game_sfml sfml;

		std::map<std::string, float> variables;
		std::vector<State> states;
		std::vector<Object> objects;
		std::stack<State> currentState;

		bool bouncedOffTop(Object& object);
		bool bouncedOffBottom(Object& object);
		bool stuckToTop(Object& object);
		bool stuckToBottom(Object& object);

		void checkEdge(Object& object, std::string side);

		bool circleRectangleCollision(Object& object, Object& otherObject);
	};
}
