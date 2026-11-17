// parts.cpp
// XML Game Engine
// author: beefviper
// date: Nov 17, 2026

#include "game/parts.h"

namespace xge
{
	ObjectId Parts::newId()
	{
		if (!freeHandles.empty())
		{
			const std::uint32_t index = freeHandles.back();
			freeHandles.pop_back();
			return { index, handleSlots[index].generation };
		}
		handleSlots.emplace_back();
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
	}

	void Parts::copyPartsFrom(const Parts& from, ObjectId id)
	{
		if (const Holder* holder = from.holders.find(id)) { holders.add(id, from.holders.orderOf(id), *holder); }
		else { holders.remove(id); }
	}
}
