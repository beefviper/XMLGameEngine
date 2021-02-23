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
	struct WindowDesc
	{
		std::string name;
		float width{};
		float height{};
		std::string background;
		std::string fullscreen;
		int framerate{};
	};

	class Game
	{
	public:
		Game(const std::string& game);
		void initXML(void) ;
		void initEXPR(void);
		void initSFML(void);

		void updateSFMLTexture(xge::Object& object);

		void updateObjects(void);
		void printGame(void);

		xge::WindowDesc& getWindowDesc(void);
		std::vector<xge::Object>& getObjects(void);
		xge::Object& getObject(const std::string& name);
		xge::SObject& getSObject(const std::string& name);
		float& getVariable(const std::string& name);
		xge::State getCurrentState(void);

		void setCurrentState(const int& index);
		void setCurrentState(const std::string& name);
		void setObjectParam(const std::string& name, const std::string& param, const float& value);

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

		bool circleRectangleCollision(xge::Object& object, xge::Object& otherObject);
	};
}
