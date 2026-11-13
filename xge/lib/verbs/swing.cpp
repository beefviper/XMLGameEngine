// swing.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/reading.h"

#include <algorithm>
#include <memory>
#include <stdexcept>

namespace xge
{
	// <swing>: One eased stroke round its pivot while held, and back when let
	// go.
	static void readSwing(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		requireAttribute(node, "direction", where);
		std::unique_ptr<XmlNode> degrees = findChild(&node, "degrees");
		std::unique_ptr<XmlNode> seconds = findChild(&node, "seconds");
		if (!degrees || !seconds) { fail(where, "<swing> gives how far it turns as <degrees>, and how long the stroke takes as <seconds>"); }
		command.distance = readValue(*degrees, where + " > <swing> > <degrees>");
		command.seconds = readValue(*seconds, where + " > <swing> > <seconds>");
	}

	static Command makeSwing(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const Direction direction = rotationFromName(raw.direction, raw.verb);
		const float seconds = evaluate(raw.seconds);
		if (!(seconds > 0.0f)) { throw std::runtime_error("<swing> takes <seconds> of " + formatDisplayNumber(seconds) + "; expected more than 0"); }
		const float degrees = evaluate(raw.distance);
		if (!(degrees > 0.0f)) { throw std::runtime_error("<swing> takes <degrees> of " + formatDisplayNumber(degrees) + "; expected more than 0"); }
		return CmdSwing{ direction == Direction::Left ? -degrees : degrees, seconds };
	}

	static bool runSwing(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		const CmdSwing& swing = std::get<CmdSwing>(command);
		Object* const self = doer.self;
		if (!self || !doer.action) { return true; }
		self->swingFrames = std::max(1, executor.game.framesFor(swing.seconds));
		self->swingBy = swing.by;
		self->swingHeld = doer.pressed;
		return true;
	}

	static void printSwing(std::ostream& o, const Command& command)
	{
		const CmdSwing& s = std::get<CmdSwing>(command);
		o << "swing(" << s.by << " degrees, " << s.seconds << "s)";
	}

	static void printSwingRaw(std::ostream& o, const RawCommand& command)
	{
		o << "(" << command.direction << " " << command.distance << " degrees, " << command.seconds << "s)";
	}

	extern const Verb swingVerb;
	const Verb swingVerb{
		.name = "swing",
		.forms = { { "<swing>", places::controls, kindOf<CmdSwing>() } },
		.formOf = nullptr,
		.onRelease = true,
		.read = readSwing,
		.make = makeSwing,
		.check = nullptr,
		.run = runSwing,
		.print = printSwing,
		.printRaw = printSwingRaw,
	};
}
