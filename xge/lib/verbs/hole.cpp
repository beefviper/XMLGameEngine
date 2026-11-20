// hole.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "verbs/verb.h"
#include "game/game.h"
#include "play/verbs.h"
#include "load/reading.h"

#include <stdexcept>
#include <string>

namespace xge
{
	// <hole>: Cuts a hole in its picture where it was touched.
	static void readHole(const XmlNode& node, const std::string& where, WrittenCommand& command)
	{
		// Its radius as its content, then how rough its edge is.
		const std::string here = where + " > <hole>";
		WrittenHole& hole = command.own.emplace<WrittenHole>();
		hole.radius = readLeadValue(node, { "rough" }, "its radius, then <rough> if wanted", where);
		if (auto roughNode = findChild(&node, "rough")) { hole.rough = readValue(*roughNode, here); }
	}

	static Command makeHole(const WrittenCommand& written, const ValueEvaluator& evaluate)
	{
		const WrittenHole& own = ownOf<WrittenHole>(written);
		const float radius = evaluate(own.radius);
		const float rough = valueOr(own.rough, 0.0f, evaluate);
		if (!(radius > 0.0f)) { throw std::runtime_error("<hole> has a radius of " + formatDisplayNumber(radius) + "; expected above 0"); }
		if (!(rough >= 0.0f && rough <= 1.0f)) { throw std::runtime_error("<hole> has a <rough> of " + formatDisplayNumber(rough) + "; expected 0 up to 1"); }
		return CmdHole{ radius, rough };
	}

	// <hole>: clears the pixels of the object's own picture within `radius`
	// of where `other` struck it - the solid pixel nearest the middle of
	// `other`, within a pixel or two of it, or that middle when there is
	// none - each at the edge kept at random by up to `rough` of the
	// radius. So shot after shot up one line goes deeper and deeper. The
	// first hole keeps the whole picture (Object::Unholed) for a reset and
	// makes the object its own copy (Resources::own); others sharing the
	// picture are never changed.
	static void cutHole(Parts& parts, Resources& resources, Object& object, const Object& other, float radius, float rough)
	{
		if (!object.drawn.bitmap) { return; }

		Bitmap cut = *object.drawn.bitmap;

		// Where it struck: the solid pixel nearest the other's middle, near
		// enough the other to be touched by it.
		const Vector2f otherSize = sizeOf(other);
		Vector2f middle = other.position + otherSize * 0.5f - object.position;
		{
			const Vector2f from = other.position - object.position;
			const int reach = 2;
			float best = -1.0f;
			Vector2f nearest = middle;
			const int x0 = std::max(0, static_cast<int>(std::floor(from.x)) - reach);
			const int y0 = std::max(0, static_cast<int>(std::floor(from.y)) - reach);
			const int x1 = std::min(cut.width - 1, static_cast<int>(std::ceil(from.x + otherSize.x)) + reach);
			const int y1 = std::min(cut.height - 1, static_cast<int>(std::ceil(from.y + otherSize.y)) + reach);
			for (int y = y0; y <= y1; ++y)
			{
				for (int x = x0; x <= x1; ++x)
				{
					if (!cut.solidAt(x, y)) { continue; }
					const Vector2f at{ static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f };
					const float away = (at.x - middle.x) * (at.x - middle.x) + (at.y - middle.y) * (at.y - middle.y);
					if (best < 0.0f || away < best) { best = away; nearest = at; }
				}
			}
			middle = nearest;
		}
		const int left = std::max(0, static_cast<int>(std::floor(middle.x - radius)));
		const int top = std::max(0, static_cast<int>(std::floor(middle.y - radius)));
		const int right = std::min(cut.width - 1, static_cast<int>(std::ceil(middle.x + radius)));
		const int bottom = std::min(cut.height - 1, static_cast<int>(std::ceil(middle.y + radius)));
		bool changed = false;
		for (int y = top; y <= bottom; ++y)
		{
			for (int x = left; x <= right; ++x)
			{
				const float dx = static_cast<float>(x) + 0.5f - middle.x;
				const float dy = static_cast<float>(y) + 0.5f - middle.y;
				const float reach = rough > 0.0f ? radius * (1.0f - rough * randomBetween(0.0f, 1.0f)) : radius;
				if (dx * dx + dy * dy > reach * reach || !cut.solidAt(x, y)) { continue; }
				cut.rgba[(static_cast<std::size_t>(y) * static_cast<std::size_t>(cut.width) + static_cast<std::size_t>(x)) * 4 + 3] = 0;
				changed = true;
			}
		}
		if (!changed) { return; }

		// The first hole makes the picture its own, kept whole for a reset;
		// each after that is cut into it.
		if (parts.unholed.has(object.id)) { resources.replace(object.drawn.bitmap, std::move(cut)); }
		else
		{
			parts.unholed.add(object.id, parts.orderOf(object.id), { object.drawn.bitmap });
			object.drawn.bitmap = resources.own(std::move(cut));
		}
		object.drawn.visualDirty = true;
	}

	static bool runHole(const CmdHole& hole, const Doer& doer)
	{
		Game& game = doer.game();
		if (doer.self && doer.touch) { cutHole(game.parts(), game.resources(), *doer.self, *doer.other, hole.radius, hole.rough); }
		return true;
	}

	static void printHole(std::ostream& o, const CmdHole& hole)
	{
		o << "hole(" << hole.radius << ", " << hole.rough << ")";
	}

	extern const Verb holeVerb;
	const Verb holeVerb{
		.name = "hole",
		.forms = { { "<hole>", places::touch, kindOf<CmdHole>() } },
		.read = readHole,
		.make = makeHole,
		.run = runAs<runHole>,
		.print = printAs<printHole>,
	};
}
