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
	static void readHold(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		command.own.emplace<RawHold>().button = requireAttribute(node, "button", where);
	}

	static Command makeHold(const RawCommand& raw, const ValueEvaluator&)
	{
		const std::string& written = ownOf<RawHold>(raw).button;
		const KeyCode button = keyCodeFromString(written);
		if (button == KeyCode::Unknown)
		{
			throw std::runtime_error("<hold button=\"" + written + "\">: there is no key named '" + written + "'");
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
