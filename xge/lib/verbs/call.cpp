// call.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"

#include <stdexcept>

namespace xge
{
	// <call>: Names what a wild card stands for.
	static Command makeCall(const WrittenCommand& raw, const ValueEvaluator&)
	{
		if (raw.object.empty()) { throw std::runtime_error("<call>: needs object=, the holder whose wild card a value is called for"); }
		return CmdCall{ raw.object };
	}

	static bool runCall(const CmdCall& call, const Doer& doer)
	{
		return doer.self && doer.game().callValue(*doer.self, call.holder);
	}

	static void printCall(std::ostream& o, const CmdCall& call)
	{
		o << "call(" << call.holder << ")";
	}

	extern const Verb callVerb;
	const Verb callVerb{
		.name = "call",
		.forms = { { "<call />", places::self, kindOf<CmdCall>() } },
		.make = makeCall,
		.run = runAs<runCall>,
		.print = printAs<printCall>,
	};
}
