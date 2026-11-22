// people.cpp
// XML Game Engine
// author: beefviper
// date: Nov 22, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "load/reading.h"

namespace xge
{
	// <people>2</people>: How many seats people take, from the first; the
	// computer plays any other a state's <computer seat> is written for.
	static void readPeople(const XmlNode& node, const std::string& where, WrittenCommand& command)
	{
		const std::string text = readText(node);
		if (text != "1" && text != "2" && text != "3" && text != "4") { fail(where + " > <people>", "holds how many people play, 1 to 4"); }
		command.own.emplace<WrittenPeople>().count = text[0] - '0';
	}

	static Command makePeople(const WrittenCommand& written, const ValueEvaluator&)
	{
		return CmdPeople{ ownOf<WrittenPeople>(written).count };
	}

	static bool runPeople(const CmdPeople& people, const Doer& doer)
	{
		doer.game().seatPeople(people.count);
		return true;
	}

	static void printPeople(std::ostream& o, const CmdPeople& people)
	{
		o << "people(" << people.count << ")";
	}

	extern const Verb peopleVerb;
	const Verb peopleVerb{
		.name = "people",
		.forms = { { "<people>2</people>", places::flow, kindOf<CmdPeople>() } },
		.read = readPeople,
		.make = makePeople,
		.run = runAs<runPeople>,
		.print = printAs<printPeople>,
	};
}
