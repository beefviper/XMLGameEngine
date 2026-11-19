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

	static bool runSlide(const CmdSlide& slide, const Doer& doer)
	{
		CommandExecutor& executor = *doer.executor;
		if (!slide.objClass.empty()) { return executor.game.slideClass(slide.objClass, slide.direction); }
		if (doer.self && doer.self->onCells) { executor.game.slideCells(*doer.self, slide.direction); }
		return true;
	}

	static void printSlide(std::ostream& o, const CmdSlide& slide)
	{
		o << "slide." << ((slide.direction == Direction::Up) ? "up" : (slide.direction == Direction::Down) ? "down" : (slide.direction == Direction::Left) ? "left" : "right");
		if (!slide.objClass.empty()) { o << "(" << slide.objClass << ")"; }
	}

	extern const Verb slideVerb;
	const Verb slideVerb{
		.name = "slide",
		.forms = { { "<slide>", places::self, kindOf<CmdSlide>() } },
		.read = readSlide,
		.make = makeSlide,
		.run = runAs<runSlide>,
		.print = printAs<printSlide>,
	};
}
