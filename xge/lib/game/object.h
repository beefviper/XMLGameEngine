// object.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "picture/bitmap.h"
#include "game/command.h"
#include "game/types.h"

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <utility>
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

		// sprite="name": the rule only runs while the object running it shows
		// its sprite of that name (see <become>). Empty means whatever it shows.
		std::string whileSprite;
		std::vector<RawCommand> commands;
	};

	// How an object's shape is tested against others: as its bounding box
	// (Box, the default; a circle as a circle) or by the pixels actually drawn
	// (Pixel) - see CollisionDetector.
	enum class CollisionType { Box, Pixel };

	// A <collision edge="..."> with sprite= or unless=, as its commands in an
	// edge's list: `count` of them from `first`, run only while the object
	// shows that look and is not touching anything of that class, looked at
	// once as the rule comes (as for a rule about another object).
	struct EdgeGuard
	{
		std::size_t first{ 0 };
		std::size_t count{ 0 };
		std::string sprite;
		std::string unlessClass;
	};

	struct RawCollisionData
	{
		bool enabled{ false };
		bool lockstep{ false };
		CollisionType type{ CollisionType::Box };
		std::vector<RawCommand> top;
		std::vector<RawCommand> bottom;
		std::vector<RawCommand> left;
		std::vector<RawCommand> right;
		// The rules among them with sprite= or unless= (see EdgeGuard).
		std::vector<EdgeGuard> topGuards;
		std::vector<EdgeGuard> bottomGuards;
		std::vector<EdgeGuard> leftGuards;
		std::vector<EdgeGuard> rightGuards;
		// Rules about another object: a <collision> with a class= and/or
		// object= selector, or with no selector at all (matches anything), as
		// against the window's edges above.
		std::vector<RawCollisionRule> others;

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
		std::string whileSprite;
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
		// The rules among them with sprite= or unless= (see EdgeGuard).
		std::vector<EdgeGuard> topGuards;
		std::vector<EdgeGuard> bottomGuards;
		std::vector<EdgeGuard> leftGuards;
		std::vector<EdgeGuard> rightGuards;
		// Rules about another object (see RawCollisionData::others).
		std::vector<CollisionRule> others;

		// TODO: add operator<< to CollisionData

	};

	// An <x>/<y> pair, each still a RawValue.
	struct RawVector2
	{
		RawValue x;
		RawValue y;
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

	// An object's <sprite>, as written: one shape, a drawing made of <line>s,
	// or a picture written as rows of text.
	// Which of the fields are used depends on `kind` (circle, rectangle, text,
	// image, line, bitmap or svg).
	// One piece of a <text>, in the order written: a fixed label
	// (<content>), a number (<number>) or a number shown as a letter
	// (<letter>, 0 is A, 25 is Z, and round again).
	struct RawTextPiece
	{
		enum class Kind { Words, Number, Letter };
		Kind kind{ Kind::Words };
		std::string words;           // Words
		RawValue value;              // Number, Letter
	};

	struct RawSprite
	{
		std::string kind;

		// <sprite name="...">: what an <animation>'s <frame> calls it. Needed
		// when an object has more than one sprite.
		std::string name;

		std::vector<RawLine> lines;  // line: one or more

		std::vector<std::string> bitmapRows; // bitmap: the <row>s, top to bottom
		// bitmap: each <color char="w">: the character and the name of its
		// color, in the order written (`color` is the '*'s).
		std::vector<std::pair<char, std::string>> bitmapColors;
		RawValue scale;              // bitmap: real pixels to a character; svg: real pixels to a unit of the drawing; only when hasScale
		bool hasScale{ false };

		// svg: the file is `path` (as for an image). The part of the drawing to
		// take, in the drawing's units, is x, y, width and height, all four
		// given or none (then all of it). hide names elements to leave out.
		RawValue svgX;
		RawValue svgY;
		RawValue svgWidth;
		RawValue svgHeight;
		bool hasSvgRegion{ false };
		std::vector<std::string> svgHide;

		RawValue radius;             // circle
		RawValue width;              // rectangle
		RawValue height;             // rectangle
		RawValue size;               // text

		// A text: its pieces, in order (see RawTextPiece); a <number> or a
		// <letter> that is just an owner.variable stays live (see
		// Object::textPieces).
		std::vector<RawTextPiece> text;

		std::string path;            // image, svg
		std::string flip;            // image: "", "horizontal" or "vertical"
		std::string color;           // "" means color.white (or the style's)

		// text: style="...", a <style> of the game's <styles>, which gives the
		// <size> and <color> the text leaves out (see applyStyles).
		std::string style;
	};

	// A game's <style name="...">: a size and a color a <text> can take by
	// style="..." instead of saying them. Either may be left out.
	struct RawStyle
	{
		std::string name;
		RawValue size;               // empty: the style gives no size
		std::string color;           // empty: the style gives no color
	};

	// An object's <animation>: the sprites it shows one after the other, each
	// for `interval` seconds, round and round. The frames are the object's own
	// <sprite>s picked out by name, in the order the <frame>s are written.
	struct RawAnimation
	{
		RawValue interval;
		std::vector<RawSprite> frames;
	};

	// Where a cell of a group laid out in <columns> and <rows> goes. Its slot
	// is as big as its row's sprite, the slots of a row follow one another
	// left to right with a gap before each, and the rows follow one another
	// down the same way (see load/expr::layOutCells). A cell whose own sprite
	// is another size sits in the middle of its slot.
	struct RawCell
	{
		int column{ 0 };             // from 1; 0 for anything that is not a cell
		int row{ 0 };                // from 1
		RawSprite slot;              // its row's sprite, as the row first shows it
		RawValue gapBefore;          // the gap before its column, in its row (0 in column 1)
		RawValue gapAbove;           // the gap above its row (0 in row 1)

		// A place on a <map> (load/xml's readMap), when mapColumn is above 0:
		// the map's name, the tile's column and row counting from 1, how many
		// columns and rows the map has, and the size of a tile. The object
		// sits in the middle of its tile (load/expr::layOutCells), measured
		// from the map's <position>, which is the object's own.
		std::string map;
		int mapColumn{ 0 };
		int mapRow{ 0 };
		int mapColumns{ 0 };
		int mapRows{ 0 };
		RawValue tileWidth;
		RawValue tileHeight;
	};

	// One <wall class="wall" sprite="normal" /> of a <maze>: the objects of
	// that class on the map stop the walker, while it shows that look (any
	// look, when sprite is empty).
	struct MazeWall
	{
		std::string objClass;
		std::string sprite;
	};

	// An object's <maze map="maze">: the <map> whose tiles it walks ("" for
	// an object that walks none), and what is a wall to it. See play/maze.h.
	struct RawMaze
	{
		std::string map;
		std::vector<MazeWall> walls;
	};

	struct RawObject
	{
		std::string name;
		std::string objClass;

		// What the object looks like: its only <sprite>, or, when it has an
		// <animation>, the first frame (the one it starts on).
		RawSprite sprite;
		bool hasAnimation{ false };
		RawAnimation animation;

		// An object's looks: its <sprite>s, each named, when it has more than
		// one and no animation (sprite is the first). Empty otherwise.
		std::vector<RawSprite> looks;
		bool isVisible{ true };

		// <drawn>false</drawn>: in play, moving and colliding, but never drawn
		// (an exit, a trigger zone).
		bool isDrawn{ true };
		RawVector2 rawPosition;
		RawVector2 rawVelocity;
		RawVector2 rawAcceleration;  // only when hasAcceleration
		bool hasAcceleration{ false };
		RawValue rawHeading;         // only when hasHeading
		bool hasHeading{ false };
		RawValue rawDrag;            // only when hasDrag
		bool hasDrag{ false };
		std::string facing;          // "" when the object has no <facing>
		RawMaze maze;                // <maze>: the map it walks, if any
		std::vector<RawTimer> timers;
		RawCollisionData rawCollisionData;
		std::map<std::string, std::vector<RawCommand>> action;
		std::map<std::string, RawValue> variable;

		// The <group> this object is a member of, or empty for a plain
		// <object>. A group is written once in the file and read as one
		// RawObject per member (see load/xml.cpp), so from here on a member
		// is an ordinary object that also remembers which group it came from.
		std::string groupName;
		// The groups around that group, outermost first, when groups are
		// written inside one another; each of their names means this object too.
		std::vector<std::string> outerGroups;

		// Whether `group` is this object's group or one around it.
		bool inGroup(const std::string& group) const
		{
			return !group.empty() && (groupName == group || std::find(outerGroups.begin(), outerGroups.end(), group) != outerGroups.end());
		}

		// Where it goes in its group's <columns> and <rows>, when it is a cell.
		RawCell cell;

		// Who owns it: empty for one of the game's own <objects>; the state's
		// name for one of a state's; and that and the scene's path
		// (playing.room1) for one of a scene's. Set when the states and
		// scenes are laid out (game/scenes.h), never read from the file.
		std::string ownerState;
		std::string ownerScene;

		friend std::ostream& operator<<(std::ostream& o, RawObject const& f);
	};

	// Where a maze walker is headed (see play/maze.h): what its last <go>, <hunt>
	// or <wander> said, and the tile it last chose its way at, so that it
	// chooses once a tile.
	struct Walk
	{
		enum class Mode { None, Go, Hunt, Wander };
		Mode mode{ Mode::None };
		float speed{};

		// <go>: the way it turns at the first gap, and keeps going.
		Direction wish{ Direction::Up };

		// <hunt>: what it is after, and how its target is found from that.
		CmdHunt hunt;

		// The tile it last chose at, and the way it was going then: a way
		// changed by anything else (a <reverse />) lets it choose again.
		int chosenColumn{ -1 };
		int chosenRow{ -1 };
		bool chose{ false };
		Direction going{ Direction::Up };
	};

	// A piece of a text as an object shows it (see RawTextPiece): what it
	// shows now, and, for a number or a letter that follows a variable, its
	// owner and name.
	struct TextPiece
	{
		RawTextPiece::Kind kind{ RawTextPiece::Kind::Words };
		std::string shown;
		std::string owner;
		std::string variable;
	};

	// What a number shows as a piece of that kind: a whole number, or a
	// letter (0 is A, 25 is Z, and round again: 26 is A, -1 is Z).
	std::string showPiece(RawTextPiece::Kind kind, float value);

	// What a text of these pieces reads: each piece a word, one space
	// between, except that letters side by side make one word (initials).
	std::string joinText(const std::vector<TextPiece>& pieces);

	struct Object
	{
		// What this object is called. Unique: a member of a group is the
		// group's name and its number (logrow3.2), a cell of a group laid out
		// in <columns> and <rows> the group's name, its column and its row,
		// counting from 1 (aliens.3.2), unless either is given a name.
		std::string name;

		// The <group> this object is a member of ("logrow3" for logrow3.1,
		// logrow3.2, ...), or empty. It can be used wherever the XML names an
		// object (a state's <show>, object= in a rule or
		// condition, <reset object="..." />) to mean every member at once.
		std::string groupName;
		// The groups around that one, outermost first (see RawObject).
		std::vector<std::string> outerGroups;

		// Whether `group` is this object's group or one around it.
		bool inGroup(const std::string& group) const
		{
			return !group.empty() && (groupName == group || std::find(outerGroups.begin(), outerGroups.end(), group) != outerGroups.end());
		}
		std::string objClass;
		bool isVisible{ true };

		// Whether it is drawn while in play (see RawObject::isDrawn).
		bool isDrawn{ true };

		// Named in the game's <keeps>: a full <reset /> passes it over, so it
		// keeps what it holds (a best score) from one game to the next.
		bool kept{ false };

		// Who owns it (see RawObject::ownerState). An object carried into
		// another scene (<enter>) belongs to that one from then on;
		// ownerSceneOriginal is the scene it started in, where a reset puts it
		// back.
		std::string ownerState;
		std::string ownerScene;
		std::string ownerSceneOriginal;

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
		// an acceleration with only a y) - see playOneFrame.
		// `accelerationOriginal` is how it started: <stop /> takes the
		// acceleration away and a reset brings it back.
		Vector2f acceleration;
		Vector2f accelerationOriginal;

		// Which way the object faces: degrees clockwise from straight up, so 0
		// is up, 90 is right, 180 is down; always from 0 up to (not including)
		// 360. Only an object whose file gives it a <heading> turns (a <turn>
		// in its actions), is pushed along it (a <thrust>) or fires along it
		// (a <fire>). `headingOriginal` is how it started, which a reset puts
		// back. A sprite of <line>s or a <bitmap> that has a heading is kept
		// once as it was written (Object::turnables, shared by every object
		// made from the one definition) and `bitmap` is that turned to the
		// heading, to the nearest whole degree, so what is shown and what a
		// pixel collision tests turn with it - see showHeading. That makes two
		// pictures an object has to hold, the original and the one it shows.
		bool hasHeading{ false };
		float heading{};
		float headingOriginal{};
		std::vector<std::shared_ptr<const Turnable>> turnables;

		// What `bitmap` was last drawn at: the whole degrees of the heading and
		// which of the turnables, so that a heading that has not moved by a
		// whole degree costs nothing. -1 for nothing drawn yet.
		int turnedDegrees{ -1 };
		std::size_t turnedFrame{};

		// An <animation>: the pictures it shows in turn (all one size, and
		// `bitmap` is whichever is showing), how many frames of the game each
		// one stays for (the seconds written, times the window's <framerate>),
		// how many it has been showing, and which one it is (an index into
		// animationBitmaps). Only an object that is shown and in play moves
		// on; a reset starts it again at the first. A sprite of one picture
		// has none of this. See advanceAnimation.
		std::vector<std::shared_ptr<const Bitmap>> animationBitmaps;
		int animationFrames{};
		int animationTick{};
		std::size_t animationIndex{};

		// Counts one frame of the game towards the next picture of the
		// animation, and shows it (marking the visual to be rebuilt) when its
		// time has come. Does nothing for an object with no animation.
		void advanceAnimation();

		// Back to the first picture, from the start of its time.
		void restartAnimation();

		// Points `bitmap` at the picture animationIndex names (turned to the
		// heading, if it turns) and marks the visual to be rebuilt.
		void showAnimationFrame();

		// Held turning, by Direction (only Left and Right are used): degrees a
		// frame, as activeThrust holds a thruster; and thrust held along the
		// heading, for each action that thrusts (a key held for it), with the
		// variable it burns, so that forward and back held together add up.
		// See playOneFrame.
		std::array<float, 4> activeTurn{};
		std::map<std::string, std::pair<float, std::string>> activeThrustAheadBy;

		// Which way an object with a <facing> is facing, one of the four
		// directions, without turning its picture (a <heading> turns it). It
		// follows the last <move>, <hop> or <jump> made, and a <fire> leaves the
		// middle of that side, moving that way at the projectile's own speed:
		// a man in a maze shoots the way he last walked, an invader facing
		// down drops its bombs below it. An object with no <facing> fires from
		// its top, at the projectile's own velocity, as before. A reset puts
		// back facingOriginal. See CommandExecutor::spawnProjectile.
		bool hasFacing{ false };
		Direction facing{ Direction::Up };
		Direction facingOriginal{ Direction::Up };

		// Its place on a <map> (see RawCell): the map's name ("" for an object
		// no map placed), its tile counting from 0, and the map's top left,
		// the size of a tile and how many tiles across and down it is. The
		// maps a walker finds its way on are made from these (buildMazes).
		std::string mapName;
		int mapColumn{};
		int mapRow{};
		Vector2f mapOrigin{};
		Vector2f mapTile{};
		int mapColumns{};
		int mapRows{};

		// A maze walker (<maze>): the map it walks (its name, and where it is
		// among the game's mazes), what stops it, and where it is headed now:
		// what its last <go>, <hunt> or <wander> said (Walk). It keeps to the
		// middle of a lane, stops at a wall and turns only where there is a
		// gap; a reset stops it. See play/maze.h.
		std::string mazeMap;
		std::vector<MazeWall> mazeWalls;
		std::size_t mazeIndex{};
		Walk walk;
		bool walksMaze() const noexcept { return !mazeMap.empty(); }

		// A <jump> under way: how far the object travels each frame, and how
		// many frames are left. While framesLeft is above 0 the object is in
		// the air: it touches no other object (playOneFrame). A reset
		// lands it where it starts.
		Vector2f jumpStep{};
		int jumpFramesLeft{ 0 };
		bool isAirborne() const noexcept { return jumpFramesLeft > 0; }

		// Standing, leaping and climbing. `grounded` is true for the frame
		// after a <land /> stood the object on something (playOneFrame
		// clears it as each frame's move starts, so it lasts exactly until the
		// next landing is worked out); a <leap> needs it. `leaping` is a leap
		// under way: the way across stays as it was at the take-off until the
		// next landing. `climbing` is on a ladder (playOneFrame): no
		// pull, no landing, no walking off. `activeClimb` is the step held up
		// (0) and down (1), and `climbClass` what it climbs.
		bool grounded{ false };
		bool leaping{ false };
		bool climbing{ false };
		std::array<float, 2> activeClimb{};
		std::string climbClass;

		// Where an <aim> pointed it: the way its next <fire>s go (a unit
		// vector), while hasAim. A move, hop or jump by key, or a reset,
		// drops it.
		bool hasAim{ false };
		Vector2f aim{};

		// A <follow> under way (see Path, Game::applyPaths): the path's name
		// (empty: on none), which of its legs is being flown, what is left of
		// that step to go, whether the leg has set off yet (its commands run
		// once, as it does), and how many frames are left to wait before the
		// first leg (a <stagger>). A reset, a <die /> or a <reveal> ends it.
		std::string followPath;
		std::size_t followLeg{};
		Vector2f followLeft{};
		bool followLegStarted{ false };
		int followWait{};
		bool isFollowing() const noexcept { return !followPath.empty(); }

		// The looks <become> switches between: each named sprite as the window
		// backends draw it (spriteParams, and the picture for a bitmap or a
		// drawing of lines), shared by every object made from the definition.
		// `look` is the one showing; a reset shows the first again. An object
		// with one sprite, or an animation, has none.
		struct Look
		{
			std::string name;
			std::vector<std::string> spriteParams;
			ShapeKind shapeKind{ ShapeKind::Unknown };
			std::shared_ptr<const Bitmap> bitmap;
		};
		std::vector<Look> looks;
		std::size_t look{ 0 };

		// The name of the look showing ("" for an object with none).
		const std::string& lookName() const;

		// Shows looks[index], and marks the visual to be built again.
		void showLook(std::size_t index);

		// The object's <timers>, which count while it is shown and in play
		// (see Timer, Game::updateTimers).
		std::vector<Timer> timers;

		// How much of its velocity an object keeps every frame is 1 minus this:
		// 0 (the default) is no drag at all, 0.02 loses a fiftieth of the speed
		// a frame, so a thruster can no longer push it faster than a top speed.
		float drag{};

		// Draws `bitmap` again at the current heading (and, for an animated
		// object, the picture that is showing) when that is not what it
		// already is, and marks the visual to be rebuilt. Does nothing for an
		// object that does not turn.
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
		// `riding` is the velocity of whatever it is riding (a collision rule
		// with <ride />, e.g. a frog on a log): worked out again every frame,
		// it counts for movement and for edge checks like velocity does, but
		// is not the object's own, so an object that steps off is at rest.
		// `hopPending` is a jump queued by a hop.*() action, made at the start
		// of the next frame's move (playOneFrame); `hopped` is true for
		// the rest of that frame, so an object that has just landed somewhere
		// still counts as moving for the collisions it is now part of.
		Vector2f riding{};
		Vector2f hopPending{};
		bool hopped{ false };

		// What it carries (<carry />): the name of one other object, which
		// goes where it goes, held where it was when it was picked up
		// (`carryOffset` from this object's top left), until a <drop />, until
		// something else is picked up, or until either is out of play
		// (playOneFrame). `dropped` is what it last let go of: not picked up
		// again until the two have stopped touching, so it can be walked away
		// from.
		std::string carrying;
		Vector2f carryOffset{};
		std::string dropped;

		// The object's own measured bounding box (width/height) - {0,0} until
		// whichever Window backend is running has actually built this
		// object's visual and measured it (see Window::init() in window/window.h).
		// Circle/rectangle sizes are implied by spriteParams alone; text/
		// image need the backend's own font/image loading to know their real
		// pixel size, so every backend writes this back the same way
		// regardless - collision/physics code (CollisionDetector,
		// CommandExecutor) only ever reads this field, never a backend type.
		Vector2f size{};

		// True once a Window backend has measured `size` at least once (see
		// finalizeVisual() in window/sfml.cpp/window_raylib.cpp/
		// window/sdl2.cpp) - stays true from then on, even across a later
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

		// A cell's offset from its group's evaluated <position> (see RawCell),
		// so that position can be worked out again later and the offset added
		// back.
		Vector2f cellOffset{};

		// Whether its start (position, velocity or a <variable>) draws a
		// <random>: a reset then draws it again (Game::drawStartAgain).
		bool startDrawsRandom{ false };

		CollisionData collisionData;
		std::vector<std::string> spriteParams;
		ShapeKind shapeKind{ ShapeKind::Unknown };

		// What a sprite of <line>s was drawn as, worked out when the game
		// loads (load/expr). The window backends draw it as a picture and a
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

		// Set when this is a text object that shows a variable (e.g. a <text>
		// sprite whose <number> is paddle1.score): every piece of the text,
		// those that follow a variable with its owner and name, the rest as
		// they were worked out, so it can be put together again whenever one
		// of its variables changes (Game::relabel). Empty means the text shows
		// no variable and only ever changes by an <inc /> naming the text
		// object itself (the older pattern for a text that counts its own
		// number).
		std::vector<TextPiece> textPieces;

		// Whether one of the pieces shows that variable of that object.
		bool showsVariable(const std::string& ownerName, const std::string& variableName) const noexcept
		{
			return std::any_of(textPieces.begin(), textPieces.end(),
				[&](const TextPiece& piece) { return piece.owner == ownerName && piece.variable == variableName; });
		}

		friend std::ostream& operator<<(std::ostream& o, Object const& f);
	};

	// A <map> (load/xml's readMap; a generated game's map function): its
	// name, its rows of characters, a character a tile, where its top left
	// is, and the size of a tile.
	struct RawMap
	{
		std::string name;
		std::vector<std::string> rows;
		RawVector2 position;
		RawValue tileWidth;
		RawValue tileHeight;
	};

	// The tiles of a map that show this character, row by row and each row
	// left to right: column, then row, counting from 0.
	std::vector<std::pair<std::size_t, std::size_t>> tilesOf(const RawMap& map, char character);

	// Puts an object on a tile of the map (column and row from 0): at the
	// map's <position>, which load/expr's layOutCells moves it on from to
	// the middle of the tile. The map's name means it too, outermost of
	// its names.
	void placeOnMap(RawObject& object, const RawMap& map, std::size_t column, std::size_t row);

	// Gives every text with a style= what it leaves out of that style: its
	// size and its color. A style no <style> has, or a text left with no
	// size, stops the load.
	void applyStyles(std::vector<RawObject>& objects, const std::vector<RawStyle>& styles);

	// Makes each <centered> of an object's <position> the expression it
	// means: where its middle goes, less half its own width (or height),
	// named by the object's final name (so after layOutScenes).
	void centerOnOwnSize(std::vector<RawObject>& objects);

	// What an object with no <sprite> is drawn as, which is never: an empty
	// rectangle (it only holds numbers; see RawObject::isDrawn).
	RawSprite nothingSprite();

	// Shows the object's look of that name, and starts over the timers that
	// follow it (Timer::sprite); false (and nothing changes) if it has none
	// of that name.
	bool showLookNamed(Object& object, const std::string& name);
}
