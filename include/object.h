// object.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "command.h"
#include "types.h"

#include <array>
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

		// unless="class": the rule does not run while the object is at the
		// same moment touching something of that class - "water kills the
		// frog, unless it is also on a log". Empty means no exception.
		std::string unlessClass;
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
		std::string unlessClass;
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
		// What this object is called. Unique: a grid() gives each of its cells
		// its own name, the object's name followed by the cell's column and row,
		// counting from 1 - a grid() called aliens has aliens.1.1, aliens.2.1,
		// ... aliens.11.5. Anything else is just the name from the XML.
		std::string name;

		// The name in the XML this object came from: "aliens" for every cell of
		// the grid above, and the same as `name` for an object that is not a
		// grid. What a state's <show>, a rule's or condition's object=, and
		// reset('...') refer to, so they can still mean the whole grid at once;
		// a cell can also be named on its own (aliens.3.2).
		std::string baseName;
		std::string objClass;
		std::string src;
		bool isVisible{ true };

		// How the object started (see Game::resetObject), so that a reset brings
		// back what has since died or been fired: die() hides an object and
		// stops its collisions, and neither would otherwise ever be undone.
		bool isVisibleOriginal{ true };
		bool collisionEnabledOriginal{ false };

		Vector2f position;
		Vector2f positionOriginal;
		Vector2f velocity;
		Vector2f velocityOriginal;

		// Per-direction move step currently being held for this object, keyed
		// by static_cast<size_t>(Direction) - sized for Direction's 4 real
		// values (Up/Down/Left/Right; it has no Count sentinel). 0 means that
		// direction isn't currently held; a nonzero entry is the magnitude of
		// the <action>'s own step. CommandExecutor::applyActionVelocity
		// recombines all four into velocity.x/velocity.y on every key
		// transition, so e.g. holding Down and tapping Up cancels out to a
		// standstill instead of Up's single key event just overwriting the
		// whole axis to 0 - and releasing Up afterward correctly resumes
		// moving Down, rather than leaving the paddle stopped. Also what lets
		// two axes combine into an 8-way diagonal from a 4-way D-pad.
		std::array<float, 4> activeMoveStep{};

		// Motion this object is given for one frame besides its own velocity.
		// `carry` is the velocity of whatever it is riding (a collision rule
		// with carry(), e.g. a frog on a log): worked out again every frame,
		// it counts for movement and for edge checks like velocity does, but
		// is not the object's own, so an object that steps off is at rest.
		// `hopPending` is a jump queued by a hop.*() action, made at the start
		// of the next frame's move (Game::moveObjects); `hopped` is true for
		// the rest of that frame, so an object that has just landed somewhere
		// still counts as moving for the collisions it is now part of.
		Vector2f carry{};
		Vector2f hopPending{};
		bool hopped{ false };

		// The object's own measured bounding box (width/height) - {0,0} until
		// whichever Window backend is running has actually built this
		// object's visual and measured it (see Window::init() in window.h).
		// Circle/rectangle sizes are implied by spriteParams alone; text/
		// image need the backend's own font/image loading to know their real
		// pixel size, so every backend writes this back the same way
		// regardless - collision/physics code (CollisionDetector,
		// CommandExecutor) only ever reads this field, never a backend type.
		Vector2f size{};

		// True once a Window backend has measured `size` at least once (see
		// finalizeVisual() in window_sfml.cpp/window_raylib.cpp/
		// window_sdl2.cpp) - stays true from then on, even across a later
		// visualDirty rebuild, since `size` always holds the last real
		// measurement rather than reverting to unknown. Lets operator<<
		// print "unknown" instead of a misleading {0,0} for a game printed
		// before Engine exists.
		bool sizeKnown{ false };

		// <position> may use any object's size by name - name.width and
		// name.height, including this object's own - e.g. to centre a text:
		// window.width.center - title.width / 2. A circle's or rectangle's
		// size follows from its sprite alone, so a position using only those
		// is exact the moment the game is loaded. A text's or image's size
		// only exists once a Window backend has measured it, so a position
		// that uses one sets positionUsesSize, lists those objects in
		// sizeDependencies, and stays positionResolved == false (printGame
		// shows it as unknown) until Game::resolveSizeDependentPositions()
		// works it out from the real sizes - and again whenever any of them
		// changes (a score gaining a digit). positionSizesUsed holds the sizes,
		// in sizeDependencies order, it was last worked out from.
		bool positionUsesSize{ false };
		bool positionResolved{ true };
		std::vector<std::string> sizeDependencies;
		std::vector<Vector2f> positionSizesUsed;

		// A grid() cell's offset from the grid's evaluated <position>, so that
		// position can be worked out again later and the offset added back.
		Vector2f gridOffset{};

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
