// pick.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/reading.h"

#include <memory>

namespace xge
{
	// <pick>: Takes up the piece, or the items, under it.
	static void readPick(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
		{
			if (child->getName() == "most") { command.most = readCount(*child, where + " > <pick> > <most>"); continue; }
			if (child->getName() != "only") { fail(where + " > <pick>", "unknown <" + child->getName() + ">; expected <only /> and <most> if wanted"); }
			command.only = true;
		}
	}

	static Command makePick(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdPick{ raw.objClass, raw.only };
	}

	static bool runPick(const CmdPick& pick, const Doer& doer)
	{
		CommandExecutor& executor = *doer.executor;
		Object* const self = doer.self;
		return self && (executor.game.isHand(*self) ? executor.game.pickItems(*self) : executor.game.pickPiece(*self, pick.objClass.empty() ? executor.game.turnClass() : pick.objClass, pick.only));
	}

	static void printPick(std::ostream& o, const CmdPick& pick)
	{
		o << "pick(" << pick.objClass << (pick.only ? ", only" : "") << ")";
	}

	extern const Verb pickVerb;
	const Verb pickVerb{
		.name = "pick",
		.forms = { { "<pick>", places::self, kindOf<CmdPick>() } },
		.read = readPick,
		.make = makePick,
		.run = runAs<runPick>,
		.print = printAs<printPick>,
	};
}
