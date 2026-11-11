// spawn.cpp
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
#include <stdexcept>
#include <string>

namespace xge
{
	// <spawn>: Makes a copy of an object; refused when no more of it may
	// exist.
	static void readSpawn(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		requireAttribute(node, "object", where);
		const std::string here = where + " > <spawn>";
		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
		{
			const std::string tag = child->getName();
			if (tag != "position" && tag != "velocity" && tag != "acceleration" && tag != "random" && tag != "column" && tag != "row" && tag != "free")
			{
				fail(here, "unknown <" + tag + ">; expected <random />, a <position> (or a <column> and a <row>, or <free />), a <velocity> and an <acceleration>, each if wanted");
			}
		}
		command.random = findChild(&node, "random") != nullptr;
		if (auto free = findChild(&node, "free"))
		{
			command.free = true;
			for (std::unique_ptr<XmlNode> child = free->getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				if (child->getName() != "below") { fail(here + " > <free>", "unknown <" + child->getName() + ">; expected only a <below>: the row it goes under"); }
				command.below = readValue(*child, here + " > <free> > <below>");
			}
		}
		if (auto column = findChild(&node, "column")) { command.column = readValue(*column, here); }
		if (auto row = findChild(&node, "row")) { command.row = readValue(*row, here); }
		if (auto position = findChild(&node, "position"))
		{
			const PartialVector2 given = readPartialVector2(*position, here);
			if (given.x) { command.positionX = *given.x; }
			if (given.y) { command.positionY = *given.y; }
		}
		if (auto velocity = findChild(&node, "velocity"))
		{
			const PartialVector2 given = readPartialVector2(*velocity, here);
			if (given.x) { command.velocityX = *given.x; }
			if (given.y) { command.velocityY = *given.y; }
		}
		if (auto acceleration = findChild(&node, "acceleration"))
		{
			const PartialVector2 given = readPartialVector2(*acceleration, here);
			if (given.x) { command.accelerationX = *given.x; }
			if (given.y) { command.accelerationY = *given.y; }
		}
	}

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
				into = evaluate(RawValue::expression(!value.offset.empty() ? std::string(middle) + " + (" + value.offset + ")" : value.text.empty() ? middle : value.text));
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

	static void checkSpawn(const Command& command, const Checking& checking)
	{
		const CmdSpawn& spawn = std::get<CmdSpawn>(command);
		checkObject(checking, spawn.target, "<spawn>", "object or group");
		if (!spawn.into.empty()) { checkCellMap(checking, spawn.into, "<spawn into=\"" + spawn.into + "\">"); }
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
		.read = readSpawn,
		.make = makeSpawn,
		.check = checkSpawn,
		.run = runSpawn,
	};
}
