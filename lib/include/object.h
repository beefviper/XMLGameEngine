// object.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "bitmap.h"
#include "command.h"
#include "types.h"

#include <array>
#include <memory>
#include <optional>
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

	// One <collision> rule about another object, still unparsed. class/object
	// let a rule only respond to a specific kind (or specific instance) of the
	// other object in the pair; empty means "matches anything". Only meaningful
	// for object-object collisions - a screen-edge collision (<collision
	// edge="...">) has no "other object" to filter against.
	struct RawCollisionRule
	{
		std::string filterClass;
		std::string filterObject;

		// unless="class": the rule does not run while the object is at the
		// same moment touching something of that class - "water kills the
		// frog, unless it is also on a log". Empty means no exception.
		std::string unlessClass;

		// <slower>N</slower> / <faster>N</faster> before the commands: the
		// rule only runs while the object's own speed is under N (slower) or
		// N or more (faster) at the moment of the touch - a landing pad that
		// only a slow ship lands on. Unset means any speed.
		std::optional<RawValue> slower;
		std::optional<RawValue> faster;
		std::vector<RawCommand> commands;
	};

	// How an object's shape is tested against others: as its bounding box
	// (Box, the default; a circle as a circle) or by the pixels actually drawn
	// (Pixel) - see CollisionDetector.
	enum class CollisionType { Box, Pixel };

	struct RawCollisionData
	{
		bool enabled{ false };
		bool lockstep{ false };
		CollisionType type{ CollisionType::Box };
		std::vector<RawCommand> top;
		std::vector<RawCommand> bottom;
		std::vector<RawCommand> left;
		std::vector<RawCommand> right;
		// Rules about another object: a <collision> with a class= and/or
		// object= selector, or with no selector at all (matches anything). The
		// name is left from when the unfiltered one was written basic="basic".
		std::vector<RawCollisionRule> basic;

		// TODO: add operator<< to RawCollisionData

	};

	// A parsed, ready-to-run version of RawCollisionRule.
	struct CollisionRule
	{
		std::string filterClass;
		std::string filterObject;
		std::string unlessClass;

		// The numbers of <slower>/<faster> (see RawCollisionRule).
		std::optional<float> slower;
		std::optional<float> faster;
		std::vector<Command> commands;
	};

	struct CollisionData
	{
		bool enabled{ false };
		int lockstep{ 0 };
		CollisionType type{ CollisionType::Box };
		std::vector<Command> top;
		std::vector<Command> bottom;
		std::vector<Command> left;
		std::vector<Command> right;
		// Rules about another object (see RawCollisionData::basic).
		std::vector<CollisionRule> basic;

		// TODO: add operator<< to CollisionData

	};

	// An <x>/<y> pair, each still a RawValue.
	struct RawVector2
	{
		RawValue x;
		RawValue y;
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

	// One <line> of a sprite drawn from lines: two points, measured in pixels
	// from the sprite's top left, and how it looks.
	struct RawLine
	{
		RawVector2 from;
		RawVector2 to;
		std::string color;           // "" means color.white
		RawValue thickness;          // only when hasThickness; otherwise 1
		bool hasThickness{ false };
	};

	// An object's <sprite>, as written: one shape, optionally repeated as a
	// <grid>, or a drawing made of <line>s. Which of the fields are used
	// depends on `kind` (circle, rectangle, text, image or line).
	struct RawSprite
	{
		std::string kind;

		std::vector<RawLine> lines;  // line: one or more

		RawValue radius;             // circle
		RawValue width;              // rectangle
		RawValue height;             // rectangle
		RawValue size;               // text

		// A text is either a fixed label (<content>) or a number (<number>);
		// a number that is just an owner.variable stays live (see
		// Object::boundVariableOwner).
		std::string content;         // text label
		bool textIsNumber{ false };
		RawValue number;             // text, when textIsNumber

		std::string path;            // image
		std::string flip;            // image: "", "horizontal" or "vertical"
		std::string color;           // "" means color.white

		// <grid>: this shape repeated columns x rows times.
		bool isGrid{ false };
		RawValue columns;
		RawValue rows;
		RawVector2 padding;          // only when hasPadding
		bool hasPadding{ false };
	};

	struct RawObject
	{
		std::string name;
		std::string objClass;
		RawSprite sprite;
		bool isVisible{ true };
		RawVector2 rawPosition;
		RawVector2 rawVelocity;
		RawVector2 rawAcceleration;  // only when hasAcceleration
		bool hasAcceleration{ false };
		RawValue rawHeading;         // only when hasHeading
		bool hasHeading{ false };
		RawValue rawDrag;            // only when hasDrag
		bool hasDrag{ false };
		RawCollisionData rawCollisionData;
		std::map<std::string, std::vector<RawCommand>> action;
		std::map<std::string, RawValue> variable;

		// The <group> this object is a member of, or empty for a plain
		// <object>. A group is written once in the file and read as one
		// RawObject per member (see game_xml.cpp), so from here on a member
		// is an ordinary object that also remembers which group it came from.
		std::string groupName;

		friend std::ostream& operator<<(std::ostream& o, RawObject const& f);
	};

	struct Object
	{
		// What this object is called. Unique: a <grid> gives each of its cells
		// its own name, the object's name followed by the cell's column and row,
		// counting from 1 - a <grid> called aliens has aliens.1.1, aliens.2.1,
		// ... aliens.11.5. Anything else is just the name from the XML.
		std::string name;

		// The name in the XML this object came from: "aliens" for every cell of
		// the grid above, and the same as `name` for an object that is not a
		// grid. What a state's <show>, a rule's or condition's object=, and
		// <reset object="..." /> refer to, so they can still mean the whole grid at once;
		// a cell can also be named on its own (aliens.3.2).
		std::string baseName;

		// The <group> this object is a member of ("logrow3" for logrow3.1,
		// logrow3.2, ...), or empty. Like baseName it can be used wherever the
		// XML names an object (a state's <show>, object= in a rule or
		// condition, <reset object="..." />) to mean every member at once.
		std::string groupName;
		std::string objClass;
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

		// A constant pull on the velocity, added to it every frame (gravity is
		// an acceleration with only a y) - see Game::applyAcceleration.
		// `accelerationOriginal` is how it started: <stop /> takes the
		// acceleration away and a reset brings it back.
		Vector2f acceleration;
		Vector2f accelerationOriginal;

		// Which way the object faces: degrees clockwise from straight up, so 0
		// is up, 90 is right, 180 is down; always from 0 up to (not including)
		// 360. Only an object whose file gives it a <heading> turns (a <turn>
		// in its actions), is pushed along it (a <thrust>) or fires along it
		// (a <fire>). `headingOriginal` is how it started, which a reset puts
		// back. A sprite of <line>s that has a heading is drawn ahead of time at
		// headingSteps headings (Object::headingBitmaps), and `bitmap` is the
		// one nearest the heading, so what is shown and what a pixel collision
		// tests turn with it - see showHeading.
		bool hasHeading{ false };
		float heading{};
		float headingOriginal{};
		std::vector<std::shared_ptr<const Bitmap>> headingBitmaps;

		// Held turning, by Direction (only Left and Right are used): degrees a
		// frame, as activeThrust holds a thruster; and thrust held along the
		// heading, with the variable it burns. See Game::applyAcceleration.
		std::array<float, 4> activeTurn{};
		float activeThrustAhead{};
		std::string activeThrustAheadBurn;

		// How much of its velocity an object keeps every frame is 1 minus this:
		// 0 (the default) is no drag at all, 0.02 loses a fiftieth of the speed
		// a frame, so a thruster can no longer push it faster than a top speed.
		float drag{};

		// Points `bitmap` at the drawing nearest the current heading, and
		// marks the visual to be rebuilt when that is a different one. Does
		// nothing for an object with no headings drawn.
		void showHeading();

		// Per-direction thrust currently being held for this object, the way
		// activeMoveStep holds a <move>'s step, and the variable (if any) each
		// one burns; set by an <accelerate> in an action, cleared when the key
		// is let go.
		std::array<float, 4> activeThrust{};
		std::array<std::string, 4> activeThrustBurn{};

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

		// A <grid> cell's offset from the grid's evaluated <position>, so that
		// position can be worked out again later and the offset added back.
		Vector2f gridOffset{};

		CollisionData collisionData;
		std::vector<std::string> spriteParams;
		ShapeKind shapeKind{ ShapeKind::Unknown };

		// What a sprite of <line>s was drawn as, worked out when the game
		// loads (game_expr). The window backends draw it as a picture and a
		// collision of type pixel tests it; null for every other shape. Shared,
		// not copied, with every copy of the object.
		std::shared_ptr<const Bitmap> bitmap;

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
		// object's own <variable> (e.g. a <text> sprite whose <number> is paddle1.score
		// -> boundVariableOwner="paddle1", boundVariableName="score"). Empty
		// owner means this text object isn't bound to anything and only ever
		// updates via <inc /> targeting its own name directly (the older,
		// still-supported pattern for a text object that just displays its own
		// counter, with nothing else deriving its number - no shipped game
		// currently needs it, now that pong.xml's score1/score2 are bound to
		// paddle1.score/paddle2.score instead).
		std::string boundVariableOwner;
		std::string boundVariableName;

		friend std::ostream& operator<<(std::ostream& o, Object const& f);
	};
}
