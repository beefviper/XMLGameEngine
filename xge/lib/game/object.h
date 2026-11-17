// object.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "picture/bitmap.h"
#include "game/command.h"
#include "game/condition.h"
#include "game/types.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <limits>
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
		// <scale>: the window is this many times the game's width and
		// height, each of the game's pixels drawn as a square of them. The
		// game itself never sees it: positions, sizes and the mouse are in
		// the game's own pixels.
		int scale{ 1 };
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
		int line{ 0 }; // of its <collision> tag in the game file; 0 when not known (a generated program)
		std::string filterClass;
		std::string filterObject;

		// unless="class": the rule does not run while the object is at the
		// same moment touching something of that class - "water kills the
		// frog, unless it is also on a log". Empty means no exception.
		std::string unlessClass;

		// through="class": the rule runs only for a touch felt through
		// something of that class (a <knock> through a floor, see
		// CommandExecutor::knock), and a rule without it only for a touch
		// itself. Empty means a touch itself.
		std::string throughClass;

		// sprite="name": the rule only runs while the object running it shows
		// its sprite of that name (see <become>). Empty means whatever it shows.
		std::string whileSprite;

		// side="...": the rule only runs when the object is touched on that
		// side of its own (top, bottom, left, right, horizontal for left or
		// right, vertical for top or bottom). Empty means any side.
		std::string side;

		// hitbox="name", by="name": the rule only runs for a touch made with
		// this object's hitbox of that name, or by the other's (each one
		// worked out where the two meet; see touchesWith). Empty: any part.
		std::string hitbox;
		std::string by;

		// <condition>s, <all>s and <any>s before the commands: the rule only
		// runs while every one holds. Gathered as an <all> (join All); join
		// None means there are none. A <condition> with no class= or object= is
		// about the object running the rule (see nameSelf).
		RawCondition check;

		std::vector<RawCommand> commands;

		// For a rule that names `other` (the object it touched): every
		// object of the file it can touch, in full (see nameOther). Its
		// commands and test are made and checked at load for each, and kept
		// as made for the first; as it runs, other is the one touched.
		// Empty for a rule that does not name other.
		std::vector<std::string> touchable;
	};

	// How an object's shape is tested against others: as its bounding box
	// (Box, the default; a circle as a circle) or by the pixels actually drawn
	// (Pixel) - see play/collision.h.
	enum class CollisionType { Box, Pixel };

	// A <collision edge="...">, as its commands in an edge's list: `count` of
	// them from `first`. A verb refused ends the rule there. With sprite= or
	// unless= it runs only while the object shows that look and is not
	// touching anything of that class, looked at once as the rule comes (as
	// for a rule about another object).
	struct EdgeGuard
	{
		std::size_t first{ 0 };
		std::size_t count{ 0 };
		std::string sprite;
		std::string unlessClass;
		// Its <condition>s and <any>s, gathered as an <all> (join None when
		// it has none), and what they are made into: it runs only while it holds.
		RawCondition rawCheck;
		std::optional<Condition> check;
	};

	// One screen edge's rules: their commands in file order, and where each
	// rule is among them (guards). `Command` is a RawCommand as written, or
	// the Command it is made into.
	template <typename CommandType>
	struct EdgeRules
	{
		std::vector<CommandType> commands;
		std::vector<EdgeGuard> guards;
	};

	// The four screen edges' rules, by Edge (edges[edgeIndex(Edge::Left)]).
	template <typename CommandType>
	using EdgesRules = std::array<EdgeRules<CommandType>, 4>;

	constexpr std::size_t edgeIndex(Edge edge) noexcept { return static_cast<std::size_t>(edge); }

	// A <hitbox> as written: a "rectangle" (width by height) or a "circle"
	// (radius), its top left x, y from the picture's (each empty text when
	// left out: 0); or "inset", the picture's box pulled in from each side
	// by top, ...: each the most particular of the side's words given
	// (<top>, then <vertical>, then <all>), empty text when none (0).
	struct RawHitbox
	{
		std::string name;           // name=: what rules pick it by (empty: none)
		std::string kind;
		RawValue width;
		RawValue height;
		RawValue radius;
		RawValue x;
		RawValue y;
		RawValue top;
		RawValue bottom;
		RawValue left;
		RawValue right;
	};

	// One, worked out: an inset one, or a box (round: a circle filling it)
	// offset from the picture's top left.
	struct Hitbox
	{
		std::string name;
		bool inset{ false };
		float top{ 0.0f };
		float bottom{ 0.0f };
		float left{ 0.0f };
		float right{ 0.0f };
		Vector2f offset{};
		Vector2f size{};
		bool round{ false };
	};

	struct RawCollisionData
	{
		bool enabled{ false };
		bool lockstep{ false };
		CollisionType type{ CollisionType::Box };

		// <solid char="*" />s: with type pixel, the characters of a <bitmap>
		// whose pixels touch (a cube's top, not its sides); empty, all of them.
		std::string solid;

		// Its <hitbox>es: what it touches other objects with instead of its
		// picture, any of them a touch. None: the picture.
		std::vector<RawHitbox> hitboxes;

		// Each screen edge's <collision edge="..."> rules (addEdgeRule).
		EdgesRules<RawCommand> edges;
		// Rules about another object: a <collision> with a class= and/or
		// object= selector, or with no selector at all (matches anything), as
		// against the window's edges above.
		std::vector<RawCollisionRule> others;

		friend std::ostream& operator<<(std::ostream& o, RawCollisionData const& f);
	};

	// A parsed, ready-to-run version of RawCollisionRule.
	struct CollisionRule
	{
		int line{ 0 }; // RawCollisionRule::line
		std::string filterClass;
		std::string filterObject;
		std::string unlessClass;
		std::string throughClass;          // see RawCollisionRule

		std::string whileSprite;
		std::vector<Edge> sides;           // empty: any side
		std::string hitbox;                // see RawCollisionRule
		std::string by;
		std::optional<Condition> check;    // see RawCollisionRule::check
		std::vector<Command> commands;
		// For one naming other: its commands as made for each other thing
		// it can touch (RawCollisionRule::touchable), only until the load's
		// checks have seen them.
		std::vector<std::vector<Command>> loadChecks;
	};

	struct CollisionData
	{
		// (Whether they are on is the object's Moment::collisionsOn.)
		int lockstep{ 0 };
		CollisionType type{ CollisionType::Box };

		// Each screen edge's rules (see RawCollisionData::edges).
		EdgesRules<Command> edges;
		// Rules about another object (see RawCollisionData::others).
		std::vector<CollisionRule> others;

		friend std::ostream& operator<<(std::ostream& o, CollisionData const& f);
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

	// One <polygon> of a sprite of filled shapes: its corners in order, in
	// pixels from the sprite's top left, and its colour.
	struct RawPolygon
	{
		std::vector<RawVector2> points;
		std::string color;           // "" means color.white
	};

	// An object's <sprite>, as written: one shape, a drawing made of <line>s,
	// or a picture written as rows of text.
	// Which of the fields are used depends on `kind` (circle, rectangle, text,
	// image, line, polygon, bitmap or svg).
	// One piece of a <text>, in the order written: a fixed label
	// (<content>), a number (<number>) or a number shown as a letter
	// (<letter>, 0 is A, 25 is Z, and round again).
	struct RawTextPiece
	{
		enum class Kind { Words, Number, Letter, Choice };
		Kind kind{ Kind::Words };
		std::string words;           // Words
		RawValue value;              // Number, Letter, Choice (which, from 0)
		std::vector<std::string> choices; // Choice: its <word>s
	};

	struct RawSprite
	{
		std::string kind;

		// <sprite name="...">: what an <animation>'s <frame> calls it. Needed
		// when an object has more than one sprite.
		std::string name;

		std::vector<RawLine> lines;  // line: one or more
		std::vector<RawPolygon> polygons; // polygon: one or more

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
		RawValue width;              // rectangle; cell: the size it is made (an item)
		RawValue height;             // rectangle; cell
		RawValue size;               // text

		// A text: its pieces, in order (see RawTextPiece); a <number> or a
		// <letter> that is just an owner.variable stays live (see
		// Object::Drawn::textPieces).
		std::vector<RawTextPiece> text;

		std::string path;            // image, svg

		// cell: a picture cut from the game's <sheet> called `sheet`, the
		// cell at `column` and `row` (from 1), or with hasSvgRegion the part
		// at svgX, svgY, svgWidth by svgHeight; drawn `scale` pixels to the
		// sheet's, or (a pack's faces) made `width` by `height` when given.
		std::string sheet;
		RawValue column;
		RawValue row;

		// tether: the two objects whose middles
		// the line joins, drawn again every frame, and how thick it is
		// (empty text: 1).
		std::string tetherFrom;
		std::string tetherTo;
		RawValue thickness;

		std::string flip;            // image: "", "horizontal" or "vertical"
		std::string color;           // "" means color.white (or the style's)
		std::string font;            // text: a font file; "" means the style's, or assets/tuffy.ttf

		// text and bitmap: style="...", a <style> of the game's <styles>,
		// which gives what the sprite leaves out (see applyStyles).
		std::string style;
		// bitmap: what applyStyles took from the style, the plain color and
		// the characters, which a <become style> swaps for another's.
		bool styleColor{ false };
		std::string styleChars;

		// bitmap: the characters whose pixels a pixel collision touches with,
		// copied from the object's <solid char>s (see markSolid); empty, all.
		std::string solid;

		// A look that moves, a named <animation>: the sprites its <frame>s
		// pick, in order, each shown `interval` seconds (or its own, in
		// `times`), round and round, or through one time (`once`). The fields
		// above are its first frame's. Empty for a still look.
		std::vector<RawSprite> frames;
		RawValue interval;
		std::vector<RawValue> times;
		// Each frame's own <hitbox>es, one list a frame, or none at all when
		// no frame has any: touched with too while that frame shows.
		std::vector<std::vector<RawHitbox>> frameHitboxes;
		std::vector<std::vector<RawCommand>> frameCommands;   // as RawFrames::frameCommands
		bool once{ false };
		bool stepped{ false };   // as RawFrames::stepped
		// <then>: the look a once look goes to when it ends (empty, the look
		// before it), and the commands it runs then.
		std::string thenSprite;
		std::vector<RawCommand> thenCommands;
	};

	// An <animation> as written: its name (empty for an object's one
	// animation, a look's name for a look that moves), how long each picture
	// is shown, and the names of the sprites it shows, in order.
	struct RawFrames
	{
		std::string name;
		RawValue interval;
		std::vector<std::string> frames;
		// Each frame's own <seconds>, one a frame, or none at all when no
		// frame has its own; an empty value is the animation's (see
		// frameTimeGiven).
		std::vector<RawValue> times;
		// Each frame's own <hitbox>es, one list a frame, or none at all when
		// no frame has any: touched with too while that frame shows.
		std::vector<std::vector<RawHitbox>> frameHitboxes;
		// Each frame's own commands, one list a frame, or none at all when no
		// frame has any: run as the animation moves on to that picture.
		std::vector<std::vector<RawCommand>> frameCommands;
		// <once />: plays through one time, not round and round.
		bool once{ false };
		// <stepped />: the object moves only as the picture changes.
		bool stepped{ false };
		// <then sprite="...">: the look it goes to when it ends, and the
		// commands it runs then. Only with once.
		std::string thenSprite;
		std::vector<RawCommand> thenCommands;
		// Frames cut from a sheet (<cell>s and runs of <cells>), named for
		// the animation and numbered ("walk.1", "frame.1" for an unnamed
		// one), each also among `frames`: sprites the object gains.
		std::vector<RawSprite> cells;
	};

	// A game's <style name="...">: what a <text> (a size and a color) or a
	// <bitmap> (a scale and colors) can take by style="..." instead of saying
	// them. Any may be left out.
	struct RawStyle
	{
		std::string name;
		RawValue size;               // empty: the style gives no size
		std::string color;           // empty: the style gives no color
		std::string font;            // a text's font file; empty: the style gives none
		// A bitmap's: its <scale>, and its other colors, a character each.
		bool hasScale{ false };
		RawValue scale;
		std::vector<std::pair<char, std::string>> bitmapColors;
	};

	// An object's <animation>: the sprites it shows one after the other, each
	// for `interval` seconds, round and round. The frames are the object's own
	// <sprite>s picked out by name, in the order the <frame>s are written.
	struct RawAnimation
	{
		RawValue interval;
		std::vector<RawSprite> frames;
		std::vector<RawValue> times;   // as RawFrames::times
		std::vector<std::vector<RawHitbox>> frameHitboxes;   // as RawFrames::frameHitboxes
		std::vector<std::vector<RawCommand>> frameCommands;   // as RawFrames::frameCommands
		bool once{ false };            // as RawFrames::once
		bool stepped{ false };         // as RawFrames::stepped
		std::vector<RawCommand> thenCommands;   // as RawFrames::thenCommands
	};

	// Whether a frame's entry in `times` holds a time of its own, rather
	// than standing for the animation's.
	inline bool frameTimeGiven(const RawValue& time)
	{
		return time.kind != RawValue::Kind::Expression || !time.text.empty() || time.operations;
	}

	// One of a cell map's <checks> (see RawCell::cells): what it asks of the
	// map after something settles, and what it does for each answer. "full"
	// is a row every cell of which holds a loose piece (of the class, when
	// it names one).
	struct RawCheck
	{
		std::string kind;
		std::string objClass;
		std::vector<RawCommand> commands;
		// A <line>'s <atleast>: how many alike in a row.
		int count{ 0 };
		// <straight />: across and down only, no slants.
		bool straight{ false };
		// <alike variable="hue" />: alike by that variable's value, not by group.
		std::string alike;
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
		std::string columnName;      // <columns name=>: what its column is also called (an item's rank)
		std::string rowName;         // <rows name=>: what its row is also called (an item's suit)
		RawSprite slot;              // its row's sprite, as the row first shows it
		RawValue gapBefore;          // the gap before its column, in its row (0 in column 1)
		RawValue gapAbove;           // the gap above its row (0 in row 1)
		RawValue drop;               // how much lower each column is than the one before it (<slope>; empty: 0)

		// A place on a <map> (load/xml's readMap), when mapColumn is above 0:
		// the map's name, the tile's column and row counting from 1, how many
		// columns and rows the map has, and the size of a tile. The object
		// sits in the middle of its tile (load/expr::layOutCells), measured
		// from the map's <position>, which is the object's own.
		std::string map;
		int mapColumn{ 0 };
		int mapRow{ 0 };
		// Its character is on two tiles side by side, and it starts between
		// them (half a tile right of mapColumn), as the arcade's Pac-Man does.
		bool between{ false };
		int mapColumns{ 0 };
		int mapRows{ 0 };
		RawValue tileWidth;
		RawValue tileHeight;

		// On a map with <cells />: its pieces move a cell at a time, a move
		// that is refused is a touch, and what the map asks after something
		// settles on it (its <checks>). Every piece of the map carries them.
		bool cells{ false };
		std::vector<RawCheck> checks;

		// The character of the map's rows that picked it ('\0' for none).
		char character{};
	};

	// One <wall class="wall" sprite="normal" /> of a <maze>: the objects of
	// that class on the map stop the walker, while it shows that look (any
	// look, when sprite is empty).
	struct MazeWall
	{
		std::string objClass;
		std::string sprite;
	};

	// A <slow class="tunnel">0.5</slow> of a <maze>: on a tile holding
	// something of that class, the walker goes at that share of its speed.
	struct MazeSlow
	{
		std::string objClass;
		float share{ 1.0f };
	};

	// An object's <maze map="maze">: the <map> whose tiles it walks ("" for
	// an object that walks none), what is a wall to it, and where it goes
	// slower. See play/maze.h.
	struct RawMaze
	{
		std::string map;
		std::vector<MazeWall> walls;
		std::vector<MazeSlow> slows{};
	};

	// What a holder <accepts> on top (<condition>s on the arriving member,
	// total="count" <equal>1 for one at a time), and what may start it
	// when it is empty (<empty>'s <condition>s; none: anything). A holder
	// with no <accepts> takes nothing but what is put back on it.
	struct HolderRule
	{
		bool given{ false };
		bool one{ false };
		// <follow />: a trick. Any item leads (the first on the empty
		// holder); after it, an item of the suit led, unless the hand laying
		// holds none. <breaks>: suits that may not lead until one has been
		// laid on the holder since the items were gathered, unless the hand
		// holds nothing else. <trumps>: an item of these suits beats every
		// item of the others, whatever was led.
		// trickBy: the variable followed, as written.
		bool follow{ false };
		std::string trickBy;
		std::vector<std::string> breaks;
		std::vector<std::string> trumps;
		// <accepts> of a <holds>: rules on the arriving member's variable.
		// `kind` compares it with `amount`, or, `top`, with the top one's
		// `topVariable` (empty: the same variable) plus `amount`
		// (<equal>top.rank - 1</equal>). Total: what is laid at once, that
		// variable added up, is `amount`. `rules` hold while something is
		// held, `emptyRules` (<empty>) for the first.
		struct VarRule
		{
			enum class Kind { Equal, Unequal, Above, Below, AtLeast, AtMost, Total };
			Kind kind{ Kind::Equal };
			std::string variable;
			float amount{};
			bool top{ false };
			std::string topVariable;
		};
		std::vector<VarRule> rules;
		std::vector<VarRule> emptyRules;
		// <any>: at least one of these holds too. <wild variable=>: what
		// keeps all of `wildRules` goes on anything, and the holder then
		// waits for a call: the one who laid it names a value of
		// `wildVariable` (from 1), which its top then counts as.
		std::vector<VarRule> anyRules;
		std::vector<VarRule> wildRules;
		std::string wildVariable;
	};

	// A held member (an item; see load/expr.cpp): the worths it may count
	// for, lowest first, and the holder it starts in.
	struct ItemOf
	{
		std::string startHolder;
		std::vector<float> worths;
	};

	// <patterns name="poker">: what a holder's items may make, the lowest
	// first; a holder with <score patterns="poker"> is scored by them. Each
	// <pattern> is one or more parts that must all hold among the items
	// scored: <alike variable>N (N items with one value of the variable;
	// two <alike>s of one variable, two values) and <run variable>N (N
	// values in a row; by worth, an item counting any of its worths).
	struct RawPatterns
	{
		enum class Part { Alike, Run };
		struct Pattern
		{
			std::string name;
			std::vector<std::pair<Part, int>> parts;
			std::vector<std::string> by;    // each part's variable=, as written
		};
		std::string name;
		std::vector<Pattern> patterns;
	};

	struct RawObject
	{
		std::string name;
		int line{ 0 }; // of its tag (a member: its group's) in the game file; 0 when not known (a generated program)
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

		// <hidden />: in play, moving and colliding, but never drawn
		// (an exit, a trigger zone).
		bool isDrawn{ true };

		// <exists>false</exists>: written, so rules and spawns can name it,
		// but not in the game until a <spawn> makes one (Object::exists).
		bool exists{ true };

		// <most>: how many spawned of it can exist at once (0: no limit).
		int most{ 0 };
		RawVector2 rawPosition;
		RawVector2 rawVelocity;
		RawVector2 rawAcceleration;  // only when hasAcceleration
		bool hasAcceleration{ false };
		RawValue rawHeading;         // only when hasHeading
		bool hasHeading{ false };
		RawVector2 rawPivot;         // only when hasPivot: the point of the picture its heading turns about
		bool hasPivot{ false };
		RawValue rawDrag;            // only when hasDrag
		bool hasDrag{ false };
		RawValue rawFriction;        // only when hasFriction
		bool hasFriction{ false };
		RawValue rawMass;            // only when hasMass
		bool hasMass{ false };
		// <surface>: ground, and the drag and grip it gives (each only when
		// its has... is true).
		bool hasSurface{ false };
		RawValue rawSurfaceDrag;
		bool hasSurfaceDrag{ false };
		RawValue rawSurfaceGrip;
		bool hasSurfaceGrip{ false };
		// <pull object="sun">strength</pull>s: what pulls it, and how hard.
		std::vector<std::pair<std::string, RawValue>> rawPulls;
		// <seek object="ship">degrees</seek>: what it turns toward, and how
		// fast; seekTarget empty for none.
		std::string seekTarget;
		RawValue rawSeek;
		// <light>radius</light>, only when hasLight.
		RawValue rawLight;
		bool hasLight{ false };
		// <light>'s <past class>es: what its light does not shine through.
		std::vector<std::string> lightPast;
		// <dark>unlit</dark> or <dark>unseen</dark>; "" for neither.
		std::string dark;
		// <altitude> with its <start>, <rise>, <fall> and <bounce>, each
		// optional; only when hasAltitude.
		bool hasAltitude{ false };
		RawValue rawAltitude;
		RawValue rawRise;
		RawValue rawFall;
		RawValue rawAltitudeBounce;
		// <shadow object="ball" />: what it is the shadow of; "" for nothing.
		std::string shadowOf;
		std::string facing;          // "" when the object has no <facing>
		// <poses>: the look for each way it can be moving, by the pose's
		// name (standing, walking, skidding, rising, falling); empty for none.
		std::map<std::string, std::string> poses;
		// <joins>: on a cell map, the look for where the piece it is joined
		// to lies (alone, left, right, up, down); empty for none.
		std::map<std::string, std::string> joins;
		// <mirror>left</mirror>: going that way, its pictures are drawn
		// flipped left to right; "" for never.
		std::string mirror;
		// <grip>: how much of the way to what its moves ask for its velocity
		// goes each frame, only when hasGrip; otherwise 1, all of it.
		RawValue rawGrip;
		bool hasGrip{ false };
		// <moves>: on a map with <cells />, the moves a piece of a board
		// game may make when <place>d (see Object::MoveSet); a set with a
		// sprite= is the one for that look, the one without for the others.
		struct RawPieceMove
		{
			bool jump{ false };
			std::string objClass;   // a jump's: what it may go over
			int x{};
			int y{};
		};
		struct RawMoveSet
		{
			std::string sprite;
			bool forced{ false };
			std::vector<RawPieceMove> moves;
			std::string promote;    // <promote sprite=>: the look it takes...
			int promoteRow{};       // ...on reaching this row
			struct RawFlank
			{
				std::string objClass;   // what it traps in a line
				bool straight{ false }; // across and down only
			};
			std::vector<RawFlank> flanks;   // <flank>: a new piece is <put> where it traps
			bool fall{ false };             // <fall />: a piece put drops down its column
			std::vector<std::string> on;    // <on class>: a piece put goes only on one of these, in its place
			std::string closeClass;          // <close class object>: a put walling in a piece of closeClass...
			std::string closeObject;         // ...puts a copy of closeObject in its cell
		};
		std::vector<RawMoveSet> moveSets;
		// <marks>: on a picker, the pieces it shows on a map of cells for
		// what it holds and can do (see Object::MarkSet); a set with a
		// sprite= while it shows that look, the one without always.
		struct RawMark
		{
			std::string kind;       // held, moves, movable, put, last
			std::string object;     // what is put in each cell marked
			std::string objClass;   // movable, put: whose pieces
			std::string variable;   // only while this is not 0 ("" always)
		};
		struct RawMarkSet
		{
			std::string sprite;
			std::vector<RawMark> marks;
		};
		std::vector<RawMarkSet> markSets;
		// <holds>: a place that holds members of a class (Object::Holder), as written.
		struct RawHolder
		{
			bool is{ false };
			RawValue spreadX;           // <spread>: each item on from the one below (empty text: 0)
			RawValue spreadY;
			RawValue lookSpreadX;       // <spread sprite>: each item showing that look instead
			RawValue lookSpreadY;
			RawValue startCount;        // <starts>: what it is sent at the start (empty text: none)
			RawValue startMore;         // a group's: each holder after the first, this many more
			bool startShuffled{ false }; // <starts> > <order group><random />: what starts in it is shuffled
			std::string pickedInto;     // <picked><send into>N: picking from it sends that many there (none: into the hand's own holder)
			RawValue pickedSends;
			bool hasPickedSend{ false };
			std::string emptyFrom;      // <picked><empty><send from>: picking from it empty brings that holder's back
			bool emptyRandom{ false };  // <empty><order><random /></order>: put back in random order, not turned over
			RawValue emptyLeave;        // <empty><send><leave>: that many of the top items stay (empty text: none)
			RawValue spreadWidth;       // <spread><width>: the spread is never wider (empty text: no limit)
			bool carry{ false };        // <carry />: a face-up item can be taken with all above it
			RawValue lift;              // <lift>: the item a hand points at, lifted out of the fan by that much (empty text: none)
			bool hasPoints{ false };    // <counts>: what its face-up items add up to
			RawValue pointsMost;        // <counts><atmost>: highs only while the total stays at most this (empty text: no limit)
			std::string score;          // <score patterns=>: scored by those patterns (empty: not scored)
			std::string scoreWith;      // <score><with object=>: with that holder's items too
			int scoreFull{};            // <with><full>: that holder holds this many when all are dealt (0: as it is)
			int scoreBest{};            // <score><best>: the best this many of the items (0: all)
			std::string melds;          // <score patterns=><melds />: its items laid out in groups that make those patterns (empty: none)
			int meldsBest{};            // <score><best> with <melds />: the best this many of the items (0: all)
			HolderRule takes;
			// <holds class sprite>: the class it holds, as written, and the
			// looks its words name: the look laid members show (none: each
			// keeps its own), the one a second <spread> is for, and
			// <top sprite>'s, which a member left on top turns to.
			std::string holdsClass;
			std::string holdsSprite;
			std::string spreadSprite;
			std::string topSprite;
			std::string pickedSound;    // <picked><play sound>: sounded when picking from it sends
			std::vector<std::string> orderBy; // <order><by variable>s: kept in order by these, the first first
			std::string counts;         // <counts variable>: the holder's own variable of that name, its face-up members' added
		};
		RawHolder holder;
		// <owns object=>: the holder of its own (a player's items), as
		// written.
		// <turns sprite=> on a group: its members take turns, in order,
		// the first first; one off its turn shows that look (none: as it
		// is). <think> on a member: the computer plays it.
		bool takesTurns{ false };
		std::string turnsSprite;
		std::vector<RawCommand> think;
		bool hand{ false };             // a picker of a held class: points at holders and carries what it picks (Object::Hand)
		std::string handHolder;         // <owns object=>: its own holder (items held by a player)
		std::string ownsClass;          // <owns class=>: the pieces a player of a board game moves
		int handSeat{};                 // its seat, a players group member's number: which player, for a trick's `winner` (0 none)
		int handTeam{};                 // <team>: the side it plays for, partners alike (0 none)
		int handHolds{ 1 };             // <pick><most>: from how many holders it may hold one at once
		ItemOf item;                    // a held member's worths and start (empty for anything else)
		bool held{ false };
		RawMaze maze;                // <maze>: the map it walks, if any
		// <shape>: on a cell map, the cells it fills, a <row> of characters
		// each, top to bottom; and the object of the map each character
		// picks, by its name. A character nothing picks is an empty cell.
		// Empty for an object that is no shape.
		std::vector<std::string> shapeRows;
		std::map<char, std::string> shapePieces;
		// <chain object="body" />: on a map with <cells />, the pieces of
		// that name or group behind it follow its trail (empty for none).
		std::string chain;
		std::vector<RawTimer> timers;
		RawCollisionData rawCollisionData;
		std::map<std::string, std::vector<RawCommand>> action;
		std::map<std::string, RawValue> variable;

		// The <group> this object is a member of, or empty for a plain
		// <object>. A group is written once in the file and read as one
		// RawObject per member (see load/xml.cpp), so from here on a member
		// is an ordinary object that also remembers which group it came from.
		std::string groupName;
		// Its group's number, the same for all its members (see Kind); 0
		// for a plain <object>, which is a kind of its own.
		int kind{ 0 };
		// The groups around that group, outermost first, when groups are
		// written inside one another; each of their names means this object too.
		std::vector<std::string> outerGroups;

		// Whether `group` is this object's group or one around it.
		bool inGroup(const std::string& group) const
		{
			return !group.empty() && (groupName == group || std::find(outerGroups.begin(), outerGroups.end(), group) != outerGroups.end());
		}

		// Whether `name` names it: its own name, or a group it is in.
		bool named(const std::string& wanted) const { return name == wanted || inGroup(wanted); }

		// Where it goes in its group's <columns> and <rows>, when it is a cell.
		RawCell cell;

		// A cell's sheet <cell>s that leave out a column or a row take the
		// cell's own (a pack's faces are a sheet's grid): its sprite, looks
		// and animation frames. Made once the cell knows its place.
		void takeOwnCell()
		{
			const auto own = [&](RawSprite& each)
			{
				if (each.kind != "cell" || each.hasSvgRegion) { return; }
				const auto unset = [](const RawValue& value) { return value.kind == RawValue::Kind::Expression && value.text.empty(); };
				if (unset(each.column)) { each.column = RawValue::expression(std::to_string(cell.column)); }
				if (unset(each.row)) { each.row = RawValue::expression(std::to_string(cell.row)); }
			};
			own(sprite);
			for (RawSprite& look : looks) { own(look); }
			for (RawSprite& frame : animation.frames) { own(frame); }
		}

		// A cell's row and column are variables of its own, `row` and
		// `column`, and the same under the names its group gives them
		// (<rows name="suit">). Returns a name its group gives that it has
		// a variable of already (none: empty), which it keeps.
		std::string numberCell()
		{
			std::string clash;
			for (const auto& [called, number] : { std::pair{ std::string("row"), cell.row }, std::pair{ std::string("column"), cell.column },
				std::pair{ cell.rowName, cell.row }, std::pair{ cell.columnName, cell.column } })
			{
				if (called.empty() || variable.count(called)) { if (!called.empty() && called != "row" && called != "column" && clash.empty()) { clash = called; } continue; }
				variable[called] = RawValue::expression(std::to_string(number));
			}
			return clash;
		}

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

		// A <hunt> with <search />: the steps from each tile to the tile its
		// target was on (mazeDistances), worked out again when the target
		// moves to another tile.
		int searchColumn{ -1 };
		int searchRow{ -1 };
		std::vector<int> searchSteps;
	};

	// A maze walker (<maze>): the map it walks, what stops it, what slows
	// it, and where it is headed now: what its last <go>, <hunt> or
	// <wander> said. It keeps to the middle of a lane, stops at a wall and
	// turns only where there is a gap; a reset stops it. A part
	// (Parts::mazes), for an object written with <maze>. See play/maze.h.
	struct Maze
	{
		std::string map;
		std::vector<MazeWall> walls;
		std::vector<MazeSlow> slows;
		Walk walk;
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
		std::vector<std::string> choices;
	};

	// What a number shows as a piece of that kind: a whole number, a
	// letter (0 is A, 25 is Z, and round again: 26 is A, -1 is Z), or the
	// word of `choices` it picks (0 the first; below 0 the first, past the
	// end the last).
	std::string showPiece(RawTextPiece::Kind kind, float value, const std::vector<std::string>& choices = {});

	// What a text of these pieces reads: each piece a word, one space
	// between, except that letters side by side make one word (initials).
	std::string joinText(const std::vector<TextPiece>& pieces);

	// What held keys drive, each by Direction (static_cast<size_t>; its
	// four real values, Up/Down/Left/Right). Set by an action's verb while
	// its key is held, cleared when it is let go (see playOneFrame). A part
	// (Game::parts().drives), given on the first key that drives it.
	struct Drive
	{
		// A <move>'s step held each way: 0 for a way not held, else the
		// magnitude of the action's own step.
		// CommandExecutor::applyActionVelocity recombines all four into
		// velocity.x/velocity.y on every key transition, so holding Down
		// and tapping Up cancels out to a standstill, and releasing Up
		// afterward resumes moving Down, rather than leaving it stopped.
		// Also what lets two axes combine into an 8-way diagonal from a
		// 4-way D-pad.
		std::array<float, 4> moveStep{};
		// A <thrust direction> held each way, and the variable (if any)
		// each one burns.
		std::array<float, 4> thrust{};
		std::array<std::string, 4> thrustBurn{};
		// Turning held (only Left and Right are used): degrees a second.
		std::array<float, 4> turn{};
		// Thrust held along the heading, for each action that thrusts,
		// with the variable it burns, so that forward and back held
		// together add up.
		std::map<std::string, std::pair<float, std::string>> thrustAhead;
	};

	// A hand: the holder it points at (its full name) and the item of it
	// (index from the bottom; -1: the holder itself, empty), the items it
	// holds and the holder they came from, and whether the mouse moves it
	// (the items then follow the pointer, `grab` off it). A part
	// (Parts::hands), for an object whose action picks up held items.
	struct Hand
	{
		std::string own;        // holder=: its own holder, as written, or none
		int seat{};             // <seat>: which player (0 none)
		int team{};             // <team>: its side; a partner is another hand on it (0 none)
		int holds{ 1 };         // <holds>: from how many holders it holds items at once
		// Holding items from more than one holder: each item taken after
		// the first, and the holder it came from.
		std::vector<std::pair<int, std::string>> also;
		std::string holder;
		int at{ -1 };
		std::vector<int> held;
		std::string from;
		bool mouse{ false };    // moved by the mouse last, not the keys
		bool over{ false };     // the mouse is over the holder pointed at (or an item of it)
		Vector2f grab{};
	};

	// Items (game/holders.cpp). A holder, a part (Parts::holders): its rules
	// and the items it holds, bottom first (each an item's number, Game::itemAt), and its live
	// numbers. Laid out by Game::layOutItems, never moved by the frame.
	struct Holder
	{
		std::string holdsClass;   // <holds class sprite>, as written
		std::string holdsSprite;
		std::string topSprite;    // <top sprite>: what a member left on top shows
		Vector2f spread{};        // <spread>: each item on from the one below
		Vector2f lookSpread{};    // <spread sprite>: each item showing that look
		std::string spreadSprite;
		int startCount{};
		int startMore{};
		bool startShuffled{ false };
		std::string pickedInto;
		int pickedSends{};
		std::string emptyFrom;
		bool emptyRandom{ false };
		int emptyLeave{};
		std::string pickedSound;
		std::vector<std::string> orderBy;
		float spreadWidth{};
		bool carry{ false };
		float lift{};
		float pointsMost{};     // 0: no limit
		std::string counts;     // <counts variable>: as written (empty: none)
		std::vector<int> broken;  // the suits of its <breaks> laid on it since the items were gathered
		std::string score;      // as written
		std::string scoreWith;
		int scoreFull{};
		int scoreBest{};
		std::string melds;      // as written
		int meldsBest{};
		bool scoreDue{ false };  // its items changed since it was last scored
		HolderRule takes;
		std::vector<int> items;
		// The live number `items`: how many the holder holds. A holder that
		// takes <wild> items: `calling` is 1 while a wild card on top waits
		// for a suit to be called, `called` the suit called (from 1; 0 none).
		float itemCount{};
		float calling{};
		float called{};
		// The live number `points`: its face-up items' worths added up, each
		// as high as it may count while the total stays within <counts><atmost>.
		float points{};
		// A <score>d holder: the best of its items (and the shared ones)
		// by its patterns: `pattern` the highest they make (from 1 as
		// written; 0 none) and `strength` to compare (higher wins); `odds`,
		// in percent, how often it beats as many unseen items when the
		// shared holder is made up to full (a tie counts half).
		float pattern{};
		float strength{};
		float odds{};
		// Whether anything reads its `odds` (Game::markOddsRead): a thousand
		// deals are tried to work them out, so holders nobody asks skip them.
		bool oddsRead{ true };
		// A <follow /> holder (a trick): `lead`, the suit of its first item
		// (from 1; 0 empty), and `winner`, the seat of the hand that laid
		// the item winning it: the highest trump, else the highest of the
		// suit led.
		float lead{};
		float winner{};
		// A hand's own holder in a game with a trick: `tricks`, how many
		// tricks its items should take (a reckoning by high items, trumps
		// and short suits, as a player counts them to bid).
		float tricks{};
		// A holder scored with <melds />: `deadwood`, the points of its items in no
		// group, the groups chosen to leave the fewest.
		float deadwood{};
	};

	// An object's handle: its slot in the game, and which object that slot
	// holds (a slot freed by a despawn is used again with the next
	// generation, so a handle kept from before finds nothing). The default
	// handle is no object's.
	struct ObjectId
	{
		std::uint32_t index{ kNoSlot };
		std::uint32_t generation{};

		static constexpr std::uint32_t kNoSlot = 0xFFFFFFFFu;
		explicit operator bool() const noexcept { return index != kNoSlot; }
		friend bool operator==(ObjectId, ObjectId) = default;
	};

	// The numbers and flags of an object that change as it plays and that a
	// reset puts back: where it is and how it goes, which way it faces, its
	// light and height, whether it is in play, touches things and exists,
	// the scene it is in, and its variables. An object is one (its current
	// values, with no prefix: object.position), and holds two more: `start`,
	// how it started (worked out from its RawObject, and drawn again when a
	// <random> start is), and `before`, how it was when the last frame
	// ended (only what the next frame reads of it, see Object::Before). The
	// RawObject itself, the file's text, is the fourth.
	struct Moment
	{
		Vector2f position;
		Vector2f velocity;

		// A constant pull on the velocity, added to it every frame (gravity
		// is an acceleration with only a y) - see playOneFrame. <stop /> takes
		// it away and a reset brings it back.
		Vector2f acceleration;

		// Which way the object faces: degrees clockwise from straight up, so
		// 0 is up, 90 is right, 180 is down; always from 0 up to (not
		// including) 360. Only an object whose file gives it a <heading>
		// turns (see Object::Turning).
		float heading{};

		// An object with a <pivot>: the whole degrees of the heading its
		// position was last laid out for, so that the pivot stays where it is
		// on the screen as it turns (Object::showHeading).
		int pivotDegrees{};

		// Which way an object with a <facing> faces, one of the four
		// directions, without turning its picture (see Object::hasFacing).
		Direction facing{ Direction::Up };

		// <light>: how far round its middle it lights, while it is in play;
		// 0 for no light. <altitude>: how high it is off the ground, and how
		// fast that is going up (see Object::Aloft).
		float light{};
		float altitude{};
		float rise{};

		// In play: shown, and touching things when its collisions are on. die()
		// takes it out of play and stops its collisions.
		bool isVisible{ true };
		bool collisionsOn{ false };

		// Whether it is in the game at all. One that does not exist is never
		// in play, and nothing (a <reveal>, a <fire>) brings it back; it is
		// a description that a <spawn> makes copies of. <despawn /> takes an
		// object out of the game; a spawned one is gone for good when the
		// frame ends, one the file wrote stays, not existing, until a reset.
		bool exists{ true };

		// The scene it is in (see RawObject::ownerState): an object carried
		// into another scene (<enter>) belongs to that one from then on.
		std::string ownerScene;

		// Its <variable>s by name, and its named timers' time left.
		std::map<std::string, float> variable;
	};

	// The way `way` points once turned that many degrees clockwise (on the
	// screen, where y grows downward), as a picture is turned.
	Vector2f turnedBy(const Vector2f& way, float degrees) noexcept;

	// What the objects of one kind have alike and never change while they
	// play (their rules, actions and timers): the members of a <group>, or
	// a piece and the copies spawned of it (RawObject::kind). Made for each object as it loads, checked
	// for each, then shared by the kind (game_expr::shareKinds).
	struct Kind
	{
		CollisionData collisionData;
		std::map<std::string, std::vector<Command>> action;
		std::vector<Timer> timers;   // their clocks are each object's (Object::clocks)
		int written{ 0 };   // RawObject::kind
	};

	struct Object : Moment
	{
		// Its handle (Game::find); given when it joins the game.
		ObjectId id;

		// RawObject::line, for the messages of the checks made once all is loaded.
		int line{ 0 };

		// How it started (see Moment).
		Moment start;

		// What the next frame reads of how the last one left it: whether it
		// moved, or came into play or into view (playOneFrame).
		struct Before
		{
			Vector2f position;
			bool collisionsOn{ false };
			bool isVisible{ true };
			Before& operator=(const Moment& moment) noexcept
			{
				position = moment.position;
				collisionsOn = moment.collisionsOn;
				isVisible = moment.isVisible;
				return *this;
			}
		};
		Before before;

		// Loaded or reset since the last frame ended: where it is when the
		// next frame starts is its place, not a move (playOneFrame). One
		// reset by a rule in the middle of a frame has moved, there and then:
		// Q*bert set down on the top cube by a disc lands on it.
		bool placed{ true };

		// Back as it started, every number of the Moment.
		void backToStart()
		{
			static_cast<Moment&>(*this) = start;
			placed = true;
		}

		// What it showed when the last frame ended: a picture that changed
		// can touch what the old one did not, though nothing moved (a
		// crocodile's jaws opening under Harry).
		struct Shown
		{
			std::size_t look{};
			std::size_t animationIndex{};
			Vector2f size{};
			bool mirrored{ false };
			bool operator==(const Shown& other) const noexcept
			{
				return look == other.look && animationIndex == other.animationIndex && size == other.size && mirrored == other.mirrored;
			}
		};
		Shown shownBefore;
		Shown shown() const noexcept { return { looks.shown, animation.index, size, mirror.flipped }; }

		// The frame is over: how it is now is how it was before the next.
		void remember()
		{
			before = *this;
			shownBefore = shown();
			placed = false;
		}

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

		// Whether `name` names it: its own name, or a group it is in.
		bool named(const std::string& wanted) const { return name == wanted || inGroup(wanted); }
		std::string objClass;

		// Whether it is drawn while in play (see RawObject::isDrawn).
		bool isDrawn{ true };

		// A spawned copy: the name of the object it was spawned from, which
		// is also its group, so whatever names that object names it too
		// (shot.1, shot.2 for copies of shot). Empty for an object the file
		// wrote. `most` is how many copies of this one may exist at once (0:
		// no limit), from <most>.
		std::string copyOf;
		int most{ 0 };
		bool isCopy() const noexcept { return !copyOf.empty(); }

		// Named in the game's <keeps>: a full <reset /> passes it over, so it
		// keeps what it holds (a best score) from one game to the next.
		bool kept{ false };

		// The state it belongs to (see RawObject::ownerState); its scene is
		// the Moment's ownerScene.
		std::string ownerState;

		// How far <shift> has moved the place it goes home to (a path's
		// <home />) since it was last put back.
		Vector2f shifted;

		// How it turns: whether it has a heading at all, the point it turns
		// about and how far it turned this frame.
		struct Turning
		{
			// Whether it has a <heading> (Moment::heading): only then does it turn
			// (a <turn> in its actions), get pushed along it (a <thrust>) or fire
			// along it (a <fire>). A sprite of <line>s or a <bitmap> that has a heading is kept
			// once as it was written (Drawn::turnables, shared by every object
			// made from the one definition) and `drawn.bitmap` is that turned to the
			// heading, to the nearest whole degree, so what is shown and what a
			// pixel collision tests turn with it - see showHeading. That makes two
			// pictures an object has to hold, the original and the one it shows.
			bool hasHeading{ false };

			// <pivot>: it turns about that point of its picture, not its middle;
			// pivotFromMiddle is the way from the middle of the picture as drawn
			// (unturned) to it. See showHeading and pivotOnScreen.
			bool hasPivot{ false };
			Vector2f pivotFromMiddle;

			// How many degrees it turned this frame (clockwise is more), from the
			// heading the last frame left it at: the move sweeps the turn through
			// the frame as it sweeps a velocity, and what it touches meets a
			// surface going that fast where they touch (velocityAt). 0 for one
			// put somewhere (a reset) and for anything that does not turn.
			float spin{};
			// The step that spin was turned over, in seconds:
			// its speed of turning is spin / spinOver.
			float spinOver{ 1.0f };
		};
		Turning turning;

		// An <animation>: the pictures it shows in turn (all one size, and
		// `bitmap` is whichever is showing), how many seconds each one stays
		// for, how long the one showing has been, and which one it is (an index into
		// bitmaps). Only an object that is shown and in play moves on; a
		// reset starts it again at the first. A sprite of one picture has
		// none of this. See advanceAnimation.
		struct Animation
		{
			std::vector<std::shared_ptr<const Bitmap>> bitmaps;
			// How long each picture shows, in seconds.
			float seconds{};
			// Each picture's own time in seconds, when a <frame> has its own
			// <seconds>; empty, every one `seconds`.
			std::vector<float> times;
			// Each picture's own <hitbox>es, touched with as well as the
			// object's while it shows; empty when none has any.
			std::vector<std::vector<Hitbox>> hitboxes;
			// Each picture's own commands, run as it comes up (Game::frameCameUp),
			// of an object's one animation (a look's are the look's own); empty
			// when none has any.
			std::vector<std::vector<Command>> commands;
			// The seconds the picture `index` has shown so far (a double, so a
			// slow animation counted in many short steps stays true).
			double elapsed{};
			std::size_t index{};
			// <once />: the animation plays through one time. At its end a look
			// goes back to `lookBefore` (the last look shown that was not itself
			// once); an object's one animation stays on its last picture and is
			// `done`.
			bool once{ false };
			bool done{ false };
			std::size_t lookBefore{};
			// <stepped />: it moves only on the step its picture changes, the
			// whole way its velocity took it over the time since the last
			// change (`since`, in seconds);
			// `share` is how much of that time this step moves (0 between
			// changes). See motionOf. `shareBefore` is the step before's, so
			// this step's edge checks, made before the move, see where that
			// step left it, and a bounce can take it back.
			bool stepped{ false };
			float since{};
			float share{};
			float shareBefore{};
			// <then>'s commands of an object's one animation, run when it ends
			// (a look's are the look's own).
			std::vector<Command> then;
		};
		Animation animation;
		// The hitboxes of the picture showing (often none).
		const std::vector<Hitbox>& frameHitboxes() const noexcept
		{
			static const std::vector<Hitbox> none;
			return animation.index < animation.hitboxes.size() ? animation.hitboxes[animation.index] : none;
		}

		// What one call of advanceAnimation did.
		enum class AnimationStep { Stayed, Moved, Ended };

		// Spends `owed` seconds on the picture showing. When its time is up
		// the next is shown (marking the visual to be rebuilt) and what is
		// left of `owed` stays in it, for the next call to spend there: a
		// step that passes several pictures calls until it Stayed. Ended: an
		// animation that plays once has just ended. Stayed for an object
		// with no animation.
		AnimationStep advanceAnimation(float& owed);

		// Back to the first picture, from the start of its time.
		void restartAnimation();

		// Whether picture `picture` of the look `look` (of its one animation,
		// for an object with no looks) has commands of its own to run.
		bool hasFrameCommands(std::size_t look, std::size_t picture) const noexcept
		{
			const auto& frames = looks.all.empty() ? animation.commands : look < looks.all.size() ? looks.all[look].frameCommands : animation.commands;
			return picture < frames.size() && !frames[picture].empty();
		}

		// A live <seconds> (Follow::Part::Seconds): `seconds` for every picture of that animation that has no time of its own
		// (frame npos) or for that one picture, of the look `look` (npos: the
		// object's one animation), and for the animation playing when it is
		// that one. A picture already showing keeps the time it has counted.
		void timeAnimation(std::size_t look, std::size_t frame, float seconds);

		// Coming back into play (fired, released, revealed, spawned) is a
		// new start: its animation and its timers start over, as at a reset.
		void startOver();

		// Points `bitmap` at the picture animation.index names (turned to the
		// heading, if it turns) and marks the visual to be rebuilt.
		void showAnimationFrame();


		// Where its turning point is on the screen: the pivot, or the middle.
		Vector2f pivotOnScreen() const;

		// <swing>: a stroke of `seconds` from the heading it started with,
		// turning `by` degrees (less than 0: counterclockwise), eased (still
		// at both ends, fastest half way). `done` is how far through it is, 0
		// to 1: held, it goes on; let go, it comes back. seconds 0: it has
		// never swung. See playOneFrame.
		struct Swing
		{
			float seconds{};
			float by{};
			float done{};
			bool held{ false };
		};
		Swing swing;

		// How fast the point `at` of it is going: its velocity, and its spin
		// about its turning point.
		Vector2f velocityAt(const Vector2f& at) const;

		// Whether it has a <facing> (Moment::facing): which way it faces, one
		// of the four directions, without turning its picture. It follows the last <move> or <hop> made, and a <fire> leaves the
		// middle of that side, moving that way at the projectile's own speed:
		// a man in a maze shoots the way he last walked, an invader facing
		// down drops its bombs below it. An object with no <facing> fires from
		// its top, at the projectile's own velocity, as before. See
		// fireProjectile in verbs/fire.cpp.
		bool hasFacing{ false };

		// Its place on a <map> (see RawCell): the map's name ("" for an object
		// no map placed), its tile counting from 0, and the map's top left,
		// the size of a tile and how many tiles across and down it is. The
		// maps a walker finds its way on are made from these (buildMazes).
		struct MapPlace
		{
			std::string name;
			int column{};
			int row{};
			Vector2f origin{};
			Vector2f tile{};
			int columns{};
			int rows{};
			// On a map with <cells /> (see RawCell::cells): it moves a cell at
			// a time, and a move that is refused touches what refused it (see
			// game/cells.cpp). The Moment's cellColumn and cellRow are the cell
			// it is in.
			bool cells{ false };
			// The map it was written on. Sent to another (a next-piece box), it
			// is only shown there: its timers do not count and its actions do
			// not run until it is back (see awayFromHome).
			std::string home;
		};
		MapPlace map;
		bool awayFromHome() const noexcept { return map.cells && map.name != map.home; }

		// <moves>: what a piece of a board game may do when a picker
		// <place>s it (game/cells.cpp). A step goes to the empty cell that
		// many cells across and down; a jump goes over a piece of its class
		// that far away into the empty cell beyond, and takes it. The set
		// for the look showing is used, else the one for no look. forced:
		// while any piece of its class can jump, it may only jump.
		// promote: reaching promoteRow, it shows that look. flanks: a new
		// piece of its kind is <put> only in an empty cell from which a line
		// of one or more pieces of a flank's class runs (any of the eight
		// ways, or across and down only when straight) and ends in a piece
		// of its own class; the pieces in those lines turn into its kind.
		// fall: a piece put goes to the lowest free cell of the picker's
		// column. close: a piece put that walls in a piece of closeClass beside
		// it (no place left round it) puts a copy of closeObject there.
		// A set with no steps or jumps is one of pieces put.
		struct Flank
		{
			std::string objClass;
			bool straight{ false };
		};
		struct PieceMove
		{
			bool jump{ false };
			std::string objClass;
			int x{};
			int y{};
		};
		struct MoveSet
		{
			std::optional<std::size_t> look;
			bool forced{ false };
			std::vector<PieceMove> moves;
			std::optional<std::size_t> promoteLook;
			int promoteRow{};
			std::vector<Flank> flanks;
			bool fall{ false };
			std::vector<std::string> on;
			std::string closeClass;
			std::string closeObject;
			bool puts() const noexcept { return moves.empty(); }
		};
		struct Moves
		{
			std::vector<MoveSet> sets;
			// How many moves it can make now: the live number `moves`, counted
			// before the conditions each frame (a side with none left is stuck).
			// A piece whose set has <flank>s counts the cells a new piece of its
			// kind could be put in as well.
			float count{};
		};
		Moves moves;

		// <marks>: copies of the mark pieces put in the cells marked (see
		// Game::showMarks), made again only when what is marked changes.
		struct Mark
		{
			enum class Kind { Held, Moves, Movable, Put, Last };
			Kind kind{ Kind::Held };
			std::string object;
			std::string objClass;
			std::string variable;
		};
		struct MarkSet
		{
			std::optional<std::size_t> look;
			std::vector<Mark> marks;
		};
		// An item (a held member, a part: Parts::items): what it is (its start holder and worths, as
		// ItemOf), its number (from 1, every held member of the game in turn),
		// the hand that last laid it and whether its face is up.
		struct Item
		{
			bool is{ false };
			std::string startHolder;
			std::vector<float> worths;
			int number{};
			int laidBy{};           // the seat of the hand that last laid it (0 none)
			bool faceUp{ false };
		};
		// A member of a group that takes <turns> (`takes`): the look it shows
		// off its turn (`sprite`; none: as it is). Its live `turn` is 1 on its
		// turn, else 0. <owns class=>: the pieces it moves; a picker's
		// <put />, <pick /> and a <think> with no class= take those of the
		// member on its turn.
		// A part (Parts::turns), only for a member that takes them.
		struct Turns
		{
			bool takes{ false };
			std::string sprite;
			std::string ownsClass;
		};

		// Drawn after everything with a smaller one; 0 keeps the file's order
		// (items and hands are drawn above the rest).
		int drawOrder{};
		struct Marked
		{
			std::string object;
			int column{};
			int row{};
			bool operator==(const Marked&) const = default;
		};
		// A picker's <marks>: its sets, the cells marked now, and the copies
		// of the mark pieces put in them.
		struct Marks
		{
			std::vector<MarkSet> sets;
			std::vector<Marked> marked;
			std::vector<std::string> copies;
		};
		// A picker (it runs <pick>) and a cursor that moves <onto> pieces, with
		// its <marks>. A part (Parts::pickers): one written with <marks> has it
		// from the start, and the first pick, place, hop or track gives it.
		struct Picker
		{
			// The piece it holds ("" for none), and whether that piece is in
			// the middle of a run of jumps, which it must finish.
			std::string picked;
			bool inChain{ false };
			// The cells the last move it placed went from and to (0 for none).
			int fromColumn{};
			int fromRow{};
			int toColumn{};
			int toRow{};
			// A hop up or down <onto> pieces keeps to the column the first of
			// them started from (keptColumn), while it is still in the cell the
			// last one left it in (keptAt): a text cursor's way.
			int keptColumn{};
			std::pair<int, int> keptAt{};
			// A <track> <onto> pieces: where the control points, in pixels, each
			// half as it last came (below 0: not yet).
			Vector2f pointer{ -1.0f, -1.0f };
			// Its <marks>: its sets, the cells marked now and the copies put in them.
			Marks marks;
		};
		// One of the map's <checks>, made ready.
		struct CellCheck
		{
			std::string kind;
			std::string objClass;
			std::vector<Command> commands;
			int count{ 0 };
			bool straight{ false };
			std::string alike;
		};
		// What a check found: the map, the row (counting from 1) and the
		// pieces, which its <clear /> and <collapse /> act on.
		struct CellMatch
		{
			std::string map;
			int row{};
			std::vector<std::string> pieces;
		};

		// A <shape>: the cells it fills, from its top left (column and row
		// from 0) with the object each is a copy of, and how big the square
		// it turns in is. While it is in play its cells are copies of those
		// objects, named in cell.shapeParts, which move and turn with it; the
		// shape itself is never drawn and touches nothing but by its moves.
		// quarter counts its quarter turns clockwise (0 to 3), which the
		// standard kicks of a turn go by.
		struct ShapeCell
		{
			int column{};
			int row{};
			std::string piece;
		};

		// What a piece on a map with <cells /> has beyond its place: its
		// <chain>, the map's checks, its <shape> and a held <move>
		// (game/cells.cpp). A part (Parts::cells): a piece of a cell map, a
		// chain or a shape has one from the start, a piece sent into a cell
		// map is given one.
		struct Cells
		{
			// Where it is on its map: the cell, counting from 1 (the live
			// numbers column and row; 0 for one not on cells), and what goes
			// with the place. A reset puts back `start`.
			struct Place
			{
				float column{};
				float row{};

				// A <go>: going that way, a cell every goEvery seconds, the
				// next in goWait.
				bool going{ false };
				Direction goWay{ Direction::Up };
				float goEvery{};
				float goWait{};

				// A <chain>: its links in order from the one behind it (found
				// from where they lie when empty), and how many it is to grow
				// by. Its links are known (found, or given by a <split />): a
				// link lost is dropped, never looked for again where they lie.
				std::vector<std::string> chainLinks;
				int chainGrow{};
				bool chainKnown{ false };

				// A piece with <joins>, settled: the other cells of the shape
				// it came in, which it falls with while they are there.
				std::vector<std::string> joinedWith;
			};
			Place place;
			Place start;

			// <chain object>: what its links are (see RawObject::chain).
			std::string chain;
			// The map's <checks>, made ready (each piece of the map has them).
			std::vector<CellCheck> checks;
			// A <shape> (ShapeCell above): its cells now and at the start,
			// the size of the square it turns in, its quarter turns and the
			// names of the copies in play.
			std::vector<ShapeCell> shape;
			std::vector<ShapeCell> shapeStart;
			int shapeSize{};
			int quarter{};
			std::vector<std::string> shapeParts;
			// A cell of a shape in play: the shape's name (empty once it has
			// settled, or for a piece that was never in a shape).
			std::string shapeOwner;
			// A held <move>: the way, the seconds until its next step, and the
			// seconds between steps (see CmdMove::wait).
			bool held{ false };
			Direction heldWay{ Direction::Up };
			float heldWait{};
			float heldEvery{};
		};


		// A timed <hop> under way (a flight): the whole way it goes, the
		// seconds it takes, and the seconds flown so far (a double, so many
		// short steps add up to the whole). Until `flown` reaches `seconds`
		// the object is in the air: it touches no other object
		// (playOneFrame). A reset lands it where it starts. `arc` is how high
		// it is drawn at the halfway point (its altitude, up then down).
		struct Flight
		{
			Vector2f whole{};
			float seconds{};
			double flown{};
			float arc{};
		};
		Flight flight;
		bool isAirborne() const noexcept { return flight.flown < static_cast<double>(flight.seconds); }

		// Standing, jumping and climbing. `grounded` is true for the frame
		// after a <land /> stood the object on something (playOneFrame
		// clears it as each frame's move starts, so it lasts exactly until the
		// next landing is worked out); a <jump> needs it. `jumping` is a jump
		// under way: the way across stays as it was at the take-off until the
		// next landing. `climbing` is on a ladder (playOneFrame): no
		// pull, no landing, no walking off. `activeClimb` is the step held up
		// (0) and down (1), and `climbClass` what it climbs.
		struct Footing
		{
			bool grounded{ false };
			bool jumping{ false };
			bool climbing{ false };
			std::array<float, 2> activeClimb{};
			std::string climbClass;
			// A climb a rule or timer started, not a key: let go at the end.
			bool climbOnce{ false };
		};
		Footing footing;

		// Where an <aim> pointed it: the way its next <fire>s go (a unit
		// vector), while hasAim. A move, hop or jump by key, or a reset,
		// drops it.
		bool hasAim{ false };
		Vector2f aim{};

		// A <follow> under way (see Path, Game::applyPaths): the path's name
		// (empty: on none), which of its legs is being flown, what is left of
		// that step to go, whether the leg has set off yet (its commands run
		// once, as it does), and how many seconds are left to wait before the
		// first leg (a <stagger>). A reset, a <die /> or a <reveal> ends it.
		struct OnPath
		{
			std::string name;
			std::size_t leg{};
			Vector2f left{};
			bool legStarted{ false };
			float wait{};
		};
		OnPath path;
		bool isFollowing() const noexcept { return !path.name.empty(); }

		// The looks <become> switches between: each named sprite as the window
		// backends draw it (its Picture, and the bitmap for a bitmap or a
		// drawing of lines), shared by every object made from the definition.
		// looks.shown is the one showing; a reset shows the first again. An object
		// with one sprite, or one unnamed animation, has none. A look that
		// moves (a named <animation>) has its pictures in `frames`, each
		// shown `frameSeconds`, and `bitmap` the first.
		struct Look
		{
			std::string name;
			Picture picture;
			std::shared_ptr<const Bitmap> bitmap;
			std::vector<std::shared_ptr<const Bitmap>> frames;
			float frameSeconds{};
			std::vector<float> frameTimes;   // as Animation::times
			std::vector<std::vector<Hitbox>> frameHitboxes;   // as Animation::hitboxes
			std::vector<std::vector<Command>> frameCommands;   // as Animation::commands
			bool once{ false };            // as Animation::once
			bool stepped{ false };         // as Animation::stepped
			// <then>: the look to go to when it ends, in place of the look
			// before, and the commands it runs then.
			std::optional<std::size_t> thenLook;
			std::vector<Command> then;
			// For an object with a <heading>: the look's pictures (one, or
			// one a frame) kept to be turned to it (see `turnables`).
			std::vector<std::shared_ptr<const Turnable>> turnables;
		};

		// The name of the look showing ("" for an object with none).
		const std::string& lookName() const;

		// The names its <hitbox>es have, each once.
		std::vector<std::string> hitboxNames() const;

		// The palettes a <become style="..."> can give it: its pictures
		// drawn again with the colors that style gives, its bitmaps that
		// take a style taking that one instead. The first is as loaded (its
		// name empty); the others, one for each style= that can reach it,
		// are drawn as the game loads and shared by its copies. looks.worn
		// is the one worn; a reset puts the first back on. Empty when
		// nothing changes its colors.
		struct Palette
		{
			std::string name;
			std::shared_ptr<const Bitmap> bitmap;
			std::vector<std::shared_ptr<const Bitmap>> animation;
			std::vector<Look> looks;
		};

		// Puts on palettes[index], keeping the look and where its animation
		// is, and marks the visual to be built again.
		void wearPalette(std::size_t index);

		// Shows looks[index], from its first picture if it moves, and marks
		// the visual to be built again.
		void showLook(std::size_t index);

		// <poses>: the look (an index into looks) for each Pose, every one
		// filled in (a pose left out takes another's, see load/expr), when
		// hasPoses. The pose is picked each frame from how it moves, while it
		// shows one of these looks (playOneFrame).
		enum class Pose { Standing, Walking, Skidding, Rising, Falling, Thrusting };

		// <joins>: the look (an index into looks) for where the piece it is
		// joined to lies, by Join, when hasJoins: the other cells of its
		// shape while it falls, the pieces in joinedWith once settled. Put
		// on at the end of each frame (Game::showJoins), while it shows one.
		enum class Join { Alone, Left, Right, Up, Down };

		// Its looks (`all`; `shown` is the one showing), the palettes it can
		// wear (`worn` the one on), and the looks of its poses and joins.
		struct Looks
		{
			std::vector<Look> all;
			std::size_t shown{ 0 };
			// The look it starts as, and a reset brings back: the first, or
			// for one with <poses> its standing look.
			std::size_t first{ 0 };
			std::vector<Palette> palettes;
			std::size_t worn{ 0 };
			std::array<std::size_t, 6> poses{};
			bool hasPoses{ false };
			std::array<std::size_t, 5> joins{};
			bool hasJoins{ false };
		};
		Looks looks;

		// <mirror>: going `way` (left or right) its pictures are drawn flipped
		// left to right, when `on`; `flipped` is whether they are now.
		// `goingLeft` is the way across it last went: what keys it holds say,
		// else the hop or jump it is making, else its velocity (a stop leaves
		// it as it was). `flips` has each of its pictures and that picture
		// flipped, both ways round, made as the game loads and shared by its
		// copies.
		struct Mirror
		{
			bool on{ false };
			Direction way{ Direction::Left };
			bool flipped{ false };
			bool goingLeft{ false };
			std::shared_ptr<const std::map<const Bitmap*, std::shared_ptr<const Bitmap>>> flips;
		};
		Mirror mirror;

		// The picture to show for `picture`: flipped while mirrored.
		std::shared_ptr<const Bitmap> facingPicture(const std::shared_ptr<const Bitmap>& shown) const;

		// Flips its picture, if it now goes the other way to the one it
		// is drawn for (see goingLeft).
		void faceMirror();

		// How it moves under what pushes it: its speed, what it loses each
		// frame, how it takes a knock.
		struct Motion
		{
			// How fast it goes (live number speed): worked out when read, except
			// while its touch rules run, when it is the speed as the touch happened
			// (speedHeld), so a rule that stops it does not change which rules after
			// it run: a landing and a crash on one pad.
			float speed{};
			bool speedHeld{ false };

			// <grip>: how fast its speed closes on the velocity its held moves
			// ask for, as a rate (kept over t is e to the -grip t; see
			// game_expr::lossRate). Infinite, the default: at once. Finite, it
			// slides into a walk and out of one.
			float grip{ std::numeric_limits<float>::infinity() };

			// <drag>: how fast it loses its velocity, as a rate (kept over t is
			// e to the -drag t). 0, the default, is no drag at all; with drag
			// a thruster can no longer push it faster than a top speed.
			float drag{};

			// <friction>: at a touch with something that does not move, the speed
			// along its surface is cut by up to this share of the speed the touch
			// took off straight into it (a slide on a rough surface); and while it
			// lies on a surface (or wedged between two) that its own pull cannot
			// slide it along, it comes to rest (see Rest). 0: none.
			float friction{};

			// <mass>: how hard it is to knock (see CmdKnock). 0, when it has
			// none, is not knocked at all: a wall, a floor.
			float mass{};

			// What its pull, thrust and drag changed its velocity by this
			// step, so the step's move is made at the mean of its velocity
			// before and after: a constant pull's v t + a t t / 2 (sweptMotion).
			// None again once a touch changes how it moves.
			Vector2f gained{};
		};
		Motion motion;

		// Seconds of blinking left (owner.blink): while above 0 it is drawn
		// only every other fifteenth of a second. It is in play all the while.
		float blink{};
		bool blinkedOut() const noexcept { return blink > 0.0f && static_cast<int>(blink * 15.0f) % 2 == 1; }

		// How far each of its <timers> (its kind's, Kind::timers) has
		// counted; they count while it is shown and in play (see Timer,
		// Game::updateTimers).
		std::vector<Timer::Clock> clocks;

		// <surface>: `is`, this is ground. A thing with a <grip> whose middle
		// is over it has `motion.drag` and `motion.grip` in place of its own (a surface
		// that gives neither leaves them as they are); the last surface in
		// the file's order wins. groundDrag and groundGrip are what this
		// object has this frame, worked out by applyGrip (play/frame.cpp).
		struct Surface
		{
			bool is{ false };
			std::optional<float> drag;
			std::optional<float> grip;
			float groundDrag{};
			float groundGrip{ std::numeric_limits<float>::infinity() };
		};
		Surface surface;

		// What something lies on, as it was: its name, where it was and the
		// way it faced.
		struct Support
		{
			std::string name;
			Vector2f at;
			float heading{};
		};

		// At rest (`resting`, at `at`) on one thing, or wedged between two,
		// that do not move (`on`, as they were then): it neither moves nor
		// is pulled, until one of them moves or turns, or something moves it
		// or gives it a speed. Worked out by a touch with friction
		// (play/verbs.cpp's rub), which also keeps the last surface it
		// settled on, the way out of it and how many seconds ago
		// (`settledOn`, `settledNormal`, `settledAge`), for one wedged
		// between two. A part (Parts::rests), for an object with <friction>.
		struct Rest
		{
			bool resting{ false };
			std::vector<Support> on;
			Vector2f at;
			Support settledOn;
			Vector2f settledNormal;
			float settledAge{ 1000.0f };
		};

		// What draws it toward other things each frame (see playOneFrame). A
		// part (Parts::pulls), for an object with a <pull> or a <seek>.
		struct Pull
		{
			// <pull>: it is pulled toward the middle of the nearest of each
			// name or group in play, by that much speed a frame (gravity
			// toward a sun, a tractor beam, a magnet).
			std::vector<std::pair<std::string, float>> toward;

			// <seek>: it turns the way it is going toward the middle of the
			// nearest of `seek` in play, by at most seekRate degrees, at the
			// same speed, and its heading with it if it has one: a homing
			// missile. Empty for none.
			std::string seek;
			float seekRate{};
		};

		// <dark>: an object drawn only where there is light. Unlit, only while
		// some part of it is within a light; unseen, once that has happened,
		// from then until it is reset (fog of war). `hidden` is whether it is
		// hidden this frame; `seen` whether an unseen one has been lit.
		enum class Darkness { None, Unlit, Unseen };
		struct Dark
		{
			Darkness kind{ Darkness::None };
			bool seen{ false };
			bool hidden{ false };
		};
		Dark dark;

		// <altitude>: how high it is off the ground, which it is drawn
		// raised by while its position stays where it is on the ground (so
		// what it touches is what is under it); how fast that is going up
		// (rise, less each frame by fall); and how much of the rise is kept,
		// turned upward, when it comes down (bounce). It cannot go below
		// the ground. Altitude and rise are the Moment's; a reset puts back
		// those it started with. `has` is whether it has an <altitude>.
		//
		// <shadow object="ball" />: kept under that object each frame, its
		// middle at the other's middle on the ground, and shown while the
		// other is (shadowOf; "" for nothing). A part (Parts::alofts), for
		// an object with an <altitude> or a <shadow>.
		struct Aloft
		{
			bool has{ false };
			float fall{};
			float bounce{};
			std::string shadowOf;
		};

		// The size of what it shows now, in whole pixels, as every window
		// draws it: its bitmap's, or its picture's (measureShapeSize).
		Vector2f pictureSize() const;

		// The picture every window shows for it, `scale` bitmap pixels to
		// each of the game's (Window::pictureScale), which a window draws
		// over pictureSize() at its position. A text, a circle and a
		// rectangle are drawn at that scale, so they stay sharp in a window
		// bigger than the game; a bitmap (lines, rows, an SVG, a turned
		// picture) and an image file are their own pixels at any scale.
		std::shared_ptr<const Bitmap> drawnPicture(float scale = 1.0f) const;

		// Whether drawnPicture changes with the scale: a text, a circle or a
		// rectangle. A window draws those again when its scale changes, and
		// smooths them when it stretches them; the others keep hard edges.
		bool drawnAtScale() const noexcept;

		// Draws `bitmap` again at the current heading (and, for an animated
		// object, the picture that is showing) when that is not what it
		// already is, and marks the visual to be rebuilt. Does nothing for an
		// object that does not turn.
		void showHeading();


		// Motion this object is given for one frame besides its own velocity.
		// `riding` is the velocity of whatever it is riding (a collision rule
		// with <ride />, e.g. a frog on a log): worked out again every frame,
		// it counts for movement and for edge checks like velocity does, but
		// is not the object's own, so an object that steps off is at rest.
		// `hopPending` is a hop queued by a hop.*() action, made at the start
		// of the next frame's move (playOneFrame).
		Vector2f riding{};
		Vector2f hopPending{};

		// What it carries (<carry />): the name of one other object (`what`),
		// which goes where it goes, held where it was when it was picked up
		// (`offset` from this object's top left), until a <drop />, until
		// something else is picked up, or until either is out of play
		// (playOneFrame). `dropped` is what it last let go of: not picked up
		// again until the two have stopped touching, so it can be walked away
		// from. A part (Parts::carries), given by the first <carry />.
		struct Carry
		{
			std::string what;
			Vector2f offset{};
			std::string dropped;
		};

		// The object's own bounding box (width/height) in whole pixels, as
		// every window draws it (pictureSize): set for every object when the
		// game loads, with no window, and again whenever what it shows changes
		// size (a look, a heading, a text's words). Collision and physics
		// code only ever reads this field.
		Vector2f size{};

		// How its position is worked out again after the game loads.
		// <position> may use any object's size by name - name.width and
		// name.height, including this object's own - e.g. to centre a text:
		// window.width.center - title.width / 2. A circle's or rectangle's
		// size follows from its sprite alone, so a position using only those
		// is exact the moment the game is loaded. A text's size can change as
		// its words do (a score gaining a digit), so a position that uses one
		// has a SizeUse part (Parts::sizeUses) listing those texts in
		// sizesOf, and Game::resolveSizeDependentPositions() works it out
		// again whenever one of them changes. sizesUsed holds the sizes, in
		// sizesOf order, it was last worked out from. cellOffset is a cell's
		// offset from its group's evaluated <position> (see RawCell), so that
		// position can be worked out again later and the offset added back.
		struct SizeUse
		{
			std::vector<std::string> sizesOf;
			std::vector<Vector2f> sizesUsed;
		};
		struct Layout
		{
			Vector2f cellOffset{};
		};
		Layout layout;

		// What it says update="live" of: a <variable>, a half of its
		// <position> (a group's, with this one's place in it added), its
		// <heading> or an animation's <seconds>, worked out again every frame
		// from what it reads (Game::follow), after the move and before the
		// conditions. No command may write one (load/check). In the file's
		// order: the position and heading first, then the variables, then the
		// animations' seconds.
		struct Follow
		{
			enum class Part { X, Y, Heading, Variable, Seconds };
			Part part{ Part::Variable };
			std::string variable;
			RawValue value;
			// Seconds: whose they are, the look (index into looks.all) or the
			// object's one animation (npos), and which picture's (index into
			// its times) or every picture's that has none of its own (npos).
			std::size_t look{ std::string::npos };
			std::size_t frame{ std::string::npos };
		};
		std::vector<Follow> follows;

		// Whether `part` (x, y, heading or a variable's name) is one it follows.
		bool followsPart(const std::string& part) const;

		// Whether its start (position, velocity or a <variable>) draws a
		// <random>: a reset then draws it again (Game::drawStartAgain).
		bool startDrawsRandom{ false };

		// <light>'s <past>es: the classes its light does not shine through.
		std::vector<std::string> lightPast;

		// How it is drawn: what it shows now and what the backends and a
		// pixel collision read of it.
		struct Drawn
		{
			// What it shows now (one of its looks, or its one sprite).
			Picture picture;

			// What a sprite of <line>s was drawn as, worked out when the game
			// loads (load/expr). The window backends draw it as a picture and a
			// collision of type pixel tests it; null for every other shape. Shared,
			// not copied, with every copy of the object.
			std::shared_ptr<const Bitmap> bitmap;

			// The picture as it was before a <hole> was first cut in it, which a
			// reset puts back; null while it has none. See xge::cutHole.
			std::shared_ptr<const Bitmap> unholed;

			// True until the active Window backend has built (or rebuilt) this
			// object's own visual - set again whenever something changes what
			// should be on screen for it (e.g. a bound text display's number
			// changing - see Game::setVariable/resetObject/resetAll) so the
			// backend knows to rebuild from its picture before its next draw()
			// instead of showing a stale image. Game/Engine never draw anything
			// themselves; they only ever flip this flag.
			bool visualDirty{ true };

			// A sprite of <line>s or a <bitmap> with a heading: as it was
			// written, shared by every object made from the one definition;
			// `bitmap` is that turned to the heading (see Object::Turning).
			std::vector<std::shared_ptr<const Turnable>> turnables;

			// What `bitmap` was last drawn at: the whole degrees of the heading and
			// which of the turnables, so that a heading that has not moved by a
			// whole degree costs nothing. -1 for nothing drawn yet.
			int turnedDegrees{ -1 };
			std::size_t turnedFrame{};

			// Set when this is a text object that shows a variable (e.g. a <text>
			// sprite whose <number> is paddle1.score): every piece of the text,
			// those that follow a variable with its owner and name, the rest as
			// they were worked out, so it can be put together again whenever one
			// of its variables changes (Game::relabel). Empty means the text shows
			// no variable and only ever changes by an <inc /> naming the text
			// object itself (the older pattern for a text that counts its own
			// number).
			std::vector<TextPiece> textPieces;
		};
		Drawn drawn;

		// A <tether>: a line from the middle of the object `from` names to
		// the middle of `to`'s, drawn again every frame wherever the two are
		// (drawTethers in play/frame), this object's position and size
		// following it. A part (Parts::tethers), for a <tether> sprite only.
		struct Tether
		{
			std::string from;
			std::string to;
			int thickness{ 1 };
			Color color{};
		};

		// Its <hitbox>es (see RawCollisionData), any of them a touch; none,
		// its picture's box. Its own, since they can be about its picture.
		std::vector<Hitbox> hitboxes;

		// What every object of its kind has alike (see Kind), shared.
		std::shared_ptr<Kind> kind{ std::make_shared<Kind>() };

		// Whether one of the pieces shows that variable of that object.
		bool showsVariable(const std::string& ownerName, const std::string& variableName) const noexcept
		{
			return std::any_of(drawn.textPieces.begin(), drawn.textPieces.end(),
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

		// <columns> and <rows>: how many tiles across and down, for a map
		// written without <row>s (0 for one with them, whose rows say).
		int columns{ 0 };
		int rowCount{ 0 };

		// <cells /> and the <checks> (see RawCell::cells).
		bool cells{ false };
		std::vector<RawCheck> checks;

		int across() const noexcept { return rows.empty() ? columns : static_cast<int>(rows.front().size()); }
		int down() const noexcept { return rows.empty() ? rowCount : static_cast<int>(rows.size()); }
	};

	// <collision edge="...">: its commands added to each screen edge it names
	// (top, bottom, left, right; vertical is top and bottom, horizontal left
	// and right, all every one), noting where the rule is among them: it may
	// hold only to a look (sprite=) and be passed over while touching a class
	// (unless=). False for an edge= that names none of those.
	bool addEdgeRule(RawCollisionData& collisions, const std::string& edges, const std::vector<RawCommand>& commands,
		const std::string& sprite = {}, const std::string& unlessClass = {}, const RawCondition& check = {});

	// The tiles of a map that show this character, row by row and each row
	// left to right: column, then row, counting from 0.
	std::vector<std::pair<std::size_t, std::size_t>> tilesOf(const RawMap& map, char character);

	// Puts an object on a tile of the map (column and row from 0): at the
	// map's <position>, which load/expr's layOutCells moves it on from to
	// the middle of the tile. The map's name means it too, outermost of
	// its names.
	void placeOnMap(RawObject& object, const RawMap& map, std::size_t column, std::size_t row, char character = '\0');

	// Puts an object where its character starts it on the map: its one tile,
	// or between two side by side on a map that is not a cell map. False
	// when the character is on no tile or on other tiles than those.
	bool startOnMap(RawObject& object, const RawMap& map, char character);

	// A piece of a cell map on none of its tiles: put on the map, not in
	// the game, waiting to be spawned, as a piece does.
	void waitOnMap(RawObject& object, const RawMap& map, char character = '\0');

	// What a cell map with nothing on it (a next-piece box) is in the game:
	// one undrawn piece, never in play, named map.cells, that holds where
	// the map is so that pieces can be spawned or sent onto it.
	RawObject mapHolder(const RawMap& map);

	// Each <shape> among objects[first] on gets the objects its characters
	// pick: of the same map, the one waiting piece or plain object picked
	// by that character, or the group whose members are on its tiles.
	void pickShapePieces(std::vector<RawObject>& objects, std::size_t first);

	// Gives every text and bitmap with a style= what it leaves out of that
	// style: a text's size and color, a bitmap's scale and colors (each char=
	// it does not give). A style no <style> has, or a text left with no
	// size, stops the load.
	void applyStyles(std::vector<RawObject>& objects, const std::vector<RawStyle>& styles);

	// The looks of an object with named <animation>s, from its `sprites`
	// (each named) and `animations` (each named): first each animation, a
	// look that moves (RawSprite::frames), then each sprite; it starts as
	// the first. A frame naming no sprite, or a look named twice, throws,
	// saying `where`.
	void lookWithAnimations(RawObject& object, const std::vector<RawSprite>& sprites, const std::vector<RawFrames>& animations, const std::string& where);

	// Gives every sprite of an object (its looks and animation frames too)
	// the characters its <collisions> say are solid (RawSprite::solid).
	void markSolid(std::vector<RawObject>& objects);

	// What an object's own rules, timers and actions leave to mean itself,
	// made its name (final, so after layOutScenes): a <trigger> with no
	// object=, a collision rule's <condition> with no class= or object=, and
	// self. at the start of a name in a command's value or variable=
	// (self.x, self.score).
	void nameSelf(std::vector<RawObject>& objects);

	// The same for one list of commands, run by the object `name` (a path's
	// step, run by whichever object flies it). The answer is whether the
	// list said anything of itself, so changed.
	bool nameSelfIn(std::vector<RawCommand>& list, const std::string& name);
	bool nameSelfIn(RawCommand& command, const std::string& name);

	// The tests in a list (a <condition>'s) made to say self of the object
	// testing them; the commands keep self (see nameSelf), a <trigger> with
	// no object= among them.
	void nameSelfInTests(std::vector<RawCommand>& list);

	// Whether a command says anything of self.
	bool namesSelf(const RawCommand& command);

	// A test's self made `name`, and a test that names nothing made about
	// `name` (with "self", one about the object testing it).
	void nameSelfInTest(RawCondition& condition, const std::string& name);

	// A copy of an object, named `to` where it was `from`: what its own
	// rules, timers, actions and values said of `from` by name they now say
	// of `to` (self says itself already). What a <spawn> makes a copy from.
	// Whether its rules, actions or timers named `from`: if not, the copy
	// is of its kind (Kind).
	bool renameOwn(RawObject& object, const std::string& from, const std::string& to);

	// What a rule about another object leaves to mean the object it touched:
	// other, as the object a command names (object="other") or at the start
	// of a name (other.score), in its commands and its tests. It stays other,
	// found as the rule runs; nameOther lists what each such rule can touch
	// (not the object itself; one of its own state, or of none:
	// RawCollisionRule::touchable), and refuses one that can touch nothing.
	void nameOther(std::vector<RawObject>& objects);

	// Whether a command, or a test, says anything of other.
	bool namesOther(const RawCommand& command);
	bool namesOther(const RawCondition& condition);

	// other made `name` in a test.
	void nameOtherInTest(RawCondition& condition, const std::string& name);

	// The object `from` made `to` in a list of commands: as the object one
	// names, and at the start of a variable or a value. Whether any was.
	bool renameObjectIn(std::vector<RawCommand>& list, const std::string& from, const std::string& to);

	// One of the numbers every object has while the game runs, read and set
	// as owner.part (ball.x, ship.velocity.y): its top left, its velocity,
	// its heading, its altitude and rise, its light and its blink. A
	// variable of the object's own of that name wins. The one list that
	// numberOf, <set> (Game::setNumber) and the expressions' live numbers
	// (Game::refreshLive) all go by: a new part is a row in liveParts.
	struct Parts;
	struct LivePart
	{
		const char* name;
		float& (*of)(Parts& parts, Object& object);
		bool notBelowZero;   // <set> keeps it at 0 or above
		// Whether an object can have it (its part is there); null: every
		// object. One that cannot reads 0 and is not linked (Game::refreshLive).
		bool (*has)(const Parts& parts, const Object& object);
	};
	const std::vector<LivePart>& liveParts();

	// The part of that name (velocity.x), or null.
	const LivePart* livePartNamed(const std::string& part);

	// The names of liveParts, in its order.
	const std::vector<std::string>& liveNumberParts();

	// The part of liveNumberParts() `name` ends with (after a dot), or ""
	// when it ends with none.
	std::string liveNumberPart(const std::string& name);

	// An object's number of that name as it is now: its own variable, or
	// else one of liveNumberParts() (a heading only if it has one); nothing
	// for neither. What a condition tests and compares with.
	std::optional<float> numberOf(const Parts& parts, const Object& object, const std::string& name);

	// `text` with every name in it that starts with `from` (a whole word,
	// followed by a dot) starting with `to` instead.
	std::string renameStart(const std::string& text, const std::string& from, const std::string& to);

	// renameStart on every text in a value, its operations' operands too;
	// whether any changed.
	bool renameInValue(RawValue& value, const std::string& from, const std::string& to);

	// Makes each <centered> and side (<left>, <right>, <top>, <bottom>) of an
	// object's <position> the operation it means: where its middle goes,
	// less half its own width (or height), or its side on the window's,
	// named by the object's final name (so after layOutScenes).
	void centerOnOwnSize(std::vector<RawObject>& objects);

	// Where a <centered> puts the middle: the name, number or operation it
	// holds, its <offset> from `windowMiddle`, or `windowMiddle` itself.
	RawValue centeredMiddle(const RawValue& value, const std::string& windowMiddle);

	// What an object with no <sprite> is drawn as, which is never: an empty
	// rectangle (it only holds numbers; see RawObject::isDrawn).
	RawSprite nothingSprite();

	// Shows the object's look of that name, and starts over the timers that
	// follow it (Timer::sprite); false (and nothing changes) if it has none
	// of that name.
	bool showLookNamed(Object& object, const std::string& name);

	// Puts on the object's palette of that name (Object::Looks::palettes); false
	// (and nothing changes) if it has none of that name.
	bool wearPaletteNamed(Object& object, const std::string& name);
}
