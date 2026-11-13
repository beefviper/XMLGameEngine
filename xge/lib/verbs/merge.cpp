// merge.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/reading.h"

namespace xge
{
	// <merge>: Two alike pieces become one.
	static void readMerge(const XmlNode& node, const std::string& where, RawCommand&)
	{
		requireAttribute(node, "variable", where);
	}

	static Command makeMerge(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdMerge{ raw.variable };
	}

	static bool runMerge(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		const CmdMerge& merge = std::get<CmdMerge>(command);
		return doer.self && doer.touch && doer.other && executor.game.mergeCells(*doer.self, *doer.other, merge.variable);
	}

	static void printMerge(std::ostream& o, const Command& command)
	{
		const CmdMerge& m = std::get<CmdMerge>(command);
		o << "merge(" << m.variable << ")";
	}

	extern const Verb mergeVerb;
	const Verb mergeVerb{
		.name = "merge",
		.forms = { { "<merge>", places::touch, kindOf<CmdMerge>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readMerge,
		.make = makeMerge,
		.check = nullptr,
		.run = runMerge,
		.print = printMerge,
	};
}
