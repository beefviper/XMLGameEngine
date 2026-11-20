// put.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "load/reading.h"

#include <memory>

namespace xge
{
	// <put>: A new piece on the cell it is over: of class=, or with none,
	// of the pieces the player on its turn owns (Game::turnClass).
	static void readPut(const XmlNode& node, const std::string& where, WrittenCommand&)
	{
		if (const std::unique_ptr<XmlNode> child = node.getFirstChild())
		{
			fail(where + " > <put>", "unknown <" + child->getName() + ">; a put holds nothing (what a piece put closes is in its <moves>)");
		}
	}

	static Command makePut(const WrittenCommand& written, const ValueEvaluator&)
	{
		return CmdPut{ written.objClass };
	}

	static bool runPut(const CmdPut& putCommand, const Doer& doer)
	{
		Game& game = doer.game();
		CmdPut put = putCommand;
		if (put.objClass.empty()) { put.objClass = game.players().turnClass(); }
		return doer.self && !put.objClass.empty() && game.cellMaps().putPiece(*doer.self, put);
	}

	static void printPut(std::ostream& o, const CmdPut& put)
	{
		o << "put(" << put.objClass << ")";
	}

	extern const Verb putVerb;
	const Verb putVerb{
		.name = "put",
		.forms = { { "<put>", places::self, kindOf<CmdPut>() } },
		.read = readPut,
		.make = makePut,
		.run = runAs<runPut>,
		.print = printAs<printPut>,
	};
}
