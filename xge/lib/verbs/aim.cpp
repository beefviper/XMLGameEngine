// aim.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/check.h"
#include "load/reading.h"

#include <stdexcept>

namespace xge
{
	// <aim>: Faces another object; with <lined>, refused unless it is lined
	// up.
	static void readAim(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		// Optionally only when lined up, within so many pixels.
		requireAttribute(node, "object", where);
		if (auto lined = findChild(&node, "lined")) { command.distance = readValue(*lined, where + " > <aim>"); }
	}

	static Command makeAim(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const bool lined = isWritten(raw.distance);
		const float within = lined ? evaluate(raw.distance) : 0.0f;
		if (within < 0.0f) { throw std::runtime_error("<aim object=\"" + raw.object + "\">: the <lined> is below 0"); }
		return CmdAim{ raw.object, lined, within };
	}

	static void checkAim(const Command& command, const Checking& checking)
	{
		checkObject(checking, std::get<CmdAim>(command).target, "<aim>");
	}

	static bool runAim(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		return !doer.self || executor.aim(*doer.self, std::get<CmdAim>(command));
	}

	static void printAim(std::ostream& o, const Command& command)
	{
		const CmdAim& a = std::get<CmdAim>(command);
		o << "aim(" << a.target << (a.lined ? ", lined " + std::to_string(a.within) : std::string()) << ")";
	}

	static void printAimRaw(std::ostream& o, const RawCommand& command)
	{
		o << "(" << command.object << ")";
	}

	extern const Verb aimVerb;
	const Verb aimVerb{
		.name = "aim",
		.forms = { { "<aim>", places::steering, kindOf<CmdAim>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readAim,
		.make = makeAim,
		.check = checkAim,
		.run = runAim,
		.print = printAim,
		.printRaw = printAimRaw,
	};
}
