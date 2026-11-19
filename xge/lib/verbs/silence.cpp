// silence.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "load/check.h"
#include "load/reading.h"

namespace xge
{
	// <silence>: Stops a sound that loops.
	static void readSilence(const XmlNode& node, const std::string& where, RawCommand&)
	{
		requireAttribute(node, "sound", where);
	}

	static Command makeSilence(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdSilence{ raw.sound };
	}

	static void checkSilence(const CmdSilence& silence, const Checking& checking)
	{
		checkSound(checking, "silence", silence.sound);
	}

	static bool runSilence(const CmdSilence& silence, const Doer& doer)
	{
		doer.game().silenceSound(silence.sound);
		return true;
	}

	static void printSilence(std::ostream& o, const CmdSilence& silence)
	{
		o << "silence(" << silence.sound << ")";
	}

	extern const Verb silenceVerb;
	const Verb silenceVerb{
		.name = "silence",
		.forms = { { "<silence>", places::changes, kindOf<CmdSilence>() } },
		.read = readSilence,
		.make = makeSilence,
		.check = checkAs<checkSilence>,
		.run = runAs<runSilence>,
		.print = printAs<printSilence>,
	};
}
