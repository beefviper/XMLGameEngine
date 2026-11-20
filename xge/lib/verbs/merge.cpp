// merge.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "load/reading.h"

namespace xge
{
	// <merge>: Two alike pieces become one.
	static void readMerge(const XmlNode& node, const std::string& where, WrittenCommand&)
	{
		requireAttribute(node, "variable", where);
	}

	static Command makeMerge(const WrittenCommand& raw, const ValueEvaluator&)
	{
		return CmdMerge{ raw.variable };
	}

	static bool runMerge(const CmdMerge& merge, const Doer& doer)
	{
		return doer.self && doer.touch && doer.other && doer.game().mergeCells(*doer.self, *doer.other, merge.variable);
	}

	static void printMerge(std::ostream& o, const CmdMerge& merge)
	{
		o << "merge(" << merge.variable << ")";
	}

	extern const Verb mergeVerb;
	const Verb mergeVerb{
		.name = "merge",
		.forms = { { "<merge>", places::touch, kindOf<CmdMerge>() } },
		.read = readMerge,
		.make = makeMerge,
		.run = runAs<runMerge>,
		.print = printAs<printMerge>,
	};
}
