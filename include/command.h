// command.h
// XML Game Engine
// author: beefviper
// date: Sept 27, 2026

#pragma once

#include <ostream>
#include <string>
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

	// inc('objectName') - increment a text object's displayed number
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

	using Command = std::variant<
		CmdBounce, CmdStick, CmdReset, CmdDie,
		CmdMove, CmdIncrement, CmdPushState, CmdPopState,
		CmdFire, CmdTriggerAction>;

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

	std::ostream& operator<<(std::ostream& o, const Command& command);
}

