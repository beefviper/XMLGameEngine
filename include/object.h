// object.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include "command.h"
#include "types.h"

#include <string>
#include <vector>
#include <map>
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
		Vector2f position;
		Vector2f positionOriginal;
		Vector2f velocity;
		Vector2f velocityOriginal;

		// The object's own measured bounding box (width/height) - unknown
		// (zero) until whichever Window backend is running has actually built
		// this object's visual and measured it (see Window::init() in
		// window.h). Circle/rectangle sizes are implied by spriteParams alone;
		// text/image need the backend's own font/image loading to know their
		// real pixel size, so every backend writes this back the same way
		// regardless - collision/physics code (CollisionDetector,
		// CommandExecutor) only ever reads this field, never a backend type.
		Vector2f size{};

		CollisionData collisionData;
		std::vector<std::string> spriteParams;
		ShapeKind shapeKind{ ShapeKind::Unknown };

		// True until the active Window backend has built (or rebuilt) this
		// object's own visual - set again whenever something changes what
		// should be on screen for it (e.g. a bound text display's number
		// changing - see Game::incrementText/resetObject/resetAll) so the
		// backend knows to rebuild from spriteParams before its next draw()
		// instead of showing a stale image. Game/Engine never draw anything
		// themselves; they only ever flip this flag.
		bool visualDirty{ true };

		std::map<std::string, std::vector<Command>> action;
		std::map<std::string, float> variable;

		// Snapshot of each <variable>'s value at construction time (mirrors
		// positionOriginal/velocityOriginal). Game::resetObject() restores
		// `variable` from this map so a new game/round can start clean (e.g.
		// resetting paddle1.score/paddle2.score back to 0), instead of a fresh
		// game instantly re-tripping a <condition> left over from the last one.
		std::map<std::string, float> variableOriginal;

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

		friend std::ostream& operator<<(std::ostream& o, Object const& f);
	};
}
