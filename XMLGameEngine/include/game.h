// game.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

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
		void initXML(void) ;
		void initEXPR(void);
		void initSFML(void);

		void updateSFMLTextIncrementValue(Object& object);

		void updateObjects(void);
		void printGame(void);

		WindowDesc& getWindowDesc(void);
		std::vector<Object>& getObjects(void);
		Object& getObject(const std::string& name);
		float& getVariable(const std::string& name);
		State getCurrentState(void);

		void setCurrentState(const int& index);
		void setCurrentState(const std::string& name);
		void setObjectParam(const std::string& name, const std::string& param, const float& value);

	private:
		std::string filename;
		WindowDesc windowDesc;

		std::map<std::string, float> variables;
		std::vector<State> states;
		std::vector<Object> objects;
		std::stack<State> currentState;

		bool bouncedOffTop(Object& object);
		bool bouncedOffBottom(Object& object);
		bool bouncedOffLeft(Object& object);
		bool bouncedOffRight(Object& object);

		bool stuckToTop(Object& object);
		bool stuckToBottom(Object& object);
		bool stuckToLeft(Object& object);
		bool stuckToRight(Object& object);

		bool circleRectangleCollision(Object& object, Object& otherObject);
	};
}
