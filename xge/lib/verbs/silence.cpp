// silence.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
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

	static void checkSilence(const Command& command, const Checking& checking)
	{
		checkSound(checking, "silence", std::get<CmdSilence>(command).sound);
	}

	static bool runSilence(const Command& command, const Doer&, CommandExecutor& executor)
	{
		executor.game.silenceSound(std::get<CmdSilence>(command).sound);
		return true;
	}

	static void printSilence(std::ostream& o, const Command& command)
	{
		const CmdSilence& s = std::get<CmdSilence>(command);
		o << "silence(" << s.sound << ")";
	}

	static void printSilenceRaw(std::ostream& o, const RawCommand& command)
	{
		o << "(" << command.sound << ")";
	}

	extern const Verb silenceVerb;
	const Verb silenceVerb{
		.name = "silence",
		.forms = { { "<silence>", places::changes, kindOf<CmdSilence>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readSilence,
		.make = makeSilence,
		.check = checkSilence,
		.run = runSilence,
		.print = printSilence,
		.printRaw = printSilenceRaw,
	};
}
