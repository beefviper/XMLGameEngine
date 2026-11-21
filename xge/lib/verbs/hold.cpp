// hold.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "engine/keys.h"
#include "load/reading.h"

#include <stdexcept>

namespace xge
{
	// <hold>: A <computer> holds a key down: run by Game, where its conditions
	// are tested.
	static void readHold(const XmlNode& node, const std::string& where, WrittenCommand& command)
	{
		command.own.emplace<WrittenHold>().button = requireAttribute(node, "button", where);
	}

	static Command makeHold(const WrittenCommand& written, const ValueEvaluator&)
	{
		const std::string& own = ownOf<WrittenHold>(written).button;
		const KeyCode button = keyCodeFromString(own);
		if (button == KeyCode::Unknown)
		{
			throw std::runtime_error("<hold button=\"" + own + "\">: there is no key named '" + own + "'");
		}
		return CmdHold{ button };
	}

	static void printHold(std::ostream& o, const CmdHold& hold)
	{
		o << "hold(" << keyCodeToString(hold.button) << ")";
	}

	extern const Verb holdVerb;
	const Verb holdVerb{
		.name = "hold",
		.forms = { { "<hold>", places::press, kindOf<CmdHold>() } },
		.read = readHold,
		.make = makeHold,
		.print = printAs<printHold>,
	};
}
