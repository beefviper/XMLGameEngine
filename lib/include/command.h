// command.h
// XML Game Engine
// author: beefviper
// date: Sept 27, 2026

#pragma once

#include "types.h"

#include <functional>
#include <ostream>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace xge
{
	// A small std::visit helper: overload{lambdas...} builds a callable overload
	// set out of several lambdas, one per Command alternative.
	template <typename... Ts>
	struct overload : Ts...
	{
		using Ts::operator()...;
	};

	template <typename... Ts>
	overload(Ts...) -> overload<Ts...>;

	enum class Direction { Up, Down, Left, Right };

	// Which side of a screen or of another object's bounding box was touched.
	enum class Edge { Top, Bottom, Left, Right };

	// The edge you'd see the same contact from, standing on the other object:
	// if you hit something's Left edge, you were hit on your own Right.
	Edge opposite(Edge edge) noexcept;

	// What kind of sprite an Object was built from. Set once in game_expr::init()
	// from the object's spriteParams tag, so collision code no longer has to
	// re-derive it by searching the object's raw XML src string.
	enum class ShapeKind { Unknown, Circle, Rectangle, Text, Image, Line };

	// --- Command: what a command tag in the XML becomes once it is loaded.
	// makeCommand() (command.cpp) is the one place that turns a RawCommand
	// into one of these.

	struct CmdBounce {};
	struct CmdStick {};
	struct CmdReset {};
	struct CmdDie {};

	// <wrap /> - once the object has gone right off the screen through an edge,
	// it comes back in from the opposite one (see CommandExecutor::wrap).
	struct CmdWrap {};

	// <carry /> - in a collision rule about an object it is touching: the
	// object rides along with that other one, at its velocity, for as long
	// as they keep touching (see CommandExecutor::carry).
	struct CmdCarry {};

	struct CmdMove
	{
		Direction direction{};
		float step{};
	};

	// <inc variable="objectName" /> - add 1 to (or, for <inc variable="...">5</inc>, add the amount to) a text object's own displayed number, OR
	// <inc variable="ownerName.variableName" /> - increment that object's named <variable>
	// and refresh every text object whose display is bound to it (see
	// Game::incrementText and Object::boundVariableOwner/boundVariableName).
	struct CmdIncrement
	{
		std::string target;
		float amount{ 1.0f };
	};

	// <dec variable="ownerName.variableName" /> - the opposite of <inc />: takes 1 off that
	// object's named <variable> (or off a text object's own number) and
	// refreshes every text bound to it.
	struct CmdDecrement
	{
		std::string target;
		float amount{ 1.0f };
	};

	// <hop direction="up">distance</hop> (also down, left, right) - only ever in an
	// object's own <action>: a one-shot
	// jump of `distance` pixels, made once for each press of the key (see
	// CommandExecutor::triggerObjectAction and Game::moveObjects). Unlike
	// <move>, holding the key does nothing more, and a state change never
	// resumes it.
	struct CmdHop
	{
		Direction direction{};
		float distance{};
	};

	// <accelerate direction="up" burn="fuel">0.04</accelerate> - only ever in an
	// object's own <action>: while the key is held the object's velocity
	// changes by `amount` every frame in that direction (a thruster), where
	// <move> would set the velocity itself. burn names one of the object's own
	// <variable>s that one is taken off every frame the thrust is on, and the
	// thrust does nothing while it is at 0 or below (fuel); empty means free
	// (see Game::applyAcceleration).
	struct CmdAccelerate
	{
		Direction direction{};
		float amount{};
		std::string burn;
	};

	// <turn direction="left">3</turn> (or right) - only ever in an object's own
	// <action>: while the key is held the object's heading changes by `rate`
	// degrees every frame, counterclockwise for left and clockwise for right
	// (see Object::heading, Game::applyAcceleration). Held like a <move>.
	struct CmdTurn
	{
		Direction direction{};
		float rate{};
	};

	// <thrust burn="fuel">0.12</thrust> - only ever in an object's own <action>:
	// while the key is held the object's velocity changes by `amount` every
	// frame along the way it is facing (its heading), where <accelerate> pushes
	// along an axis. burn works as it does for <accelerate>.
	struct CmdThrust
	{
		float amount{};
		std::string burn;
	};

	// <release object="mediumrocks">2</release> - in a collision rule: puts
	// `count` (1 if it is left out) of the objects of that name that are out of
	// play - hidden at the start, or taken out with <die /> - back in play, in
	// the middle of the object that is running the rule, at their own starting
	// velocity. A group is how a pool of them is named: a rock that breaks
	// into smaller rocks releases them from the pool of smaller rocks.
	struct CmdRelease
	{
		std::string target;
		int count{ 1 };
	};

	// <stop /> - in a collision rule: the object comes to rest where it is and
	// stays there. Its velocity goes to 0, it is no longer pulled by its
	// <acceleration> or pushed by a held <accelerate>, until a reset gives
	// those back. A landing.
	struct CmdStop {};

	struct CmdPushState
	{
		std::string name;
	};

	struct CmdPopState {};

	// <fire object="projectileName" /> - spawn/launch a named projectile object
	struct CmdFire
	{
		std::string projectileName;
	};

	// <trigger object="objectName" action="actionName" /> - only ever appears in a
	// State's <input> list; triggers one of another object's own named <action> entries.
	struct CmdTriggerAction
	{
		std::string object;
		std::string action;
	};

	// <reset object="objectName" /> - only ever appears in a State's <input> or
	// <condition> (unlike the bare, untargeted CmdReset above, which is
	// also what a collision's own <reset /> produces); resets that named
	// object's position, velocity, and every <variable> back to their
	// starting values (see Game::resetObject).
	struct CmdResetObject
	{
		std::string target;
	};

	using Command = std::variant<
		CmdBounce, CmdStick, CmdReset, CmdDie, CmdWrap, CmdCarry,
		CmdMove, CmdHop, CmdAccelerate, CmdTurn, CmdThrust, CmdRelease, CmdStop, CmdIncrement, CmdDecrement, CmdPushState, CmdPopState,
		CmdFire, CmdTriggerAction, CmdResetObject>;

	// --- What the XML says, before any of it is evaluated.

	// A value in the XML: an element's content wherever a number is wanted.
	// It is either a math expression written out as text (`window.width.center
	// - title.width / 2`), or one tag that makes the number, such as
	// <random min="-7" max="7"/>. game_expr evaluates it (see
	// game_expr::evaluate), so a new kind of value tag is one more Kind here,
	// one more case in game_xml's readValue and one more in game_expr's
	// evaluate.
	struct RawValue
	{
		enum class Kind { Expression, Random };

		Kind kind{ Kind::Expression };

		// Expression: the expression. Random: unused.
		std::string text;

		// Random: the two ends, each itself an expression.
		std::string min;
		std::string max;

		static RawValue expression(std::string text)
		{
			RawValue value;
			value.text = std::move(text);
			return value;
		}

		// Every expression written inside this value, for the ones that only
		// need to look at what the expressions name (see
		// game_expr::sizeDependenciesOf).
		std::vector<const std::string*> expressions() const
		{
			if (kind == Kind::Random) { return { &min, &max }; }
			return { &text };
		}
	};

	// One command tag from the XML - <bounce/>, <inc variable="a.b"/>,
	// <move direction="up">step</move>, ... - before its number is worked out.
	// `verb` is the tag name; which of the other fields are used depends on it
	// (see makeCommand).
	struct RawCommand
	{
		std::string verb;
		std::string object;    // reset, fire, release, trigger
		std::string variable;  // inc, dec
		std::string state;     // push
		std::string action;    // trigger
		std::string direction; // move, hop, accelerate, turn
		std::string burn;      // accelerate, thrust
		RawValue amount;       // move, hop, accelerate, turn, thrust, release (how many)
	};

	// Works out a RawValue to a number; makeCommand is given one so that it can
	// stay free of the expression library (and be tested without it).
	using ValueEvaluator = std::function<float(const RawValue&)>;

	// Turns what the XML says into the typed Commands the engine runs. This is
	// the one place that knows what each command tag means; an unknown tag or
	// direction throws std::runtime_error naming it, so a typo in a game file
	// is reported when it loads.
	Command makeCommand(const RawCommand& raw, const ValueEvaluator& evaluate);
	std::vector<Command> makeCommands(const std::vector<RawCommand>& raw, const ValueEvaluator& evaluate);

	// Maps a spriteParams tag ("circle", "rectangle", "text", "image", "line") to a
	// ShapeKind. Returns ShapeKind::Unknown for anything else (including an
	// empty tag).
	ShapeKind shapeKindFromTag(const std::string& tag) noexcept;

	// The object's own footprint, straight from its spriteParams - no Window
	// backend required. Circle/rectangle sizes are fully implied by the
	// numeric params already sitting in spriteParams (radius, or width and
	// height), so this is exact for Rectangle and a very close approximation
	// for Circle (matches a rendered circle's bounding box to within
	// sub-pixel rounding). Text/image footprints genuinely depend on a real
	// font/image load - there's no way to know them without a backend - so
	// those (and ShapeKind::Unknown) come back {0,0}. Used by
	// game_expr::init() to finalize <grid> spacing entirely within Game's
	// own construction, before any Window exists (see window.h's Window::
	// init(), which now only measures the *real* rendered Object::size for
	// drawing/collision, not position).
	Vector2f measureShapeSize(const std::vector<std::string>& spriteParams, ShapeKind shapeKind) noexcept;

	// If `expression` is nothing but an "owner.variable" reference (for
	// example paddle1.score), returns {owner, variable}. Returns nullopt for
	// anything else - a number, a sum, a name with no owner or with more than
	// one dot. Used to tell a <number> in a text sprite that should stay live
	// (bound to that variable) from one that is just worked out once; it is a
	// plain scan of the text rather than an exprtk lookup, since once
	// exprtk has evaluated the expression only the resolved value is left.
	std::optional<std::pair<std::string, std::string>> parseVariableReference(const std::string& expression);

	// Formats a live numeric value for on-screen display: whole numbers print
	// without a decimal point (scores, HP, ammo, ...), matching what someone
	// hand-typing a <text> content of 0 would have written.
	std::string formatDisplayNumber(float value);

	std::ostream& operator<<(std::ostream& o, const Command& command);

	// How a value and a command tag read in printGame()'s output.
	std::ostream& operator<<(std::ostream& o, const RawValue& value);
	std::ostream& operator<<(std::ostream& o, const RawCommand& command);
	std::ostream& operator<<(std::ostream& o, const std::vector<RawCommand>& commands);
}
