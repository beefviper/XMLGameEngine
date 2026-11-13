// send.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"
#include "load/check.h"
#include "load/reading.h"

#include <cmath>
#include <memory>
#include <optional>
#include <string>

namespace xge
{
	// <send>: A piece from one map to another, or what one holder holds to
	// another.
	static void readSend(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		// With no object=, members of a held class, holder to holder.
		if (node.getAttribute("object").empty())
		{
			readSendHeld(node, where, command);
			return;
		}
		command.from = requireAttribute(node, "from", where);
		command.into = requireAttribute(node, "into", where);
		const std::string here = where + " > <send>";
		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
		{
			if (child->getName() != "column" && child->getName() != "row") { fail(here, "unknown <" + child->getName() + ">; expected a <column> and a <row>, if wanted"); }
		}
		const auto column = findChild(&node, "column");
		const auto row = findChild(&node, "row");
		if (!column != !row) { fail(here, "has a <column> or a <row> and not the other; give both, or neither to keep its cell"); }
		if (column) { command.column = readValue(*column, here); }
		if (row) { command.row = readValue(*row, here); }
	}

	static Command makeSend(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		// With no object=, members of a held class go holder to holder, as a
		// <distribute> sends them.
		if (raw.object.empty()) { return makeSendHeld(raw, evaluate); }
		CmdSend send{ raw.object, raw.from, raw.into, std::nullopt, std::nullopt, false, 0, false, {} };
		if (isWritten(raw.column)) { send.column = evaluate(raw.column); }
		if (isWritten(raw.row)) { send.row = evaluate(raw.row); }
		return send;
	}

	static void checkSend(const Command& command, const Checking& checking)
	{
		// Holder to holder (from a group: its members): the holders are known
		// only once settled (settleHolders), so a name that is none is a send
		// refused.
		const CmdSend& send = std::get<CmdSend>(command);
		if (send.held) { return; }
		checkObject(checking, send.target, "<send>", "object or group");
		checkCellMap(checking, send.from, "<send from=\"" + send.from + "\">");
		checkCellMap(checking, send.into, "<send into=\"" + send.into + "\">");
	}

	static bool runSend(const Command& command, const Doer&, CommandExecutor& executor)
	{
		// Members of a held class, holder to holder; else a piece, map to map.
		const CmdSend& send = std::get<CmdSend>(command);
		if (send.held) { return executor.game.sendHeld(send.from, send.into, send.count, send.all, send.sprite); }
		const auto cell = [](const std::optional<float>& given) { return given ? std::optional<int>(static_cast<int>(std::lround(*given))) : std::nullopt; };
		return executor.game.sendCells(send.target, send.from, send.into, cell(send.column), cell(send.row));
	}

	static void printSend(std::ostream& o, const Command& command)
	{
		const CmdSend& s = std::get<CmdSend>(command);
		o << "send(" << s.target << ", " << s.from << " into " << s.into;
		if (s.column && s.row) { o << ", " << *s.column << " " << *s.row; }
		o << ")";
	}

	static void printSendRaw(std::ostream& o, const RawCommand& command)
	{
		o << "(" << command.object << ", " << command.from << " into " << command.into << ", " << command.column << " " << command.row << ")";
	}

	extern const Verb sendVerb;
	const Verb sendVerb{
		.name = "send",
		.forms = { { "<send>", places::changes, kindOf<CmdSend>() } },
		.formOf = nullptr,
		.onRelease = false,
		.read = readSend,
		.make = makeSend,
		.check = checkSend,
		.run = runSend,
		.print = printSend,
		.printRaw = printSendRaw,
	};
}
