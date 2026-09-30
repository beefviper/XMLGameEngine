// command.cpp
// XML Game Engine
// author: beefviper
// date: Sept 27, 2026

#include "command.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace xge
{
	namespace
	{
		Direction directionFromName(const std::string& name, const std::string& verb)
		{
			if (name == "up")    { return Direction::Up; }
			if (name == "down")  { return Direction::Down; }
			if (name == "left")  { return Direction::Left; }
			if (name == "right") { return Direction::Right; }

			throw std::runtime_error("<" + verb + "> has direction=\"" + name + "\"; expected up, down, left or right");
		}
	}

	Command makeCommand(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const std::string& verb = raw.verb;

		if (verb == "bounce") { return CmdBounce{}; }
		if (verb == "stick")  { return CmdStick{}; }
		if (verb == "die")    { return CmdDie{}; }
		if (verb == "wrap")   { return CmdWrap{}; }
		if (verb == "carry")  { return CmdCarry{}; }

		// <reset/> puts the object in the rule (or, in a state's input or
		// condition, the whole game) back; <reset object="name"/> that one object.
		if (verb == "reset")
		{
			if (raw.object.empty()) { return CmdReset{}; }
			return CmdResetObject{ raw.object };
		}

		if (verb == "move") { return CmdMove{ directionFromName(raw.direction, verb), evaluate(raw.amount) }; }
		if (verb == "hop")  { return CmdHop{ directionFromName(raw.direction, verb), evaluate(raw.amount) }; }

		if (verb == "inc") { return CmdIncrement{ raw.variable }; }
		if (verb == "dec") { return CmdDecrement{ raw.variable }; }

		if (verb == "push") { return CmdPushState{ raw.state }; }
		if (verb == "pop")  { return CmdPopState{}; }

		if (verb == "fire")    { return CmdFire{ raw.object }; }
		if (verb == "trigger") { return CmdTriggerAction{ raw.object, raw.action }; }

		throw std::runtime_error("unknown command <" + verb + ">");
	}

	std::vector<Command> makeCommands(const std::vector<RawCommand>& raw, const ValueEvaluator& evaluate)
	{
		std::vector<Command> commands;
		commands.reserve(raw.size());

		for (const RawCommand& rawCommand : raw)
		{
			commands.push_back(makeCommand(rawCommand, evaluate));
		}

		return commands;
	}

	Edge opposite(Edge edge) noexcept
	{
		switch (edge)
		{
		case Edge::Left:   return Edge::Right;
		case Edge::Right:  return Edge::Left;
		case Edge::Top:    return Edge::Bottom;
		case Edge::Bottom: return Edge::Top;
		}
		return edge;
	}

	ShapeKind shapeKindFromTag(const std::string& tag) noexcept
	{
		if (tag == "circle")    { return ShapeKind::Circle; }
		if (tag == "rectangle") { return ShapeKind::Rectangle; }
		if (tag == "text")      { return ShapeKind::Text; }
		if (tag == "image")     { return ShapeKind::Image; }
		return ShapeKind::Unknown;
	}

	Vector2f measureShapeSize(const std::vector<std::string>& spriteParams, ShapeKind shapeKind) noexcept
	{
		// spriteParams is always well-formed here - it was built moments
		// earlier from the object's <circle> or <rectangle> sprite (see
		// game_expr.cpp), never handed in from outside, so the indices
		// below are guaranteed present whenever shapeKind says they should
		// be - same assumption SFMLWindow::buildCircle/buildRectangle (and
		// their Raylib/SDL2 equivalents) already make.
		switch (shapeKind)
		{
		case ShapeKind::Circle:
		{
			const float radius = std::stof(spriteParams.at(1));
			return { radius * 2.0f, radius * 2.0f };
		}
		case ShapeKind::Rectangle:
			return { std::stof(spriteParams.at(1)), std::stof(spriteParams.at(2)) };
		case ShapeKind::Text:
		case ShapeKind::Image:
		case ShapeKind::Unknown:
		default:
			return {};
		}
	}

	std::optional<std::pair<std::string, std::string>> parseVariableReference(const std::string& expression)
	{
		const auto first = expression.find_first_not_of(" \t\r\n");
		if (first == std::string::npos) { return std::nullopt; }
		const auto last = expression.find_last_not_of(" \t\r\n");
		const std::string reference = expression.substr(first, last - first + 1);

		const auto dot = reference.find('.');
		if (dot == std::string::npos || dot == 0 || dot == reference.size() - 1)
		{
			return std::nullopt;
		}

		const auto isIdentChar = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; };

		for (char c : reference)
		{
			if (c != '.' && !isIdentChar(c)) { return std::nullopt; }
		}

		if (std::count(reference.begin(), reference.end(), '.') != 1)
		{
			return std::nullopt; // only a single "owner.variable" level is supported
		}

		return std::make_pair(reference.substr(0, dot), reference.substr(dot + 1));
	}

	std::string formatDisplayNumber(float value)
	{
		if (value == std::floor(value))
		{
			return std::to_string(static_cast<long long>(value));
		}
		return std::to_string(value);
	}

	std::ostream& operator<<(std::ostream& o, const Command& command)
	{
		std::visit(overload{
			[&](const CmdBounce&) { o << "bounce"; },
			[&](const CmdStick&) { o << "stick"; },
			[&](const CmdReset&) { o << "reset"; },
			[&](const CmdDie&) { o << "die"; },
			[&](const CmdWrap&) { o << "wrap"; },
			[&](const CmdCarry&) { o << "carry"; },
			[&](const CmdMove& m)
			{
				const char* direction = (m.direction == Direction::Up) ? "up"
					: (m.direction == Direction::Down) ? "down"
					: (m.direction == Direction::Left) ? "left"
					: "right";
				o << "move." << direction << "(" << m.step << ")";
			},
			[&](const CmdHop& h)
			{
				const char* direction = (h.direction == Direction::Up) ? "up"
					: (h.direction == Direction::Down) ? "down"
					: (h.direction == Direction::Left) ? "left"
					: "right";
				o << "hop." << direction << "(" << h.distance << ")";
			},
			[&](const CmdIncrement& c) { o << "inc(" << c.target << ")"; },
			[&](const CmdDecrement& c) { o << "dec(" << c.target << ")"; },
			[&](const CmdPushState& s) { o << "state(" << s.name << ")"; },
			[&](const CmdPopState&) { o << "state()"; },
			[&](const CmdFire& f) { o << "fire(" << f.projectileName << ")"; },
			[&](const CmdTriggerAction& a) { o << "action(" << a.object << "," << a.action << ")"; },
			[&](const CmdResetObject& r) { o << "reset(" << r.target << ")"; },
		}, command);

		return o;
	}

	std::ostream& operator<<(std::ostream& o, const RawValue& value)
	{
		if (value.kind == RawValue::Kind::Random)
		{
			return o << "random(" << value.min << ", " << value.max << ")";
		}

		return o << value.text;
	}

	std::ostream& operator<<(std::ostream& o, const RawCommand& command)
	{
		o << command.verb;

		if (command.verb == "move" || command.verb == "hop")
		{
			o << "." << command.direction << "(" << command.amount << ")";
		}
		else if (command.verb == "inc" || command.verb == "dec")
		{
			o << "(" << command.variable << ")";
		}
		else if (command.verb == "push")
		{
			o << "(" << command.state << ")";
		}
		else if (command.verb == "fire" || (command.verb == "reset" && !command.object.empty()))
		{
			o << "(" << command.object << ")";
		}
		else if (command.verb == "trigger")
		{
			o << "(" << command.object << "," << command.action << ")";
		}

		return o;
	}

	std::ostream& operator<<(std::ostream& o, const std::vector<RawCommand>& commands)
	{
		for (std::size_t i = 0; i < commands.size(); ++i)
		{
			o << (i ? ";" : "") << commands[i];
		}

		return o;
	}
}
