// slide.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/reading.h"

namespace xge
{
	// <slide>: As far as it can go one way over the cells.
	static void readSlide(const XmlNode& node, const std::string& where, RawCommand&)
	{
		requireAttribute(node, "direction", where);
	}

	static Command makeSlide(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdSlide{ directionFromName(raw.direction, raw.verb), raw.objClass };
	}

	static bool runSlide(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		const CmdSlide& slide = std::get<CmdSlide>(command);
		if (!slide.objClass.empty()) { return executor.game.slideClass(slide.objClass, slide.direction); }
		if (doer.self && doer.self->onCells) { executor.game.slideCells(*doer.self, slide.direction); }
		return true;
	}

	static void printSlide(std::ostream& o, const Command& command)
	{
		const CmdSlide& m = std::get<CmdSlide>(command);
		o << "slide." << ((m.direction == Direction::Up) ? "up" : (m.direction == Direction::Down) ? "down" : (m.direction == Direction::Left) ? "left" : "right");
		if (!m.objClass.empty()) { o << "(" << m.objClass << ")"; }
	}

	static void printSlideRaw(std::ostream& o, const RawCommand& command)
	{
		o << "." << command.direction << (command.objClass.empty() ? "" : "(" + command.objClass + ")");
	}

	extern const Verb slideVerb;
	const Verb slideVerb{
		.name = "slide",
		.forms = { { "<slide>", places::self, kindOf<CmdSlide>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readSlide,
		.make = makeSlide,
		.check = nullptr,
		.run = runSlide,
		.print = printSlide,
		.printRaw = printSlideRaw,
	};
}
