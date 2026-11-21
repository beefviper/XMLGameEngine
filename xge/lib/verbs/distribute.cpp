// distribute.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "load/reading.h"

namespace xge
{
	// <distribute>: What one holder holds, to others. It makes a <send>'s
	// command, which runs it.
	static void readDistribute(const XmlNode& node, const std::string& where, WrittenCommand& command)
	{
		readSendHeld(node, where, command, false);
	}

	static Command makeDistribute(const WrittenCommand& written, const ValueEvaluator& evaluate)
	{
		return makeSendHeld(written, evaluate);
	}

	extern const Verb distributeVerb;
	const Verb distributeVerb{
		.name = "distribute",
		.forms = { { "<distribute>", places::changes, kindOf<CmdSend>() } },
		.read = readDistribute,
		.make = makeDistribute,
	};
}
