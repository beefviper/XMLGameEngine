// object.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include <SFML/Graphics.hpp>

#include "command.h"

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <ostream>

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

		friend std::ostream& operator<<(std::ostream& o, WindowDesc const& f);
	};

	// One <collision basic="basic" .../> rule, still unparsed. class/object let
	// a rule only respond to a specific kind (or specific instance) of the
	// other object in the pair; empty means "matches anything" (unchanged
	// behaviour). Only meaningful for object-object ("basic") collisions -
	// a screen-edge collision has no "other object" to filter against.
	struct RawCollisionRule
	{
		std::string filterClass;
		std::string filterObject;
		std::string action;
	};

	struct RawCollisionData
	{
		bool enabled{ false };
		bool group{ false };
		std::string top;
		std::string bottom;
		std::string left;
		std::string right;
		std::vector<RawCollisionRule> basic;

		// TODO: add operator<< to RawCollisionData

	};

	// A parsed, ready-to-run version of RawCollisionRule.
	struct CollisionRule
	{
		std::string filterClass;
		std::string filterObject;
		std::vector<Command> commands;
	};

	struct CollisionData
	{
		bool enabled{ false };
		int group{ 0 };
		std::vector<Command> top;
		std::vector<Command> bottom;
		std::vector<Command> left;
		std::vector<Command> right;
		std::vector<CollisionRule> basic;

		// TODO: add operator<< to CollisionData

	};

	struct Vector2str
	{
		std::string x;
		std::string y;
	};

	struct Vector2i
	{
		int x = 0;
		int y = 0;
	};

	struct GridData
	{
		Vector2i max{ 1,1 };
		Vector2i padding{ 0,0 };
		Vector2i obj{ 0,0 };
	};

	struct RawObject
	{
		std::string name;
		std::string objClass;
		std::string src;
		bool isVisible{ true };
		Vector2str rawPosition;
		Vector2str rawVelocity;
		RawCollisionData rawCollisionData;
		std::map<std::string, std::string> action;
		std::map<std::string, std::string> variable;

		friend std::ostream& operator<<(std::ostream& o, RawObject const& f);
	};

	struct Object
	{
		std::string name;
		std::string objClass;
		std::string src;
		bool isVisible{ true };
		sf::Vector2f position;
		sf::Vector2f positionOriginal;
		sf::Vector2f velocity;
		sf::Vector2f velocityOriginal;
		CollisionData collisionData;
		std::vector<std::string> spriteParams;
		ShapeKind shapeKind{ ShapeKind::Unknown };
		std::map<std::string, std::vector<Command>> action;
		std::map<std::string, float> variable;

		// Set when this is a text object whose displayed number tracks another
		// object's own <variable> (e.g. sprite src="text(paddle1.score,128,...)"
		// -> boundVariableOwner="paddle1", boundVariableName="score"). Empty
		// owner means this text object isn't bound to anything and only ever
		// updates via inc() targeting its own name directly (the older,
		// still-supported pattern for a text object that just displays its own
		// counter, with nothing else deriving its number - no shipped game
		// currently needs it, now that pong.xml's score1/score2 are bound to
		// paddle1.score/paddle2.score instead).
		std::string boundVariableOwner;
		std::string boundVariableName;
		std::unique_ptr<sf::RenderTexture> renderTexture = nullptr;
		std::unique_ptr<sf::Sprite> sprite = nullptr;

		friend std::ostream& operator<<(std::ostream& o, Object const& f);
	};
}
