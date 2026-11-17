// store.h
// XML Game Engine
// author: beefviper
// date: Nov 17, 2026

#pragma once

#include "game/object.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace xge
{
	// One kind of part, held only by the objects that have it, packed in one
	// list in object order. An object is found by its ObjectId; `order` is
	// where it stands among the game's objects (they join in order and keep
	// it), so `each` walks them as a frame pass does. A part that a reset
	// puts back keeps its start beside it.
	template <typename Part>
	class Store
	{
	public:
		bool has(ObjectId id) const noexcept { return slotOf(id) != kNone; }

		Part* find(ObjectId id) noexcept
		{
			const std::size_t at = slotOf(id);
			return at == kNone ? nullptr : &entries[at].part;
		}
		const Part* find(ObjectId id) const noexcept
		{
			const std::size_t at = slotOf(id);
			return at == kNone ? nullptr : &entries[at].part;
		}

		Part& get(ObjectId id)
		{
			if (Part* part = find(id)) { return *part; }
			throw std::out_of_range("an object has no such part (slot " + std::to_string(id.index) + ")");
		}

		// Gives the object the part (or replaces the one it has).
		Part& add(ObjectId id, std::uint64_t order, Part part)
		{
			if (Part* had = find(id))
			{
				*had = std::move(part);
				return *had;
			}
			const auto place = std::upper_bound(entries.begin(), entries.end(), order, [](std::uint64_t value, const Entry& entry) { return value < entry.order; });
			const std::size_t at = static_cast<std::size_t>(place - entries.begin());
			entries.insert(place, Entry{ id, order, std::move(part), std::nullopt });
			reindexFrom(at);
			return entries[at].part;
		}

		void remove(ObjectId id)
		{
			const std::size_t at = slotOf(id);
			if (at == kNone) { return; }
			positions[id.index] = kNone;
			entries.erase(entries.begin() + static_cast<std::ptrdiff_t>(at));
			reindexFrom(at);
		}

		// work(ObjectId, Part&), for each object that has the part, in object order.
		template <typename Work>
		void each(Work&& work)
		{
			for (Entry& entry : entries) { work(entry.owner, entry.part); }
		}

		// What a reset puts back: the part as it is now, kept (keepStart),
		// and given back (restart). An object whose start was never kept
		// is left as it is.
		void keepStart(ObjectId id)
		{
			const std::size_t at = slotOf(id);
			if (at != kNone) { entries[at].start = entries[at].part; }
		}
		void restart(ObjectId id)
		{
			const std::size_t at = slotOf(id);
			if (at != kNone && entries[at].start) { entries[at].part = *entries[at].start; }
		}

		std::size_t size() const noexcept { return entries.size(); }

	private:
		static constexpr std::size_t kNone = static_cast<std::size_t>(-1);

		struct Entry
		{
			ObjectId owner;
			std::uint64_t order{};
			Part part;
			std::optional<Part> start;
		};

		std::size_t slotOf(ObjectId id) const noexcept
		{
			if (!id || id.index >= positions.size()) { return kNone; }
			const std::size_t at = positions[id.index];
			return at != kNone && entries[at].owner == id ? at : kNone;
		}

		void reindexFrom(std::size_t from)
		{
			for (std::size_t i = from; i < entries.size(); ++i)
			{
				const std::uint32_t slot = entries[i].owner.index;
				if (slot >= positions.size()) { positions.resize(static_cast<std::size_t>(slot) + 1, kNone); }
				positions[slot] = i;
			}
		}

		std::vector<Entry> entries;
		std::vector<std::size_t> positions;   // by slot: where its entry is, or kNone
	};
}
