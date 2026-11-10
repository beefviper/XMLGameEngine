// send.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

#include <cmath>
#include <optional>

namespace xge
{
	// <send>: A piece from one map to another, or what one holder holds to
	// another.
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

	static bool runSend(const Command& command, const Doer&, CommandExecutor& executor)
	{
		// Members of a held class, holder to holder; else a piece, map to map.
		const CmdSend& send = std::get<CmdSend>(command);
		if (send.held) { return executor.game.sendHeld(send.from, send.into, send.count, send.all, send.sprite); }
		const auto cell = [](const std::optional<float>& given) { return given ? std::optional<int>(static_cast<int>(std::lround(*given))) : std::nullopt; };
		return executor.game.sendCells(send.target, send.from, send.into, cell(send.column), cell(send.row));
	}

	extern const Verb sendVerb;
	const Verb sendVerb{
		.name = "send",
		.forms = { { "<send>", places::changes, kindOf<CmdSend>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeSend,
		.run = runSend,
	};
}
