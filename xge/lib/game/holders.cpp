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
#include "game/holders.h"
#include "game/words.h"
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
		float valueOf(const Object& item, Name variable) { return item.variable.valueOr(variable, 0.0f); }
		float valueOf(const Object& item, const std::string& variable)
		{
			const auto value = item.variable.findText(variable);
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

	Object* Holders::itemAt(int number)
	{
		if (number <= 0) { return nullptr; }
		const std::size_t at = static_cast<std::size_t>(number - 1);
		if (at < itemIndex.size() && itemIndex[at] < game.getCurrentObjects().size() && game.itemOf(game.getCurrentObjects()[itemIndex[at]]).number == number) { return &game.getCurrentObjects()[itemIndex[at]]; }
		// Objects moved (a copy taken away): found again.
		itemIndex.clear();
		for (std::size_t i = 0; i < game.getCurrentObjects().size(); ++i)
		{
			if (game.itemOf(game.getCurrentObjects()[i]).number <= 0) { continue; }
			const std::size_t slot = static_cast<std::size_t>(game.itemOf(game.getCurrentObjects()[i]).number - 1);
			if (itemIndex.size() <= slot) { itemIndex.resize(slot + 1, game.getCurrentObjects().size()); }
			itemIndex[slot] = i;
		}
		return at < itemIndex.size() && itemIndex[at] < game.getCurrentObjects().size() ? &game.getCurrentObjects()[itemIndex[at]] : nullptr;
	}

	Object* Holders::holderNamed(Name name)
	{
		const std::string full = game.fullName(name);
		for (Object& object : game.getCurrentObjects())
		{
			if (game.isHolder(object) && object.name == full) { return &object; }
		}
		return nullptr;
	}

	void Holders::showHeld(Object& item, Name look)
	{
		const auto found = std::find_if(item.looks.all().begin(), item.looks.all().end(), [&](const Object::Look& each) { return each.name == look; });
		if (found != item.looks.all().end()) { turnItem(item, found == item.looks.all().begin()); }
	}

	void Holders::turnItem(Object& item, bool up)
	{
		game.itemFor(item).faceUp = up;
		const std::size_t look = up ? 0 : 1;
		if (item.looks.shown != look && look < item.looks.all().size()) { item.showLook(look); }
	}

	void Holders::putInHolder(Object& holder, int number)
	{
		Object* item = itemAt(number);
		if (!item) { return; }
		if (!game.holderOf(holder).holdsSprite.empty()) { showHeld(*item, game.holderOf(holder).holdsSprite); }
		auto at = game.holderOf(holder).items.end();
		if (!game.holderOf(holder).orderBy.empty())
		{
			// <order>: by each variable named, the first first.
			const auto order = [&](const Object& of)
			{
				std::vector<float> values;
				for (const std::string& name : game.holderOf(holder).orderBy)
				{
					const auto found = of.variable.findText(name);
					values.push_back(found == of.variable.end() ? 0.0f : found->second);
				}
				return values;
			};
			const std::vector<float> mine = order(*item);
			at = std::find_if(game.holderOf(holder).items.begin(), game.holderOf(holder).items.end(), [&](int other)
				{
					const Object* each = itemAt(other);
					return each && mine < order(*each);
				});
		}
		game.holderOf(holder).items.insert(at, number);
		// A suit that <breaks>, broken.
		const std::vector<Name>& breaks = game.holderOf(holder).takes.breaks;
		if (std::find(breaks.begin(), breaks.end(), followedName(game.holderOf(holder).takes, *item)) != breaks.end()
			&& std::find(game.holderOf(holder).broken.begin(), game.holderOf(holder).broken.end(), followed(game.holderOf(holder).takes, *item)) == game.holderOf(holder).broken.end())
		{
			game.holderOf(holder).broken.push_back(followed(game.holderOf(holder).takes, *item));
		}
		// An item laid answers any call.
		game.holderOf(holder).called = 0.0f;
		refreshHolder(holder);
	}

	float Holders::itemPoints(const Object& holder, const Object& item)
	{
		if (!game.holderOf(holder).counts.empty() && game.holderOf(holder).counts != "worth")
		{
			const auto value = item.variable.find(game.holderOf(holder).counts);
			return value == item.variable.end() ? 0.0f : value->second;
		}
		return game.itemOf(item).worths.empty() ? 0.0f : game.itemOf(item).worths.front();
	}

	// Whether these items, every one of them, make the pattern: each part
	// holds for all of them (<alike>: one value of its variable, <run>:
	// values in a row, by worth an item counting any of its worths), and
	// there are at least as many as its number.
	bool Holders::meldFits(const std::vector<const Object*>& group, const Patterns::Pattern& pattern)
	{
		const std::size_t n = group.size();
		for (std::size_t p = 0; p < pattern.parts.size(); ++p)
		{
			const auto& [part, count] = pattern.parts[p];
			const Name by = pattern.by[p];
			if (n < static_cast<std::size_t>(count)) { return false; }
			if (part == Patterns::Part::Alike)
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
				const auto valuesOf = [&](const Object& item) { return by == words::worth ? game.itemOf(item).worths : std::vector<float>{ valueOf(item, by) }; };
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
	float Holders::meldDeadwood(const Object& holder, const std::vector<int>& numbers, int best, std::vector<std::vector<int>>* melds)
	{
		std::vector<const Object*> items;
		for (const int number : numbers)
		{
			if (const Object* item = itemAt(number)) { items.push_back(item); }
		}
		const std::size_t n = items.size();
		std::vector<float> value(n);
		for (std::size_t i = 0; i < n; ++i) { value[i] = itemPoints(holder, *items[i]); }
		const Patterns* set = game.patternsNamed(game.holderOf(holder).melds);
		if (!set || n > 16 || n != numbers.size())
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
			if (std::any_of(set->patterns.begin(), set->patterns.end(), [&](const Patterns::Pattern& pattern) { return meldFits(group, pattern); }))
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
	void Holders::layOutMelds(Object& holder)
	{
		std::vector<std::vector<int>> melds;
		const std::vector<int> items = game.holderOf(holder).items;
		game.holderOf(holder).deadwood = meldDeadwood(holder, items, game.holderOf(holder).meldsBest, nullptr);
		meldDeadwood(holder, items, 0, &melds);
		std::vector<int> order;
		std::vector<bool> placed(items.size(), false);
		const auto byWorth = [&](int a, int b)
		{
			const Object& x = *itemAt(items[static_cast<std::size_t>(a)]);
			const Object& y = *itemAt(items[static_cast<std::size_t>(b)]);
			return game.itemOf(x).number < game.itemOf(y).number;
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
		game.holderOf(holder).items = order;
		game.variableChanged(holder.name, words::deadwood, game.holderOf(holder).deadwood);
	}

	bool Holders::isTrump(const HolderRule& rule, const Object& item)
	{
		return std::find(rule.trumps.begin(), rule.trumps.end(), followedName(rule, item)) != rule.trumps.end();
	}

	// Whether `item` laid now takes the trick from `best`: a trump over
	// any other item, else a higher item of the same suit.
	bool Holders::beatsInTrick(const HolderRule& rule, const Object& item, const Object& best)
	{
		const bool trump = isTrump(rule, item);
		if (trump != isTrump(rule, best)) { return trump; }
		return followed(rule, item) == followed(rule, best) && game.itemOf(item).worths.back() > game.itemOf(best).worths.back();
	}

	const Object* Holders::trickWinning(const Object& holder)
	{
		if (game.holderOf(holder).items.empty()) { return nullptr; }
		const Object* best = itemAt(game.holderOf(holder).items.front());
		for (const int number : game.holderOf(holder).items)
		{
			const Object* item = itemAt(number);
			if (item && best && beatsInTrick(game.holderOf(holder).takes, *item, *best)) { best = item; }
		}
		return best;
	}

	// How many tricks a hand's items should take, as a player reckons it
	// to bid: each side suit's ace, a king with another item of its suit,
	// half a queen with two more and a higher item; each trump honour with
	// as many trumps under it as there are above it, and each trump past
	// three; with trumps to spare, a missing suit one and a single item a
	// half. Rounded to the nearest.
	float Holders::reckonTricks(const Object& own, const HolderRule& trick)
	{
		float top = 0.0f;
		std::vector<int> kinds;
		for (const Object& object : game.getCurrentObjects())
		{
			if (!game.itemOf(object).is || game.itemOf(object).worths.empty()) { continue; }
			top = std::max(top, game.itemOf(object).worths.back());
			if (std::find(kinds.begin(), kinds.end(), followed(trick, object)) == kinds.end()) { kinds.push_back(followed(trick, object)); }
		}
		std::map<int, std::vector<float>> held;
		std::map<int, bool> trumped;
		int trumps = 0;
		for (const int number : game.holderOf(own).items)
		{
			const Object* item = itemAt(number);
			if (!item || game.itemOf(*item).worths.empty()) { continue; }
			held[followed(trick, *item)].push_back(game.itemOf(*item).worths.back());
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

	const Object* Holders::trickHolder()
	{
		const auto found = std::find_if(game.getCurrentObjects().begin(), game.getCurrentObjects().end(), [this](const Object& object) { return game.isHolder(object) && game.holderOf(object).takes.follow; });
		return found == game.getCurrentObjects().end() ? nullptr : &*found;
	}

	bool Holders::isWild(const HolderRule& rule, const Object& item)
	{
		return !rule.wildRules.empty() && acceptsBy(rule.wildRules, item, nullptr);
	}

	void Holders::refreshHolder(Object& holder)
	{
		game.holderOf(holder).itemCount = static_cast<float>(game.holderOf(holder).items.size());
		refreshCall(holder);

		// top.<variable>: the top one's, 0 while it holds nothing.
		const Object* top = game.holderOf(holder).items.empty() ? nullptr : itemAt(game.holderOf(holder).items.back());
		for (auto& [name, value] : holder.variable)
		{
			if (name.str().rfind("top.", 0) != 0) { continue; }
			float now = 0.0f;
			if (top)
			{
				if (const auto own = top->variable.findText(name.str().substr(4)); own != top->variable.end()) { now = own->second; }
			}
			if (now != value) { value = now; game.variableChanged(holder.name, name, now); }
		}

		// Points: each face-up item at its lowest worth, then each that may
		// count higher raised as far as the total stays within the limit.
		float total = 0.0f;
		std::vector<const Object*> shown;
		// <counts variable>: a variable of each other than its worth, as it is.
		const Name counted = game.holderOf(holder).counts;
		const bool byVariable = !counted.empty() && counted != words::worth;
		for (const int number : game.holderOf(holder).items)
		{
			const Object* item = itemAt(number);
			if (!item || !game.itemOf(*item).faceUp || game.itemOf(*item).worths.empty()) { continue; }
			if (byVariable)
			{
				const auto value = item->variable.find(counted);
				total += value == item->variable.end() ? 0.0f : value->second;
				continue;
			}
			total += game.itemOf(*item).worths.front();
			shown.push_back(item);
		}
		for (const Object* item : shown)
		{
			for (auto higher = game.itemOf(*item).worths.rbegin(); higher != game.itemOf(*item).worths.rend(); ++higher)
			{
				const float more = *higher - game.itemOf(*item).worths.front();
				if (more > 0.0f && (game.holderOf(holder).pointsMost <= 0.0f || total + more <= game.holderOf(holder).pointsMost))
				{
					total += more;
					break;
				}
			}
		}
		game.holderOf(holder).points = total;
		if (!counted.empty()) { game.setVariable(holder.name, counted, total); }
		game.variableChanged(holder.name, words::cards, game.holderOf(holder).itemCount);
		game.variableChanged(holder.name, words::held, game.holderOf(holder).itemCount);
		game.variableChanged(holder.name, words::points, game.holderOf(holder).points);

		// A holder with <melds>: laid out in its groups, and its deadwood.
		if (!game.holderOf(holder).melds.empty() && std::all_of(game.holderOf(holder).items.begin(), game.holderOf(holder).items.end(), [&](int number) { return itemAt(number) != nullptr; })) { layOutMelds(holder); }

		// A hand's own holder in a game with a trick: the tricks it should take.
		if (const Object* trick = trickHolder(); trick && trick != &holder)
		{
			const bool owned = std::any_of(game.getCurrentObjects().begin(), game.getCurrentObjects().end(), [&](const Object& hand) { return game.isHand(hand) && !game.handOf(hand).own.empty() && holderNamed(game.handOf(hand).own) == &holder; });
			if (owned)
			{
				game.holderOf(holder).tricks = reckonTricks(holder, game.holderOf(*trick).takes);
				game.variableChanged(holder.name, words::tricks, game.holderOf(holder).tricks);
			}
		}

		// A trick: the suit led, and who laid the item winning it.
		if (game.holderOf(holder).takes.follow)
		{
			const Object* lead = game.holderOf(holder).items.empty() ? nullptr : itemAt(game.holderOf(holder).items.front());
			const Object* best = trickWinning(holder);
			game.holderOf(holder).lead = lead ? static_cast<float>(followed(game.holderOf(holder).takes, *lead)) : 0.0f;
			game.holderOf(holder).winner = best ? static_cast<float>(game.itemOf(*best).laidBy) : 0.0f;
			game.variableChanged(holder.name, words::lead, game.holderOf(holder).lead);
			game.variableChanged(holder.name, words::winner, game.holderOf(holder).winner);
		}

		// Scored holders, this one and those that score its items with theirs,
		// are scored again before the conditions are asked (scoreDueHolders):
		// once, however many items came and went this frame.
		if (!game.holderOf(holder).score.empty()) { game.holderOf(holder).scoreDue = scoresDue = true; }
		for (Object& other : game.getCurrentObjects())
		{
			if (&other != &holder && game.isHolder(other) && !game.holderOf(other).score.empty() && !game.holderOf(other).scoreWith.empty() && holderNamed(game.holderOf(other).scoreWith) == &holder)
			{
				game.holderOf(other).scoreDue = scoresDue = true;
			}
		}
	}

	void Holders::scoreDueHolders()
	{
		if (!scoresDue) { return; }
		scoresDue = false;
		for (Object& object : game.getCurrentObjects())
		{
			if (game.isHolder(object) && game.holderOf(object).scoreDue)
			{
				game.holderOf(object).scoreDue = false;
				scoreHolder(object);
			}
		}
	}

	namespace
	{
		// What a set's <alike> parts compare: each variable once, at most
		// four (Game checks), and each part's place among them (-1: a run).
		struct Compared
		{
			std::vector<Name> by;
			std::vector<std::vector<int>> parts;   // a list for each pattern, a place for each part
		};

		Compared comparedOf(const Patterns& set)
		{
			Compared compared;
			for (const Patterns::Pattern& pattern : set.patterns)
			{
				std::vector<int>& places = compared.parts.emplace_back();
				for (std::size_t i = 0; i < pattern.parts.size(); ++i)
				{
					if (pattern.parts[i].first != Patterns::Part::Alike) { places.push_back(-1); continue; }
					const Name by = pattern.by[i];
					auto at = std::find(compared.by.begin(), compared.by.end(), by);
					if (at == compared.by.end()) { at = compared.by.insert(compared.by.end(), by); }
					places.push_back(static_cast<int>(at - compared.by.begin()));
				}
			}
			return compared;
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
		int bestPattern(const Patterns& set, const Compared& compared, const std::vector<Scored>& items, std::vector<float>& deciding, int only = -1)
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
				for (std::size_t s = 0; s < compared.by.size(); ++s) { ++of[s][static_cast<std::size_t>(item.values[s])]; }
				if (!compared.by.empty())
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
				if (compared.by.empty())
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
				const Patterns::Pattern& pattern = set.patterns[index];
				const std::vector<int>& places = compared.parts[index];
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
					if (part == Patterns::Part::Alike && many > static_cast<int>(items.size())) { holds = false; }
					else if (part == Patterns::Part::Run) { longestRun = std::max(longestRun, many); }
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
		float bestOf(const Patterns& set, const Compared& compared, const std::vector<Scored>& items, std::size_t best, float base, int& made)
		{
			const std::size_t size = best == 0 ? items.size() : std::min(best, items.size());
			// Room reused call after call: the odds call this thousands of times.
			thread_local std::vector<float> deciding;
			thread_local std::vector<Scored> some;
			thread_local std::vector<char> chosen;
			// What all the items make is the most any `size` of them can
			// (every part asks for at least so many), so that is tried
			// first, and only when none of them makes it, every pattern.
			made = bestPattern(set, compared, items, deciding);
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
					const int pattern = bestPattern(set, compared, some, deciding, onlyTheMost ? made : -1);
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

	void Holders::markOddsRead()
	{
		// Generous: a name that might mean the holder counts as reading it.
		const auto means = [](const Object& holder, const Name name)
		{
			if (name.empty()) { return false; }
			if (holder.named(name)) { return true; }
			const std::string tail = "." + name;
			return holder.name.size() > tail.size() && holder.name.str().compare(holder.name.size() - tail.size(), tail.size(), tail) == 0;
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
		for (Object& holder : game.getCurrentObjects())
		{
			if (!game.isHolder(holder) || game.holderOf(holder).score.empty()) { continue; }
			// Every expression worked out while loading (a condition's
			// value, a command's), by the names it reads.
			bool read = false;
			for (const auto& [text, program] : game.evaluator().compiled)
			{
				if (text.find("odds") == std::string::npos) { continue; }
				for (const std::string& name : game.evaluator().namesIn(text))
				{
					const std::size_t dot = name.rfind('.');
					read = read || (dot != std::string::npos && name.compare(dot + 1, std::string::npos, "odds") == 0 && means(holder, Name(name.substr(0, dot))));
				}
			}
			for (const Object& other : game.getCurrentObjects()) { read = read || game.showsVariable(other, holder.name, words::odds); }
			for (const State& state : game.getStates())
			{
				read = read || readIn(holder, state.conditions, readIn) || readIn(holder, state.computer, readIn);
				for (const Scene& scene : state.scenes) { read = read || readIn(holder, scene.conditions, readIn); }
			}
			game.holderOf(holder).oddsRead = read;
		}
	}

	void Holders::scoreHolder(Object& holder)
	{
		const Patterns* set = game.patternsNamed(game.holderOf(holder).score);
		if (!set) { return; }
		const Compared compared = comparedOf(*set);
		const auto scored = [&](const Object& item)
		{
			std::uint64_t bits = 0;
			for (const float worth : game.itemOf(item).worths)
			{
				if (worth < 0.0f || worth > 63.0f || worth != std::floor(worth)) { bits = 0; break; }
				bits |= std::uint64_t{ 1 } << static_cast<int>(worth);
			}
			Scored one{ {}, &game.itemOf(item).worths, bits };
			for (std::size_t s = 0; s < compared.by.size(); ++s) { one.values[s] = static_cast<int>(std::lround(valueOf(item, compared.by[s]))); }
			return one;
		};
		float base = 1.0f;
		for (const Object& object : game.getCurrentObjects())
		{
			if (!game.itemOf(object).is) { continue; }
			// The values compared are counted in room for 0 to 63 (bestPattern).
			const Scored one = scored(object);
			if (std::any_of(one.values.begin(), one.values.begin() + static_cast<std::ptrdiff_t>(compared.by.size()), [](int value) { return value < 0 || value >= 64; })) { return; }
			base = std::max(base, highest(one) + 1.0f);
		}

		std::vector<Scored> own;
		std::vector<int> seen;
		const auto add = [&](const Object& from)
		{
			for (const int number : game.holderOf(from).items)
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
		Object* shared = game.holderOf(holder).scoreWith.empty() ? nullptr : holderNamed(game.holderOf(holder).scoreWith);
		if (shared) { add(*shared); }
		const std::size_t best = static_cast<std::size_t>(game.holderOf(holder).scoreBest);

		int made = 0;
		game.holderOf(holder).strength = own.empty() ? 0.0f : bestOf(*set, compared, own, best, base, made);
		game.holderOf(holder).pattern = static_cast<float>(made);

		// The odds: as many unseen items as this holder holds, against it,
		// the shared holder made up to full from the unseen too; the same
		// each time for the same items (its own generator, seeded by them).
		game.holderOf(holder).odds = 0.0f;
		if (mine > 0 && game.holderOf(holder).oddsRead)
		{
			std::vector<Scored> unseen;
			for (const Object& object : game.getCurrentObjects())
			{
				if (game.itemOf(object).is && std::find(seen.begin(), seen.end(), game.itemOf(object).number) == seen.end()) { unseen.push_back(scored(object)); }
			}
			const std::size_t board = own.size() - mine;
			const std::size_t full = static_cast<std::size_t>(game.holderOf(holder).scoreFull);
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
					const float us = bestOf(*set, compared, ours, best, base, ignored);
					const float them = bestOf(*set, compared, theirs, best, base, ignored);
					won += us > them ? 1.0f : us == them ? 0.5f : 0.0f;
				}
				game.holderOf(holder).odds = std::round(won * 100.0f / static_cast<float>(trials));
			}
		}
		game.variableChanged(holder.name, words::pattern, game.holderOf(holder).pattern);
		game.variableChanged(holder.name, words::strength, game.holderOf(holder).strength);
		game.variableChanged(holder.name, words::odds, game.holderOf(holder).odds);
	}

	void Holders::refreshCall(Object& holder)
	{
		const Object* top = game.holderOf(holder).items.empty() ? nullptr : itemAt(game.holderOf(holder).items.back());
		game.holderOf(holder).calling = !game.holderOf(holder).takes.wildRules.empty() && top && isWild(game.holderOf(holder).takes, *top) && game.holderOf(holder).called == 0.0f ? 1.0f : 0.0f;
		game.variableChanged(holder.name, words::calling, game.holderOf(holder).calling);
		game.variableChanged(holder.name, words::called, game.holderOf(holder).called);
	}

	int Holders::takeTop(Object& holder)
	{
		if (game.holderOf(holder).items.empty()) { return 0; }
		const int number = game.holderOf(holder).items.back();
		game.holderOf(holder).items.pop_back();
		refreshHolder(holder);
		return number;
	}

	void Holders::showTop(Object& holder)
	{
		if (game.holderOf(holder).topSprite.empty() || game.holderOf(holder).items.empty()) { return; }
		if (Object* top = itemAt(game.holderOf(holder).items.back()); top && top->lookName() != game.holderOf(holder).topSprite) { showHeld(*top, game.holderOf(holder).topSprite); refreshHolder(holder); }
	}

	void Holders::sendStarts()
	{
		// Numbered, every held member in turn.
		int number = 0;
		for (Object& object : game.getCurrentObjects())
		{
			if (game.itemOf(object).is) { game.itemFor(object).number = ++number; }
		}
		itemIndex.clear();
		if (number == 0) { return; }

		for (Object& object : game.getCurrentObjects())
		{
			if (game.isHolder(object)) { game.holderOf(object).items.clear(); game.holderOf(object).broken.clear(); game.holderOf(object).called = 0.0f; refreshHolder(object); }
			if (game.itemOf(object).is) { game.itemFor(object).laidBy = 0; }
			if (game.isHand(object))
			{
				game.handOf(object).held.clear();
				game.handOf(object).from.clear();
				game.handOf(object).holder.clear();
				game.handOf(object).at = -1;
			}
		}

		// Each group's held members, in random order, into the holder it starts in.
		std::map<Name, std::vector<int>> starts;
		std::vector<Name> order;
		for (Object& object : game.getCurrentObjects())
		{
			if (!game.itemOf(object).is) { continue; }
			if (!starts.count(game.itemOf(object).startHolder)) { order.push_back(game.itemOf(object).startHolder); }
			starts[game.itemOf(object).startHolder].push_back(game.itemOf(object).number);
		}
		for (const Name name : order)
		{
			Object* holder = holderNamed(name);
			if (!holder) { throw std::runtime_error("<starts> > <send into=\"" + name + "\">: there is no holder called '" + name + "'"); }
			std::vector<int>& items = starts[name];
			// In the pack's order, the first on top, unless an <order> shuffles it.
			if (!game.holderOf(*holder).startShuffled) { std::reverse(items.begin(), items.end()); }
			for (std::size_t i = game.holderOf(*holder).startShuffled ? items.size() : 0; i > 1; --i)
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
			for (Object& object : game.getCurrentObjects())
			{
				if (!game.isHolder(object) || &object == holder) { continue; }
				int count = game.holderOf(object).startCount;
				if (!object.groupName.empty())
				{
					count += game.holderOf(object).startMore * inGroup[object.groupName];
					++inGroup[object.groupName];
				}
				if (count > 0) { wanting.emplace_back(&object, count); }
			}
			for (int round = 1; !wanting.empty() && !game.holderOf(*holder).items.empty(); ++round)
			{
				bool any = false;
				for (auto& [each, count] : wanting)
				{
					if (count < round || game.holderOf(*holder).items.empty()) { continue; }
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
		for (Object& object : game.getCurrentObjects())
		{
			if (game.isHolder(object) && game.holderOf(object).calling > 0.0f)
			{
				const Object* top = itemAt(game.holderOf(object).items.back());
				game.holderOf(object).called = valueOf(*top, game.holderOf(object).takes.wildVariable);
				refreshCall(object);
			}
		}

		// Each hand at its own holder, or else the first holder.
		for (Object& object : game.getCurrentObjects())
		{
			if (!game.isHand(object)) { continue; }
			Object* own = game.handOf(object).own.empty() ? nullptr : holderNamed(game.handOf(object).own);
			for (Object& holder : game.getCurrentObjects())
			{
				if (!game.isHolder(holder) || (own && &holder != own)) { continue; }
				game.handOf(object).holder = holder.name;
				game.handOf(object).at = static_cast<int>(game.holderOf(holder).items.size()) - 1;
				break;
			}
		}
		layOutItems();
	}

	// Where each item of a holder goes: its place, then each item on from the
	// one below by its <spread>, or by <spread sprite> when it shows that look.
	Vector2f Holders::itemPlace(const Object& holder, std::size_t index)
	{
		const std::size_t count = game.holderOf(holder).items.size();
		const auto step = [&](std::size_t i)
		{
			const Object* item = itemAt(game.holderOf(holder).items[i]);
			const bool looked = item && !game.holderOf(holder).spreadSprite.empty() && item->lookName() == game.holderOf(holder).spreadSprite;
			return looked ? game.holderOf(holder).lookSpread : game.holderOf(holder).spread;
		};
		// A fan never wider than its <width>: the steps across shrink.
		float across = 1.0f;
		if (game.holderOf(holder).spreadWidth > 0.0f && count > 1)
		{
			float wide = 0.0f;
			for (std::size_t i = 0; i + 1 < count; ++i) { wide += std::fabs(step(i).x); }
			const Object* first = itemAt(game.holderOf(holder).items.front());
			const float room = game.holderOf(holder).spreadWidth - (first ? sizeOf(*first).x : 0.0f);
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

	void Holders::layOutItems()
	{
		int order = 1;
		for (Object& holder : game.getCurrentObjects())
		{
			if (!game.isHolder(holder)) { continue; }
			for (std::size_t i = 0; i < game.holderOf(holder).items.size(); ++i)
			{
				Object* item = itemAt(game.holderOf(holder).items[i]);
				if (!item) { continue; }
				item->position = itemPlace(holder, i);
				item->drawOrder = order++;
			}
		}

		for (Object& hand : game.getCurrentObjects())
		{
			if (!game.isHand(hand)) { continue; }
			Object* holder = game.handOf(hand).holder.empty() ? nullptr : holderNamed(game.handOf(hand).holder);
			// A hand pointing into its own holder, emptied then dealt to again:
			// at its first item.
			if (holder && game.handOf(hand).at < 0 && !game.holderOf(*holder).items.empty() && !game.handOf(hand).own.empty() && holderNamed(game.handOf(hand).own) == holder) { game.handOf(hand).at = 0; }
			Vector2f pointed = holder ? holder->position : hand.position;
			Vector2f size = holder ? sizeOf(*holder) : Vector2f{};
			if (holder && game.handOf(hand).at >= 0 && static_cast<std::size_t>(game.handOf(hand).at) < game.holderOf(*holder).items.size())
			{
				pointed = itemPlace(*holder, static_cast<std::size_t>(game.handOf(hand).at));
				if (Object* item = itemAt(game.holderOf(*holder).items[static_cast<std::size_t>(game.handOf(hand).at)]))
				{
					size = sizeOf(*item);
					// A holder with a <lift>: a face-up item pointed at is lifted
					// out of the fan, so all of it shows.
					if (game.itemOf(*item).faceUp && game.holderOf(*holder).lift != 0.0f)
					{
						pointed.y -= game.holderOf(*holder).lift;
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
			if (holder && !game.holderOf(*holder).items.empty()) { at = itemPlace(*holder, game.holderOf(*holder).items.size() - 1) + game.holderOf(*holder).spread + Vector2f{ 6.0f, 6.0f }; }
			if (game.handOf(hand).mouse && game.pickerOf(hand).pointer.x >= 0.0f && game.pickerOf(hand).pointer.y >= 0.0f) { at = game.pickerOf(hand).pointer - game.handOf(hand).grab; }
			for (const int number : game.handOf(hand).held)
			{
				Object* item = itemAt(number);
				if (!item) { continue; }
				item->position = at;
				item->drawOrder = 100000 + order++;
				at += holder ? game.holderOf(*holder).spread : Vector2f{ 0.0f, 20.0f };
				if (holder && game.holderOf(*holder).spread.x == 0.0f && game.holderOf(*holder).spread.y == 0.0f) { at += Vector2f{ 0.0f, 20.0f }; }
			}
			hand.drawOrder = 200000;
		}
	}

	// Whether `items` (the lowest first) may go on the holder, by its <accepts>.
	bool Holders::holderTakes(Object& holder, const std::vector<int>& items, const Object* from)
	{
		const HolderRule& rule = game.holderOf(holder).takes;
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
				for (const int number : game.holderOf(*from).items)
				{
					const Object* item = itemAt(number);
					if (item && number != game.itemOf(*first).number && test(*item)) { return true; }
				}
				return false;
			};
			if (game.holderOf(holder).items.empty())
			{
				const auto unbroken = [&](const Object& item)
				{
					return std::find(rule.breaks.begin(), rule.breaks.end(), followedName(rule, item)) != rule.breaks.end()
						&& std::find(game.holderOf(holder).broken.begin(), game.holderOf(holder).broken.end(), followed(rule, item)) == game.holderOf(holder).broken.end();
				};
				// A suit not yet broken leads only when the hand holds nothing else.
				if (unbroken(*first) && holds([&](const Object& item) { return !unbroken(item); })) { return false; }
			}
			else if (const Object* lead = itemAt(game.holderOf(holder).items.front()); lead && followed(rule, *first) != followed(rule, *lead))
			{
				if (holds([&](const Object& item) { return followed(rule, item) == followed(rule, *lead); })) { return false; }
			}
		}
		if (game.holderOf(holder).items.empty())
		{
			return acceptsBy(rule.emptyRules, *first, nullptr);
		}
		// Waiting for a suit to be called: nothing goes on it.
		if (game.holderOf(holder).calling > 0.0f) { return false; }
		if (isWild(rule, *first)) { return true; }
		const Object* top = itemAt(game.holderOf(holder).items.back());
		if (!top || !game.itemOf(*top).faceUp) { return false; }
		// <any>: one of them, a value called standing for the top's own.
		if (!rule.anyRules.empty())
		{
			const float called = rule.wildVariable.empty() ? 0.0f : game.holderOf(holder).called;
			const bool any = std::any_of(rule.anyRules.begin(), rule.anyRules.end(), [&](const HolderRule::VarRule& each) { return acceptsBy({ each }, *first, top, rule.wildVariable, called); });
			if (!any) { return false; }
		}
		return follows(rule, *top, *first);
	}

	// Whether `next` may lie on `below` by the rule (not counting <one />).
	bool Holders::follows(const HolderRule& rule, const Object& below, const Object& next)
	{
		return acceptsBy(rule.rules, next, &below);
	}

	// Whether `next` keeps an <accepts>'s rules on its variables, against
	// the one under it (`below`; none for the first on an empty holder).
	bool Holders::acceptsBy(const std::vector<HolderRule::VarRule>& rules, const Object& next, const Object* below, const std::string& called, float value)
	{
		using Rule = HolderRule::VarRule::Kind;
		const auto valueOf = [](const Object& of, const std::string& name)
		{
			const auto found = of.variable.findText(name);
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
	bool Holders::isCovered(const Object& item, const Object& own)
	{
		const Vector2f size = sizeOf(item);
		for (const Object& holder : game.getCurrentObjects())
		{
			if (!game.isHolder(holder) || &holder == &own || !game.isShown(holder)) { continue; }
			for (const int number : game.holderOf(holder).items)
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

	bool Holders::takeItems(Object& hand)
	{
		if (!game.isHand(hand) || (game.handOf(hand).mouse && !game.handOf(hand).over)) { return false; }
		Object* holder = holderNamed(game.handOf(hand).holder);
		if (!holder || game.holderOf(*holder).pickedSends > 0 || game.holderOf(*holder).items.empty()) { return false; }
		// Covering: a top item that an item of another holder, drawn over it,
		// overlaps stays where it is.
		if (const Object* top = itemAt(game.holderOf(*holder).items.back()); top && isCovered(*top, *holder)) { return false; }
		// Holding already: the top item of another holder too, while the hand
		// <holds> items from that many holders.
		if (!game.handOf(hand).held.empty())
		{
			const auto taken = [&](const Name name) { return name == game.handOf(hand).from || std::any_of(game.handOf(hand).also.begin(), game.handOf(hand).also.end(), [&](const auto& each) { return each.second == name; }); };
			if (static_cast<int>(game.handOf(hand).also.size()) + 1 >= game.handOf(hand).holds || taken(holder->name)) { return false; }
			const int number = game.holderOf(*holder).items.back();
			const Object* top = itemAt(number);
			if (!top || !game.itemOf(*top).faceUp) { return false; }
			game.holderOf(*holder).items.pop_back();
			refreshHolder(*holder);
			game.handOf(hand).held.push_back(number);
			game.handOf(hand).also.emplace_back(number, holder->name);
			layOutItems();
			return true;
		}
		const int count = static_cast<int>(game.holderOf(*holder).items.size());
		const int at = game.handOf(hand).at < 0 ? count - 1 : std::min(game.handOf(hand).at, count - 1);
		// A hand with a holder of its own takes from it alone, any one item,
		// face up or down (it knows its own items).
		if (!game.handOf(hand).own.empty())
		{
			if (holder != holderNamed(game.handOf(hand).own)) { return false; }
			game.handOf(hand).held = { game.holderOf(*holder).items[static_cast<std::size_t>(at)] };
			game.holderOf(*holder).items.erase(game.holderOf(*holder).items.begin() + at);
			refreshHolder(*holder);
			game.handOf(hand).from = holder->name;
			game.handOf(hand).at = std::min(at, static_cast<int>(game.holderOf(*holder).items.size()) - 1);
			if (game.handOf(hand).mouse && game.pickerOf(hand).pointer.x >= 0.0f && game.pickerOf(hand).pointer.y >= 0.0f)
			{
				game.handOf(hand).grab = game.pickerOf(hand).pointer - itemPlace(*holder, static_cast<std::size_t>(std::max(0, game.handOf(hand).at)));
			}
			layOutItems();
			return true;
		}
		if (at < count - 1 && !game.holderOf(*holder).carry) { return false; }
		// Face up, and each on the one below it by the holder's own rule.
		for (int i = at; i < count; ++i)
		{
			const Object* item = itemAt(game.holderOf(*holder).items[static_cast<std::size_t>(i)]);
			if (!item || !game.itemOf(*item).faceUp) { return false; }
			if (i > at && !follows(game.holderOf(*holder).takes, *itemAt(game.holderOf(*holder).items[static_cast<std::size_t>(i - 1)]), *item)) { return false; }
		}
		game.handOf(hand).held.assign(game.holderOf(*holder).items.begin() + at, game.holderOf(*holder).items.end());
		game.holderOf(*holder).items.resize(static_cast<std::size_t>(at));
		refreshHolder(*holder);
		game.handOf(hand).from = holder->name;
		game.handOf(hand).at = at - 1;
		// Held where the pointer took them.
		if (game.handOf(hand).mouse && game.pickerOf(hand).pointer.x >= 0.0f && game.pickerOf(hand).pointer.y >= 0.0f)
		{
			game.handOf(hand).grab = game.pickerOf(hand).pointer - itemPlace(*holder, static_cast<std::size_t>(at));
		}
		layOutItems();
		return true;
	}

	bool Holders::pickItems(Object& hand)
	{
		// From a holder with <picked>: it sends instead, and nothing is
		// picked up, so the picker's verbs after <pick /> do not run.
		const Object* holder = game.isHand(hand) ? holderNamed(game.handOf(hand).holder) : nullptr;
		if (holder && game.holderOf(*holder).pickedSends > 0)
		{
			const Name sound = game.holderOf(*holder).pickedSound;
			if (sendPicked(hand) && !sound.empty()) { game.requestSound(sound); }
			return false;
		}
		return takeItems(hand);
	}

	// The items held go on the holder, the top one pointed at, and the holder
	// they came from shows its top.
	void Holders::layHeld(Object& hand, Object& holder)
	{
		for (const int number : game.handOf(hand).held)
		{
			if (Object* item = itemAt(number)) { game.itemFor(*item).laidBy = game.handOf(hand).seat; }
			putInHolder(holder, number);
		}
		game.handOf(hand).held.clear();
		if (Object* from = holderNamed(game.handOf(hand).from); from && from != &holder) { showTop(*from); }
		game.handOf(hand).from.clear();
		for (const auto& [number, name] : game.handOf(hand).also)
		{
			if (Object* from = holderNamed(name); from && from != &holder) { showTop(*from); }
		}
		game.handOf(hand).also.clear();
		// A hand with a holder of its own stays pointing into it.
		if (Object* own = game.handOf(hand).own.empty() ? nullptr : holderNamed(game.handOf(hand).own))
		{
			game.handOf(hand).holder = own->name;
			game.handOf(hand).at = std::min(std::max(game.handOf(hand).at, 0), static_cast<int>(game.holderOf(*own).items.size()) - 1);
		}
		else
		{
			game.handOf(hand).holder = holder.name;
			game.handOf(hand).at = static_cast<int>(game.holderOf(holder).items.size()) - 1;
		}
		layOutItems();
	}

	bool Holders::layItems(Object& hand, Name onto)
	{
		if (!game.isHand(hand) || game.handOf(hand).held.empty()) { return false; }
		std::vector<Object*> choices;
		if (onto.empty())
		{
			// Let go over nothing: laid nowhere.
			if (game.handOf(hand).mouse && !game.handOf(hand).over) { return false; }
			if (Object* holder = holderNamed(game.handOf(hand).holder)) { choices.push_back(holder); }
		}
		else
		{
			const Name full = game.fullName(onto);
			for (Object& object : game.getCurrentObjects())
			{
				if (game.isHolder(object) && object.named(full) && game.isShown(object)) { choices.push_back(&object); }
			}
		}
		for (Object* holder : choices)
		{
			// Back where they came from: put back, always allowed.
			if (holder->name == game.handOf(hand).from && onto.empty())
			{
				layHeld(hand, *holder);
				return true;
			}
			if (holderTakes(*holder, game.handOf(hand).held, game.handOf(hand).own.empty() ? nullptr : holderNamed(game.handOf(hand).own)))
			{
				layHeld(hand, *holder);
				return true;
			}
		}
		return false;
	}

	bool Holders::dropItems(Object& hand)
	{
		if (!game.isHand(hand) || game.handOf(hand).held.empty()) { return false; }
		Object* from = holderNamed(game.handOf(hand).from);
		if (!from) { return false; }
		const Name pointed = game.handOf(hand).holder;
		const int at = game.handOf(hand).at;
		const int item = game.handOf(hand).held.front();
		// Items taken from other holders go back to theirs first.
		for (const auto& [number, name] : game.handOf(hand).also)
		{
			game.handOf(hand).held.erase(std::find(game.handOf(hand).held.begin(), game.handOf(hand).held.end(), number));
			if (Object* back = holderNamed(name)) { putInHolder(*back, number); }
		}
		game.handOf(hand).also.clear();
		layHeld(hand, *from);
		// Back in a hand's own holder: pointing at it again.
		if (!game.handOf(hand).own.empty() && from == holderNamed(game.handOf(hand).own))
		{
			const auto found = std::find(game.holderOf(*from).items.begin(), game.holderOf(*from).items.end(), item);
			game.handOf(hand).holder = from->name;
			game.handOf(hand).at = static_cast<int>(found - game.holderOf(*from).items.begin());
			layOutItems();
			return true;
		}
		// Put back, the hand stays where it was.
		game.handOf(hand).holder = pointed;
		game.handOf(hand).at = at;
		if (Object* holder = holderNamed(pointed)) { game.handOf(hand).at = std::min(at < 0 ? static_cast<int>(game.holderOf(*holder).items.size()) - 1 : at, static_cast<int>(game.holderOf(*holder).items.size()) - 1); }
		layOutItems();
		return true;
	}

	bool Holders::sendPicked(Object& hand, Name fromHolder, Name faces)
	{
		if (!game.isHand(hand) || !game.handOf(hand).held.empty() || (fromHolder.empty() && game.handOf(hand).mouse && !game.handOf(hand).over)) { return false; }
		// From the holder named, or else the one pointed at.
		Object* holder = holderNamed(fromHolder.empty() ? Name(game.handOf(hand).holder) : fromHolder);
		if (!holder || game.holderOf(*holder).pickedSends <= 0) { return false; }
		// Into the holder it names, or else the hand's own.
		const Name intoName = game.holderOf(*holder).pickedInto.empty() ? game.handOf(hand).own : game.holderOf(*holder).pickedInto;
		Object* into = intoName.empty() ? nullptr : holderNamed(intoName);
		if (!into) { return false; }
		if (!game.holderOf(*holder).items.empty())
		{
			for (int i = 0; i < game.holderOf(*holder).pickedSends && !game.holderOf(*holder).items.empty(); ++i)
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
			Object* from = game.holderOf(*holder).emptyFrom.empty() ? nullptr : holderNamed(game.holderOf(*holder).emptyFrom);
			const std::size_t leave = static_cast<std::size_t>(game.holderOf(*holder).emptyLeave);
			if (!from || game.holderOf(*from).items.size() <= leave) { return false; }
			const auto end = game.holderOf(*from).items.end() - static_cast<std::ptrdiff_t>(leave);
			std::vector<int> items(game.holderOf(*from).items.begin(), end);
			game.holderOf(*from).items.erase(game.holderOf(*from).items.begin(), end);
			refreshHolder(*from);
			if (game.holderOf(*holder).emptyRandom)
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
		if (fromHolder.empty()) { game.handOf(hand).at = static_cast<int>(game.holderOf(*holder).items.size()) - 1; }
		layOutItems();
		return true;
	}

	namespace
	{
		// Which way up an item showing that look is: its face (its first
		// look) up; another of its looks down; none of its looks, nothing.
		std::optional<bool> faceFor(const Object& item, const Name sprite)
		{
			for (std::size_t i = 0; i < item.looks.all().size(); ++i)
			{
				if (item.looks.all()[i].name == sprite) { return i == 0; }
			}
			return std::nullopt;
		}
	}

	bool Holders::sendHeld(Name fromName, Name intoName, int count, bool all, Name sprite)
	{
		Object* into = holderNamed(intoName);
		if (!into)
		{
			// A group's: each member's own holder in turn, an item a round.
			std::vector<std::pair<int, Name>> owners;
			for (const Object& object : game.getCurrentObjects())
			{
				if (game.isHand(object) && !game.handOf(object).own.empty() && (object.inGroup(intoName) || object.inGroup(game.fullName(intoName)))) { owners.emplace_back(game.handOf(object).seat, game.handOf(object).own); }
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
			if (from == into || game.holderOf(*from).items.empty()) { return false; }
			const std::size_t many = all ? game.holderOf(*from).items.size() : std::min(game.holderOf(*from).items.size(), static_cast<std::size_t>(count));
			// The top ones, the topmost first.
			for (std::size_t i = 0; i < many; ++i) { sent.push_back(takeTop(*from)); }
		}
		else
		{
			// A group's name: every member, wherever it is.
			if (!all) { return false; }
			for (Object& object : game.getCurrentObjects())
			{
				if (game.itemOf(object).is && (object.inGroup(fromName) || object.inGroup(game.fullName(fromName)))) { sent.push_back(game.itemOf(object).number); }
			}
			if (sent.empty()) { return false; }
			for (Object& object : game.getCurrentObjects())
			{
				if (game.isHolder(object))
				{
					auto& items = game.holderOf(object).items;
					const auto gone = std::remove_if(items.begin(), items.end(), [&](int number) { return std::find(sent.begin(), sent.end(), number) != sent.end(); });
					if (gone != items.end())
					{
						items.erase(gone, items.end());
						game.holderOf(object).broken.clear();
						game.holderOf(object).called = 0.0f;
						refreshHolder(object);
					}
				}
				if (game.isHand(object))
				{
					game.handOf(object).held.clear();
					game.handOf(object).from.clear();
				}
			}
			std::sort(sent.begin(), sent.end());
		}
		for (const int number : sent)
		{
			Object* item = itemAt(number);
			if (item) { game.itemFor(*item).laidBy = 0; }
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
		for (Object& object : game.getCurrentObjects())
		{
			if (!game.isHand(object)) { continue; }
			Object* pointed = holderNamed(game.handOf(object).holder);
			if (pointed && game.handOf(object).at >= static_cast<int>(game.holderOf(*pointed).items.size())) { game.handOf(object).at = static_cast<int>(game.holderOf(*pointed).items.size()) - 1; }
		}
		layOutItems();
		return true;
	}

	bool Holders::orderHeld(Name name)
	{
		Object* holder = holderNamed(name);
		if (!holder) { return false; }
		std::vector<int>& items = game.holderOf(*holder).items;
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

	bool Holders::turnHeld(Object& holder, Name sprite)
	{
		for (const int number : game.holderOf(holder).items)
		{
			Object* item = itemAt(number);
			if (!item) { continue; }
			const auto up = faceFor(*item, sprite);
			if (!up) { return false; }
			if (game.itemOf(*item).faceUp != *up) { turnItem(*item, *up); }
		}
		refreshHolder(holder);
		layOutItems();
		return true;
	}

	// Every holder in play, and its middle and size as drawn.
	std::vector<Object*> Holders::shownHolders()
	{
		std::vector<Object*> holders;
		for (Object& object : game.getCurrentObjects())
		{
			if (game.isHolder(object) && game.isShown(object)) { holders.push_back(&object); }
		}
		return holders;
	}

	bool Holders::hopHand(Object& hand, Direction direction)
	{
		Object* holder = holderNamed(game.handOf(hand).holder);
		const std::vector<Object*> holders = shownHolders();
		if (holders.empty()) { return false; }
		if (!holder) { holder = holders.front(); }
		const int count = static_cast<int>(game.holderOf(*holder).items.size());
		const bool own = !game.handOf(hand).own.empty() && holder == holderNamed(game.handOf(hand).own);
		const auto takeable = [&](int index)
		{
			if (index < 0 || index >= count) { return false; }
			const Object* item = itemAt(game.holderOf(*holder).items[static_cast<std::size_t>(index)]);
			return item && (game.itemOf(*item).faceUp || own);
		};
		const auto pointAt = [&](Object& to)
		{
			game.handOf(hand).holder = to.name;
			game.handOf(hand).at = static_cast<int>(game.holderOf(to).items.size()) - 1;
			game.handOf(hand).mouse = false;
			layOutItems();
			return true;
		};

		hand.facing = direction;
		// A holder fanned across: left and right go through its items first
		// (any of a hand's own, else a run's face-up ones).
		const bool fannedAcross = std::fabs(game.holderOf(*holder).spread.x) > std::fabs(game.holderOf(*holder).spread.y);
		if (fannedAcross && (direction == Direction::Left || direction == Direction::Right) && (own || game.holderOf(*holder).carry))
		{
			const int at = game.handOf(hand).at < 0 ? count - 1 : game.handOf(hand).at;
			const int next = at + ((direction == Direction::Right) == (game.holderOf(*holder).spread.x > 0.0f) ? 1 : -1);
			if (takeable(next) && takeable(at))
			{
				game.handOf(hand).at = next;
				game.handOf(hand).mouse = false;
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
		const int at = game.handOf(hand).at < 0 ? count - 1 : game.handOf(hand).at;
		if (!fannedAcross && game.holderOf(*holder).carry && up && takeable(at - 1) && takeable(at))
		{
			game.handOf(hand).at = at - 1;
			game.handOf(hand).mouse = false;
			layOutItems();
			return true;
		}
		if (!fannedAcross && game.holderOf(*holder).carry && !up && at < count - 1)
		{
			game.handOf(hand).at = at + 1;
			game.handOf(hand).mouse = false;
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

	void Holders::trackHand(Object& hand, bool horizontal, float at)
	{
		(horizontal ? game.pickerFor(hand).pointer.x : game.pickerFor(hand).pointer.y) = at;
		if (game.pickerOf(hand).pointer.x < 0.0f || game.pickerOf(hand).pointer.y < 0.0f) { return; }
		game.handOf(hand).mouse = true;
		game.handOf(hand).over = false;
		const Vector2f point = game.pickerOf(hand).pointer;
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
			for (int i = static_cast<int>(game.holderOf(*holder).items.size()) - 1; i >= 0 && !hit; --i)
			{
				const Object* item = itemAt(game.holderOf(*holder).items[static_cast<std::size_t>(i)]);
				if (item && inside(itemPlace(*holder, static_cast<std::size_t>(i)), sizeOf(*item)))
				{
					hit = true;
					index = i;
				}
			}
			if (!hit && inside(holder->position, sizeOf(*holder))) { hit = true; }
			if (!hit) { continue; }
			const Object* item = index >= 0 ? itemAt(game.holderOf(*holder).items[static_cast<std::size_t>(index)]) : nullptr;
			const int order = item ? item->drawOrder : 0;
			if (found && order <= foundOrder) { continue; }
			found = holder;
			foundAt = index;
			foundOrder = order;
		}
		if (found)
		{
			// Holding: the holder, its top item.
			if (!game.handOf(hand).held.empty() && game.handOf(hand).holds <= 1) { foundAt = static_cast<int>(game.holderOf(*found).items.size()) - 1; }
			game.handOf(hand).holder = found->name;
			game.handOf(hand).at = foundAt;
			game.handOf(hand).over = true;
		}
		layOutItems();
	}

	bool Holders::callValue(Object& hand, Name name)
	{
		if (!game.isHand(hand) || !game.handOf(hand).held.empty() || game.handOf(hand).own.empty() || (game.handOf(hand).mouse && !game.handOf(hand).over)) { return false; }
		Object* holder = holderNamed(name);
		Object* own = holderNamed(game.handOf(hand).own);
		if (!holder || !own || game.holderOf(*holder).calling <= 0.0f || holderNamed(game.handOf(hand).holder) != own) { return false; }
		const int count = static_cast<int>(game.holderOf(*own).items.size());
		if (count == 0) { return false; }
		const int at = game.handOf(hand).at < 0 ? count - 1 : std::min(game.handOf(hand).at, count - 1);
		const Object* item = itemAt(game.holderOf(*own).items[static_cast<std::size_t>(at)]);
		if (!item) { return false; }
		// The value named: the item's <wild variable=>.
		game.holderOf(*holder).called = valueOf(*item, game.holderOf(*holder).takes.wildVariable);
		refreshCall(*holder);
		return true;
	}

	// A computer playing a card game, a step each time it is due: point the
	// hand at what it will do, then press. Calling, the suit it holds most
	// of; else the item that goes somewhere, a wild one last, from the suit
	// it holds most of and the highest; else a draw from the stock.
	int Holders::playToTrick(Object& trick, const Object& hand, const Object& own, bool avoid)
	{
		int players = 0;
		for (const Object& object : game.getCurrentObjects())
		{
			if (game.isHand(object) && game.handOf(object).seat > 0 && game.isShown(object)) { ++players; }
		}
		const bool last = static_cast<int>(game.holderOf(trick).items.size()) + 1 >= std::max(players, 2);
		const Object* winning = trickWinning(trick);
		const auto worth = [this](const Object& item) { return game.itemOf(item).worths.empty() ? 0.0f : game.itemOf(item).worths.back(); };

		std::vector<std::pair<int, const Object*>> legal;
		for (int i = 0; i < static_cast<int>(game.holderOf(own).items.size()); ++i)
		{
			const int number = game.holderOf(own).items[static_cast<std::size_t>(i)];
			const Object* item = itemAt(number);
			if (item && holderTakes(trick, { number }, &own)) { legal.emplace_back(i, item); }
		}
		if (legal.empty()) { return -1; }
		const Object* led = game.holderOf(trick).items.empty() ? nullptr : itemAt(game.holderOf(trick).items.front());
		const bool following = led && std::any_of(legal.begin(), legal.end(), [&](const auto& each) { return followed(game.holderOf(trick).takes, *each.second) == followed(game.holderOf(trick).takes, *led); });
		const bool ducking = following && std::any_of(legal.begin(), legal.end(), [&](const auto& each) { return worth(*each.second) < worth(*winning); });
		const HolderRule& rule = game.holderOf(trick).takes;
		float top = 0.0f;
		for (const Object& object : game.getCurrentObjects())
		{
			if (game.itemOf(object).is && !game.itemOf(object).worths.empty()) { top = std::max(top, game.itemOf(object).worths.back()); }
		}
		// A partner (another hand on its <team>) winning safely: it is the
		// last item, or the partner's is the highest or next of its suit.
		bool partnerWins = false;
		if (winning && game.handOf(hand).team > 0 && game.itemOf(*winning).laidBy != game.handOf(hand).seat)
		{
			for (const Object& other : game.getCurrentObjects())
			{
				if (game.isHand(other) && game.handOf(other).seat == game.itemOf(*winning).laidBy && game.handOf(other).team == game.handOf(hand).team) { partnerWins = last || worth(*winning) >= top - 1.0f || isTrump(rule, *winning); }
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

	std::optional<KeyCode> Holders::thinkHeld(const CmdThink& think, Object& hand, float seconds)
	{
		Object* own = game.handOf(hand).own.empty() ? nullptr : holderNamed(game.handOf(hand).own);
		if (!own || !game.handOf(hand).held.empty()) { return std::nullopt; }
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
			if (calledBy.empty()) { calledBy = game.holderOf(*each).takes.wildVariable; }
		}
		const auto kindOf = [&](const Object& item) { return calledBy.empty() ? 0 : static_cast<int>(std::lround(valueOf(item, calledBy))); };
		std::map<int, int> ofKind;
		for (const int number : game.holderOf(*own).items)
		{
			if (const Object* item = itemAt(number)) { ++ofKind[kindOf(*item)]; }
		}
		std::vector<Object*> targets;
		Object* stock = nullptr;
		for (Object* holder : shownHolders())
		{
			if (holder == own) { continue; }
			if (game.holderOf(*holder).takes.given) { targets.push_back(holder); }
			if (!stock && game.holderOf(*holder).pickedSends > 0) { stock = holder; }
		}

		Object* aim = nullptr;
		int index = -1;
		const bool calling = std::any_of(targets.begin(), targets.end(), [this](const Object* holder) { return game.holderOf(*holder).calling > 0.0f; });
		const auto trick = std::find_if(targets.begin(), targets.end(), [this](const Object* holder) { return game.holderOf(*holder).takes.follow; });
		if (!game.holderOf(*own).melds.empty())
		{
			// Melds: holding no more than its best, it draws, the face-up top
			// of a holder when that leaves less deadwood, else the face-down
			// one; holding more, it lets go of the item leaving the least
			// (the one worth most, of two alike).
			const int best = game.holderOf(*own).meldsBest > 0 ? game.holderOf(*own).meldsBest : static_cast<int>(game.holderOf(*own).items.size());
			if (static_cast<int>(game.holderOf(*own).items.size()) <= best)
			{
				Object* up = nullptr;
				Object* down = nullptr;
				for (Object* holder : shownHolders())
				{
					if (holder == own || game.holderOf(*holder).pickedSends <= 0 || game.holderOf(*holder).items.empty()) { continue; }
					const Object* top = itemAt(game.holderOf(*holder).items.back());
					if (top && game.itemOf(*top).faceUp) { up = up ? up : holder; }
					else { down = down ? down : holder; }
				}
				aim = down ? down : up;
				if (up && down)
				{
					std::vector<int> with = game.holderOf(*own).items;
					with.push_back(game.holderOf(*up).items.back());
					if (meldDeadwood(*own, with, best, nullptr) < meldDeadwood(*own, game.holderOf(*own).items, best, nullptr)) { aim = up; }
				}
				if (aim) { index = static_cast<int>(game.holderOf(*aim).items.size()) - 1; }
			}
			else
			{
				float least = 0.0f;
				float worth = 0.0f;
				for (int i = 0; i < static_cast<int>(game.holderOf(*own).items.size()); ++i)
				{
					std::vector<int> rest = game.holderOf(*own).items;
					rest.erase(rest.begin() + i);
					const float left = meldDeadwood(*own, rest, 0, nullptr);
					const float points = itemAt(game.holderOf(*own).items[static_cast<std::size_t>(i)]) ? itemPoints(*own, *itemAt(game.holderOf(*own).items[static_cast<std::size_t>(i)])) : 0.0f;
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
			for (int i = 0; i < static_cast<int>(game.holderOf(*own).items.size()); ++i)
			{
				const Object* item = itemAt(game.holderOf(*own).items[static_cast<std::size_t>(i)]);
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
			for (int i = 0; i < static_cast<int>(game.holderOf(*own).items.size()); ++i)
			{
				const int number = game.holderOf(*own).items[static_cast<std::size_t>(i)];
				const Object* item = itemAt(number);
				if (!item) { continue; }
				for (Object* holder : targets)
				{
					if (!holderTakes(*holder, { number }, own)) { continue; }
					const float worth = isWild(game.holderOf(*holder).takes, *item) ? 1.0f : 100.0f + static_cast<float>(ofKind[kindOf(*item)] * 20) + (game.itemOf(*item).worths.empty() ? 0.0f : game.itemOf(*item).worths.back());
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
				index = static_cast<int>(game.holderOf(*stock).items.size()) - 1;
			}
		}
		if (!aim) { return std::nullopt; }

		itemWait = kItemPace;
		if (game.handOf(hand).holder != aim->name || game.handOf(hand).at != index)
		{
			game.handOf(hand).holder = aim->name;
			game.handOf(hand).at = index;
			game.handOf(hand).mouse = false;
			layOutItems();
			return std::nullopt;
		}
		return think.button;
	}
}
