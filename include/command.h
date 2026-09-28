// command.h
// XML Game Engine
// author: beefviper
// date: Sept 27, 2026

#pragma once

#include "types.h"

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
	enum class ShapeKind { Unknown, Circle, Rectangle, Text, Image };

	// --- Command: a typed replacement for the flat vector<string> "token" format
	// that used to be hand-decoded (inconsistently) in three separate places:
	// Game::checkEdge, Game::circleRectangleCollision, and Engine::execute_action.
	// parseCommands() (command.cpp) is now the one and only place that turns the
	// raw exprtk output tokens into these.

	struct CmdBounce {};
	struct CmdStick {};
	struct CmdReset {};
	struct CmdDie {};

	struct CmdMove
	{
		Direction direction{};
		float step{};
	};

	// inc('objectName') - increment a text object's own displayed number, OR
	// inc('ownerName.variableName') - increment that object's named <variable>
	// and refresh every text object whose display is bound to it (see
	// Game::incrementText and Object::boundVariableOwner/boundVariableName).
	struct CmdIncrement
	{
		std::string target;
	};

	struct CmdPushState
	{
		std::string name;
	};

	struct CmdPopState {};

	// fire('projectileName') - spawn/launch a named projectile object
	struct CmdFire
	{
		std::string projectileName;
	};

	// action('objectName', 'actionName') - only ever appears in a State's <input>
	// list; triggers one of another object's own named <action> entries.
	struct CmdTriggerAction
	{
		std::string object;
		std::string action;
	};

	// reset('objectName') - only ever appears in a State's <input> or
	// <condition> action (unlike the bare, untargeted CmdReset above, which is
	// only ever produced by a collision's own reset()); resets that named
	// object's position, velocity, and every <variable> back to their
	// starting values (see Game::resetObject).
	struct CmdResetObject
	{
		std::string target;
	};

	using Command = std::variant<
		CmdBounce, CmdStick, CmdReset, CmdDie,
		CmdMove, CmdIncrement, CmdPushState, CmdPopState,
		CmdFire, CmdTriggerAction, CmdResetObject>;

	// Turns the flat token stream produced by game_expr's exprtk functors (e.g.
	// {"collide", "bounce"} or {"moveup", "2"}) into a sequence of typed Commands.
	// This replaces the duplicated, subtly-inconsistent token walking that used to
	// live in Game::checkEdge, Game::circleRectangleCollision, and
	// Engine::execute_action.
	std::vector<Command> parseCommands(const std::vector<std::string>& tokens);

	// Maps a spriteParams tag ("circle", "rectangle", "text", "image") to a
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
	// game_expr::init() to finalize grid() spacing entirely within Game's
	// own construction, before any Window exists (see window.h's Window::
	// init(), which now only measures the *real* rendered Object::size for
	// drawing/collision, not position).
	Vector2f measureShapeSize(const std::vector<std::string>& spriteParams, ShapeKind shapeKind) noexcept;

	// If `src` contains a text(...) call whose first argument is an unquoted
	// "owner.variable" reference (e.g. text(paddle1.score,128,'color.white')),
	// returns {owner, variable}. Returns nullopt for a literal string label
	// (e.g. text('0',128,'color.blue')) or anything else - a plain string scan
	// rather than an exprtk lookup, since by the time an exprtk function call
	// evaluates, the original argument text (a symbol name vs. a literal) is
	// already gone - only the resolved value is left.
	std::optional<std::pair<std::string, std::string>> parseTextVariableBinding(const std::string& src);

	// Formats a live numeric value for on-screen display: whole numbers print
	// without a decimal point (scores, HP, ammo, ...), matching what someone
	// hand-typing text('0', ...) would have written.
	std::string formatDisplayNumber(float value);

	std::ostream& operator<<(std::ostream& o, const Command& command);
}

