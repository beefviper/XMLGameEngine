// grow.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"

namespace xge
{
	// <grow>: A chain gets one link longer.
	static Command makeGrow(const RawCommand&, const ValueEvaluator&)
	{
		return CmdGrow{};
	}

	static bool runGrow(const Command&, const Doer& doer, CommandExecutor& executor)
	{
		if (doer.self && !executor.game.cellPart(*doer.self).chain.empty()) { ++executor.game.placeFor(*doer.self).chainGrow; }
		return true;
	}

	static void printGrow(std::ostream& o, const Command&)
	{
		o << "grow";
	}

	extern const Verb growVerb;
	const Verb growVerb{
		.name = "grow",
		.forms = { { "<grow />", places::self, kindOf<CmdGrow>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = nullptr,
		.make = makeGrow,
		.check = nullptr,
		.run = runGrow,
		.print = printGrow,
	};
}
