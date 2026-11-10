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
		command.button = requireAttribute(node, "button", where);
	}

	static Command makeHold(const RawCommand& raw, const ValueEvaluator&)
	{
		const KeyCode button = keyCodeFromString(raw.button);
		if (button == KeyCode::Unknown)
		{
			throw std::runtime_error("<hold button=\"" + raw.button + "\">: there is no key named '" + raw.button + "'");
		}
		return CmdHold{ button };
	}

	extern const Verb holdVerb;
	const Verb holdVerb{
		.name = "hold",
		.forms = { { "<hold>", places::press, kindOf<CmdHold>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readHold,
		.make = makeHold,
		.check = nullptr,
		.run = nullptr,
	};
}
