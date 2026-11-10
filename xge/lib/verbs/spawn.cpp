// spawn.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/executor.h"

#include <cmath>
#include <optional>
#include <stdexcept>

namespace xge
{
	// <spawn>: Makes a copy of an object; refused when no more of it may
	// exist.
	static Command makeSpawn(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		CmdSpawn spawn;
		spawn.target = raw.object;
		// A <centered> half holds where the middle goes; left empty, the
		// middle of the window.
		const auto half = [&](const RawValue& value, std::optional<float>& into, bool* centered, const char* middle)
		{
			if (!isWritten(value)) { return; }
			if (value.kind == RawValue::Kind::Centered)
			{
				if (!centered)
				{
					throw std::runtime_error("<spawn object=\"" + raw.object + "\">: <centered> is for its <position>, not its <velocity> or <acceleration>");
				}
				*centered = true;
				into = evaluate(RawValue::expression(value.text.empty() ? middle : value.text));
				return;
			}
			into = evaluate(value);
		};
		half(raw.positionX, spawn.x, &spawn.centerX, "window.width.center");
		half(raw.positionY, spawn.y, &spawn.centerY, "window.height.center");
		half(raw.velocityX, spawn.velocityX, nullptr, "");
		half(raw.velocityY, spawn.velocityY, nullptr, "");
		half(raw.accelerationX, spawn.accelerationX, nullptr, "");
		half(raw.accelerationY, spawn.accelerationY, nullptr, "");
		half(raw.column, spawn.column, nullptr, "");
		half(raw.row, spawn.row, nullptr, "");
		spawn.random = raw.random;
		spawn.into = raw.into;
		spawn.free = raw.free;
		std::optional<float> below;
		half(raw.below, below, nullptr, "");
		spawn.freeBelow = below ? static_cast<int>(std::lround(*below)) : 0;
		return spawn;
	}

	static bool runSpawn(const Command& command, const Doer& doer, CommandExecutor& executor)
	{
		return executor.spawn(std::get<CmdSpawn>(command), doer.self);
	}

	extern const Verb spawnVerb;
	const Verb spawnVerb{
		.name = "spawn",
		.forms = { { "<spawn>", places::changes, kindOf<CmdSpawn>() } },
		.formOf = nullptr,
		.onRelease = false,
		.make = makeSpawn,
		.run = runSpawn,
	};
}
