// command.cpp
// XML Game Engine
// author: beefviper
// date: Sept 27, 2026

#include "command.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>

namespace xge
{
	std::vector<Command> parseCommands(const std::vector<std::string>& tokens)
	{
		std::vector<Command> commands;

		std::size_t i = 0;
		while (i < tokens.size())
		{
			const std::string& tag = tokens.at(i);

			if (tag == "collide")
			{
				const std::string& verb = tokens.at(i + 1);
				if (verb == "bounce")    { commands.push_back(CmdBounce{}); }
				else if (verb == "stick") { commands.push_back(CmdStick{}); }
				else if (verb == "reset") { commands.push_back(CmdReset{}); }
				else if (verb == "die")   { commands.push_back(CmdDie{}); }
				i += 2;
			}
			else if (tag == "moveup" || tag == "movedown" || tag == "moveleft" || tag == "moveright")
			{
				const Direction direction = (tag == "moveup") ? Direction::Up
					: (tag == "movedown") ? Direction::Down
					: (tag == "moveleft") ? Direction::Left
					: Direction::Right;
				const float step = std::stof(tokens.at(i + 1));
				commands.push_back(CmdMove{ direction, step });
				i += 2;
			}
			else if (tag == "inc")
			{
				commands.push_back(CmdIncrement{ tokens.at(i + 1) });
				i += 2;
			}
			else if (tag == "state")
			{
				const std::string& target = tokens.at(i + 1);
				if (target == "pop") { commands.push_back(CmdPopState{}); }
				else { commands.push_back(CmdPushState{ target }); }
				i += 2;
			}
			else if (tag == "fire")
			{
				commands.push_back(CmdFire{ tokens.at(i + 1) });
				i += 2;
			}
			else if (tag == "action")
			{
				commands.push_back(CmdTriggerAction{ tokens.at(i + 1), tokens.at(i + 2) });
				i += 3;
			}
			else if (tag == "resetobject")
			{
				commands.push_back(CmdResetObject{ tokens.at(i + 1) });
				i += 2;
			}
			else
			{
				// Unrecognized tag: warn and skip just this one token rather than
				// throwing, so a typo in the XML doesn't take down the whole engine.
				std::cout << "warning: unrecognized command token '" << tag << "'\n";
				i += 1;
			}
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
		// earlier by the matching shape.circle()/shape.rectangle() functor
		// (see game_expr.h), never handed in from outside, so the indices
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

	std::optional<std::pair<std::string, std::string>> parseTextVariableBinding(const std::string& src)
	{
		const auto call = src.find("text(");
		if (call == std::string::npos) { return std::nullopt; }

		const auto argStart = call + 5; // length of "text("

		std::size_t i = argStart;
		int depth = 0;
		bool inQuote = false;

		while (i < src.size())
		{
			const char c = src[i];

			if (inQuote)
			{
				if (c == '\'') { inQuote = false; }
			}
			else if (c == '\'') { inQuote = true; }
			else if (c == '(') { ++depth; }
			else if (c == ')' && depth > 0) { --depth; }
			else if ((c == ',' || c == ')') && depth == 0) { break; }

			++i;
		}

		std::string firstArg = src.substr(argStart, i - argStart);

		const auto first = firstArg.find_first_not_of(" \t");
		if (first == std::string::npos) { return std::nullopt; }
		const auto last = firstArg.find_last_not_of(" \t");
		firstArg = firstArg.substr(first, last - first + 1);

		if (firstArg.empty() || firstArg.front() == '\'')
		{
			return std::nullopt; // a literal string label, e.g. text('0', ...)
		}

		const auto dot = firstArg.find('.');
		if (dot == std::string::npos || dot == 0 || dot == firstArg.size() - 1)
		{
			return std::nullopt;
		}

		const auto isIdentChar = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; };

		for (char c : firstArg)
		{
			if (c != '.' && !isIdentChar(c)) { return std::nullopt; }
		}

		if (std::count(firstArg.begin(), firstArg.end(), '.') != 1)
		{
			return std::nullopt; // only a single "owner.variable" level is supported
		}

		return std::make_pair(firstArg.substr(0, dot), firstArg.substr(dot + 1));
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
			[&](const CmdMove& m)
			{
				const char* direction = (m.direction == Direction::Up) ? "up"
					: (m.direction == Direction::Down) ? "down"
					: (m.direction == Direction::Left) ? "left"
					: "right";
				o << "move." << direction << "(" << m.step << ")";
			},
			[&](const CmdIncrement& c) { o << "inc(" << c.target << ")"; },
			[&](const CmdPushState& s) { o << "state(" << s.name << ")"; },
			[&](const CmdPopState&) { o << "state()"; },
			[&](const CmdFire& f) { o << "fire(" << f.projectileName << ")"; },
			[&](const CmdTriggerAction& a) { o << "action(" << a.object << "," << a.action << ")"; },
			[&](const CmdResetObject& r) { o << "reset(" << r.target << ")"; },
		}, command);

		return o;
	}
}

