// enter.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "load/check.h"
#include "load/reading.h"

namespace xge
{
	// <enter>: Goes to another scene of the state.
	static void readEnter(const XmlNode&, const std::string& where, WrittenCommand& command)
	{
		if (command.scene.empty() == command.way.empty())
		{
			fail(where, "<enter> names a scene=\"...\" or a way=\"...\", one of them");
		}
	}

	static Command makeEnter(const WrittenCommand& written, const ValueEvaluator&)
	{
		return CmdEnterScene{ written.scene, written.way };
	}

	static void checkEnter(const CmdEnterScene& enter, const Checking& checking)
	{
		checkScene(checking, "enter", enter.scene, enter.way);
	}

	static bool runEnter(const CmdEnterScene& enterScene, const Doer& doer)
	{
		doer.game().enter(enterScene);
		return true;
	}

	static void printEnter(std::ostream& o, const CmdEnterScene& enterScene)
	{
		o << "enter(" << (enterScene.way.empty() ? enterScene.scene.str() : "way " + enterScene.way) << ")";
	}

	extern const Verb enterVerb;
	const Verb enterVerb{
		.name = "enter",
		.forms = { { "<enter>", places::changes, kindOf<CmdEnterScene>() } },
		.read = readEnter,
		.make = makeEnter,
		.check = checkAs<checkEnter>,
		.run = runAs<runEnter>,
		.print = printAs<printEnter>,
	};
}
