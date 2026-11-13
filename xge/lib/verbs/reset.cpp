// reset.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/check.h"
#include "load/reading.h"
#include "load/spelling.h"

#include <stdexcept>

namespace xge
{
	// <reset>: Back to how it started: the object running it, a named one, a
	// scene, or the whole game.
	static void readReset(const XmlNode&, const std::string& where, RawCommand& command)
	{
		if ((!command.object.empty()) + (!command.scene.empty()) + (!command.way.empty()) > 1)
		{
			fail(where, "<reset> puts back an object=, a scene= or a way=, one of them");
		}
	}

	static Command makeReset(const RawCommand& raw, const ValueEvaluator&)
	{
		// <reset /> puts the object in the rule (or, in a state's input or
		// condition, the whole game) back; <reset object="name" /> that one object.
		if (!raw.scene.empty() || !raw.way.empty()) { return CmdResetScene{ raw.scene, raw.way }; }
		if (raw.object.empty()) { return CmdReset{}; }
		return CmdResetObject{ raw.object };
	}

	static void checkReset(const Command& command, const Checking& checking)
	{
		if (const auto* scene = std::get_if<CmdResetScene>(&command)) { checkScene(checking, "reset", scene->scene, scene->way); }
		else if (const auto* object = std::get_if<CmdResetObject>(&command))
		{
			checkObject(checking, object->target, "<reset object>");
		}
	}

	static bool runReset(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		if (const auto* object = std::get_if<CmdResetObject>(&command)) { executor.game.resetObject(object->target); }
		else if (const auto* scene = std::get_if<CmdResetScene>(&command)) { executor.game.reset(*scene); }
		else if (doer.self) { executor.restart(*doer.self); }
		else { executor.game.resetAll(); }
		return true;
	}

	static void printReset(std::ostream& o, const Command& command)
	{
		if (std::holds_alternative<CmdReset>(command))
		{
			o << "reset";
			return;
		}
		if (const auto* pr = std::get_if<CmdResetObject>(&command))
		{
			const CmdResetObject& r = *pr;
			o << "reset(" << r.target << ")";
			return;
		}
		if (const auto* pr = std::get_if<CmdResetScene>(&command))
		{
			const CmdResetScene& r = *pr;
			o << "reset(" << (r.way.empty() ? "scene " + r.scene : "way " + r.way) << ")";
			return;
		}
	}

	static void printResetRaw(std::ostream& o, const RawCommand& command)
	{
		if (!command.scene.empty()) { o << "(scene " << command.scene << ")"; }
		else if (!command.object.empty()) { o << "(" << command.object << ")"; }
	}

	extern const Verb resetVerb;
	const Verb resetVerb{
		.name = "reset",
		.forms = { { "<reset />", places::changes, kindOf<CmdReset>() }, { "<reset object>", places::changes, kindOf<CmdResetObject>() }, { "<reset scene>", places::changes, kindOf<CmdResetScene>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readReset,
		.make = makeReset,
		.check = checkReset,
		.run = runReset,
		.print = printReset,
		.printRaw = printResetRaw,
	};
}
