// spawn.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/verbs.h"
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
	static void readSpawn(const XmlNode& node, const std::string& where, WrittenCommand& command)
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
		WrittenSpawn& spawn = command.own.emplace<WrittenSpawn>();
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

	static Command makeSpawn(const WrittenCommand& written, const ValueEvaluator& evaluate)
	{
		const WrittenSpawn& own = ownOf<WrittenSpawn>(written);
		CmdSpawn spawn;
		spawn.target = written.object;
		if (own.count)
		{
			spawn.count = static_cast<int>(std::lround(evaluate(*own.count)));
			if (spawn.count < 1) { throw std::runtime_error("<spawn object=\"" + written.object + "\">: a <count> under 1"); }
		}
		// A <centered> half holds where the middle goes; left empty, the
		// middle of the window.
		const auto half = [&](const std::optional<WrittenValue>& given, std::optional<float>& into, bool* centered, const char* middle)
		{
			if (!given) { return; }
			const WrittenValue& value = *given;
			if (value.kind == WrittenValue::Kind::Centered)
			{
				if (value.edge != WrittenValue::Edge::Middle) { throw std::runtime_error("<spawn object=\"" + written.object + "\">: <left>, <right>, <top> and <bottom> are for an object's <position>"); }
				if (!centered)
				{
					throw std::runtime_error("<spawn object=\"" + written.object + "\">: <centered> is for its <position>, not its <velocity> or <acceleration>");
				}
				*centered = true;
				into = evaluate(centeredMiddle(value, middle));
				return;
			}
			into = evaluate(value);
		};
		half(own.positionX, spawn.x, &spawn.centerX, "window.width.center");
		half(own.positionY, spawn.y, &spawn.centerY, "window.height.center");
		half(own.velocityX, spawn.velocityX, nullptr, "");
		half(own.velocityY, spawn.velocityY, nullptr, "");
		half(own.accelerationX, spawn.accelerationX, nullptr, "");
		half(own.accelerationY, spawn.accelerationY, nullptr, "");
		half(own.column, spawn.column, nullptr, "");
		half(own.row, spawn.row, nullptr, "");
		half(own.member, spawn.member, nullptr, "");
		spawn.random = own.random;
		spawn.into = written.into;
		spawn.free = own.free;
		std::optional<float> below;
		half(own.freeBelow, below, nullptr, "");
		spawn.freeBelow = below ? static_cast<int>(std::lround(*below)) : 0;
		return spawn;
	}

	static void checkSpawn(const CmdSpawn& spawn, const Checking& checking)
	{
		checkObject(checking, spawn.target, "<spawn>", "object or group");
		if (!spawn.into.empty()) { checkCellMap(checking, spawn.into, "<spawn into=\"" + spawn.into + "\">"); }
	}

	// One of them. A half of the position left out is the spawner's middle
	// (with no spawner, where the file says it starts); on a cell map, the
	// spawner's cell. Velocity and acceleration left out are its own.
	static bool spawnOne(Game& game, const CmdSpawn& spawn, const Object* self)
	{
		Object* made = game.spawn(spawn.target, spawn.random, spawn.member ? std::optional<int>(static_cast<int>(std::lround(*spawn.member))) : std::nullopt);
		if (!made) { return false; }

		// On a cell map: in a cell, a shape's top left one (counting from 1).
		if (made->onCells)
		{
			// Onto another map: its first cell unless told; on its own, its own.
			const bool elsewhere = !spawn.into.empty() && spawn.into != game.mapOf(*made).name;
			// Left out: the spawner's own cell, when it is on that map.
			const bool besideSelf = self && self->onCells && game.mapOf(*self).name == (spawn.into.empty() ? game.mapOf(*made).name : spawn.into);
			const auto cell = [&](const std::optional<float>& given, int own, float selfCell)
			{
				if (given) { return static_cast<int>(std::lround(*given)); }
				if (besideSelf) { return static_cast<int>(std::lround(selfCell)); }
				return elsewhere ? 1 : own + 1;
			};
			if (spawn.free)
			{
				if (!game.putInFreeCell(*made, spawn.into, spawn.freeBelow)) { return false; }
			}
			else if (!game.putOnCells(*made, cell(spawn.column, game.mapOf(*made).column, self ? game.place(*self).column : 0.0f), cell(spawn.row, game.mapOf(*made).row, self ? game.place(*self).row : 0.0f), spawn.into)) { return false; }
			if (spawn.velocityX) { made->velocity.x = *spawn.velocityX; }
			if (spawn.velocityY) { made->velocity.y = *spawn.velocityY; }
			if (spawn.accelerationX) { made->acceleration.x = *spawn.accelerationX; }
			if (spawn.accelerationY) { made->acceleration.y = *spawn.accelerationY; }
			return true;
		}

		const Vector2f size = sizeOf(*made);
		const Vector2f middle = self ? self->position + sizeOf(*self) * 0.5f - size * 0.5f : made->position;
		made->position.x = !spawn.x ? middle.x : spawn.centerX ? *spawn.x - size.x * 0.5f : *spawn.x;
		made->position.y = !spawn.y ? middle.y : spawn.centerY ? *spawn.y - size.y * 0.5f : *spawn.y;
		if (spawn.velocityX) { made->velocity.x = *spawn.velocityX; }
		if (spawn.velocityY) { made->velocity.y = *spawn.velocityY; }
		if (spawn.accelerationX) { made->acceleration.x = *spawn.accelerationX; }
		if (spawn.accelerationY) { made->acceleration.y = *spawn.accelerationY; }
		return true;
	}

	// `command.count` new objects, copies of `spawn.target` (Game::spawn),
	// each put and sent where the command says. False only when not one more
	// of it may exist; past its <most> the rest are simply not made.
	static bool spawnCopies(Game& game, const CmdSpawn& spawn, const Object* self)
	{
		bool made = false;
		for (int each = 0; each < spawn.count && spawnOne(game, spawn, self); ++each) { made = true; }
		return made;
	}

	static bool runSpawn(const CmdSpawn& spawn, const Doer& doer)
	{
		return spawnCopies(doer.game(), spawn, doer.self);
	}

	static void printSpawn(std::ostream& o, const CmdSpawn& spawn)
	{
		const auto half = [&](const std::optional<float>& value, bool centered)
		{
			if (!value) { o << "-"; return; }
			o << (centered ? "centered " : "") << *value;
		};
		o << "spawn(" << spawn.target << (spawn.count != 1 ? " x" + std::to_string(spawn.count) : "") << (spawn.member ? " member " + std::to_string(std::lround(*spawn.member)) : "") << ", ";
		half(spawn.x, spawn.centerX);
		o << " ";
		half(spawn.y, spawn.centerY);
		o << ", ";
		half(spawn.velocityX, false);
		o << " ";
		half(spawn.velocityY, false);
		if (spawn.accelerationX || spawn.accelerationY)
		{
			o << ", ";
			half(spawn.accelerationX, false);
			o << " ";
			half(spawn.accelerationY, false);
		}
		o << ")";
	}

	extern const Verb spawnVerb;
	const Verb spawnVerb{
		.name = "spawn",
		.forms = { { "<spawn>", places::changes, kindOf<CmdSpawn>() } },
		.names = { .into = Naming::Object },
		.alwaysLive = true,
		.read = readSpawn,
		.make = makeSpawn,
		.check = checkAs<checkSpawn>,
		.run = runAs<runSpawn>,
		.print = printAs<printSpawn>,
	};
}
