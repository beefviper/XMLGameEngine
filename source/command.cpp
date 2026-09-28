// command.cpp
// XML Game Engine
// author: beefviper
// date: Sept 27, 2026

#include "command.h"

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
		}, command);

		return o;
	}
}

