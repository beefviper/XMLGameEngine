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
#include <variant>

namespace xge
{
	// <spawn>: Makes copies of a piece (one, or a <count> of them), put at
	// the spawner's middle unless a <position> says; refused when no more
	// of it may exist.
	static void readSpawn(const XmlNode& node, const std::string& where, RawCommand& command)
	{
		requireAttribute(node, "object", where);
		const std::string here = where + " > <spawn>";
		for (std::unique_ptr<XmlNode> child = node.getFirstChild(); child != nullptr; child = child->getNextSibling())
		{
			const std::string tag = child->getName();
			if (tag != "count" && tag != "position" && tag != "velocity" && tag != "acceleration" && tag != "random" && tag != "member" && tag != "column" && tag != "row" && tag != "free")
			{
				fail(here, "unknown <" + tag + ">; expected a <count>, <random /> or a <member>, a <position> (or a <column> and a <row>, or <free />), a <velocity> and an <acceleration>, each if wanted");
			}
		}
		RawSpawn& spawn = command.own.emplace<RawSpawn>();
		if (auto count = findChild(&node, "count")) { spawn.count = readValue(*count, here + " > <count>"); }
		spawn.random = findChild(&node, "random") != nullptr;
		if (auto member = findChild(&node, "member")) { spawn.member = readValue(*member, here + " > <member>"); }
		if (auto free = findChild(&node, "free"))
		{
			spawn.free = true;
			for (std::unique_ptr<XmlNode> child = free->getFirstChild(); child != nullptr; child = child->getNextSibling())
			{
				if (child->getName() != "below") { fail(here + " > <free>", "unknown <" + child->getName() + ">; expected only a <below>: the row it goes under"); }
				spawn.freeBelow = readValue(*child, here + " > <free> > <below>");
			}
		}
		if (auto column = findChild(&node, "column")) { spawn.column = readValue(*column, here); }
		if (auto row = findChild(&node, "row")) { spawn.row = readValue(*row, here); }
		if (auto position = findChild(&node, "position"))
		{
			const PartialVector2 given = readPartialVector2(*position, here);
			spawn.positionX = given.x;
			spawn.positionY = given.y;
		}
		if (auto velocity = findChild(&node, "velocity"))
		{
			const PartialVector2 given = readPartialVector2(*velocity, here);
			spawn.velocityX = given.x;
			spawn.velocityY = given.y;
		}
		if (auto acceleration = findChild(&node, "acceleration"))
		{
			const PartialVector2 given = readPartialVector2(*acceleration, here);
			spawn.accelerationX = given.x;
			spawn.accelerationY = given.y;
		}
	}

	static Command makeSpawn(const RawCommand& raw, const ValueEvaluator& evaluate)
	{
		const RawSpawn& written = std::get<RawSpawn>(raw.own);
		CmdSpawn spawn;
		spawn.target = raw.object;
		if (written.count)
		{
			spawn.count = static_cast<int>(std::lround(evaluate(*written.count)));
			if (spawn.count < 1) { throw std::runtime_error("<spawn object=\"" + raw.object + "\">: a <count> under 1"); }
		}
		// A <centered> half holds where the middle goes; left empty, the
		// middle of the window.
		const auto half = [&](const std::optional<RawValue>& given, std::optional<float>& into, bool* centered, const char* middle)
		{
			if (!given) { return; }
			const RawValue& value = *given;
			if (value.kind == RawValue::Kind::Centered)
			{
				if (value.edge != RawValue::Edge::Middle) { throw std::runtime_error("<spawn object=\"" + raw.object + "\">: <left>, <right>, <top> and <bottom> are for an object's <position>"); }
				if (!centered)
				{
					throw std::runtime_error("<spawn object=\"" + raw.object + "\">: <centered> is for its <position>, not its <velocity> or <acceleration>");
				}
				*centered = true;
				into = evaluate(centeredMiddle(value, middle));
				return;
			}
			into = evaluate(value);
		};
		half(written.positionX, spawn.x, &spawn.centerX, "window.width.center");
		half(written.positionY, spawn.y, &spawn.centerY, "window.height.center");
		half(written.velocityX, spawn.velocityX, nullptr, "");
		half(written.velocityY, spawn.velocityY, nullptr, "");
		half(written.accelerationX, spawn.accelerationX, nullptr, "");
		half(written.accelerationY, spawn.accelerationY, nullptr, "");
		half(written.column, spawn.column, nullptr, "");
		half(written.row, spawn.row, nullptr, "");
		half(written.member, spawn.member, nullptr, "");
		spawn.random = written.random;
		spawn.into = raw.into;
		spawn.free = written.free;
		std::optional<float> below;
		half(written.freeBelow, below, nullptr, "");
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

	static void printSpawn(std::ostream& o, const Command& command)
	{
		const CmdSpawn& s = std::get<CmdSpawn>(command);
		const auto half = [&](const std::optional<float>& value, bool centered)
		{
			if (!value) { o << "-"; return; }
			o << (centered ? "centered " : "") << *value;
		};
		o << "spawn(" << s.target << (s.count != 1 ? " x" + std::to_string(s.count) : "") << (s.member ? " member " + std::to_string(std::lround(*s.member)) : "") << ", ";
		half(s.x, s.centerX);
		o << " ";
		half(s.y, s.centerY);
		o << ", ";
		half(s.velocityX, false);
		o << " ";
		half(s.velocityY, false);
		if (s.accelerationX || s.accelerationY)
		{
			o << ", ";
			half(s.accelerationX, false);
			o << " ";
			half(s.accelerationY, false);
		}
		o << ")";
	}

	static void printSpawnRaw(std::ostream& o, const RawCommand& command)
	{
		const RawSpawn& spawn = std::get<RawSpawn>(command.own);
		const auto half = [&](const std::optional<RawValue>& value) { if (value) { o << *value; } else { o << "-"; } };
		o << "(" << command.object << ", ";
		half(spawn.count);
		o << ", ";
		half(spawn.positionX);
		o << " ";
		half(spawn.positionY);
		o << ", ";
		half(spawn.velocityX);
		o << " ";
		half(spawn.velocityY);
		o << ", ";
		half(spawn.accelerationX);
		o << " ";
		half(spawn.accelerationY);
		o << (spawn.random ? ", random" : "") << (spawn.free ? ", free " : " ");
		half(spawn.freeBelow);
		o << ", ";
		half(spawn.column);
		o << " ";
		half(spawn.row);
		o << (command.into.empty() ? "" : ", into " + command.into);
		if (spawn.member) { o << ", member " << *spawn.member; }
		o << ")";
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
		.print = printSpawn,
		.printRaw = printSpawnRaw,
	};
}
