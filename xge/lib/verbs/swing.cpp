// swing.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "load/reading.h"

#include <algorithm>
#include <memory>
#include <stdexcept>

namespace xge
{
	// <swing>: One eased stroke round its pivot while held, and back when let
	// go.
	static void readSwing(const XmlNode& node, const std::string& where, WrittenCommand& command)
	{
		requireAttribute(node, "direction", where);
		std::unique_ptr<XmlNode> degrees = findChild(&node, "degrees");
		std::unique_ptr<XmlNode> seconds = findChild(&node, "seconds");
		if (!degrees || !seconds) { fail(where, "<swing> gives how far it turns as <degrees>, and how long the stroke takes as <seconds>"); }
		WrittenSwing& swing = command.own.emplace<WrittenSwing>();
		swing.degrees = readValue(*degrees, where + " > <swing> > <degrees>");
		swing.seconds = readValue(*seconds, where + " > <swing> > <seconds>");
	}

	static Command makeSwing(const WrittenCommand& raw, const ValueEvaluator& evaluate)
	{
		const Direction direction = rotationFromName(raw.direction, raw.verb);
		const WrittenSwing& written = ownOf<WrittenSwing>(raw);
		const float seconds = evaluate(written.seconds);
		if (!(seconds > 0.0f)) { throw std::runtime_error("<swing> takes <seconds> of " + formatDisplayNumber(seconds) + "; expected more than 0"); }
		const float degrees = evaluate(written.degrees);
		if (!(degrees > 0.0f)) { throw std::runtime_error("<swing> takes <degrees> of " + formatDisplayNumber(degrees) + "; expected more than 0"); }
		return CmdSwing{ direction == Direction::Left ? -degrees : degrees, seconds };
	}

	static bool runSwing(const CmdSwing& swing, const Doer& doer)
	{
		Game& game = doer.game();
		Object* const self = doer.self;
		if (!self || !doer.action) { return true; }
		Object::Swing& stroke = game.parts().give(game.parts().swings, self->id);
		stroke.seconds = swing.seconds;
		stroke.by = swing.by;
		stroke.held = doer.pressed;
		return true;
	}

	static bool keepSwinging(const CmdSwing& swing, const Doer& doer)
	{
		if (!doer.self) { return false; }
		Object::Swing& stroke = doer.game().parts().give(doer.game().parts().swings, doer.self->id);
		stroke.seconds = swing.seconds;
		stroke.by = swing.by;
		stroke.held = true;
		return true;
	}

	static void printSwing(std::ostream& o, const CmdSwing& swing)
	{
		o << "swing(" << swing.by << " degrees, " << swing.seconds << "s)";
	}

	extern const Verb swingVerb;
	const Verb swingVerb{
		.name = "swing",
		.forms = { { "<swing>", places::controls, kindOf<CmdSwing>() } },
		.onRelease = true,
		.read = readSwing,
		.make = makeSwing,
		.run = runAs<runSwing>,
		.hold = runAs<keepSwinging>,
		.print = printAs<printSwing>,
	};
}
