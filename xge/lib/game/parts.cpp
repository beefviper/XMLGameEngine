// parts.cpp
// XML Game Engine
// author: beefviper
// date: Nov 17, 2026

#include "game/parts.h"

namespace xge
{
	const Object::MapPlace& mapOf(const Parts& parts, const Object& object) noexcept
	{
		static const Object::MapPlace none{};
		const Object::MapPlace* found = parts.mapPlaces.find(object.id);
		return found ? *found : none;
	}

	const Moment& startOf(const Parts& parts, const Object& object) noexcept
	{
		static const Moment none{};
		const Moment* found = parts.starts.find(object.id);
		return found ? *found : none;
	}

	ObjectId Parts::newId()
	{
		if (!freeHandles.empty())
		{
			const std::uint32_t index = freeHandles.back();
			freeHandles.pop_back();
			handleSlots[index].order = ++joined;
			return { index, handleSlots[index].generation };
		}
		handleSlots.emplace_back();
		handleSlots.back().order = ++joined;
		return { static_cast<std::uint32_t>(handleSlots.size() - 1), 0 };
	}

	void Parts::freeId(ObjectId id)
	{
		if (!current(id)) { return; }
		removeParts(id);
		++handleSlots[id.index].generation;
		handleSlots[id.index].at = kNowhere;
		freeHandles.push_back(id.index);
	}

	void Parts::removeParts(ObjectId id)
	{
		holders.remove(id);
		drives.remove(id);
		hands.remove(id);
		mazes.remove(id);
		cells.remove(id);
		tethers.remove(id);
		pulls.remove(id);
		alofts.remove(id);
		rests.remove(id);
		carries.remove(id);
		pickers.remove(id);
		turns.remove(id);
		items.remove(id);
		sizeUses.remove(id);
		climbs.remove(id);
		moves.remove(id);
		swings.remove(id);
		lightPasts.remove(id);
		mapPlaces.remove(id);
		boundTexts.remove(id);
		follows.remove(id);
		unholed.remove(id);
		surfaces.remove(id);
		starts.remove(id);
		clocks.remove(id);
	}

	namespace
	{
		template <typename Part>
		void copyPart(Store<Part>& into, const Store<Part>& from, ObjectId id)
		{
			if (const Part* part = from.find(id)) { into.add(id, from.orderOf(id), *part); }
			else { into.remove(id); }
		}
	}

	void Parts::copyPartsFrom(const Parts& from, ObjectId id)
	{
		copyPart(holders, from.holders, id);
		copyPart(drives, from.drives, id);
		copyPart(hands, from.hands, id);
		copyPart(mazes, from.mazes, id);
		copyPart(cells, from.cells, id);
		copyPart(tethers, from.tethers, id);
		copyPart(pulls, from.pulls, id);
		copyPart(alofts, from.alofts, id);
		copyPart(rests, from.rests, id);
		copyPart(carries, from.carries, id);
		copyPart(pickers, from.pickers, id);
		copyPart(turns, from.turns, id);
		copyPart(items, from.items, id);
		copyPart(sizeUses, from.sizeUses, id);
		copyPart(climbs, from.climbs, id);
		copyPart(moves, from.moves, id);
		copyPart(swings, from.swings, id);
		copyPart(lightPasts, from.lightPasts, id);
		copyPart(mapPlaces, from.mapPlaces, id);
		copyPart(boundTexts, from.boundTexts, id);
		copyPart(follows, from.follows, id);
		copyPart(unholed, from.unholed, id);
		copyPart(surfaces, from.surfaces, id);
		copyPart(starts, from.starts, id);
		copyPart(clocks, from.clocks, id);
	}
}
