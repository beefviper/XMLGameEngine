// game_parts.h
// XML Game Engine
// author: beefviper
// date: Oct 11, 2026
//
// What a generated game's data (source/<game>.cpp, written by
// games/generators/windows-cpp/data.xsl) is written with: one small function for
// each kind of tag, so each tag of the game file is one line of C++ that
// makes the engine's own data (game_data.h) as game_xml.cpp reads it. Copied
// into every generated program unchanged.

#pragma once

#include "game_data.h"

#include <algorithm>
#include <initializer_list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace xge::parts
{
	// A number, or an expression worked out when the game loads.
	inline RawValue value(std::string text)
	{
		return RawValue::expression(std::move(text));
	}

	// <random min="..." max="..." />
	inline RawValue random(std::string min, std::string max)
	{
		RawValue drawn;
		drawn.kind = RawValue::Kind::Random;
		drawn.min = std::move(min);
		drawn.max = std::move(max);
		return drawn;
	}

	// One operation of an <equation> or a <formula>: <subtract>, its name, its operands.
	inline RawOperation operation(std::string op, std::string name, std::vector<RawOperand> operands)
	{
		return RawOperation{ std::move(op), std::move(name), std::move(operands) };
	}

	// An <equation>'s steps.
	inline RawValue equation(std::vector<RawOperation> steps)
	{
		RawValue worked;
		worked.kind = RawValue::Kind::Equation;
		worked.operations = std::make_shared<const std::vector<RawOperation>>(std::move(steps));
		return worked;
	}

	// A <formula>'s one operation, or an operation standing in for an operand of one.
	inline RawValue formula(RawOperation operation)
	{
		RawValue worked;
		worked.kind = RawValue::Kind::Formula;
		worked.operations = std::make_shared<const std::vector<RawOperation>>(1, std::move(operation));
		return worked;
	}

	// A part of the game, its fields filled in by `fill`.
	template <typename Part, typename Fill>
	Part make(Fill fill)
	{
		Part part{};
		fill(part);
		return part;
	}

	// A verb tag on its own: <bounce />.
	inline RawCommand verb(std::string name)
	{
		RawCommand command;
		command.verb = std::move(name);
		return command;
	}

	// A verb tag and what it says: <move direction="up">8</move>.
	template <typename Fill>
	RawCommand verb(std::string name, Fill fill)
	{
		RawCommand command = verb(std::move(name));
		fill(command);
		return command;
	}

	// <timer><every>seconds</every> commands </timer>
	inline RawTimer every(RawValue seconds, std::vector<RawCommand> commands)
	{
		return RawTimer{ true, std::move(seconds), std::move(commands), {} };
	}

	// <timer><after>seconds</after> commands </timer>
	inline RawTimer after(RawValue seconds, std::vector<RawCommand> commands)
	{
		return RawTimer{ false, std::move(seconds), std::move(commands), {} };
	}

	// <timer name="...">: the timer with its name, by which a scene from= this
	// one's puts another in its place.
	inline RawTimer named(std::string name, RawTimer timer)
	{
		timer.name = std::move(name);
		return timer;
	}

	// A <collision edge="...">: its commands added to each screen edge it
	// names, noting where they are when it holds only to a look (sprite=) or
	// is passed over while touching a class (unless=).
	inline void edge(RawCollisionData& collisions, const std::string& which, const std::vector<RawCommand>& commands,
		const std::string& look = {}, const std::string& unlessClass = {})
	{
		const auto add = [&](std::vector<RawCommand>& list, std::vector<EdgeGuard>& guards)
		{
			if ((!look.empty() || !unlessClass.empty()) && !commands.empty())
			{
				guards.push_back(EdgeGuard{ list.size(), commands.size(), look, unlessClass });
			}
			list.insert(list.end(), commands.begin(), commands.end());
		};

		const bool all = which == "all";
		if (all || which == "vertical" || which == "top") { add(collisions.top, collisions.topGuards); }
		if (all || which == "vertical" || which == "bottom") { add(collisions.bottom, collisions.bottomGuards); }
		if (all || which == "horizontal" || which == "left") { add(collisions.left, collisions.leftGuards); }
		if (all || which == "horizontal" || which == "right") { add(collisions.right, collisions.rightGuards); }
	}

	// <sprite name="..."> and its shape, what that says filled in by `fill`.
	template <typename Fill>
	RawSprite sprite(std::string kind, std::string name, Fill fill)
	{
		RawSprite drawn;
		drawn.kind = std::move(kind);
		drawn.name = std::move(name);
		fill(drawn);
		return drawn;
	}

	// What an object with no <animation> looks like: its one sprite, or the
	// looks <become> picks from, starting as the first.
	inline void looks(RawObject& object, const std::vector<RawSprite>& sprites)
	{
		if (sprites.size() > 1)
		{
			object.looks = sprites;
		}
		object.sprite = sprites.front();
	}

	// What an object with an <animation> looks like: the sprites its <frame>s
	// name, in order, starting as the first.
	inline void animate(RawObject& object, const std::vector<RawSprite>& sprites, RawValue interval, const std::vector<std::string>& frames)
	{
		object.hasAnimation = true;
		object.animation.interval = std::move(interval);
		for (const std::string& frame : frames)
		{
			object.animation.frames.push_back(*std::find_if(sprites.begin(), sprites.end(), [&](const RawSprite& each) { return each.name == frame; }));
		}
		object.sprite = object.animation.frames.front();
	}

	// A member of a <group>, or a cell of one, as it is made: the object, and
	// the sprites and <animation> it will look by.
	struct Member
	{
		RawObject object;
		std::vector<RawSprite> sprites;
		bool animated{ false };
		RawValue interval;
		std::vector<std::string> frames;
	};

	// What a <member>, <row>, <column> or <cell> changes of what its group gives.
	using Part = void (*)(Member&);

	// A member or cell: what its group gives, named, then changed by each
	// of its parts in turn, the most particular last.
	inline Member made(Member member, std::string name, std::initializer_list<Part> changes)
	{
		member.object.name = std::move(name);
		for (const Part change : changes)
		{
			change(member);
		}
		return member;
	}

	// A <sprite> of a member, row, column or cell: of the same shape as the
	// group's sprite of its name, it changes only what it gives (`fill`);
	// of another shape, or as lines, or when the group has none of that name,
	// it is a sprite of its own.
	template <typename Fill>
	void change(Member& member, const std::string& kind, const std::string& name, Fill fill)
	{
		const auto found = std::find_if(member.sprites.begin(), member.sprites.end(), [&](const RawSprite& each) { return each.name == name; });
		if (found == member.sprites.end())
		{
			member.sprites.push_back(sprite(kind, name, fill));
		}
		else if (found->kind == kind && kind != "line")
		{
			fill(*found);
		}
		else
		{
			*found = sprite(kind, name, fill);
		}
	}

	// The object a member is, now its looks are settled.
	inline RawObject finish(const Member& member)
	{
		RawObject object = member.object;
		if (member.animated)
		{
			animate(object, member.sprites, member.interval, member.frames);
		}
		else
		{
			looks(object, member.sprites);
		}
		return object;
	}

	// A cell of a group laid out in <columns> and <rows>: the cell, where it
	// is, and its slot, the sprite its row first shows (`rowsOnly`: what the
	// group gives changed by the rows that pick it alone).
	inline RawObject place(const Member& cell, const Member& rowsOnly, int column, int row)
	{
		RawObject object = finish(cell);
		object.cell.slot = finish(rowsOnly).sprite;
		object.cell.column = column;
		object.cell.row = row;
		if (column == 1)
		{
			object.cell.gapBefore = value("0");
		}
		if (row == 1)
		{
			object.cell.gapAbove = value("0");
		}
		return object;
	}

	// One member of a <group>, made and added, and one cell of a group in
	// <columns> and <rows>, made, placed and added. Each is a function of its
	// own so that a group of many members or cells does not hold all their
	// temporaries in one stack frame: a Debug build (MSVC, or GCC at -O0)
	// gives every statement's temporaries a slot of their own, and Breakout's
	// wall of bricks overflowed a 1 MB stack that way.
	inline void addMember(std::vector<RawObject>& all, const Member& group, std::string name, std::initializer_list<Part> changes)
	{
		all.push_back(finish(made(group, std::move(name), changes)));
	}

	inline void addCell(std::vector<RawObject>& all, const Member& group, std::string name, std::initializer_list<Part> changes,
		std::initializer_list<Part> rowChanges, int column, int row)
	{
		all.push_back(place(made(group, std::move(name), changes), made(group, "", rowChanges), column, row));
	}

	// The keys a state's <inputs> or a <keys> set gives, and what each runs.
	using Keys = std::map<std::string, std::vector<RawCommand>>;

	// A <keys> set a state uses (keys="..."), over what it already has.
	inline void use(Keys& input, const Keys& set)
	{
		for (const auto& [button, commands] : set)
		{
			input[button] = commands;
		}
	}

	// <note pitch="..." wave="..." to="...">length</note>
	inline RawNote note(std::string pitch, RawValue length, std::string wave = {}, std::string to = {})
	{
		RawNote played;
		played.pitch = std::move(pitch);
		played.length = std::move(length);
		played.wave = std::move(wave);
		played.to = std::move(to);
		return played;
	}

	// <rest>length</rest>
	inline RawNote rest(RawValue length)
	{
		RawNote quiet;
		quiet.rest = true;
		quiet.length = std::move(length);
		return quiet;
	}

	// A <path>'s <step>: where to, and the commands run on getting there.
	inline RawPathStep step(RawValue x, RawValue y, std::vector<RawCommand> commands = {})
	{
		return RawPathStep{ false, std::move(x), std::move(y), std::move(commands) };
	}

	// A <path>'s <home />.
	inline RawPathStep home()
	{
		RawPathStep back;
		back.home = true;
		return back;
	}

	// A piece a <scene> takes from its state's kit (or from the scene it is
	// from=), as it is.
	inline RawSceneEntry take(std::string name)
	{
		RawSceneEntry entry;
		entry.take = std::move(name);
		return entry;
	}

	// A piece a <scene> takes, and what it changes in it (`fill`).
	template <typename Fill>
	RawSceneEntry take(std::string name, Fill fill)
	{
		RawSceneEntry entry = take(std::move(name));
		fill(entry.change);
		return entry;
	}

	// An <object> of a <scene>'s own.
	inline RawSceneEntry own(RawObject object)
	{
		RawSceneEntry entry;
		entry.objects.push_back(std::move(object));
		return entry;
	}

	// A <group> of a <scene>'s own: its members, as its function adds them.
	inline RawSceneEntry own(void (*group)(std::vector<RawObject>&))
	{
		RawSceneEntry entry;
		group(entry.objects);
		return entry;
	}
}
