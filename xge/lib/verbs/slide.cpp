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
	};
}
