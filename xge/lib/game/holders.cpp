// holders.cpp
// XML Game Engine
// author: beefviper
// date: Nov 1, 2026
//
// The game's holders and hands (Game): what each state's <starts> sends
// where, a picker pointing at holders and picking and placing what they
// hold by each holder's rules, and where every held member is drawn.

#include "game/data.h"
#include "game/game.h"
#include "game/random.h"
#include "play/verbs.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <set>
#include <map>
#include <optional>
#include <random>
#include <stdexcept>
#include <utility>

namespace xge
{
	namespace
	{
		// An item's variable of that name (0: it has none).
		float valueOf(const Object& item, const std::string& variable)
		{
			const auto value = item.variable.find(variable);
			return value == item.variable.end() ? 0.0f : value->second;
		}

		// What a trick follows on an item: its <trick variable>, as a whole
		// number; and the same as text, as <breaks> and <trumps> are written.
		int followed(const HolderRule& rule, const Object& item) { return static_cast<int>(std::lround(valueOf(item, rule.trickBy))); }
		std::string followedName(const HolderRule& rule, const Object& item) { return std::to_string(followed(rule, item)); }
	}

	// ===================================================================
	// The game's holders and hands
	// ===================================================================

	Object* Game::itemAt(int number)
	{
		if (number <= 0) { return nullptr; }
		const std::size_t at = static_cast<std::size_t>(number - 1);
		if (at < itemIndex.size() && itemIndex[at] < objects.size() && objects[itemIndex[at]].item.number == number) { return &objects[itemIndex[at]]; }
		// Objects moved (a copy taken away): found again.
		itemIndex.clear();
		for (std::size_t i = 0; i < objects.size(); ++i)
		{
			if (objects[i].item.number <= 0) { continue; }
			const std::size_t slot = static_cast<std::size_t>(objects[i].item.number - 1);
			if (itemIndex.size() <= slot) { itemIndex.resize(slot + 1, objects.size()); }
			itemIndex[slot] = i;
		}
		return at < itemIndex.size() && itemIndex[at] < objects.size() ? &objects[itemIndex[at]] : nullptr;
	}

	Object* Game::holderNamed(const std::string& name)
	{
		const std::string full = fullName(name);
		for (Object& object : objects)
		{
			if (isHolder(object) && object.name == full) { return &object; }
		}
		return nullptr;
	}

	void Game::showHeld(Object& item, const std::string& look)
	{
		const auto found = std::find_if(item.looks.all.begin(), item.looks.all.end(), [&](const Object::Look& each) { return each.name == look; });
		if (found != item.looks.all.end()) { turnItem(item, found == item.looks.all.begin()); }
	}

	void Game::turnItem(Object& item, bool up)
	{
		item.item.faceUp = up;
		const std::size_t look = up ? 0 : 1;
		if (item.looks.shown != look && look < item.looks.all.size()) { item.showLook(look); }
	}

	void Game::putInHolder(Object& holder, int number)
	{
		Object* item = itemAt(number);
		if (!item) { return; }
		if (!holderOf(holder).holdsSprite.empty()) { showHeld(*item, holderOf(holder).holdsSprite); }
		auto at = holderOf(holder).items.end();
		if (!holderOf(holder).orderBy.empty())
		{
			// <order>: by each variable named, the first first.
			const auto order = [&](const Object& of)
			{
				std::vector<float> values;
				for (const std::string& name : holderOf(holder).orderBy)
				{
					const auto found = of.variable.find(name);
					values.push_back(found == of.variable.end() ? 0.0f : found->second);
				}
				return values;
			};
			const std::vector<float> mine = order(*item);
			at = std::find_if(holderOf(holder).items.begin(), holderOf(holder).items.end(), [&](int other)
				{
					const Object* each = itemAt(other);
					return each && mine < order(*each);
				});
		}
		holderOf(holder).items.insert(at, number);
		// A suit that <breaks>, broken.
		const std::vector<std::string>& breaks = holderOf(holder).takes.breaks;
		if (std::find(breaks.begin(), breaks.end(), followedName(holderOf(holder).takes, *item)) != breaks.end()
			&& std::find(holderOf(holder).broken.begin(), holderOf(holder).broken.end(), followed(holderOf(holder).takes, *item)) == holderOf(holder).broken.end())
		{
			holderOf(holder).broken.push_back(followed(holderOf(holder).takes, *item));
		}
		// An item laid answers any call.
		holderOf(holder).called = 0.0f;
		refreshHolder(holder);
	}

	float Game::itemPoints(const Object& holder, const Object& item)
	{
		if (!holderOf(holder).counts.empty() && holderOf(holder).counts != "worth")
		{
			const auto value = item.variable.find(holderOf(holder).counts);
			return value == item.variable.end() ? 0.0f : value->second;
		}
		return item.item.worths.empty() ? 0.0f : item.item.worths.front();
	}

	// Whether these items, every one of them, make the pattern: each part
	// holds for all of them (<alike>: one value of its variable, <run>:
	// values in a row, by worth an item counting any of its worths), and
	// there are at least as many as its number.
	bool Game::meldFits(const std::vector<const Object*>& group, const RawPatterns::Pattern& pattern)
	{
		const std::size_t n = group.size();
		for (std::size_t p = 0; p < pattern.parts.size(); ++p)
		{
			const auto& [part, count] = pattern.parts[p];
			const std::string& by = pattern.by[p];
			if (n < static_cast<std::size_t>(count)) { return false; }
			if (part == RawPatterns::Part::Alike)
			{
				for (const Object* item : group)
				{
					if (valueOf(*item, by) != valueOf(*group.front(), by)) { return false; }
				}
				continue;
			}
			// A run: some start from which each value in turn is a different item's.
			bool found = false;
			for (const Object* starter : group)
			{
				// By worth, an item counting any of its worths; else the variable's value.
				const auto valuesOf = [&](const Object& item) { return by == "worth" ? item.item.worths : std::vector<float>{ valueOf(item, by) }; };
				for (const float start : valuesOf(*starter))
				{
					std::vector<bool> used(n, false);
					const auto take = [&](const auto& self, std::size_t step) -> bool
					{
						if (step == n) { return true; }
						const float wanted = start + static_cast<float>(step);
						for (std::size_t i = 0; i < n; ++i)
						{
							const std::vector<float> worths = valuesOf(*group[i]);
							if (used[i] || std::find(worths.begin(), worths.end(), wanted) == worths.end()) { continue; }
							used[i] = true;
							if (self(self, step + 1)) { return true; }
							used[i] = false;
						}
						return false;
					};
					if (take(take, 0)) { found = true; break; }
				}
				if (found) { break; }
			}
			if (!found) { return false; }
		}
		return true;
	}

	// The fewest points left in no group, these items (or the best `best`
	// of them) laid out in groups each making one of the holder's <melds>
	// patterns; `melds`, when asked, gets the groups (indexes into
	// `numbers`). Each item counts its points by the holder's <counts>.
	float Game::meldDeadwood(const Object& holder, const std::vector<int>& numbers, int best, std::vector<std::vector<int>>* melds)
	{
		std::vector<const Object*> items;
		for (const int number : numbers)
		{
			if (const Object* item = itemAt(number)) { items.push_back(item); }
		}
		const std::size_t n = items.size();
		std::vector<float> value(n);
		for (std::size_t i = 0; i < n; ++i) { value[i] = itemPoints(holder, *items[i]); }
		const auto set = std::find_if(patterns.begin(), patterns.end(), [&](const RawPatterns& each) { return each.name == holderOf(holder).melds; });
		if (set == patterns.end() || n > 16 || n != numbers.size())
		{
			float total = 0.0f;
			for (const float each : value) { total += each; }
			return total;
		}

		// Every group of the items that makes a pattern, by its lowest item.
		const std::uint32_t all = (std::uint32_t{ 1 } << n) - 1;
		std::vector<std::vector<std::uint32_t>> groups(n);
		std::vector<const Object*> group;
		for (std::uint32_t mask = 1; mask <= all; ++mask)
		{
			if (std::popcount(mask) < 2) { continue; }
			group.clear();
			for (std::size_t i = 0; i < n; ++i)
			{
				if (mask & (std::uint32_t{ 1 } << i)) { group.push_back(items[i]); }
			}
			if (std::any_of(set->patterns.begin(), set->patterns.end(), [&](const RawPatterns::Pattern& pattern) { return meldFits(group, pattern); }))
			{
				groups[static_cast<std::size_t>(std::countr_zero(mask))].push_back(mask);
			}
		}
		// The least left over by each set of the items: its lowest item left
		// over, or in one of its groups.
		std::vector<float> left(std::size_t{ all } + 1, 0.0f);
		std::vector<std::uint32_t> chosen(std::size_t{ all } + 1, 0);
		for (std::uint32_t mask = 1; mask <= all; ++mask)
		{
			const int low = std::countr_zero(mask);
			const std::uint32_t lowBit = std::uint32_t{ 1 } << low;
			left[mask] = value[static_cast<std::size_t>(low)] + left[mask ^ lowBit];
			for (const std::uint32_t each : groups[static_cast<std::size_t>(low)])
			{
				if ((each & mask) == each && left[mask ^ each] < left[mask])
				{
					left[mask] = left[mask ^ each];
					chosen[mask] = each;
				}
			}
		}
		std::uint32_t target = all;
		if (best > 0 && static_cast<std::size_t>(best) < n)
		{
			float least = -1.0f;
			for (std::uint32_t mask = 1; mask <= all; ++mask)
			{
				if (std::popcount(mask) == best && (least < 0.0f || left[mask] < least))
				{
					least = left[mask];
					target = mask;
				}
			}
		}
		if (melds)
		{
			melds->clear();
			for (std::uint32_t mask = target; mask != 0;)
			{
				const std::uint32_t each = chosen[mask];
				if (each == 0)
				{
					mask &= mask - 1;
					continue;
				}
				std::vector<int> indexes;
				for (std::size_t i = 0; i < n; ++i)
				{
					if (each & (std::uint32_t{ 1 } << i)) { indexes.push_back(static_cast<int>(i)); }
				}
				melds->push_back(std::move(indexes));
				mask ^= each;
			}
		}
		return left[target];
	}

	// A holder with <melds>: its groups first, each lowest item first, the
	// rest after them in suit and rank order; `deadwood` from its best.
	void Game::layOutMelds(Object& holder)
	{
		std::vector<std::vector<int>> melds;
		const std::vector<int> items = holderOf(holder).items;
		holderOf(holder).deadwood = meldDeadwood(holder, items, holderOf(holder).meldsBest, nullptr);
		meldDeadwood(holder, items, 0, &melds);
		std::vector<int> order;
		std::vector<bool> placed(items.size(), false);
		const auto byWorth = [&](int a, int b)
		{
			const Object& x = *itemAt(items[static_cast<std::size_t>(a)]);
			const Object& y = *itemAt(items[static_cast<std::size_t>(b)]);
			return x.item.number < y.item.number;
		};
		for (auto& meld : melds)
		{
			std::sort(meld.begin(), meld.end(), byWorth);
			for (const int i : meld)
			{
				order.push_back(items[static_cast<std::size_t>(i)]);
				placed[static_cast<std::size_t>(i)] = true;
			}
		}
		std::vector<int> rest;
		for (std::size_t i = 0; i < items.size(); ++i)
		{
			if (!placed[i]) { rest.push_back(static_cast<int>(i)); }
		}
		std::sort(rest.begin(), rest.end(), byWorth);
		for (const int i : rest) { order.push_back(items[static_cast<std::size_t>(i)]); }
		holderOf(holder).items = order;
		refreshBoundTexts(holder.name, "deadwood", holderOf(holder).deadwood);
	}

	bool Game::isTrump(const HolderRule& rule, const Object& item)
	{
		return std::find(rule.trumps.begin(), rule.trumps.end(), followedName(rule, item)) != rule.trumps.end();
	}

	// Whether `item` laid now takes the trick from `best`: a trump over
	// any other item, else a higher item of the same suit.
	bool Game::beatsInTrick(const HolderRule& rule, const Object& item, const Object& best)
	{
		const bool trump = isTrump(rule, item);
		if (trump != isTrump(rule, best)) { return trump; }
		return followed(rule, item) == followed(rule, best) && item.item.worths.back() > best.item.worths.back();
	}

	const Object* Game::trickWinning(const Object& holder)
	{
		if (holderOf(holder).items.empty()) { return nullptr; }
		const Object* best = itemAt(holderOf(holder).items.front());
		for (const int number : holderOf(holder).items)
		{
			const Object* item = itemAt(number);
			if (item && best && beatsInTrick(holderOf(holder).takes, *item, *best)) { best = item; }
		}
		return best;
	}

	// How many tricks a hand's items should take, as a player reckons it
	// to bid: each side suit's ace, a king with another item of its suit,
	// half a queen with two more and a higher item; each trump honour with
	// as many trumps under it as there are above it, and each trump past
	// three; with trumps to spare, a missing suit one and a single item a
	// half. Rounded to the nearest.
	float Game::reckonTricks(const Object& own, const HolderRule& trick)
	{
		float top = 0.0f;
		std::vector<int> kinds;
		for (const Object& object : objects)
		{
			if (!object.item.is || object.item.worths.empty()) { continue; }
			top = std::max(top, object.item.worths.back());
			if (std::find(kinds.begin(), kinds.end(), followed(trick, object)) == kinds.end()) { kinds.push_back(followed(trick, object)); }
		}
		std::map<int, std::vector<float>> held;
		std::map<int, bool> trumped;
		int trumps = 0;
		for (const int number : holderOf(own).items)
		{
			const Object* item = itemAt(number);
			if (!item || item->item.worths.empty()) { continue; }
			held[followed(trick, *item)].push_back(item->item.worths.back());
			if (isTrump(trick, *item))
			{
				trumped[followed(trick, *item)] = true;
				++trumps;
			}
		}
		float total = 0.0f;
		int honours = 0;
		for (auto& [kind, worths] : held)
		{
			std::sort(worths.rbegin(), worths.rend());
			const std::size_t length = worths.size();
			const auto has = [&](float worth) { return std::find(worths.begin(), worths.end(), worth) != worths.end(); };
			if (trumped[kind])
			{
				for (std::size_t k = 0; k < 3; ++k)
				{
					if (has(top - static_cast<float>(k)) && length > k) { ++honours; }
				}
				total += static_cast<float>(length > 3 ? length - 3 : 0);
				continue;
			}
			if (has(top)) { total += 1.0f; }
			if (has(top - 1.0f) && length >= 2) { total += length <= 5 ? 1.0f : 0.5f; }
			if (has(top - 2.0f) && length >= 3 && (has(top) || has(top - 1.0f))) { total += 0.5f; }
		}
		total += static_cast<float>(honours);
		if (!trick.trumps.empty())
		{
			float ruffs = 0.0f;
			for (const int kind : kinds)
			{
				if (trumped[kind]) { continue; }
				const auto it = held.find(kind);
				const std::size_t length = it == held.end() ? 0 : it->second.size();
				if (length == 0) { ruffs += 1.0f; }
				else if (length == 1) { ruffs += 0.5f; }
			}
			const int spare = trumps - honours;
			total += std::min(ruffs, static_cast<float>(std::max(spare, 0)));
		}
		return std::floor(total + 0.5f);
	}

	const Object* Game::trickHolder()
	{
		const auto found = std::find_if(objects.begin(), objects.end(), [this](const Object& object) { return isHolder(object) && holderOf(object).takes.follow; });
		return found == objects.end() ? nullptr : &*found;
	}

	bool Game::isWild(const HolderRule& rule, const Object& item)
	{
		return !rule.wildRules.empty() && acceptsBy(rule.wildRules, item, nullptr);
	}

	void Game::refreshHolder(Object& holder)
	{
		holderOf(holder).itemCount = static_cast<float>(holderOf(holder).items.size());
		refreshCall(holder);

		// top.<variable>: the top one's, 0 while it holds nothing.
		const Object* top = holderOf(holder).items.empty() ? nullptr : itemAt(holderOf(holder).items.back());
		for (auto& [name, value] : holder.variable)
		{
			if (name.rfind("top.", 0) != 0) { continue; }
			float now = 0.0f;
			if (top)
			{
				if (const auto own = top->variable.find(name.substr(4)); own != top->variable.end()) { now = own->second; }
			}
			if (now != value) { value = now; refreshBoundTexts(holder.name, name, now); }
		}

		// Points: each face-up item at its lowest worth, then each that may
		// count higher raised as far as the total stays within the limit.
		float total = 0.0f;
		std::vector<const Object*> shown;
		// <counts variable>: a variable of each other than its worth, as it is.
		const std::string& counted = holderOf(holder).counts;
		const bool byVariable = !counted.empty() && counted != "worth";
		for (const int number : holderOf(holder).items)
		{
			const Object* item = itemAt(number);
			if (!item || !item->item.faceUp || item->item.worths.empty()) { continue; }
			if (byVariable)
			{
				const auto value = item->variable.find(counted);
				total += value == item->variable.end() ? 0.0f : value->second;
				continue;
			}
			total += item->item.worths.front();
			shown.push_back(item);
		}
		for (const Object* item : shown)
		{
			for (auto higher = item->item.worths.rbegin(); higher != item->item.worths.rend(); ++higher)
			{
				const float more = *higher - item->item.worths.front();
				if (more > 0.0f && (holderOf(holder).pointsMost <= 0.0f || total + more <= holderOf(holder).pointsMost))
				{
					total += more;
					break;
				}
			}
		}
		holderOf(holder).points = total;
		if (!counted.empty()) { setVariable(holder.name, counted, total); }
		refreshBoundTexts(holder.name, "cards", holderOf(holder).itemCount);
		refreshBoundTexts(holder.name, "held", holderOf(holder).itemCount);
		refreshBoundTexts(holder.name, "points", holderOf(holder).points);

		// A holder with <melds>: laid out in its groups, and its deadwood.
		if (!holderOf(holder).melds.empty() && std::all_of(holderOf(holder).items.begin(), holderOf(holder).items.end(), [&](int number) { return itemAt(number) != nullptr; })) { layOutMelds(holder); }

		// A hand's own holder in a game with a trick: the tricks it should take.
		if (const Object* trick = trickHolder(); trick && trick != &holder)
		{
			const bool owned = std::any_of(objects.begin(), objects.end(), [&](const Object& hand) { return isHand(hand) && !handOf(hand).own.empty() && holderNamed(handOf(hand).own) == &holder; });
			if (owned)
			{
				holderOf(holder).tricks = reckonTricks(holder, holderOf(*trick).takes);
				refreshBoundTexts(holder.name, "tricks", holderOf(holder).tricks);
			}
		}

		// A trick: the suit led, and who laid the item winning it.
		if (holderOf(holder).takes.follow)
		{
			const Object* lead = holderOf(holder).items.empty() ? nullptr : itemAt(holderOf(holder).items.front());
			const Object* best = trickWinning(holder);
			holderOf(holder).lead = lead ? static_cast<float>(followed(holderOf(holder).takes, *lead)) : 0.0f;
			holderOf(holder).winner = best ? static_cast<float>(best->item.laidBy) : 0.0f;
			refreshBoundTexts(holder.name, "lead", holderOf(holder).lead);
			refreshBoundTexts(holder.name, "winner", holderOf(holder).winner);
		}

		// Scored holders, this one and those that score its items with theirs,
		// are scored again before the conditions are asked (scoreDueHolders):
		// once, however many items came and went this frame.
		if (!holderOf(holder).score.empty()) { holderOf(holder).scoreDue = scoresDue = true; }
		for (Object& other : objects)
		{
			if (&other != &holder && isHolder(other) && !holderOf(other).score.empty() && !holderOf(other).scoreWith.empty() && holderNamed(holderOf(other).scoreWith) == &holder)
			{
				holderOf(other).scoreDue = scoresDue = true;
			}
		}
	}

	void Game::scoreDueHolders()
	{
		if (!scoresDue) { return; }
		scoresDue = false;
		for (Object& object : objects)
		{
			if (isHolder(object) && holderOf(object).scoreDue)
			{
				holderOf(object).scoreDue = false;
				scoreHolder(object);
			}
		}
	}

	namespace
	{
		// What a set's <alike> parts compare: each variable once, at most
		// four (Game checks), and each part's place among them (-1: a run).
		struct Slots
		{
			std::vector<std::string> by;
			std::vector<std::vector<int>> parts;   // a list for each pattern, a place for each part
		};

		Slots slotsOf(const RawPatterns& set)
		{
			Slots slots;
			for (const RawPatterns::Pattern& pattern : set.patterns)
			{
				std::vector<int>& places = slots.parts.emplace_back();
				for (std::size_t i = 0; i < pattern.parts.size(); ++i)
				{
					if (pattern.parts[i].first != RawPatterns::Part::Alike) { places.push_back(-1); continue; }
					const std::string& by = pattern.by[i];
					auto at = std::find(slots.by.begin(), slots.by.end(), by);
					if (at == slots.by.end()) { at = slots.by.insert(slots.by.end(), by); }
					places.push_back(static_cast<int>(at - slots.by.begin()));
				}
			}
			return slots;
		}

		// An item as patterns see it: the value of each variable its set's
		// <alike>s compare (0 to 63), and the worths it may count for,
		// lowest first.
		struct Scored
		{
			std::array<int, 4> values;
			const std::vector<float>* worths;
			// A bit for each worth when all are whole numbers under 64
			// (the usual), else 0: runs are then found by bits.
			std::uint64_t mask;
		};

		float highest(const Scored& item) { return item.worths->back(); }

		// Room reused by bestPattern, call after call.
		std::vector<float>& scratchWorths()
		{
			thread_local std::vector<float> worths;
			return worths;
		}

		// The highest pattern the items make (from 1; 0 none), with what
		// ranks it among others making the same, deciding first: a run's
		// worths from its top down, else the items in groups of the set's
		// first <alike> variable, bigger groups first, each group by worth;
		// then the rest by worth. With `only` (from 1), that pattern alone
		// is tried: -1 when the items do not make it.
		int bestPattern(const RawPatterns& set, const Slots& slots, const std::vector<Scored>& items, std::vector<float>& deciding, int only = -1)
		{
			// Counted in place (values are under 64; scoreHolder sees to
			// that), as this runs thousands of times for the odds.
			std::array<std::array<int, 64>, 4> of{};
			std::array<float, 64> groupWorth{};
			std::array<int, 64> groups{};
			std::size_t groupCount = 0;
			std::uint64_t mask = 0;
			bool byBits = true;
			for (const Scored& item : items)
			{
				for (std::size_t s = 0; s < slots.by.size(); ++s) { ++of[s][static_cast<std::size_t>(item.values[s])]; }
				if (!slots.by.empty())
				{
					const auto group = static_cast<std::size_t>(item.values[0]);
					if (of[0][group] == 1) { groups[groupCount++] = item.values[0]; }
					groupWorth[group] = std::max(groupWorth[group], highest(item));
				}
				mask |= item.mask;
				byBits = byBits && item.mask != 0;
			}
			std::vector<float>& worths = scratchWorths();
			if (!byBits)
			{
				worths.clear();
				for (const Scored& item : items) { worths.insert(worths.end(), item.worths->begin(), item.worths->end()); }
				std::sort(worths.begin(), worths.end());
				worths.erase(std::unique(worths.begin(), worths.end()), worths.end());
			}
			// Groups by how many, then by worth, highest first.
			const auto byGroup = [&](int a, int b)
			{
				const auto ia = static_cast<std::size_t>(a);
				const auto ib = static_cast<std::size_t>(b);
				return of[0][ia] != of[0][ib] ? of[0][ia] > of[0][ib] : groupWorth[ia] > groupWorth[ib];
			};
			std::sort(groups.begin(), groups.begin() + static_cast<std::ptrdiff_t>(groupCount), byGroup);
			const auto inGroups = [&]()
			{
				deciding.clear();
				if (slots.by.empty())
				{
					for (const Scored& item : items) { deciding.push_back(highest(item)); }
					std::sort(deciding.rbegin(), deciding.rend());
					return;
				}
				for (std::size_t i = 0; i < groupCount; ++i)
				{
					const auto group = static_cast<std::size_t>(groups[i]);
					deciding.insert(deciding.end(), static_cast<std::size_t>(of[0][group]), groupWorth[group]);
				}
			};
			// The top of the highest run of that many worths, or nothing.
			const auto runTop = [&](int many) -> std::optional<float>
			{
				if (byBits)
				{
					if (many > 64) { return std::nullopt; }
					const std::uint64_t run = many == 64 ? ~std::uint64_t{ 0 } : (std::uint64_t{ 1 } << many) - 1;
					for (int top = 63; top >= many - 1; --top)
					{
						if (((mask >> (top - many + 1)) & run) == run) { return static_cast<float>(top); }
					}
					return std::nullopt;
				}
				for (auto top = worths.rbegin(); top != worths.rend(); ++top)
				{
					int length = 1;
					while (length < many && std::binary_search(worths.begin(), worths.end(), *top - static_cast<float>(length))) { ++length; }
					if (length == many) { return *top; }
				}
				return std::nullopt;
			};

			const std::size_t highestTried = only > 0 ? static_cast<std::size_t>(only) : only == 0 ? 0 : set.patterns.size();
			const std::size_t lowestTried = only > 0 ? highestTried - 1 : 0;
			for (std::size_t index = highestTried; index-- > lowestTried;)
			{
				const RawPatterns::Pattern& pattern = set.patterns[index];
				const std::vector<int>& places = slots.parts[index];
				int longestRun = 0;
				bool holds = true;
				// Each <alike> a value of its own, the biggest first, each
				// taking the smallest group of that value that is enough.
				std::array<std::uint64_t, 4> used{};
				for (int wanted = static_cast<int>(items.size()); wanted > 0 && holds; --wanted)
				{
					for (std::size_t p = 0; p < pattern.parts.size(); ++p)
					{
						const int many = pattern.parts[p].second;
						if (places[p] < 0 || many != wanted) { continue; }
						const auto s = static_cast<std::size_t>(places[p]);
						int fit = -1;
						for (int value = 0; value < 64; ++value)
						{
							const int count = of[s][static_cast<std::size_t>(value)];
							if (count >= many && !(used[s] & (std::uint64_t{ 1 } << value)) && (fit < 0 || count < of[s][static_cast<std::size_t>(fit)])) { fit = value; }
						}
						if (fit < 0) { holds = false; break; }
						used[s] |= std::uint64_t{ 1 } << fit;
					}
				}
				for (const auto& [part, many] : pattern.parts)
				{
					if (part == RawPatterns::Part::Alike && many > static_cast<int>(items.size())) { holds = false; }
					else if (part == RawPatterns::Part::Run) { longestRun = std::max(longestRun, many); }
				}
				if (!holds) { continue; }
				const std::optional<float> top = longestRun > 0 ? runTop(longestRun) : std::nullopt;
				if (longestRun > 0 && !top) { continue; }

				if (top)
				{
					deciding.clear();
					std::vector<Scored> rest = items;
					for (int step = 0; step < longestRun; ++step)
					{
						const float worth = *top - static_cast<float>(step);
						deciding.push_back(worth);
						rest.erase(std::find_if(rest.begin(), rest.end(), [&](const Scored& item)
						{
							return std::find(item.worths->begin(), item.worths->end(), worth) != item.worths->end();
						}));
					}
					std::vector<float> others;
					for (const Scored& item : rest) { others.push_back(highest(item)); }
					std::sort(others.rbegin(), others.rend());
					deciding.insert(deciding.end(), others.begin(), others.end());
				}
				else { inGroups(); }
				return static_cast<int>(index) + 1;
			}
			if (only > 0) { return -1; }
			inGroups();
			return 0;
		}

		// The best `best` of the items (0: all of them): the pattern they
		// make, and a number ordering any two the same size by the pattern,
		// then by what decides between two of it, in base `base` (one more
		// than any worth).
		float bestOf(const RawPatterns& set, const Slots& slots, const std::vector<Scored>& items, std::size_t best, float base, int& made)
		{
			const std::size_t size = best == 0 ? items.size() : std::min(best, items.size());
			// Room reused call after call: the odds call this thousands of times.
			thread_local std::vector<float> deciding;
			thread_local std::vector<Scored> some;
			thread_local std::vector<char> chosen;
			// What all the items make is the most any `size` of them can
			// (every part asks for at least so many), so that is tried
			// first, and only when none of them makes it, every pattern.
			made = bestPattern(set, slots, items, deciding);
			if (size == items.size())
			{
				float strength = static_cast<float>(made);
				for (const float worth : deciding) { strength = strength * base + worth; }
				return strength;
			}
			float strongest = -1.0f;
			for (const bool onlyTheMost : { true, false })
			{
				chosen.assign(items.size(), 0);
				std::fill(chosen.begin(), chosen.begin() + static_cast<std::ptrdiff_t>(size), char{ 1 });
				int bestMade = 0;
				do
				{
					some.clear();
					for (std::size_t i = 0; i < items.size(); ++i)
					{
						if (chosen[i]) { some.push_back(items[i]); }
					}
					const int pattern = bestPattern(set, slots, some, deciding, onlyTheMost ? made : -1);
					if (pattern < 0) { continue; }
					float strength = static_cast<float>(pattern);
					for (const float worth : deciding) { strength = strength * base + worth; }
					if (strength > strongest)
					{
						strongest = strength;
						bestMade = pattern;
					}
				} while (std::prev_permutation(chosen.begin(), chosen.end()));
				if (strongest >= 0.0f)
				{
					made = bestMade;
					break;
				}
			}
			return std::max(strongest, 0.0f);
		}
	}

	void Game::markOddsRead()
	{
		// Generous: a name that might mean the holder counts as reading it.
		const auto means = [](const Object& holder, const std::string& name)
		{
			if (name.empty()) { return false; }
			if (holder.named(name)) { return true; }
			const std::string tail = "." + name;
			return holder.name.size() > tail.size() && holder.name.compare(holder.name.size() - tail.size(), tail.size(), tail) == 0;
		};
		const auto readIn = [&](const Object& holder, const std::vector<Condition>& conditions, const auto& self) -> bool
		{
			for (const Condition& condition : conditions)
			{
				if (condition.variableName == "odds" && (means(holder, condition.filterObject) || (!condition.filterClass.empty() && condition.filterClass == holder.objClass))) { return true; }
				for (const Condition::Read& read : condition.reads)
				{
					if (read.variable == "odds" && means(holder, read.owner)) { return true; }
				}
				if (self(holder, condition.parts, self)) { return true; }
			}
			return false;
		};
		for (Object& holder : objects)
		{
			if (!isHolder(holder) || holderOf(holder).score.empty()) { continue; }
			// Every expression worked out while loading (a condition's
			// value, a command's), by the names it reads.
			bool read = false;
			for (const auto& [text, program] : expr.compiled)
			{
				if (text.find("odds") == std::string::npos) { continue; }
				for (const std::string& name : expr.namesIn(text))
				{
					const std::size_t dot = name.rfind('.');
					read = read || (dot != std::string::npos && name.compare(dot + 1, std::string::npos, "odds") == 0 && means(holder, name.substr(0, dot)));
				}
			}
			for (const Object& other : objects) { read = read || other.showsVariable(holder.name, "odds"); }
			for (const State& state : states)
			{
				read = read || readIn(holder, state.conditions, readIn) || readIn(holder, state.computer, readIn);
				for (const Scene& scene : state.scenes) { read = read || readIn(holder, scene.conditions, readIn); }
			}
			holderOf(holder).oddsRead = read;
		}
	}

	void Game::scoreHolder(Object& holder)
	{
		const auto set = std::find_if(patterns.begin(), patterns.end(), [&](const RawPatterns& each) { return each.name == holderOf(holder).score; });
		if (set == patterns.end()) { return; }
		const Slots slots = slotsOf(*set);
		const auto scored = [&](const Object& item)
		{
			std::uint64_t bits = 0;
			for (const float worth : item.item.worths)
			{
				if (worth < 0.0f || worth > 63.0f || worth != std::floor(worth)) { bits = 0; break; }
				bits |= std::uint64_t{ 1 } << static_cast<int>(worth);
			}
			Scored one{ {}, &item.item.worths, bits };
			for (std::size_t s = 0; s < slots.by.size(); ++s) { one.values[s] = static_cast<int>(std::lround(valueOf(item, slots.by[s]))); }
			return one;
		};
		float base = 1.0f;
		for (const Object& object : objects)
		{
			if (!object.item.is) { continue; }
			// The values compared are counted in room for 0 to 63 (bestPattern).
			const Scored one = scored(object);
			if (std::any_of(one.values.begin(), one.values.begin() + static_cast<std::ptrdiff_t>(slots.by.size()), [](int value) { return value < 0 || value >= 64; })) { return; }
			base = std::max(base, highest(one) + 1.0f);
		}

		std::vector<Scored> own;
		std::vector<int> seen;
		const auto add = [&](const Object& from)
		{
			for (const int number : holderOf(from).items)
			{
				if (const Object* item = itemAt(number))
				{
					own.push_back(scored(*item));
					seen.push_back(number);
				}
			}
		};
		add(holder);
		const std::size_t mine = own.size();
		Object* shared = holderOf(holder).scoreWith.empty() ? nullptr : holderNamed(holderOf(holder).scoreWith);
		if (shared) { add(*shared); }
		const std::size_t best = static_cast<std::size_t>(holderOf(holder).scoreBest);

		int made = 0;
		holderOf(holder).strength = own.empty() ? 0.0f : bestOf(*set, slots, own, best, base, made);
		holderOf(holder).pattern = static_cast<float>(made);

		// The odds: as many unseen items as this holder holds, against it,
		// the shared holder made up to full from the unseen too; the same
		// each time for the same items (its own generator, seeded by them).
		holderOf(holder).odds = 0.0f;
		if (mine > 0 && holderOf(holder).oddsRead)
		{
			std::vector<Scored> unseen;
			for (const Object& object : objects)
			{
				if (object.item.is && std::find(seen.begin(), seen.end(), object.item.number) == seen.end()) { unseen.push_back(scored(object)); }
			}
			const std::size_t board = own.size() - mine;
			const std::size_t full = static_cast<std::size_t>(holderOf(holder).scoreFull);
			const std::size_t more = full > board ? full - board : 0;
			if (unseen.size() >= more + mine)
			{
				std::uint64_t seed = 1469598103934665603ull;
				for (const int number : seen) { seed = (seed ^ static_cast<std::uint64_t>(static_cast<unsigned>(number))) * std::uint64_t{ 1099511628211u }; }
				std::mt19937 random(static_cast<std::mt19937::result_type>(seed));
				constexpr int trials = 500;
				float won = 0.0f;
				std::vector<Scored> ours = own;
				std::vector<Scored> theirs(own.begin() + static_cast<std::ptrdiff_t>(mine), own.end());
				for (int trial = 0; trial < trials; ++trial)
				{
					// The first few of the unseen, shuffled that far.
					for (std::size_t i = 0; i < more + mine; ++i)
					{
						std::uniform_int_distribution<std::size_t> pick(i, unseen.size() - 1);
						std::swap(unseen[i], unseen[pick(random)]);
					}
					ours.resize(own.size());
					theirs.resize(own.size() - mine);
					for (std::size_t i = 0; i < more; ++i)
					{
						ours.push_back(unseen[i]);
						theirs.push_back(unseen[i]);
					}
					for (std::size_t i = 0; i < mine; ++i) { theirs.push_back(unseen[more + i]); }
					int ignored = 0;
					const float us = bestOf(*set, slots, ours, best, base, ignored);
					const float them = bestOf(*set, slots, theirs, best, base, ignored);
					won += us > them ? 1.0f : us == them ? 0.5f : 0.0f;
				}
				holderOf(holder).odds = std::round(won * 100.0f / static_cast<float>(trials));
			}
		}
		refreshBoundTexts(holder.name, "pattern", holderOf(holder).pattern);
		refreshBoundTexts(holder.name, "strength", holderOf(holder).strength);
		refreshBoundTexts(holder.name, "odds", holderOf(holder).odds);
	}

	void Game::refreshCall(Object& holder)
	{
		const Object* top = holderOf(holder).items.empty() ? nullptr : itemAt(holderOf(holder).items.back());
		holderOf(holder).calling = !holderOf(holder).takes.wildRules.empty() && top && isWild(holderOf(holder).takes, *top) && holderOf(holder).called == 0.0f ? 1.0f : 0.0f;
		refreshBoundTexts(holder.name, "calling", holderOf(holder).calling);
		refreshBoundTexts(holder.name, "called", holderOf(holder).called);
	}

	int Game::takeTop(Object& holder)
	{
		if (holderOf(holder).items.empty()) { return 0; }
		const int number = holderOf(holder).items.back();
		holderOf(holder).items.pop_back();
		refreshHolder(holder);
		return number;
	}

	void Game::showTop(Object& holder)
	{
		if (holderOf(holder).topSprite.empty() || holderOf(holder).items.empty()) { return; }
		if (Object* top = itemAt(holderOf(holder).items.back()); top && top->lookName() != holderOf(holder).topSprite) { showHeld(*top, holderOf(holder).topSprite); refreshHolder(holder); }
	}

	void Game::sendStarts()
	{
		// Numbered, every held member in turn.
		int number = 0;
		for (Object& object : objects)
		{
			if (object.item.is) { object.item.number = ++number; }
		}
		itemIndex.clear();
		if (number == 0) { return; }

		for (Object& object : objects)
		{
			if (isHolder(object)) { holderOf(object).items.clear(); holderOf(object).broken.clear(); holderOf(object).called = 0.0f; refreshHolder(object); }
			if (object.item.is) { object.item.laidBy = 0; }
			if (isHand(object))
			{
				handOf(object).held.clear();
				handOf(object).from.clear();
				handOf(object).holder.clear();
				handOf(object).at = -1;
			}
		}

		// Each group's held members, in random order, into the holder it starts in.
		std::map<std::string, std::vector<int>> starts;
		std::vector<std::string> order;
		for (Object& object : objects)
		{
			if (!object.item.is) { continue; }
			if (!starts.count(object.item.startHolder)) { order.push_back(object.item.startHolder); }
			starts[object.item.startHolder].push_back(object.item.number);
		}
		for (const std::string& name : order)
		{
			Object* holder = holderNamed(name);
			if (!holder) { throw std::runtime_error("<starts> > <send into=\"" + name + "\">: there is no holder called '" + name + "'"); }
			std::vector<int>& items = starts[name];
			// In the pack's order, the first on top, unless an <order> shuffles it.
			if (!holderOf(*holder).startShuffled) { std::reverse(items.begin(), items.end()); }
			for (std::size_t i = holderOf(*holder).startShuffled ? items.size() : 0; i > 1; --i)
			{
				const std::size_t j = std::min(i - 1, static_cast<std::size_t>(randomBetween(0.0f, static_cast<float>(i))));
				std::swap(items[i - 1], items[j]);
			}
			// Stacked (tests): those items last, so on top, the first last of all.
			for (auto each = stackedStart.rbegin(); each != stackedStart.rend(); ++each)
			{
				const auto at = std::find(items.begin(), items.end(), *each);
				if (at == items.end()) { continue; }
				items.erase(at);
				items.push_back(*each);
			}
			for (const int item : items)
			{
				if (Object* each = itemAt(item)) { turnItem(*each, false); }
				putInHolder(*holder, item);
			}

			// Dealt in rounds, an item to each holder that wants one more.
			std::vector<std::pair<Object*, int>> wanting;
			std::map<std::string, int> inGroup;
			for (Object& object : objects)
			{
				if (!isHolder(object) || &object == holder) { continue; }
				int count = holderOf(object).startCount;
				if (!object.groupName.empty())
				{
					count += holderOf(object).startMore * inGroup[object.groupName];
					++inGroup[object.groupName];
				}
				if (count > 0) { wanting.emplace_back(&object, count); }
			}
			for (int round = 1; !wanting.empty() && !holderOf(*holder).items.empty(); ++round)
			{
				bool any = false;
				for (auto& [each, count] : wanting)
				{
					if (count < round || holderOf(*holder).items.empty()) { continue; }
					putInHolder(*each, takeTop(*holder));
					any = true;
				}
				if (!any) { break; }
			}
			for (auto& [each, count] : wanting) { showTop(*each); }
			showTop(*holder);
		}

		stackedStart.clear();

		// A wild card turned up to start: its own suit is the one called.
		for (Object& object : objects)
		{
			if (isHolder(object) && holderOf(object).calling > 0.0f)
			{
				const Object* top = itemAt(holderOf(object).items.back());
				holderOf(object).called = valueOf(*top, holderOf(object).takes.wildVariable);
				refreshCall(object);
			}
		}

		// Each hand at its own holder, or else the first holder.
		for (Object& object : objects)
		{
			if (!isHand(object)) { continue; }
			Object* own = handOf(object).own.empty() ? nullptr : holderNamed(handOf(object).own);
			for (Object& holder : objects)
			{
				if (!isHolder(holder) || (own && &holder != own)) { continue; }
				handOf(object).holder = holder.name;
				handOf(object).at = static_cast<int>(holderOf(holder).items.size()) - 1;
				break;
			}
		}
		layOutItems();
	}

	// Where each item of a holder goes: its place, then each item on from the
	// one below by its <spread>, or by <spread sprite> when it shows that look.
	Vector2f Game::itemPlace(const Object& holder, std::size_t index)
	{
		const std::size_t count = holderOf(holder).items.size();
		const auto step = [&](std::size_t i)
		{
			const Object* item = itemAt(holderOf(holder).items[i]);
			const bool looked = item && !holderOf(holder).spreadSprite.empty() && item->lookName() == holderOf(holder).spreadSprite;
			return looked ? holderOf(holder).lookSpread : holderOf(holder).spread;
		};
		// A fan never wider than its <width>: the steps across shrink.
		float across = 1.0f;
		if (holderOf(holder).spreadWidth > 0.0f && count > 1)
		{
			float wide = 0.0f;
			for (std::size_t i = 0; i + 1 < count; ++i) { wide += std::fabs(step(i).x); }
			const Object* first = itemAt(holderOf(holder).items.front());
			const float room = holderOf(holder).spreadWidth - (first ? sizeOf(*first).x : 0.0f);
			if (wide > room && wide > 0.0f) { across = std::max(0.0f, room) / wide; }
		}
		Vector2f at = holder.position;
		for (std::size_t i = 0; i < index && i < count; ++i)
		{
			const Vector2f by = step(i);
			at += Vector2f{ by.x * across, by.y };
		}
		return at;
	}

	void Game::layOutItems()
	{
		int order = 1;
		for (Object& holder : objects)
		{
			if (!isHolder(holder)) { continue; }
			for (std::size_t i = 0; i < holderOf(holder).items.size(); ++i)
			{
				Object* item = itemAt(holderOf(holder).items[i]);
				if (!item) { continue; }
				item->position = itemPlace(holder, i);
				item->drawOrder = order++;
			}
		}

		for (Object& hand : objects)
		{
			if (!isHand(hand)) { continue; }
			Object* holder = handOf(hand).holder.empty() ? nullptr : holderNamed(handOf(hand).holder);
			// A hand pointing into its own holder, emptied then dealt to again:
			// at its first item.
			if (holder && handOf(hand).at < 0 && !holderOf(*holder).items.empty() && !handOf(hand).own.empty() && holderNamed(handOf(hand).own) == holder) { handOf(hand).at = 0; }
			Vector2f pointed = holder ? holder->position : hand.position;
			Vector2f size = holder ? sizeOf(*holder) : Vector2f{};
			if (holder && handOf(hand).at >= 0 && static_cast<std::size_t>(handOf(hand).at) < holderOf(*holder).items.size())
			{
				pointed = itemPlace(*holder, static_cast<std::size_t>(handOf(hand).at));
				if (Object* item = itemAt(holderOf(*holder).items[static_cast<std::size_t>(handOf(hand).at)]))
				{
					size = sizeOf(*item);
					// A holder with a <lift>: a face-up item pointed at is lifted
					// out of the fan, so all of it shows.
					if (item->item.faceUp && holderOf(*holder).lift != 0.0f)
					{
						pointed.y -= holderOf(*holder).lift;
						item->position = pointed;
					}
				}
			}
			// The hand around what it points at.
			const Vector2f own = sizeOf(hand);
			hand.position = pointed + (size - own) * 0.5f;

			// What it holds: following the pointer, or laid as it would be
			// on the holder pointed at, a little off it.
			Vector2f at = pointed + Vector2f{ 6.0f, 6.0f };
			if (holder && !holderOf(*holder).items.empty()) { at = itemPlace(*holder, holderOf(*holder).items.size() - 1) + holderOf(*holder).spread + Vector2f{ 6.0f, 6.0f }; }
			if (handOf(hand).mouse && pickerOf(hand).pointer.x >= 0.0f && pickerOf(hand).pointer.y >= 0.0f) { at = pickerOf(hand).pointer - handOf(hand).grab; }
			for (const int number : handOf(hand).held)
			{
				Object* item = itemAt(number);
				if (!item) { continue; }
				item->position = at;
				item->drawOrder = 100000 + order++;
				at += holder ? holderOf(*holder).spread : Vector2f{ 0.0f, 20.0f };
				if (holder && holderOf(*holder).spread.x == 0.0f && holderOf(*holder).spread.y == 0.0f) { at += Vector2f{ 0.0f, 20.0f }; }
			}
			hand.drawOrder = 200000;
		}
	}

	// Whether `items` (the lowest first) may go on the holder, by its <accepts>.
	bool Game::holderTakes(Object& holder, const std::vector<int>& items, const Object* from)
	{
		const HolderRule& rule = holderOf(holder).takes;
		if (!rule.given || items.empty()) { return false; }
		// <total variable>N: what is laid together, that variable added up, is N.
		for (const HolderRule::VarRule& each : rule.rules)
		{
			if (each.kind != HolderRule::VarRule::Kind::Total) { continue; }
			float total = 0.0f;
			for (const int number : items)
			{
				const Object* item = itemAt(number);
				if (!item) { return false; }
				const auto value = item->variable.find(each.variable);
				total += value == item->variable.end() ? 0.0f : value->second;
			}
			if (total != each.amount) { return false; }
		}
		if (rule.one && items.size() > 1) { return false; }
		const Object* first = itemAt(items.front());
		if (!first) { return false; }
		// A trick: what the hand laying holds (its own holder) decides.
		if (rule.follow)
		{
			const auto holds = [&](const auto& test)
			{
				if (!from) { return false; }
				for (const int number : holderOf(*from).items)
				{
					const Object* item = itemAt(number);
					if (item && number != first->item.number && test(*item)) { return true; }
				}
				return false;
			};
			if (holderOf(holder).items.empty())
			{
				const auto unbroken = [&](const Object& item)
				{
					return std::find(rule.breaks.begin(), rule.breaks.end(), followedName(rule, item)) != rule.breaks.end()
						&& std::find(holderOf(holder).broken.begin(), holderOf(holder).broken.end(), followed(rule, item)) == holderOf(holder).broken.end();
				};
				// A suit not yet broken leads only when the hand holds nothing else.
				if (unbroken(*first) && holds([&](const Object& item) { return !unbroken(item); })) { return false; }
			}
			else if (const Object* lead = itemAt(holderOf(holder).items.front()); lead && followed(rule, *first) != followed(rule, *lead))
			{
				if (holds([&](const Object& item) { return followed(rule, item) == followed(rule, *lead); })) { return false; }
			}
		}
		if (holderOf(holder).items.empty())
		{
			return acceptsBy(rule.emptyRules, *first, nullptr);
		}
		// Waiting for a suit to be called: nothing goes on it.
		if (holderOf(holder).calling > 0.0f) { return false; }
		if (isWild(rule, *first)) { return true; }
		const Object* top = itemAt(holderOf(holder).items.back());
		if (!top || !top->item.faceUp) { return false; }
		// <any>: one of them, a value called standing for the top's own.
		if (!rule.anyRules.empty())
		{
			const float called = rule.wildVariable.empty() ? 0.0f : holderOf(holder).called;
			const bool any = std::any_of(rule.anyRules.begin(), rule.anyRules.end(), [&](const HolderRule::VarRule& each) { return acceptsBy({ each }, *first, top, rule.wildVariable, called); });
			if (!any) { return false; }
		}
		return follows(rule, *top, *first);
	}

	// Whether `next` may lie on `below` by the rule (not counting <one />).
	bool Game::follows(const HolderRule& rule, const Object& below, const Object& next)
	{
		return acceptsBy(rule.rules, next, &below);
	}

	// Whether `next` keeps an <accepts>'s rules on its variables, against
	// the one under it (`below`; none for the first on an empty holder).
	bool Game::acceptsBy(const std::vector<HolderRule::VarRule>& rules, const Object& next, const Object* below, const std::string& called, float value)
	{
		using Rule = HolderRule::VarRule::Kind;
		const auto valueOf = [](const Object& of, const std::string& name)
		{
			const auto found = of.variable.find(name);
			return found == of.variable.end() ? 0.0f : found->second;
		};
		for (const HolderRule::VarRule& each : rules)
		{
			if (each.kind == Rule::Total) { continue; }   // what is laid together, added up (holderTakes)
			const float mine = valueOf(next, each.variable);
			const std::string& named = each.topVariable.empty() ? each.variable : each.topVariable;
			const float theirs = !below ? 0.0f : value > 0.0f && named == called ? value : valueOf(*below, named);
			const float against = each.top ? theirs + each.amount : each.amount;
			bool holds = true;
			switch (each.kind)
			{
			case Rule::Equal: holds = mine == against; break;
			case Rule::Unequal: holds = mine != against; break;
			case Rule::Above: holds = mine > against; break;
			case Rule::Below: holds = mine < against; break;
			case Rule::AtLeast: holds = mine >= against; break;
			case Rule::AtMost: holds = mine <= against; break;
			case Rule::Total: break;
			}
			if (!holds) { return false; }
		}
		return true;
	}

	// Whether an item of a holder lies under another holder's item drawn over it.
	bool Game::isCovered(const Object& item, const Object& own)
	{
		const Vector2f size = sizeOf(item);
		for (const Object& holder : objects)
		{
			if (!isHolder(holder) || &holder == &own || !isShown(holder)) { continue; }
			for (const int number : holderOf(holder).items)
			{
				const Object* other = itemAt(number);
				if (!other || other == &item || other->drawOrder <= item.drawOrder) { continue; }
				const Vector2f otherSize = sizeOf(*other);
				if (other->position.x < item.position.x + size.x && item.position.x < other->position.x + otherSize.x
					&& other->position.y < item.position.y + size.y && item.position.y < other->position.y + otherSize.y)
				{
					return true;
				}
			}
		}
		return false;
	}

	bool Game::takeItems(Object& hand)
	{
		if (!isHand(hand) || (handOf(hand).mouse && !handOf(hand).over)) { return false; }
		Object* holder = holderNamed(handOf(hand).holder);
		if (!holder || holderOf(*holder).pickedSends > 0 || holderOf(*holder).items.empty()) { return false; }
		// Covering: a top item that an item of another holder, drawn over it,
		// overlaps stays where it is.
		if (const Object* top = itemAt(holderOf(*holder).items.back()); top && isCovered(*top, *holder)) { return false; }
		// Holding already: the top item of another holder too, while the hand
		// <holds> items from that many holders.
		if (!handOf(hand).held.empty())
		{
			const auto taken = [&](const std::string& name) { return name == handOf(hand).from || std::any_of(handOf(hand).also.begin(), handOf(hand).also.end(), [&](const auto& each) { return each.second == name; }); };
			if (static_cast<int>(handOf(hand).also.size()) + 1 >= handOf(hand).holds || taken(holder->name)) { return false; }
			const int number = holderOf(*holder).items.back();
			const Object* top = itemAt(number);
			if (!top || !top->item.faceUp) { return false; }
			holderOf(*holder).items.pop_back();
			refreshHolder(*holder);
			handOf(hand).held.push_back(number);
			handOf(hand).also.emplace_back(number, holder->name);
			layOutItems();
			return true;
		}
		const int count = static_cast<int>(holderOf(*holder).items.size());
		const int at = handOf(hand).at < 0 ? count - 1 : std::min(handOf(hand).at, count - 1);
		// A hand with a holder of its own takes from it alone, any one item,
		// face up or down (it knows its own items).
		if (!handOf(hand).own.empty())
		{
			if (holder != holderNamed(handOf(hand).own)) { return false; }
			handOf(hand).held = { holderOf(*holder).items[static_cast<std::size_t>(at)] };
			holderOf(*holder).items.erase(holderOf(*holder).items.begin() + at);
			refreshHolder(*holder);
			handOf(hand).from = holder->name;
			handOf(hand).at = std::min(at, static_cast<int>(holderOf(*holder).items.size()) - 1);
			if (handOf(hand).mouse && pickerOf(hand).pointer.x >= 0.0f && pickerOf(hand).pointer.y >= 0.0f)
			{
				handOf(hand).grab = pickerOf(hand).pointer - itemPlace(*holder, static_cast<std::size_t>(std::max(0, handOf(hand).at)));
			}
			layOutItems();
			return true;
		}
		if (at < count - 1 && !holderOf(*holder).carry) { return false; }
		// Face up, and each on the one below it by the holder's own rule.
		for (int i = at; i < count; ++i)
		{
			const Object* item = itemAt(holderOf(*holder).items[static_cast<std::size_t>(i)]);
			if (!item || !item->item.faceUp) { return false; }
			if (i > at && !follows(holderOf(*holder).takes, *itemAt(holderOf(*holder).items[static_cast<std::size_t>(i - 1)]), *item)) { return false; }
		}
		handOf(hand).held.assign(holderOf(*holder).items.begin() + at, holderOf(*holder).items.end());
		holderOf(*holder).items.resize(static_cast<std::size_t>(at));
		refreshHolder(*holder);
		handOf(hand).from = holder->name;
		handOf(hand).at = at - 1;
		// Held where the pointer took them.
		if (handOf(hand).mouse && pickerOf(hand).pointer.x >= 0.0f && pickerOf(hand).pointer.y >= 0.0f)
		{
			handOf(hand).grab = pickerOf(hand).pointer - itemPlace(*holder, static_cast<std::size_t>(at));
		}
		layOutItems();
		return true;
	}

	bool Game::pickItems(Object& hand)
	{
		// From a holder with <picked>: it sends instead, and nothing is
		// picked up, so the picker's verbs after <pick /> do not run.
		const Object* holder = isHand(hand) ? holderNamed(handOf(hand).holder) : nullptr;
		if (holder && holderOf(*holder).pickedSends > 0)
		{
			const std::string sound = holderOf(*holder).pickedSound;
			if (sendPicked(hand, "", "") && !sound.empty()) { requestSound(sound); }
			return false;
		}
		return takeItems(hand);
	}

	// The items held go on the holder, the top one pointed at, and the holder
	// they came from shows its top.
	void Game::layHeld(Object& hand, Object& holder)
	{
		for (const int number : handOf(hand).held)
		{
			if (Object* item = itemAt(number)) { item->item.laidBy = handOf(hand).seat; }
			putInHolder(holder, number);
		}
		handOf(hand).held.clear();
		if (Object* from = holderNamed(handOf(hand).from); from && from != &holder) { showTop(*from); }
		handOf(hand).from.clear();
		for (const auto& [number, name] : handOf(hand).also)
		{
			if (Object* from = holderNamed(name); from && from != &holder) { showTop(*from); }
		}
		handOf(hand).also.clear();
		// A hand with a holder of its own stays pointing into it.
		if (Object* own = handOf(hand).own.empty() ? nullptr : holderNamed(handOf(hand).own))
		{
			handOf(hand).holder = own->name;
			handOf(hand).at = std::min(std::max(handOf(hand).at, 0), static_cast<int>(holderOf(*own).items.size()) - 1);
		}
		else
		{
			handOf(hand).holder = holder.name;
			handOf(hand).at = static_cast<int>(holderOf(holder).items.size()) - 1;
		}
		layOutItems();
	}

	bool Game::layItems(Object& hand, const std::string& onto)
	{
		if (!isHand(hand) || handOf(hand).held.empty()) { return false; }
		std::vector<Object*> choices;
		if (onto.empty())
		{
			// Let go over nothing: laid nowhere.
			if (handOf(hand).mouse && !handOf(hand).over) { return false; }
			if (Object* holder = holderNamed(handOf(hand).holder)) { choices.push_back(holder); }
		}
		else
		{
			const std::string full = fullName(onto);
			for (Object& object : objects)
			{
				if (isHolder(object) && object.named(full) && isShown(object)) { choices.push_back(&object); }
			}
		}
		for (Object* holder : choices)
		{
			// Back where they came from: put back, always allowed.
			if (holder->name == handOf(hand).from && onto.empty())
			{
				layHeld(hand, *holder);
				return true;
			}
			if (holderTakes(*holder, handOf(hand).held, handOf(hand).own.empty() ? nullptr : holderNamed(handOf(hand).own)))
			{
				layHeld(hand, *holder);
				return true;
			}
		}
		return false;
	}

	bool Game::dropItems(Object& hand)
	{
		if (!isHand(hand) || handOf(hand).held.empty()) { return false; }
		Object* from = holderNamed(handOf(hand).from);
		if (!from) { return false; }
		const std::string pointed = handOf(hand).holder;
		const int at = handOf(hand).at;
		const int item = handOf(hand).held.front();
		// Items taken from other holders go back to theirs first.
		for (const auto& [number, name] : handOf(hand).also)
		{
			handOf(hand).held.erase(std::find(handOf(hand).held.begin(), handOf(hand).held.end(), number));
			if (Object* back = holderNamed(name)) { putInHolder(*back, number); }
		}
		handOf(hand).also.clear();
		layHeld(hand, *from);
		// Back in a hand's own holder: pointing at it again.
		if (!handOf(hand).own.empty() && from == holderNamed(handOf(hand).own))
		{
			const auto found = std::find(holderOf(*from).items.begin(), holderOf(*from).items.end(), item);
			handOf(hand).holder = from->name;
			handOf(hand).at = static_cast<int>(found - holderOf(*from).items.begin());
			layOutItems();
			return true;
		}
		// Put back, the hand stays where it was.
		handOf(hand).holder = pointed;
		handOf(hand).at = at;
		if (Object* holder = holderNamed(pointed)) { handOf(hand).at = std::min(at < 0 ? static_cast<int>(holderOf(*holder).items.size()) - 1 : at, static_cast<int>(holderOf(*holder).items.size()) - 1); }
		layOutItems();
		return true;
	}

	bool Game::sendPicked(Object& hand, const std::string& fromHolder, const std::string& faces)
	{
		if (!isHand(hand) || !handOf(hand).held.empty() || (fromHolder.empty() && handOf(hand).mouse && !handOf(hand).over)) { return false; }
		// From the holder named, or else the one pointed at.
		Object* holder = holderNamed(fromHolder.empty() ? handOf(hand).holder : fromHolder);
		if (!holder || holderOf(*holder).pickedSends <= 0) { return false; }
		// Into the holder it names, or else the hand's own.
		const std::string& intoName = holderOf(*holder).pickedInto.empty() ? handOf(hand).own : holderOf(*holder).pickedInto;
		Object* into = intoName.empty() ? nullptr : holderNamed(intoName);
		if (!into) { return false; }
		if (!holderOf(*holder).items.empty())
		{
			for (int i = 0; i < holderOf(*holder).pickedSends && !holderOf(*holder).items.empty(); ++i)
			{
				const int number = takeTop(*holder);
				putInHolder(*into, number);
				if (Object* item = itemAt(number); item && !faces.empty())
				{
					turnItem(*item, faces == "up");
					refreshHolder(*into);
				}
			}
		}
		else
		{
			// Empty: the other holder turned back over onto it (or shuffled),
			// but for the top items it leaves.
			Object* from = holderOf(*holder).emptyFrom.empty() ? nullptr : holderNamed(holderOf(*holder).emptyFrom);
			const std::size_t leave = static_cast<std::size_t>(holderOf(*holder).emptyLeave);
			if (!from || holderOf(*from).items.size() <= leave) { return false; }
			const auto end = holderOf(*from).items.end() - static_cast<std::ptrdiff_t>(leave);
			std::vector<int> items(holderOf(*from).items.begin(), end);
			holderOf(*from).items.erase(holderOf(*from).items.begin(), end);
			refreshHolder(*from);
			if (holderOf(*holder).emptyRandom)
			{
				for (std::size_t i = items.size(); i > 1; --i)
				{
					const std::size_t j = std::min(i - 1, static_cast<std::size_t>(randomBetween(0.0f, static_cast<float>(i))));
					std::swap(items[i - 1], items[j]);
				}
			}
			else
			{
				// Turned over: the top first.
				std::reverse(items.begin(), items.end());
			}
			for (const int item : items) { putInHolder(*holder, item); }
		}
		// Pointing at the stock, still pointing at its top.
		if (fromHolder.empty()) { handOf(hand).at = static_cast<int>(holderOf(*holder).items.size()) - 1; }
		layOutItems();
		return true;
	}

	namespace
	{
		// Which way up an item showing that look is: its face (its first
		// look) up; another of its looks down; none of its looks, nothing.
		std::optional<bool> faceFor(const Object& item, const std::string& sprite)
		{
			for (std::size_t i = 0; i < item.looks.all.size(); ++i)
			{
				if (item.looks.all[i].name == sprite) { return i == 0; }
			}
			return std::nullopt;
		}
	}

	bool Game::sendHeld(const std::string& fromName, const std::string& intoName, int count, bool all, const std::string& sprite)
	{
		Object* into = holderNamed(intoName);
		if (!into)
		{
			// A group's: each member's own holder in turn, an item a round.
			std::vector<std::pair<int, std::string>> owners;
			for (const Object& object : objects)
			{
				if (isHand(object) && !handOf(object).own.empty() && (object.inGroup(intoName) || object.inGroup(fullName(intoName)))) { owners.emplace_back(handOf(object).seat, handOf(object).own); }
			}
			if (owners.empty() || all) { return false; }
			std::stable_sort(owners.begin(), owners.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
			bool any = false;
			for (int round = 0; round < count; ++round)
			{
				for (const auto& [seat, own] : owners) { any = sendHeld(fromName, own, 1, false, sprite) || any; }
			}
			return any;
		}
		std::vector<int> sent;
		Object* from = holderNamed(fromName);
		if (from)
		{
			if (from == into || holderOf(*from).items.empty()) { return false; }
			const std::size_t many = all ? holderOf(*from).items.size() : std::min(holderOf(*from).items.size(), static_cast<std::size_t>(count));
			// The top ones, the topmost first.
			for (std::size_t i = 0; i < many; ++i) { sent.push_back(takeTop(*from)); }
		}
		else
		{
			// A group's name: every member, wherever it is.
			if (!all) { return false; }
			for (Object& object : objects)
			{
				if (object.item.is && (object.inGroup(fromName) || object.inGroup(fullName(fromName)))) { sent.push_back(object.item.number); }
			}
			if (sent.empty()) { return false; }
			for (Object& object : objects)
			{
				if (isHolder(object))
				{
					auto& items = holderOf(object).items;
					const auto gone = std::remove_if(items.begin(), items.end(), [&](int number) { return std::find(sent.begin(), sent.end(), number) != sent.end(); });
					if (gone != items.end())
					{
						items.erase(gone, items.end());
						holderOf(object).broken.clear();
						holderOf(object).called = 0.0f;
						refreshHolder(object);
					}
				}
				if (isHand(object))
				{
					handOf(object).held.clear();
					handOf(object).from.clear();
				}
			}
			std::sort(sent.begin(), sent.end());
		}
		for (const int number : sent)
		{
			Object* item = itemAt(number);
			if (item) { item->item.laidBy = 0; }
			putInHolder(*into, number);
			if (item && !sprite.empty())
			{
				if (const auto up = faceFor(*item, sprite)) { turnItem(*item, *up); }
			}
		}
		refreshHolder(*into);
		// What is left on top of where they came from turns as its <top> says.
		if (from) { showTop(*from); }
		// A hand pointing where items were: at its own, or at that holder.
		for (Object& object : objects)
		{
			if (!isHand(object)) { continue; }
			Object* pointed = holderNamed(handOf(object).holder);
			if (pointed && handOf(object).at >= static_cast<int>(holderOf(*pointed).items.size())) { handOf(object).at = static_cast<int>(holderOf(*pointed).items.size()) - 1; }
		}
		layOutItems();
		return true;
	}

	bool Game::orderHeld(const std::string& name)
	{
		Object* holder = holderNamed(name);
		if (!holder) { return false; }
		std::vector<int>& items = holderOf(*holder).items;
		for (std::size_t i = items.size(); i > 1; --i)
		{
			const std::size_t j = std::min(i - 1, static_cast<std::size_t>(randomBetween(0.0f, static_cast<float>(i))));
			std::swap(items[i - 1], items[j]);
		}
		// Stacked (tests): those items last, so on top, the first last of all.
		for (auto each = stackedStart.rbegin(); each != stackedStart.rend(); ++each)
		{
			const auto at = std::find(items.begin(), items.end(), *each);
			if (at == items.end()) { continue; }
			items.erase(at);
			items.push_back(*each);
		}
		stackedStart.clear();
		refreshHolder(*holder);
		showTop(*holder);
		layOutItems();
		return true;
	}

	bool Game::turnHeld(Object& holder, const std::string& sprite)
	{
		for (const int number : holderOf(holder).items)
		{
			Object* item = itemAt(number);
			if (!item) { continue; }
			const auto up = faceFor(*item, sprite);
			if (!up) { return false; }
			if (item->item.faceUp != *up) { turnItem(*item, *up); }
		}
		refreshHolder(holder);
		layOutItems();
		return true;
	}

	// Every holder in play, and its middle and size as drawn.
	std::vector<Object*> Game::shownHolders()
	{
		std::vector<Object*> holders;
		for (Object& object : objects)
		{
			if (isHolder(object) && isShown(object)) { holders.push_back(&object); }
		}
		return holders;
	}

	bool Game::hopHand(Object& hand, Direction direction)
	{
		Object* holder = holderNamed(handOf(hand).holder);
		const std::vector<Object*> holders = shownHolders();
		if (holders.empty()) { return false; }
		if (!holder) { holder = holders.front(); }
		const int count = static_cast<int>(holderOf(*holder).items.size());
		const bool own = !handOf(hand).own.empty() && holder == holderNamed(handOf(hand).own);
		const auto takeable = [&](int index)
		{
			if (index < 0 || index >= count) { return false; }
			const Object* item = itemAt(holderOf(*holder).items[static_cast<std::size_t>(index)]);
			return item && (item->item.faceUp || own);
		};
		const auto pointAt = [&](Object& to)
		{
			handOf(hand).holder = to.name;
			handOf(hand).at = static_cast<int>(holderOf(to).items.size()) - 1;
			handOf(hand).mouse = false;
			layOutItems();
			return true;
		};

		hand.facing = direction;
		// A holder fanned across: left and right go through its items first
		// (any of a hand's own, else a run's face-up ones).
		const bool fannedAcross = std::fabs(holderOf(*holder).spread.x) > std::fabs(holderOf(*holder).spread.y);
		if (fannedAcross && (direction == Direction::Left || direction == Direction::Right) && (own || holderOf(*holder).carry))
		{
			const int at = handOf(hand).at < 0 ? count - 1 : handOf(hand).at;
			const int next = at + ((direction == Direction::Right) == (holderOf(*holder).spread.x > 0.0f) ? 1 : -1);
			if (takeable(next) && takeable(at))
			{
				handOf(hand).at = next;
				handOf(hand).mouse = false;
				layOutItems();
				return true;
			}
		}
		if (direction == Direction::Left || direction == Direction::Right)
		{
			// The next holder in the row, at the same height.
			const float ahead = direction == Direction::Right ? 1.0f : -1.0f;
			Object* best = nullptr;
			for (Object* each : holders)
			{
				if (each == holder || std::fabs(each->position.y - holder->position.y) > 0.5f) { continue; }
				const float along = (each->position.x - holder->position.x) * ahead;
				if (along <= 0.0f) { continue; }
				if (!best || along < (best->position.x - holder->position.x) * ahead) { best = each; }
			}
			return best && pointAt(*best);
		}

		// Up and down: through a run's face-up items, then to the row of
		// holders above or below, the holder nearest across.
		const bool up = direction == Direction::Up;
		const int at = handOf(hand).at < 0 ? count - 1 : handOf(hand).at;
		if (!fannedAcross && holderOf(*holder).carry && up && takeable(at - 1) && takeable(at))
		{
			handOf(hand).at = at - 1;
			handOf(hand).mouse = false;
			layOutItems();
			return true;
		}
		if (!fannedAcross && holderOf(*holder).carry && !up && at < count - 1)
		{
			handOf(hand).at = at + 1;
			handOf(hand).mouse = false;
			layOutItems();
			return true;
		}
		Object* best = nullptr;
		float bestRow = 0.0f;
		float bestAcross = 0.0f;
		for (Object* each : holders)
		{
			const float row = (each->position.y - holder->position.y) * (up ? -1.0f : 1.0f);
			if (row <= 0.5f) { continue; }
			const float across = std::fabs(each->position.x - holder->position.x);
			if (!best || row < bestRow - 0.5f || (std::fabs(row - bestRow) <= 0.5f && across < bestAcross))
			{
				best = each;
				bestRow = row;
				bestAcross = across;
			}
		}
		return best && pointAt(*best);
	}

	void Game::trackHand(Object& hand, bool horizontal, float at)
	{
		(horizontal ? pickerFor(hand).pointer.x : pickerFor(hand).pointer.y) = at;
		if (pickerOf(hand).pointer.x < 0.0f || pickerOf(hand).pointer.y < 0.0f) { return; }
		handOf(hand).mouse = true;
		handOf(hand).over = false;
		const Vector2f point = pickerOf(hand).pointer;
		const auto inside = [&](Vector2f where, Vector2f size)
		{
			return point.x >= where.x && point.y >= where.y && point.x < where.x + size.x && point.y < where.y + size.y;
		};
		// The top item under the pointer (of holders overlapping, the one
		// drawn over), else the holder's place.
		Object* found = nullptr;
		int foundAt = -1;
		int foundOrder = -1;
		for (Object* holder : shownHolders())
		{
			bool hit = false;
			int index = -1;
			for (int i = static_cast<int>(holderOf(*holder).items.size()) - 1; i >= 0 && !hit; --i)
			{
				const Object* item = itemAt(holderOf(*holder).items[static_cast<std::size_t>(i)]);
				if (item && inside(itemPlace(*holder, static_cast<std::size_t>(i)), sizeOf(*item)))
				{
					hit = true;
					index = i;
				}
			}
			if (!hit && inside(holder->position, sizeOf(*holder))) { hit = true; }
			if (!hit) { continue; }
			const Object* item = index >= 0 ? itemAt(holderOf(*holder).items[static_cast<std::size_t>(index)]) : nullptr;
			const int order = item ? item->drawOrder : 0;
			if (found && order <= foundOrder) { continue; }
			found = holder;
			foundAt = index;
			foundOrder = order;
		}
		if (found)
		{
			// Holding: the holder, its top item.
			if (!handOf(hand).held.empty() && handOf(hand).holds <= 1) { foundAt = static_cast<int>(holderOf(*found).items.size()) - 1; }
			handOf(hand).holder = found->name;
			handOf(hand).at = foundAt;
			handOf(hand).over = true;
		}
		layOutItems();
	}

	bool Game::callValue(Object& hand, const std::string& name)
	{
		if (!isHand(hand) || !handOf(hand).held.empty() || handOf(hand).own.empty() || (handOf(hand).mouse && !handOf(hand).over)) { return false; }
		Object* holder = holderNamed(name);
		Object* own = holderNamed(handOf(hand).own);
		if (!holder || !own || holderOf(*holder).calling <= 0.0f || holderNamed(handOf(hand).holder) != own) { return false; }
		const int count = static_cast<int>(holderOf(*own).items.size());
		if (count == 0) { return false; }
		const int at = handOf(hand).at < 0 ? count - 1 : std::min(handOf(hand).at, count - 1);
		const Object* item = itemAt(holderOf(*own).items[static_cast<std::size_t>(at)]);
		if (!item) { return false; }
		// The value named: the item's <wild variable=>.
		holderOf(*holder).called = valueOf(*item, holderOf(*holder).takes.wildVariable);
		refreshCall(*holder);
		return true;
	}

	// A computer playing a card game, a step each time it is due: point the
	// hand at what it will do, then press. Calling, the suit it holds most
	// of; else the item that goes somewhere, a wild one last, from the suit
	// it holds most of and the highest; else a draw from the stock.
	int Game::playToTrick(Object& trick, const Object& hand, const Object& own, bool avoid)
	{
		int players = 0;
		for (const Object& object : objects)
		{
			if (isHand(object) && handOf(object).seat > 0 && isShown(object)) { ++players; }
		}
		const bool last = static_cast<int>(holderOf(trick).items.size()) + 1 >= std::max(players, 2);
		const Object* winning = trickWinning(trick);
		const auto worth = [](const Object& item) { return item.item.worths.empty() ? 0.0f : item.item.worths.back(); };

		std::vector<std::pair<int, const Object*>> legal;
		for (int i = 0; i < static_cast<int>(holderOf(own).items.size()); ++i)
		{
			const int number = holderOf(own).items[static_cast<std::size_t>(i)];
			const Object* item = itemAt(number);
			if (item && holderTakes(trick, { number }, &own)) { legal.emplace_back(i, item); }
		}
		if (legal.empty()) { return -1; }
		const Object* led = holderOf(trick).items.empty() ? nullptr : itemAt(holderOf(trick).items.front());
		const bool following = led && std::any_of(legal.begin(), legal.end(), [&](const auto& each) { return followed(holderOf(trick).takes, *each.second) == followed(holderOf(trick).takes, *led); });
		const bool ducking = following && std::any_of(legal.begin(), legal.end(), [&](const auto& each) { return worth(*each.second) < worth(*winning); });
		const HolderRule& rule = holderOf(trick).takes;
		float top = 0.0f;
		for (const Object& object : objects)
		{
			if (object.item.is && !object.item.worths.empty()) { top = std::max(top, object.item.worths.back()); }
		}
		// A partner (another hand on its <team>) winning safely: it is the
		// last item, or the partner's is the highest or next of its suit.
		bool partnerWins = false;
		if (winning && handOf(hand).team > 0 && winning->item.laidBy != handOf(hand).seat)
		{
			for (const Object& other : objects)
			{
				if (isHand(other) && handOf(other).seat == winning->item.laidBy && handOf(other).team == handOf(hand).team) { partnerWins = last || worth(*winning) >= top - 1.0f || isTrump(rule, *winning); }
			}
		}

		// How much each item is wanted: the highest is played.
		const auto wanted = [&](const Object& item)
		{
			const float value = worth(item);
			const float points = itemPoints(trick, item);
			if (!winning)                                                                  // leading
			{
				if (avoid) { return -(points * 100.0f + value); }
				return value >= top ? 100.0f + value : -value - (isTrump(rule, item) ? 50.0f : 0.0f);  // an ace, else the lowest side item
			}
			if (!following && avoid) { return points * 100.0f + value; }                  // none of the suit led: shed
			if (avoid)
			{
				if (ducking) { return value < worth(*winning) ? value : -1000.0f - value; }  // the highest that still loses
				return last ? value : -value;                                               // taking it anyway
			}
			const float spare = isTrump(rule, item) ? 50.0f : 0.0f;                           // a trump kept back
			if (partnerWins) { return -value - spare; }                                       // the lowest: the partner has it
			return beatsInTrick(rule, item, *winning) ? 1000.0f - value : -value - spare;     // the lowest that wins, else the lowest
		};
		int best = legal.front().first;
		float most = wanted(*legal.front().second);
		for (const auto& [i, item] : legal)
		{
			const float each = wanted(*item);
			if (each > most)
			{
				most = each;
				best = i;
			}
		}
		return best;
	}

	std::optional<KeyCode> Game::thinkHeld(const CmdThink& think, Object& hand, float seconds)
	{
		Object* own = handOf(hand).own.empty() ? nullptr : holderNamed(handOf(hand).own);
		if (!own || !handOf(hand).held.empty()) { return std::nullopt; }
		if (itemWait > kTimeSlack)
		{
			itemWait -= seconds;
			return std::nullopt;
		}

		// How many of its own items share each value of the variable a wild
		// item calls (none called: all alike).
		std::string calledBy;
		for (const Object* each : shownHolders())
		{
			if (calledBy.empty()) { calledBy = holderOf(*each).takes.wildVariable; }
		}
		const auto kindOf = [&](const Object& item) { return calledBy.empty() ? 0 : static_cast<int>(std::lround(valueOf(item, calledBy))); };
		std::map<int, int> ofKind;
		for (const int number : holderOf(*own).items)
		{
			if (const Object* item = itemAt(number)) { ++ofKind[kindOf(*item)]; }
		}
		std::vector<Object*> targets;
		Object* stock = nullptr;
		for (Object* holder : shownHolders())
		{
			if (holder == own) { continue; }
			if (holderOf(*holder).takes.given) { targets.push_back(holder); }
			if (!stock && holderOf(*holder).pickedSends > 0) { stock = holder; }
		}

		Object* aim = nullptr;
		int index = -1;
		const bool calling = std::any_of(targets.begin(), targets.end(), [this](const Object* holder) { return holderOf(*holder).calling > 0.0f; });
		const auto trick = std::find_if(targets.begin(), targets.end(), [this](const Object* holder) { return holderOf(*holder).takes.follow; });
		if (!holderOf(*own).melds.empty())
		{
			// Melds: holding no more than its best, it draws, the face-up top
			// of a holder when that leaves less deadwood, else the face-down
			// one; holding more, it lets go of the item leaving the least
			// (the one worth most, of two alike).
			const int best = holderOf(*own).meldsBest > 0 ? holderOf(*own).meldsBest : static_cast<int>(holderOf(*own).items.size());
			if (static_cast<int>(holderOf(*own).items.size()) <= best)
			{
				Object* up = nullptr;
				Object* down = nullptr;
				for (Object* holder : shownHolders())
				{
					if (holder == own || holderOf(*holder).pickedSends <= 0 || holderOf(*holder).items.empty()) { continue; }
					const Object* top = itemAt(holderOf(*holder).items.back());
					if (top && top->item.faceUp) { up = up ? up : holder; }
					else { down = down ? down : holder; }
				}
				aim = down ? down : up;
				if (up && down)
				{
					std::vector<int> with = holderOf(*own).items;
					with.push_back(holderOf(*up).items.back());
					if (meldDeadwood(*own, with, best, nullptr) < meldDeadwood(*own, holderOf(*own).items, best, nullptr)) { aim = up; }
				}
				if (aim) { index = static_cast<int>(holderOf(*aim).items.size()) - 1; }
			}
			else
			{
				float least = 0.0f;
				float worth = 0.0f;
				for (int i = 0; i < static_cast<int>(holderOf(*own).items.size()); ++i)
				{
					std::vector<int> rest = holderOf(*own).items;
					rest.erase(rest.begin() + i);
					const float left = meldDeadwood(*own, rest, 0, nullptr);
					const float points = itemAt(holderOf(*own).items[static_cast<std::size_t>(i)]) ? itemPoints(*own, *itemAt(holderOf(*own).items[static_cast<std::size_t>(i)])) : 0.0f;
					if (index < 0 || left < least || (left == least && points > worth))
					{
						least = left;
						worth = points;
						index = i;
					}
				}
				if (index >= 0) { aim = own; }
			}
		}
		else if (trick != targets.end() && !calling)
		{
			// A trick: of the items it takes, the one that best keeps out
			// of it (<avoid />) or takes it.
			index = playToTrick(**trick, hand, *own, think.avoid);
			if (index >= 0) { aim = own; }
		}
		else if (calling)
		{
			int most = 0;
			for (int i = 0; i < static_cast<int>(holderOf(*own).items.size()); ++i)
			{
				const Object* item = itemAt(holderOf(*own).items[static_cast<std::size_t>(i)]);
				if (item && ofKind[kindOf(*item)] > most)
				{
					most = ofKind[kindOf(*item)];
					aim = own;
					index = i;
				}
			}
		}
		else
		{
			float best = 0.0f;
			for (int i = 0; i < static_cast<int>(holderOf(*own).items.size()); ++i)
			{
				const int number = holderOf(*own).items[static_cast<std::size_t>(i)];
				const Object* item = itemAt(number);
				if (!item) { continue; }
				for (Object* holder : targets)
				{
					if (!holderTakes(*holder, { number }, own)) { continue; }
					const float worth = isWild(holderOf(*holder).takes, *item) ? 1.0f : 100.0f + static_cast<float>(ofKind[kindOf(*item)] * 20) + (item->item.worths.empty() ? 0.0f : item->item.worths.back());
					if (worth > best)
					{
						best = worth;
						aim = own;
						index = i;
					}
				}
			}
			if (!aim && stock)
			{
				aim = stock;
				index = static_cast<int>(holderOf(*stock).items.size()) - 1;
			}
		}
		if (!aim) { return std::nullopt; }

		itemWait = kItemPace;
		if (handOf(hand).holder != aim->name || handOf(hand).at != index)
		{
			handOf(hand).holder = aim->name;
			handOf(hand).at = index;
			handOf(hand).mouse = false;
			layOutItems();
			return std::nullopt;
		}
		return think.button;
	}
}
