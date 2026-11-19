// play.cpp
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
	// <play>: A sound; one that loops stops when its key is let go.
	static void readPlay(const XmlNode& node, const std::string& where, RawCommand&)
	{
		requireAttribute(node, "sound", where);
	}

	static Command makePlay(const RawCommand& raw, const ValueEvaluator&)
	{
		return CmdPlay{ raw.sound };
	}

	static void checkPlay(const CmdPlay& play, const Checking& checking)
	{
		checkSound(checking, "play", play.sound);
	}

	static bool runPlay(const CmdPlay& play, const Doer& doer)
	{
		CommandExecutor& executor = *doer.executor;
		executor.playHeld(play.sound, doer.pressed);
		return true;
	}

	static void printPlay(std::ostream& o, const CmdPlay& play)
	{
		o << "play(" << play.sound << ")";
	}

	extern const Verb playVerb;
	const Verb playVerb{
		.name = "play",
		.forms = { { "<play>", places::changes, kindOf<CmdPlay>() } },
		.onRelease = true,
		.read = readPlay,
		.make = makePlay,
		.check = checkAs<checkPlay>,
		.run = runAs<runPlay>,
		.print = printAs<printPlay>,
	};
}
