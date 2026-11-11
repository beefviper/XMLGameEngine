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
	namespace
	{
		std::string placeText(const CellPlace& place)
		{
			if (!place.any()) { return {}; }
			std::string text = place.where == CellPlace::Where::Here ? ", here" : ", around";
			if (place.straight) { text += " straight"; }
			if (!place.alike.empty()) { text += " alike " + place.alike; }
			return text;
		}
	}

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

	const Command& shapeOf(const Command& command) noexcept
	{
		if (const auto* live = std::get_if<CmdLive>(&command))
		{
			return live->loaded->command;
		}
		return command;
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

	Vector2f measureShapeSize(const Picture& picture) noexcept
	{
		switch (picture.kind)
		{
		case ShapeKind::Circle:
			return { picture.radius * 2.0f, picture.radius * 2.0f };
		case ShapeKind::Rectangle:
		case ShapeKind::Line:
			return picture.size;
		case ShapeKind::Text:
		case ShapeKind::Image:
		case ShapeKind::Unknown:
		default:
			return {};
		}
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
			[&](const CmdBounce& b) { o << "bounce(" << b.share << ")"; },
			[&](const CmdStick&) { o << "stick"; },
			[&](const CmdReset&) { o << "reset"; },
			[&](const CmdDie&) { o << "die"; },
			[&](const CmdDespawn& d) { o << "despawn" << (d.target.empty() ? "" : "(" + d.target + ")"); },
			[&](const CmdStop&) { o << "stop"; },
			[&](const CmdSettle&) { o << "settle"; },
			[&](const CmdClear&) { o << "clear"; },
			[&](const CmdCollapse&) { o << "collapse"; },
			[&](const CmdFall& fall) { o << "fall"; if (!fall.objClass.empty()) { o << "(" << fall.objClass << ")"; } },
			[&](const CmdCycle& c) { o << (c.backward ? "cycle backward" : "cycle"); },
			[&](const CmdGrow&) { o << "grow"; },
			[&](const CmdSplit&) { o << "split"; },
			[&](const CmdThrough&) { o << "through"; },
			[&](const CmdShove&) { o << "shove"; },
			[&](const CmdMerge& m) { o << "merge(" << m.variable << ")"; },
			[&](const CmdUndo&) { o << "undo"; },
			[&](const CmdWhen& when)
			{
				o << "when(";
				if (when.body)
				{
					for (std::size_t i = 0; i < when.body->commands.size(); ++i) { o << (i ? ", " : "") << when.body->commands[i]; }
				}
				o << ")";
			},
			[&](const CmdPick& p) { o << "pick(" << p.objClass << (p.only ? ", only" : "") << ")"; },
			[&](const CmdPlace& p) { o << "place" << (p.into.empty() ? "" : "(" + p.into + ")"); },
			[&](const CmdCall& c) { o << "call(" << c.pile << ")"; },
			[&](const CmdOrder& r) { o << "order(" << r.group << ", random)"; },
			[&](const CmdPut& p) { o << "put(" << p.objClass << (p.closeClass.empty() ? "" : ", close " + p.closeClass + " " + p.closeObject) << ")"; },
			[&](const CmdCount& c) { o << "count(" << c.objClass << ", " << c.target.written() << placeText(c.place) << ")"; },
			[&](const CmdSlide& m)
			{
				o << "slide." << ((m.direction == Direction::Up) ? "up" : (m.direction == Direction::Down) ? "down" : (m.direction == Direction::Left) ? "left" : "right");
				if (!m.objClass.empty()) { o << "(" << m.objClass << ")"; }
			},
			[&](const CmdWrap&) { o << "wrap"; },
			[&](const CmdRide&) { o << "ride"; },
			[&](const CmdCarry&) { o << "carry"; },
			[&](const CmdDrop&) { o << "drop"; },
			[&](const CmdTrack& t)
			{
				o << "track" << (t.path.empty() ? "" : "(" + t.path + ")");
				for (const std::string& onto : t.onto) { o << " onto " << onto; }
			},
			[&](const CmdDeflect& d) { o << "deflect(" << d.maxAngle << ")"; },
			[&](const CmdKnock& k) { o << "knock(" << k.bounce << ")"; },
			[&](const CmdHold& h) { o << "hold(" << keyCodeToString(h.button) << ")"; },
			[&](const CmdThink& t)
			{
				o << "think(" << t.objClass << ", " << t.picker << ", " << keyCodeToString(t.button) << ", " << t.depth;
				for (const auto& [look, worth] : t.worths) { o << ", " << look << "=" << worth; }
				o << ")";
			},
			[&](const CmdHole& h) { o << "hole(" << h.radius << ", " << h.rough << ")"; },
			[&](const CmdReverse&) { o << "reverse"; },
			[&](const CmdMove& m)
			{
				const char* direction = (m.direction == Direction::Up) ? "up"
					: (m.direction == Direction::Down) ? "down"
					: (m.direction == Direction::Left) ? "left"
					: "right";
				o << "move." << direction << "(" << m.step << (m.wait > 0.0f ? ", wait " + formatDisplayNumber(m.wait) : std::string{}) << ")";
			},
			[&](const CmdHop& h)
			{
				const char* direction = (h.direction == Direction::Up) ? "up"
					: (h.direction == Direction::Down) ? "down"
					: (h.direction == Direction::Left) ? "left"
					: "right";
				if (h.step.x != 0.0f && h.step.y != 0.0f) { o << "hop(" << h.step.x << ", " << h.step.y; }
				else { o << "hop." << direction << "(" << h.distance; }
				o << (h.seconds > 0.0f ? ", " + formatDisplayNumber(h.seconds) + "s" : std::string{})
					<< (h.arc > 0.0f ? ", arc " + formatDisplayNumber(h.arc) : std::string{});
				for (const std::string& onto : h.onto) { o << ", onto " << onto; }
				o << ")";
			},
			[&](const CmdHuntHop& h)
			{
				o << "hunt(" << h.target << ", by " << (h.diagonal ? "diagonal " : "") << "hops of " << h.across << ", " << h.down
					<< (h.seconds > 0.0f ? ", " + formatDisplayNumber(h.seconds) + "s" : std::string{})
					<< (h.arc > 0.0f ? ", arc " + formatDisplayNumber(h.arc) : std::string{}) << ")";
			},
			[&](const CmdLand&) { o << "land()"; },
			[&](const CmdJump& j) { o << "jump(" << j.height << ")"; },
			[&](const CmdClimb& c) { o << "climb." << (c.direction == Direction::Up ? "up" : "down") << "(" << c.step << ", " << c.ladderClass << ")"; },
			[&](const CmdChase& c) { o << "chase(" << c.target << ", " << c.speed << (c.stopWithin > 0.0f ? ", near " + formatDisplayNumber(c.stopWithin) : std::string{}) << ")"; },
			[&](const CmdGo& g)
			{
				const char* direction = (g.direction == Direction::Up) ? "up"
					: (g.direction == Direction::Down) ? "down"
					: (g.direction == Direction::Left) ? "left"
					: "right";
				o << "go." << direction << "(" << g.speed << ")";
			},
			[&](const CmdHunt& h)
			{
				o << "hunt(" << h.target << ", " << h.speed;
				if (h.ahead != 0.0f) { o << ", ahead " << h.ahead; }
				if (!h.partner.empty()) { o << ", with " << h.partner; }
				if (!h.retreatTo.empty()) { o << ", retreat " << h.retreatTo << " within " << h.retreatWithin; }
				o << ")";
			},
			[&](const CmdWander& w) { o << "wander(" << w.speed << ")"; },
			[&](const CmdAim& a) { o << "aim(" << a.target << (a.lined ? ", lined " + std::to_string(a.within) : std::string()) << ")"; },
			[&](const CmdThrustAcross& a)
			{
				const char* direction = (a.direction == Direction::Up) ? "up"
					: (a.direction == Direction::Down) ? "down"
					: (a.direction == Direction::Left) ? "left"
					: "right";
				o << "thrust." << direction << "(" << a.amount << (a.burn.empty() ? "" : ", burn " + a.burn) << ")";
			},
			[&](const CmdTurn& t) { o << "turn." << (t.direction == Direction::Left ? "counterclockwise" : "clockwise") << "(" << t.rate << ")"; },
			[&](const CmdSwing& s) { o << "swing(" << s.by << " degrees, " << s.seconds << "s)"; },
			[&](const CmdThrust& t) { o << "thrust(" << t.amount << (t.burn.empty() ? "" : ", burn " + t.burn) << ")"; },
			[&](const CmdIncrement& c) { o << "inc(" << c.target.written() << (c.amount == 1.0f ? "" : ", " + formatDisplayNumber(c.amount)) << ")"; },
			[&](const CmdDecrement& c) { o << "dec(" << c.target.written() << (c.amount == 1.0f ? "" : ", " + formatDisplayNumber(c.amount)) << ")"; },
			[&](const CmdCopy& c) { o << "copy(" << c.from.written() << ", " << c.into.written() << ")"; },
			[&](const CmdRank& c) { o << "rank(" << c.entry.written() << ", " << c.table << ")"; },
			[&](const CmdAlternate& c)
			{
				o << "alternate(";
				for (std::size_t i = 0; i < c.players.size(); ++i) { o << (i ? ", " : "") << (c.players[i].empty() ? "-" : c.players[i]); }
				o << ")";
			},
			[&](const CmdPass&) { o << "pass()"; },
			[&](const CmdOut&) { o << "out()"; },
			[&](const CmdDemo&) { o << "demo()"; },
			[&](const CmdPushState& s) { o << "state(" << s.name << ")"; },
			[&](const CmdPopState& s) { o << "state()"; if (!s.name.empty()) { o << " state(" << s.name << ")"; } },
			[&](const CmdFire& f) { o << "fire(" << f.projectileName << ")"; },
			[&](const CmdTriggerAction& a) { o << "action(" << a.object << "," << a.action << (a.from.empty() ? std::string{} : "," + a.from) << (a.objClass.empty() ? std::string{} : ", " + a.objClass) << placeText(a.place) << ")"; },
			[&](const CmdResetObject& r) { o << "reset(" << r.target << ")"; },
			[&](const CmdPlay& p) { o << "play(" << p.sound << ")"; },
			[&](const CmdSilence& s) { o << "silence(" << s.sound << ")"; },
			[&](const CmdBecome& b) { o << "become(" << b.sprite << (b.target.empty() ? "" : ", " + b.target) << (b.from.empty() ? "" : ", from " + b.from) << (b.style.empty() ? "" : ", style " + b.style) << ")"; },
			[&](const CmdReveal& r) { o << "reveal(" << r.target << ", " << r.count << ")"; },
			[&](const CmdShift& s) { o << "shift(" << s.target << ", " << s.by.x << ", " << s.by.y << ")"; },
			[&](const CmdFollow& f) { o << "follow(" << f.path << (f.target.empty() ? "" : ", " + f.target) << ")"; },
			[&](const CmdEnterScene& e) { o << "enter(" << (e.way.empty() ? e.scene : "way " + e.way) << ")"; },
			[&](const CmdResetScene& r) { o << "reset(" << (r.way.empty() ? "scene " + r.scene : "way " + r.way) << ")"; },
			[&](const CmdSet& s) { o << "set(" << s.target.written() << ", " << s.value << ")"; },
			[&](const CmdSpawn& s)
			{
				const auto half = [&](const std::optional<float>& value, bool centered)
				{
					if (!value) { o << "-"; return; }
					o << (centered ? "centered " : "") << *value;
				};
				o << "spawn(" << s.target << (s.count != 1 ? " x" + std::to_string(s.count) : "") << ", ";
				half(s.x, s.centerX);
				o << " ";
				half(s.y, s.centerY);
				o << ", ";
				half(s.velocityX, false);
				o << " ";
				half(s.velocityY, false);
				if (s.accelerationX || s.accelerationY)
				{
					o << ", ";
					half(s.accelerationX, false);
					o << " ";
					half(s.accelerationY, false);
				}
				o << ")";
			},
			[&](const CmdSend& s)
			{
				o << "send(" << s.target << ", " << s.from << " into " << s.into;
				if (s.column && s.row) { o << ", " << *s.column << " " << *s.row; }
				o << ")";
			},
			[&](const CmdLive& l) { o << "live " << l.loaded->command; },
		}, command);

		return o;
	}

	namespace
	{
		const OperationShape kOperationShapes[] = {
			{ "add", "augend", "addend", '+' },
			{ "subtract", "minuend", "subtrahend", '-' },
			{ "multiply", "multiplicand", "multiplier", '*' },
			{ "divide", "dividend", "divisor", '/' },
			{ "remainder", "dividend", "divisor", 0, "remainder", 2, 2 },
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
			// A <random> with a <formula> end (RawValue::operations).
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

		if (kind == Kind::Formula || kind == Kind::Random)
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
		o << command.verb;

		if (command.verb == "swing")
		{
			o << "(" << command.direction << " " << command.distance << " degrees, " << command.seconds << "s)";
		}
		else if (command.verb == "thrust" || command.verb == "deflect")
		{
			o << "(" << command.amount << (command.burn.empty() ? "" : ", burn " + command.burn) << ")";
		}
		else if (command.verb == "hold")
		{
			o << "(" << command.button << ")";
		}
		else if (command.verb == "hole")
		{
			o << "(" << command.amount << ", " << command.distance << ")";
		}
		else if (command.verb == "jump" || ((command.verb == "knock" || command.verb == "bounce") && (command.amount.kind != RawValue::Kind::Expression || !command.amount.text.empty())))
		{
			o << "(" << command.amount << ")";
		}
		else if (command.verb == "climb")
		{
			o << "." << command.direction << "(" << command.amount << ", " << command.objClass << ")";
		}
		else if (command.verb == "hop" || (command.verb == "hunt" && command.byHops))
		{
			o << (command.byHops ? "(" + command.object + ", by hops of " : "." + command.direction + "(") << command.amount << ", " << command.positionX << " " << command.positionY
				<< ", " << command.seconds << ")";
		}
		else if (command.verb == "move" || command.verb == "turn" || command.verb == "go")
		{
			o << "." << command.direction << "(" << command.amount << (command.burn.empty() ? "" : ", burn " + command.burn) << ")";
			if (command.verb == "move" && !(command.seconds.kind == RawValue::Kind::Expression && command.seconds.text.empty())) { o << "(wait " << command.seconds << ")"; }
		}
		else if (command.verb == "slide")
		{
			o << "." << command.direction << (command.objClass.empty() ? "" : "(" + command.objClass + ")");
		}
		else if (command.verb == "inc" || command.verb == "dec")
		{
			o << "(" << command.variable;
			if (!(command.amount.kind == RawValue::Kind::Expression && (command.amount.text == "1" || command.amount.text.empty()))) { o << ", " << command.amount; }
			o << ")";
		}
		else if (command.verb == "push")
		{
			o << "(" << command.state << ")";
		}
		else if (command.verb == "enter" || (command.verb == "reset" && !command.scene.empty()))
		{
			o << "(scene " << command.scene << ")";
		}
		else if (command.verb == "chase")
		{
			o << "(" << command.object << ", " << command.amount << ", " << command.distance << ")";
		}
		else if (command.verb == "hunt")
		{
			o << "(" << command.object << ", " << command.amount << ", ahead " << command.ahead << ", with " << command.partner
				<< ", retreat " << command.retreat << " " << command.distance << ")";
		}
		else if (command.verb == "wander")
		{
			o << "(" << command.amount << ")";
		}
		else if (command.verb == "fire" || command.verb == "aim" || (command.verb == "reset" && !command.object.empty()) || (command.verb == "despawn" && !command.object.empty()))
		{
			o << "(" << command.object << ")";
		}
		else if (command.verb == "play" || command.verb == "silence")
		{
			o << "(" << command.sound << ")";
		}
		else if (command.verb == "trigger")
		{
			o << "(" << command.object << "," << command.action << (command.place.empty() ? "" : ", " + command.objClass + " " + command.place) << ")";
		}
		else if (command.verb == "follow")
		{
			o << "(" << command.path << (command.object.empty() ? "" : ", " + command.object) << ")";
		}
		else if (command.verb == "track" && !command.path.empty())
		{
			o << "(" << command.path << ")";
		}
		else if (command.verb == "set")
		{
			o << "(" << command.variable << ", " << command.amount << ")";
		}
		else if (command.verb == "spawn")
		{
			o << "(" << command.object << ", " << command.amount << ", " << command.positionX << " " << command.positionY << ", " << command.velocityX << " " << command.velocityY << ", " << command.accelerationX << " " << command.accelerationY
				<< (command.random ? ", random" : "") << (command.free ? ", free" : "") << " " << command.below << ", " << command.column << " " << command.row << (command.into.empty() ? "" : ", into " + command.into) << ")";
		}
		else if (command.verb == "send")
		{
			o << "(" << command.object << ", " << command.from << " into " << command.into << ", " << command.column << " " << command.row << ")";
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
