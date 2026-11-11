// put.cpp
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
	// <put>: A new piece on the cell it is over: of class=, or with none,
	// of the pieces the player on its turn owns (Game::turnClass).
	static void readPut(const XmlNode& node, const std::string& where, RawCommand&)
	{
		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
		{
			fail(where + " > <put>", "unknown <" + child->getName() + ">; a put holds nothing (what a piece put closes is in its <moves>)");
		}
	}

	static Command makePut(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdPut{ raw.objClass };
	}

	static bool runPut(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		CmdPut put = std::get<CmdPut>(command);
		if (put.objClass.empty()) { put.objClass = executor.game.turnClass(); }
		return doer.self && !put.objClass.empty() && executor.game.putPiece(*doer.self, put);
	}

	extern const Verb putVerb;
	const Verb putVerb{
		.name = "put",
		.forms = { { "<put>", places::self, kindOf<CmdPut>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readPut,
		.make = makePut,
		.check = nullptr,
		.run = runPut,
	};
}
