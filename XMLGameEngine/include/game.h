// game.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stack>
#include <algorithm>
#include <cmath>
#include <assert.h>

#include "utils.h"
#include "object.h"
#include "states.h"

namespace xge
{
	class Game
	{
	public:
		Game(std::string game);
		void initXML(void) ;
		void initEXPR(void);
		void initSFML(void);

		void updateObjects(void);
		void printGame(void);

		xge::WindowDesc& getWindowDesc(void);
		std::vector<xge::Object>& getObjects(void);
		xge::Object& getObject(std::string name);
		xge::SObject& getSObject(std::string name);
		float& getVariable(std::string name);
		xge::State getCurrentState(void);

		void setCurrentState(int index);
		void setCurrentState(std::string name);
		void setObjectParam(std::string name, std::string param, float value);

	private:
		std::string filename;
		xge::WindowDesc windowDesc;

		std::map<std::string, float> variables;
		std::vector<xge::State> states;
		std::vector<xge::SObject> sObjects;
		std::vector<xge::Object> objects;
		std::stack<xge::State> currentState;

		bool bouncedOffTop(xge::Object& object);
		bool bouncedOffBottom(xge::Object& object);
		bool bouncedOffLeft(xge::Object& object);
		bool bouncedOffRight(xge::Object& object);

		bool stuckToTop(xge::Object& object);
		bool stuckToBottom(xge::Object& object);
		bool stuckToLeft(xge::Object& object);
		bool stuckToRight(xge::Object& object);
	};

	template<class T>
	constexpr const T& clamp(const T& v, const T& lo, const T& hi)
	{
		assert(!(hi < lo));
		return (v < lo) ? lo : (hi < v) ? hi : v;
	}
}
