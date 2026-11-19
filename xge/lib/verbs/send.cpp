// send.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
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
		RawSend& send = command.own.emplace<RawSend>();
		if (column) { send.column = readValue(*column, here); }
		if (row) { send.row = readValue(*row, here); }
	}

	static Command makeSend(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		// With no object=, members of a held class go holder to holder, as a
		// <distribute> sends them.
		if (raw.object.empty()) { return makeSendHeld(raw, evaluate); }
		CmdSend send{ raw.object, raw.from, raw.into, std::nullopt, std::nullopt, false, 0, false, {} };
		const RawSend& written = ownOf<RawSend>(raw);
		if (written.column) { send.column = evaluate(*written.column); }
		if (written.row) { send.row = evaluate(*written.row); }
		return send;
	}

	static void checkSend(const CmdSend& send, const Checking& checking)
	{
		// Holder to holder (from a group: its members): what is held is known
		// only once the game is made (load/expr.cpp), so a name that is none
		// is a send refused.
		if (send.held) { return; }
		checkObject(checking, send.target, "<send>", "object or group");
		checkCellMap(checking, send.from, "<send from=\"" + send.from + "\">");
		checkCellMap(checking, send.into, "<send into=\"" + send.into + "\">");
	}

	static bool runSend(const CmdSend& send, const Doer& doer)
	{
		Game& game = doer.game();
		// Members of a held class, holder to holder; else a piece, map to map.
		if (send.held) { return game.sendHeld(send.from, send.into, send.count, send.all, send.sprite); }
		const auto cell = [](const std::optional<float>& given) { return given ? std::optional<int>(static_cast<int>(std::lround(*given))) : std::nullopt; };
		return game.sendCells(send.target, send.from, send.into, cell(send.column), cell(send.row));
	}

	static void printSend(std::ostream& o, const CmdSend& send)
	{
		o << "send(" << send.target << ", " << send.from << " into " << send.into;
		if (send.column && send.row) { o << ", " << *send.column << " " << *send.row; }
		o << ")";
	}

	extern const Verb sendVerb;
	const Verb sendVerb{
		.name = "send",
		.forms = { { "<send>", places::changes, kindOf<CmdSend>() } },
		.read = readSend,
		.make = makeSend,
		.check = checkAs<checkSend>,
		.run = runAs<runSend>,
		.print = printAs<printSend>,
	};
}
