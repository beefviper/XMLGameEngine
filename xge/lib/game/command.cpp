// command.cpp
// XML Game Engine
// author: beefviper
// date: Sept 27, 2026

#include "game/command.h"

#include "game/object.h"
#include "play/geometry.h"
#include "verbs/verb.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace xge
{
	VariableName VariableName::of(const std::string& written)
	{
		const auto dot = written.rfind('.');
		if (dot == std::string::npos) { return { written, {} }; }
		return { written.substr(0, dot), written.substr(dot + 1) };
	}

	Command makeCommand(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const Verb* verb = findVerb(raw.verb);
		if (!verb) { throw std::runtime_error("unknown command <" + raw.verb + ">"); }
		return verb->make(raw, evaluate);
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

	std::ostream& operator<<(std::ostream& o, const Picture& picture)
	{
		switch (picture.kind)
		{
		case ShapeKind::Circle:    return o << "circle(" << picture.radius << ", " << picture.color << ")";
		case ShapeKind::Rectangle: return o << "rectangle(" << picture.size.x << "x" << picture.size.y << ", " << picture.color << ")";
		case ShapeKind::Text:      return o << "text(\"" << picture.text << "\", " << picture.textSize << ", " << picture.color << (picture.font.empty() ? "" : ", " + picture.font) << ")";
		case ShapeKind::Image:     return o << "image(" << picture.file << (picture.flipHorizontal ? ", flip.horizontal" : picture.flipVertical ? ", flip.vertical" : "") << ")";
		case ShapeKind::Line:      return o << "drawing(" << picture.size.x << "x" << picture.size.y << ")";
		case ShapeKind::Unknown:
		default:                   return o << "nothing";
		}
	}

	std::optional<std::pair<std::string, std::string>> parseVariableReference(const std::string& expression)
	{
		const auto first = expression.find_first_not_of(" \t\r\n");
		if (first == std::string::npos) { return std::nullopt; }
		const auto last = expression.find_last_not_of(" \t\r\n");
		const std::string reference = expression.substr(first, last - first + 1);

		// The variable is after the last dot; the owner, before it, may be a
		// path (playing.room1.tally.kills).
		const auto dot = reference.rfind('.');
		if (dot == std::string::npos || dot == 0 || dot == reference.size() - 1)
		{
			return std::nullopt;
		}

		const auto isIdentChar = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; };

		for (std::size_t i = 0; i < reference.size(); ++i)
		{
			const char c = reference[i];
			if (c == '.' ? (i == 0 || reference[i - 1] == '.') : !isIdentChar(c)) { return std::nullopt; }
		}

		return std::make_pair(reference.substr(0, dot), reference.substr(dot + 1));
	}

	std::ostream& operator<<(std::ostream& o, const Command& command)
	{
		// Two kinds are no verb's: a condition in a list, and a command
		// whose numbers are worked out as it runs.
		if (const auto* when = std::get_if<CmdWhen>(&command))
		{
			o << "when(";
			if (when->body)
			{
				for (std::size_t i = 0; i < when->body->commands.size(); ++i) { o << (i ? ", " : "") << when->body->commands[i]; }
			}
			return o << ")";
		}
		if (const auto* live = std::get_if<CmdLive>(&command))
		{
			return o << "live " << live->loaded->command;
		}

		// Every other is a verb's, which prints it.
		if (const Verb* verb = verbOf(command); verb && verb->print)
		{
			verb->print(o, command);
		}
		return o;
	}

	namespace
	{
		const OperationShape kOperationShapes[] = {
			{ "add", "value", "value", '+' },
			{ "subtract", "value", "value", '-' },
			{ "multiply", "value", "value", '*' },
			{ "divide", "value", "value", '/' },
			{ "remainder", "value", "value", 0, "remainder", 2, 2 },
			{ "smallest", "value", "value", 0, "min" },
			{ "largest", "value", "value", 0, "max" },
			{ "absolute", "value", "value", 0, "abs", 1, 1 },
			{ "negate", "value", "value", 0, "negate", 1, 1 },
			{ "sign", "value", "value", 0, "sign", 1, 1 },
			{ "floor", "value", "value", 0, "floor", 1, 1 },
			{ "ceiling", "value", "value", 0, "ceiling", 1, 1 },
			{ "round", "value", "value", 0, "round", 1, 1 },
			{ "sine", "value", "value", 0, "sine", 1, 1 },
			{ "cosine", "value", "value", 0, "cosine", 1, 1 },
			{ "clamp", "value", "atleast", 0, "clamp", 3, 3, "atmost" },
			// A <value> and a test of it, the same words as a condition's: 1 or 0.
			{ "compare", "value", "test", 0, "compare", 2, 2, nullptr, true },
			// A <random> with an end that is an operation (RawValue::operations).
			{ "random", "min", "max", 0, "random", 2, 2 },
		};
	}

	const OperationShape* operationShape(const std::string& tag)
	{
		for (const OperationShape& shape : kOperationShapes)
		{
			if (tag == shape.tag) { return &shape; }
		}

		return nullptr;
	}

	bool isComparisonWord(const std::string& word)
	{
		return word == "above" || word == "below" || word == "atleast" || word == "atmost" || word == "equal";
	}

	std::string operationTags()
	{
		std::string text;
		const std::size_t count = std::size(kOperationShapes) - 1; // not random
		for (std::size_t i = 0; i < count; ++i)
		{
			if (i > 0) { text += i + 1 == count ? " or " : ", "; }
			text += std::string("<") + kOperationShapes[i].tag + ">";
		}
		return text;
	}

	namespace
	{
		std::string operationText(const RawOperation& operation)
		{
			const OperationShape* shape = operationShape(operation.op);
			if (shape && shape->compares && operation.operands.size() == 2)
			{
				const std::string& test = operation.operands[1].role;
				const char* sign = test == "above" ? ">" : test == "below" ? "<" : test == "atleast" ? ">=" : test == "atmost" ? "<=" : "==";
				return "(" + valueText(operation.operands[0].value) + " " + sign + " " + valueText(operation.operands[1].value) + ")";
			}
			const bool infix = shape && shape->symbol != 0;
			const std::string between = infix ? " " + std::string(1, shape->symbol) + " " : ", ";

			std::string text = infix ? "(" : (shape ? shape->function : operation.op) + std::string("(");
			for (std::size_t i = 0; i < operation.operands.size(); ++i)
			{
				if (i > 0) { text += between; }
				text += valueText(operation.operands[i].value);
			}
			return text + ")";
		}
	}

	std::string valueText(const RawValue& value)
	{
		switch (value.kind)
		{
		case RawValue::Kind::Random:
			if (value.operations && !value.operations->empty()) { return operationText(value.operations->front()); }
			return "random(" + value.min + ", " + value.max + ")";

		case RawValue::Kind::Centered:
			if (value.edge != RawValue::Edge::Middle) { return std::string(value.edge == RawValue::Edge::Start ? "start" : "end") + "(offset " + (value.offset.empty() ? std::string("0") : value.offset) + ")"; }
			if (value.operations && !value.operations->empty()) { return "centered(" + operationText(value.operations->front()) + ")"; }
			return value.offset.empty() ? "centered(" + value.text + ")" : "centered(offset " + value.offset + ")";

		case RawValue::Kind::Formula:
			return value.operations && !value.operations->empty() ? operationText(value.operations->front()) : "";

		case RawValue::Kind::Expression:
			break;
		}

		return value.text;
	}

	std::vector<const std::string*> RawValue::expressions() const
	{
		if (kind == Kind::Random && !operations) { return { &min, &max }; }

		if (kind == Kind::Formula || kind == Kind::Random || (kind == Kind::Centered && operations))
		{
			std::vector<const std::string*> all;
			if (operations)
			{
				for (const RawOperation& operation : *operations)
				{
					for (const RawOperand& operand : operation.operands)
					{
						for (const std::string* expression : operand.value.expressions()) { all.push_back(expression); }
					}
				}
			}
			return all;
		}

		if (kind == Kind::Centered && !offset.empty()) { return { &text, &offset }; }
		return { &text };
	}

	bool RawValue::drawsRandom() const
	{
		if (kind == Kind::Random) { return true; }
		if (!operations) { return false; }
		for (const RawOperation& operation : *operations)
		{
			for (const RawOperand& operand : operation.operands)
			{
				if (operand.value.drawsRandom()) { return true; }
			}
		}
		return false;
	}

	std::ostream& operator<<(std::ostream& o, const RawValue& value)
	{
		return o << valueText(value);
	}

	std::ostream& operator<<(std::ostream& o, const RawCommand& command)
	{
		// The tag, then what its verb says it holds.
		o << command.verb;
		if (const Verb* verb = findVerb(command.verb); verb && verb->printRaw)
		{
			verb->printRaw(o, command);
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

	std::ostream& operator<<(std::ostream& o, const WindowDesc& f)
	{
		o << "window: ";
		o << "title=" << f.name;
		o << ", width=" << f.width;
		o << ", height=" << f.height << '\n';

		return o;
	}

	namespace
	{
		// A rule's selectors and speeds, as both kinds of rule print them.
		template <typename Rule>
		void printRuleFilters(std::ostream& o, const Rule& rule)
		{
			if (!rule.filterClass.empty()) { o << " (class=" << rule.filterClass << ")"; }
			if (!rule.filterObject.empty()) { o << " (object=" << rule.filterObject << ")"; }
			if (!rule.unlessClass.empty()) { o << " (unless=" << rule.unlessClass << ")"; }
			if (!rule.throughClass.empty()) { o << " (through=" << rule.throughClass << ")"; }
		}

		void printCommands(std::ostream& o, const std::vector<Command>& commands)
		{
			for (std::size_t i = 0; i < commands.size(); ++i) { o << (i ? ";" : "") << commands[i]; }
		}
	}

	std::ostream& operator<<(std::ostream& o, const RawCollisionData& f)
	{
		if (f.type == CollisionType::Pixel) { o << ", type=pixel"; }
		for (const Edge edge : { Edge::Top, Edge::Bottom, Edge::Left, Edge::Right })
		{
			const auto& rules = f.edges[edgeIndex(edge)].commands;
			if (!rules.empty()) { o << ", " << edgeName(edge) << "=" << rules; }
		}
		for (const auto& rule : f.others)
		{
			o << ", others=" << rule.commands;
			printRuleFilters(o, rule);
		}
		return o;
	}

	std::ostream& operator<<(std::ostream& o, const CollisionData& f)
	{
		if (f.type == CollisionType::Pixel) { o << ", type=pixel"; }
		if (f.lockstep > 0) { o << ", lockstep=" << f.lockstep; }
		for (const Edge edge : { Edge::Top, Edge::Bottom, Edge::Left, Edge::Right })
		{
			const auto& commands = f.edges[edgeIndex(edge)].commands;
			if (!commands.empty()) { o << ", " << edgeName(edge) << "="; printCommands(o, commands); }
		}
		for (const auto& rule : f.others)
		{
			o << ", others=";
			printCommands(o, rule.commands);
			printRuleFilters(o, rule);
		}
		return o;
	}

	std::ostream& operator<<(std::ostream& o, const RawObject& f) {
		o << "rawObject: " << "name=" << f.name << (f.groupName.empty() ? "" : ", group=" + f.groupName) << ", sprite=" << f.sprite.kind
			<< (f.cell.column > 0 ? " (cell " + std::to_string(f.cell.column) + "." + std::to_string(f.cell.row) + ")" : "")
			<< (f.hasAnimation ? " (animated, " + std::to_string(f.animation.frames.size()) + " frames)" : "") << '\n'
			<< "\tpos.x=" << f.rawPosition.x << ", pos.y=" << f.rawPosition.y << '\n'
			<< "\tvel.x=" << f.rawVelocity.x << ", vel.y=" << f.rawVelocity.y << '\n'
			<< "\tcollision=" << (f.rawCollisionData.enabled ? "true" : "false") << f.rawCollisionData;
		o << '\n';

		for (auto& action : f.action)
		{
			o << "\taction=" << action.first << ", value=" << action.second << '\n';
		}

		for (auto& variable : f.variable)
		{
			o << "\tvariable=" << variable.first << ", value=" << variable.second << '\n';
		}

		return o;
	}

	std::ostream& operator<<(std::ostream& o, const Object& f) {
		o << "Object: " << "name=" << f.name << ", sprite=" << f.picture << '\n';

		o << "\tpos.x=" << f.position.x << ", pos.y=" << f.position.y << '\n';

		o << "\tvel.x=" << f.velocity.x << ", vel.y=" << f.velocity.y << '\n';

		if (f.acceleration.x != 0 || f.acceleration.y != 0)
		{
			o << "\tacc.x=" << f.acceleration.x << ", acc.y=" << f.acceleration.y << '\n';
		}

		if (f.hasHeading)
		{
			o << "\theading=" << f.heading << '\n';
		}

		if (f.animationBitmaps.size() > 1)
		{
			o << "\tanimation=" << f.animationBitmaps.size() << " frames, " << f.animationFrames << " game frames each\n";
		}

		if (f.drag != 0)
		{
			o << "\tdrag=" << f.drag << '\n';
		}

		if (f.mass != 0)
		{
			o << "\tmass=" << f.mass << '\n';
		}

		for (const auto& [name, strength] : f.pulls)
		{
			o << "\tpull=" << name << " " << strength << '\n';
		}

		if (f.light > 0.0f) { o << "\tlight=" << f.light << '\n'; }
		if (f.dark != Object::Darkness::None) { o << "\tdark=" << (f.dark == Object::Darkness::Unlit ? "unlit" : "unseen") << '\n'; }
		if (f.hasAltitude) { o << "\taltitude=" << f.altitude << " rise " << f.rise << " fall " << f.fall << " bounce " << f.altitudeBounce << '\n'; }
		if (!f.shadowOf.empty()) { o << "\tshadow of " << f.shadowOf << '\n'; }
		if (!f.seekTarget.empty())
		{
			o << "\tseek=" << f.seekTarget << " " << f.seekRate << '\n';
		}

		o << "\tsize.x=" << f.size.x << ", size.y=" << f.size.y << '\n';

		o << "\tcollision=" << (f.collisionsOn ? "true" : "false") << f.collisionData;
		o << '\n';

		for (auto& action : f.action)
		{
			o << "\taction=" << action.first << ", value=";
			for (auto& command : action.second)
			{
				o << command << (&command != &action.second.back() ? ";" : "");
			}
			o << '\n';
		}

		return o;
	}
}
