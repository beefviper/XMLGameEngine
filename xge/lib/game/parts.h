// parts.h
// XML Game Engine
// author: beefviper
// date: Nov 17, 2026

#pragma once

#include "game/object.h"
#include "game/store.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace xge
{
	// A game's handles and its parts: every object's ObjectId, given as it
	// joins and freed as a copy leaves, and a Store of each part an object
	// has only if it uses it. Owned by the Game; what loads an object adds
	// its parts here.
	struct Parts
	{
		Store<Holder> holders;
		Store<Drive> drives;
		Store<Hand> hands;
		Store<Maze> mazes;
		Store<Object::Cells> cells;
		Store<Object::Tether> tethers;
		Store<Object::Pull> pulls;
		Store<Object::Aloft> alofts;
		Store<Object::Rest> rests;
		Store<Object::Carry> carries;
		Store<Object::Picker> pickers;
		Store<Object::Turns> turns;

		// A new handle, in the slot a copy that left freed if there is one,
		// a generation on.
		ObjectId newId();

		// The handle's slot is free again, and the object's parts are gone.
		void freeId(ObjectId id);

		// Whether the handle is the one its slot holds now.
		bool current(ObjectId id) const noexcept
		{
			return id && id.index < handleSlots.size() && handleSlots[id.index].generation == id.generation;
		}

		// Where an object stands among the game's objects: they join in this
		// order, a handle given as each joins, and keep it (Store::add).
		std::uint64_t orderOf(ObjectId id) const noexcept { return current(id) ? handleSlots[id.index].order : 0; }

		// Its part in `store`, added (in its place in object order) the
		// first time it is asked for: a part a verb gives while playing.
		template <typename Part>
		Part& give(Store<Part>& store, ObjectId id)
		{
			if (Part* part = store.find(id)) { return *part; }
			return store.add(id, orderOf(id), {});
		}

		// Where a handle's object is in the game's list (kNowhere: not in
		// it now, spawned this frame or in another player's turn).
		static constexpr std::size_t kNowhere = static_cast<std::size_t>(-1);
		std::size_t at(ObjectId id) const noexcept { return current(id) ? handleSlots[id.index].at : kNowhere; }
		void placeAt(ObjectId id, std::size_t at) noexcept
		{
			if (id.index < handleSlots.size()) { handleSlots[id.index].at = at; }
		}
		void placeNowhere() noexcept
		{
			for (Slot& slot : handleSlots) { slot.at = kNowhere; }
		}

		// The object's parts are gone, its handle kept (a player's copies
		// put away while another plays).
		void removeParts(ObjectId id);

		// The parts of `id` as `from` has them, in place of its own (a
		// player's game taken up again, Game::takeTurnUp).
		void copyPartsFrom(const Parts& from, ObjectId id);

	private:
		struct Slot
		{
			std::uint32_t generation{};
			std::size_t at{ kNowhere };
			std::uint64_t order{};
		};
		std::vector<Slot> handleSlots;
		std::vector<std::uint32_t> freeHandles;
		std::uint64_t joined{ 0 };
	};
}
