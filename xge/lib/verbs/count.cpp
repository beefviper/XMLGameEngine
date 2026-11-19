// count.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "load/reading.h"

namespace xge
{
	// <count>: How many of a class there are, or lie about it, into a
	// variable.
	static void readCount(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		requireAttribute(node, "class", where);
		requireAttribute(node, "into", where);
		if (node.getFirstChild()) { command.own.emplace<RawCount>().cells = readCellPlace(node, where + " > <count>"); }
	}

	static Command makeCount(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdCount{ raw.objClass, VariableName::of(raw.into), ownOf<RawCount>(raw).cells };
	}

	static bool runCount(const CmdCount& count, const Doer& doer)
	{
		Game& game = doer.game();
		if (!count.place.any()) { game.setNumber(count.target, game.countClass(count.objClass)); }
		else if (doer.self) { game.setNumber(count.target, static_cast<float>(game.piecesAbout(*doer.self, count.objClass, count.place).size())); }
		return true;
	}

	static void printCount(std::ostream& o, const CmdCount& count)
	{
		o << "count(" << count.objClass << ", " << count.target.written() << cellPlaceText(count.place) << ")";
	}

	extern const Verb countVerb;
	const Verb countVerb{
		.name = "count",
		.forms = { { "<count>", places::changes, kindOf<CmdCount>() } },
		.read = readCount,
		.make = makeCount,
		.run = runAs<runCount>,
		.print = printAs<printCount>,
	};
}
