// holders.cpp
// XML Game Engine
// author: beefviper
// date: Nov 1, 2026
//
// Cards: the members of a held class and the holders that hold them
// (settleHolders), and the game's holders and hands (Game): the deal at the
// start, a picker pointing at holders and picking and placing cards by
// each holder's rules, and where every card is drawn.

#include "game/holders.h"
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
		// A card's variable of that name (0: it has none).
		float valueOf(const Object& card, const std::string& variable)
		{
			const auto value = card.variable.find(variable);
			return value == card.variable.end() ? 0.0f : value->second;
		}

		// What a trick follows on a card: its <trick variable>, as a whole
		// number; and the same as text, as <breaks> and <trumps> are written.
		int followed(const HolderRule& rule, const Object& card) { return static_cast<int>(std::lround(valueOf(card, rule.trickBy))); }
		std::string followedName(const HolderRule& rule, const Object& card) { return std::to_string(followed(rule, card)); }
	}

	// ===================================================================
	// The game's holders and hands
	// ===================================================================

	void settleHolders(GameData& data)
	{
		std::vector<RawObject>& objects = data.objects;

		// The classes held, and their members: cards.
		std::vector<std::string> held;
		for (const RawObject& object : objects)
		{
			if (!object.holder.holdsClass.empty() && std::find(held.begin(), held.end(), object.holder.holdsClass) == held.end()) { held.push_back(object.holder.holdsClass); }
		}
		if (held.empty()) { return; }

		std::map<std::string, const RawObject*> sample;  // a member of each class held
		for (RawObject& object : objects)
		{
			if (object.objClass.empty() || std::find(held.begin(), held.end(), object.objClass) == held.end()) { continue; }
			const std::string where = "object '" + object.name + "' (of class " + object.objClass + ", which a <holds> holds)";
			if (object.groupName.empty() || object.cell.column < 1) { throw std::runtime_error(where + ": is a member of a <group> laid out in <columns> and <rows>, its row and column what sort of card it is"); }
			if (object.looks.size() < 2) { throw std::runtime_error(where + ": has two looks, its face first and its back"); }
			object.held = true;
			float worth = static_cast<float>(object.cell.column);
			if (const auto given = object.variable.find("worth"); given != object.variable.end())
			{
				try { worth = std::stof(given->second.text); }
				catch (const std::exception&) { throw std::runtime_error(where + ": its worth is \"" + given->second.text + "\"; expected a number"); }
			}
			object.card.worths = { worth };
			// `high`: what it may count instead, where a holder counts within a limit (an ace's 11).
			if (const auto high = object.variable.find("high"); high != object.variable.end())
			{
				float more = 0.0f;
				try { more = std::stof(high->second.text); }
				catch (const std::exception&) { throw std::runtime_error(where + ": its high is \"" + high->second.text + "\"; expected a number"); }
				if (more != worth) { object.card.worths.push_back(more); }
				std::sort(object.card.worths.begin(), object.card.worths.end());
			}
			// Its row and column, and the same under the names its group gives them (suit, rank).
			for (const auto& [variable, number] : { std::pair{ std::string("row"), object.cell.row }, std::pair{ std::string("column"), object.cell.column },
				std::pair{ object.cell.rowName, object.cell.row }, std::pair{ object.cell.columnName, object.cell.column } })
			{
				if (variable.empty()) { continue; }
				if (variable == "worth" || variable == "high" || variable == "color") { throw std::runtime_error(where + ": its group names its rows or columns '" + variable + "', which a card has of its own"); }
				object.variable[variable] = RawValue::expression(std::to_string(number));
			}
			object.variable["worth"] = RawValue::expression(formatDisplayNumber(worth));
			object.sprite = object.looks.front();
			sample.emplace(object.objClass, &object);
		}

		// Each holder: the looks its words name are ones its members have.
		for (RawObject& object : objects)
		{
			RawObject::RawHolder& holder = object.holder;
			if (holder.holdsClass.empty()) { continue; }
			const std::string where = "object '" + object.name + "' > <holds class=\"" + holder.holdsClass + "\">";
			const auto member = sample.find(holder.holdsClass);
			if (member == sample.end()) { throw std::runtime_error(where + ": nothing is of class " + holder.holdsClass); }
			const std::vector<RawSprite>& looks = member->second->looks;
			const std::string face = looks[0].name;
			const auto known = [&](const std::string& look, const std::string& part)
			{
				if (std::none_of(looks.begin(), looks.end(), [&](const RawSprite& each) { return each.name == look; }))
				{
					throw std::runtime_error(where + ": " + part + " sprite=\"" + look + "\", a look the members of class " + holder.holdsClass + " have not (they have '" + face + "' and '" + looks[1].name + "')");
				}
			};
			if (!holder.holdsSprite.empty()) { known(holder.holdsSprite, "its"); }
			if (!holder.topSprite.empty()) { known(holder.topSprite, "<top>"); }
			if (!holder.spreadSprite.empty()) { known(holder.spreadSprite, "<spread>"); }
		}

		// A picker of a class held is a hand of cards.
		for (RawObject& object : objects)
		{
			for (const auto& [action, commands] : object.action)
			{
				for (const RawCommand& command : commands)
				{
					if (command.verb == "pick" && std::find(held.begin(), held.end(), command.objClass) != held.end())
					{
						object.hand = true;
						if (command.most > 0) { object.handHolds = command.most; }
					}
				}
			}
		}

		// Each state's <starts>: the deal.
		const auto holderFor = [&](const std::string& name, const std::string& state) -> std::vector<RawObject*>
		{
			std::vector<RawObject*> found;
			for (RawObject& object : objects)
			{
				if (object.holder.is && object.ownerState == state && (object.named(state + "." + name) || object.named(name))) { found.push_back(&object); }
			}
			return found;
		};
		for (RawState& state : data.states)
		{
			std::string start;   // the holder every card starts in
			for (const RawStart& step : state.starts)
			{
				const std::string where = "state '" + state.name + "' > <starts> > <" + step.verb + ">";
				if (step.verb == "send" && step.all)
				{
					bool any = false;
					for (RawObject& object : objects)
					{
						if (object.held && object.ownerState == state.name && (object.named(state.name + "." + step.from) || object.named(step.from))) { object.card.startHolder = step.into; any = true; }
					}
					if (!any) { throw std::runtime_error(where + ": from=\"" + step.from + "\" names no held members of this state"); }
					if (holderFor(step.into, state.name).size() != 1) { throw std::runtime_error(where + ": into=\"" + step.into + "\" names no one holder of this state"); }
					start = step.into;
					continue;
				}
				if (start.empty() || step.from != start)
				{
					throw std::runtime_error(where + ": for now a start sends <all /> of a group into one holder first, then orders and sends from that holder");
				}
				if (step.verb == "order") { continue; }   // the cards are always shuffled where they start
				std::vector<RawObject*> into = holderFor(step.into, state.name);
				// A group's: each member's own holder.
				if (into.empty())
				{
					for (const RawObject& object : objects)
					{
						if (object.ownerState != state.name || object.handHolder.empty() || (object.groupName != step.into && object.groupName != state.name + "." + step.into)) { continue; }
						for (RawObject* own : holderFor(object.handHolder, state.name)) { into.push_back(own); }
					}
				}
				if (into.empty()) { throw std::runtime_error(where + ": into=\"" + step.into + "\" names no holder of this state"); }
				for (RawObject* each : into)
				{
					each->holder.startCount = RawValue::expression(std::to_string(step.count));
					if (step.more != 0) { each->holder.startMore = RawValue::expression(std::to_string(step.more)); }
				}
			}
		}
		for (const RawObject& object : objects)
		{
			if (object.held && object.card.startHolder.empty())
			{
				throw std::runtime_error("object '" + object.name + "': of a held class, but no state's <starts> sends it into a holder");
			}
		}
	}

	Object* Game::cardAt(int number)
	{
		if (number <= 0) { return nullptr; }
		const std::size_t at = static_cast<std::size_t>(number - 1);
		if (at < cardIndex.size() && cardIndex[at] < objects.size() && objects[cardIndex[at]].cardNumber == number) { return &objects[cardIndex[at]]; }
		// Objects moved (a copy taken away): found again.
		cardIndex.clear();
		for (std::size_t i = 0; i < objects.size(); ++i)
		{
			if (objects[i].cardNumber <= 0) { continue; }
			const std::size_t slot = static_cast<std::size_t>(objects[i].cardNumber - 1);
			if (cardIndex.size() <= slot) { cardIndex.resize(slot + 1, objects.size()); }
			cardIndex[slot] = i;
		}
		return at < cardIndex.size() && cardIndex[at] < objects.size() ? &objects[cardIndex[at]] : nullptr;
	}

	Object* Game::holderNamed(const std::string& name)
	{
		const std::string full = fullName(name);
		for (Object& object : objects)
		{
			if (object.holder.is && object.name == full) { return &object; }
		}
		return nullptr;
	}

	void Game::showHeld(Object& card, const std::string& look)
	{
		const auto found = std::find_if(card.looks.begin(), card.looks.end(), [&](const Object::Look& each) { return each.name == look; });
		if (found != card.looks.end()) { turnCard(card, found == card.looks.begin()); }
	}

	void Game::turnCard(Object& card, bool up)
	{
		card.faceUp = up;
		const std::size_t look = up ? 0 : 1;
		if (card.look != look && look < card.looks.size()) { card.showLook(look); }
	}

	void Game::putInHolder(Object& holder, int number)
	{
		Object* card = cardAt(number);
		if (!card) { return; }
		if (!holder.holder.holdsSprite.empty()) { showHeld(*card, holder.holder.holdsSprite); }
		auto at = holder.holder.cards.end();
		if (!holder.holder.orderBy.empty())
		{
			// <order>: by each variable named, the first first.
			const auto order = [&](const Object& of)
			{
				std::vector<float> values;
				for (const std::string& name : holder.holder.orderBy)
				{
					const auto found = of.variable.find(name);
					values.push_back(found == of.variable.end() ? 0.0f : found->second);
				}
				return values;
			};
			const std::vector<float> mine = order(*card);
			at = std::find_if(holder.holder.cards.begin(), holder.holder.cards.end(), [&](int other)
				{
					const Object* each = cardAt(other);
					return each && mine < order(*each);
				});
		}
		holder.holder.cards.insert(at, number);
		// A suit that <breaks>, broken.
		const std::vector<std::string>& breaks = holder.holder.takes.breaks;
		if (std::find(breaks.begin(), breaks.end(), followedName(holder.holder.takes, *card)) != breaks.end()
			&& std::find(holder.holder.broken.begin(), holder.holder.broken.end(), followed(holder.holder.takes, *card)) == holder.holder.broken.end())
		{
			holder.holder.broken.push_back(followed(holder.holder.takes, *card));
		}
		// A card laid answers any call.
		holder.called = 0.0f;
		refreshHolder(holder);
	}

	float Game::cardPoints(const Object& holder, const Object& card)
	{
		if (!holder.holder.counts.empty() && holder.holder.counts != "worth")
		{
			const auto value = card.variable.find(holder.holder.counts);
			return value == card.variable.end() ? 0.0f : value->second;
		}
		return card.card.worths.empty() ? 0.0f : card.card.worths.front();
	}

	// Whether these cards, every one of them, make the pattern: each part
	// holds for all of them (<alike>: one value of its variable, <run>:
	// values in a row, by worth a card counting any of its worths), and
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
				for (const Object* card : group)
				{
					if (valueOf(*card, by) != valueOf(*group.front(), by)) { return false; }
				}
				continue;
			}
			// A run: some start from which each value in turn is a different card's.
			bool found = false;
			for (const Object* starter : group)
			{
				// By worth, a card counting any of its worths; else the variable's value.
				const auto valuesOf = [&](const Object& card) { return by == "worth" ? card.card.worths : std::vector<float>{ valueOf(card, by) }; };
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

	// The fewest points left in no group, these cards (or the best `best`
	// of them) laid out in groups each making one of the holder's <melds>
	// patterns; `melds`, when asked, gets the groups (indexes into
	// `numbers`). Each card counts its points by the holder's <points>.
	float Game::meldDeadwood(const Object& holder, const std::vector<int>& numbers, int best, std::vector<std::vector<int>>* melds)
	{
		std::vector<const Object*> cards;
		for (const int number : numbers)
		{
			if (const Object* card = cardAt(number)) { cards.push_back(card); }
		}
		const std::size_t n = cards.size();
		std::vector<float> value(n);
		for (std::size_t i = 0; i < n; ++i) { value[i] = cardPoints(holder, *cards[i]); }
		const auto set = std::find_if(patterns.begin(), patterns.end(), [&](const RawPatterns& each) { return each.name == holder.holder.melds; });
		if (set == patterns.end() || n > 16 || n != numbers.size())
		{
			float total = 0.0f;
			for (const float each : value) { total += each; }
			return total;
		}

		// Every group of the cards that makes a pattern, by its lowest card.
		const std::uint32_t all = (std::uint32_t{ 1 } << n) - 1;
		std::vector<std::vector<std::uint32_t>> groups(n);
		std::vector<const Object*> group;
		for (std::uint32_t mask = 1; mask <= all; ++mask)
		{
			if (std::popcount(mask) < 2) { continue; }
			group.clear();
			for (std::size_t i = 0; i < n; ++i)
			{
				if (mask & (std::uint32_t{ 1 } << i)) { group.push_back(cards[i]); }
			}
			if (std::any_of(set->patterns.begin(), set->patterns.end(), [&](const RawPatterns::Pattern& pattern) { return meldFits(group, pattern); }))
			{
				groups[static_cast<std::size_t>(std::countr_zero(mask))].push_back(mask);
			}
		}
		// The least left over by each set of the cards: its lowest card left
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

	// A holder with <melds>: its groups first, each lowest card first, the
	// rest after them in suit and rank order; `deadwood` from its best.
	void Game::layOutMelds(Object& holder)
	{
		std::vector<std::vector<int>> melds;
		const std::vector<int> cards = holder.holder.cards;
		holder.deadwood = meldDeadwood(holder, cards, holder.holder.meldsBest, nullptr);
		meldDeadwood(holder, cards, 0, &melds);
		std::vector<int> order;
		std::vector<bool> placed(cards.size(), false);
		const auto byWorth = [&](int a, int b)
		{
			const Object& x = *cardAt(cards[static_cast<std::size_t>(a)]);
			const Object& y = *cardAt(cards[static_cast<std::size_t>(b)]);
			return x.cardNumber < y.cardNumber;
		};
		for (auto& meld : melds)
		{
			std::sort(meld.begin(), meld.end(), byWorth);
			for (const int i : meld)
			{
				order.push_back(cards[static_cast<std::size_t>(i)]);
				placed[static_cast<std::size_t>(i)] = true;
			}
		}
		std::vector<int> rest;
		for (std::size_t i = 0; i < cards.size(); ++i)
		{
			if (!placed[i]) { rest.push_back(static_cast<int>(i)); }
		}
		std::sort(rest.begin(), rest.end(), byWorth);
		for (const int i : rest) { order.push_back(cards[static_cast<std::size_t>(i)]); }
		holder.holder.cards = order;
		refreshBoundTexts(holder.name, "deadwood", holder.deadwood);
	}

	bool Game::isTrump(const HolderRule& rule, const Object& card)
	{
		return std::find(rule.trumps.begin(), rule.trumps.end(), followedName(rule, card)) != rule.trumps.end();
	}

	// Whether `card` laid now takes the trick from `best`: a trump over
	// any other card, else a higher card of the same suit.
	bool Game::beatsInTrick(const HolderRule& rule, const Object& card, const Object& best)
	{
		const bool trump = isTrump(rule, card);
		if (trump != isTrump(rule, best)) { return trump; }
		return followed(rule, card) == followed(rule, best) && card.card.worths.back() > best.card.worths.back();
	}

	const Object* Game::trickWinning(const Object& holder)
	{
		if (holder.holder.cards.empty()) { return nullptr; }
		const Object* best = cardAt(holder.holder.cards.front());
		for (const int number : holder.holder.cards)
		{
			const Object* card = cardAt(number);
			if (card && best && beatsInTrick(holder.holder.takes, *card, *best)) { best = card; }
		}
		return best;
	}

	// How many tricks a hand's cards should take, as a player reckons it
	// to bid: each side suit's ace, a king with another card of its suit,
	// half a queen with two more and a higher card; each trump honour with
	// as many trumps under it as there are above it, and each trump past
	// three; with trumps to spare, a missing suit one and a single card a
	// half. Rounded to the nearest.
	float Game::reckonTricks(const Object& own, const HolderRule& trick)
	{
		float top = 0.0f;
		std::vector<int> kinds;
		for (const Object& object : objects)
		{
			if (!object.held || object.card.worths.empty()) { continue; }
			top = std::max(top, object.card.worths.back());
			if (std::find(kinds.begin(), kinds.end(), followed(trick, object)) == kinds.end()) { kinds.push_back(followed(trick, object)); }
		}
		std::map<int, std::vector<float>> held;
		std::map<int, bool> trumped;
		int trumps = 0;
		for (const int number : own.holder.cards)
		{
			const Object* card = cardAt(number);
			if (!card || card->card.worths.empty()) { continue; }
			held[followed(trick, *card)].push_back(card->card.worths.back());
			if (isTrump(trick, *card))
			{
				trumped[followed(trick, *card)] = true;
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
		const auto found = std::find_if(objects.begin(), objects.end(), [](const Object& object) { return object.holder.is && object.holder.takes.follow; });
		return found == objects.end() ? nullptr : &*found;
	}

	bool Game::isWild(const HolderRule& rule, const Object& card)
	{
		return !rule.wildRules.empty() && acceptsBy(rule.wildRules, card, nullptr);
	}

	void Game::refreshHolder(Object& holder)
	{
		holder.cardCount = static_cast<float>(holder.holder.cards.size());
		refreshCall(holder);

		// Points: each face-up card at its lowest worth, then each that may
		// count higher raised as far as the total stays within the limit.
		float total = 0.0f;
		std::vector<const Object*> shown;
		// <counts variable>: a variable of each other than its worth, as it is.
		const std::string& counted = holder.holder.counts;
		const bool byVariable = !counted.empty() && counted != "worth";
		for (const int number : holder.holder.cards)
		{
			const Object* card = cardAt(number);
			if (!card || !card->faceUp || card->card.worths.empty()) { continue; }
			if (byVariable)
			{
				const auto value = card->variable.find(counted);
				total += value == card->variable.end() ? 0.0f : value->second;
				continue;
			}
			total += card->card.worths.front();
			shown.push_back(card);
		}
		for (const Object* card : shown)
		{
			for (auto higher = card->card.worths.rbegin(); higher != card->card.worths.rend(); ++higher)
			{
				const float more = *higher - card->card.worths.front();
				if (more > 0.0f && (holder.holder.pointsMost <= 0.0f || total + more <= holder.holder.pointsMost))
				{
					total += more;
					break;
				}
			}
		}
		holder.points = total;
		if (!counted.empty()) { setVariable(holder.name, counted, total); }
		refreshBoundTexts(holder.name, "cards", holder.cardCount);
		refreshBoundTexts(holder.name, "held", holder.cardCount);
		refreshBoundTexts(holder.name, "points", holder.points);

		// A holder with <melds>: laid out in its groups, and its deadwood.
		if (!holder.holder.melds.empty() && std::all_of(holder.holder.cards.begin(), holder.holder.cards.end(), [&](int number) { return cardAt(number) != nullptr; })) { layOutMelds(holder); }

		// A hand's own holder in a game with a trick: the tricks it should take.
		if (const Object* trick = trickHolder(); trick && trick != &holder)
		{
			const bool owned = std::any_of(objects.begin(), objects.end(), [&](const Object& hand) { return hand.hand.is && !hand.hand.own.empty() && holderNamed(hand.hand.own) == &holder; });
			if (owned)
			{
				holder.tricks = reckonTricks(holder, trick->holder.takes);
				refreshBoundTexts(holder.name, "tricks", holder.tricks);
			}
		}

		// A trick: the suit led, and who laid the card winning it.
		if (holder.holder.takes.follow)
		{
			const Object* lead = holder.holder.cards.empty() ? nullptr : cardAt(holder.holder.cards.front());
			const Object* best = trickWinning(holder);
			holder.lead = lead ? static_cast<float>(followed(holder.holder.takes, *lead)) : 0.0f;
			holder.winner = best ? static_cast<float>(best->laidBy) : 0.0f;
			refreshBoundTexts(holder.name, "lead", holder.lead);
			refreshBoundTexts(holder.name, "winner", holder.winner);
		}

		// Scored holders, this one and those that score its cards with theirs,
		// are scored again before the conditions are asked (scoreDueHolders):
		// once, however many cards came and went this frame.
		if (!holder.holder.score.empty()) { holder.holder.scoreDue = scoresDue = true; }
		for (Object& other : objects)
		{
			if (&other != &holder && other.holder.is && !other.holder.score.empty() && !other.holder.scoreWith.empty() && holderNamed(other.holder.scoreWith) == &holder)
			{
				other.holder.scoreDue = scoresDue = true;
			}
		}
	}

	void Game::scoreDueHolders()
	{
		if (!scoresDue) { return; }
		scoresDue = false;
		for (Object& object : objects)
		{
			if (object.holder.is && object.holder.scoreDue)
			{
				object.holder.scoreDue = false;
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

		// A card as patterns see it: the value of each variable its set's
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

		float highest(const Scored& card) { return card.worths->back(); }

		// Room reused by bestPattern, call after call.
		std::vector<float>& scratchWorths()
		{
			thread_local std::vector<float> worths;
			return worths;
		}

		// The highest pattern the cards make (from 1; 0 none), with what
		// ranks it among others making the same, deciding first: a run's
		// worths from its top down, else the cards in groups of the set's
		// first <alike> variable, bigger groups first, each group by worth;
		// then the rest by worth. With `only` (from 1), that pattern alone
		// is tried: -1 when the cards do not make it.
		int bestPattern(const RawPatterns& set, const Slots& slots, const std::vector<Scored>& cards, std::vector<float>& deciding, int only = -1)
		{
			// Counted in place (values are under 64; scoreHolder sees to
			// that), as this runs thousands of times for the odds.
			std::array<std::array<int, 64>, 4> of{};
			std::array<float, 64> groupWorth{};
			std::array<int, 64> groups{};
			std::size_t groupCount = 0;
			std::uint64_t mask = 0;
			bool byBits = true;
			for (const Scored& card : cards)
			{
				for (std::size_t s = 0; s < slots.by.size(); ++s) { ++of[s][static_cast<std::size_t>(card.values[s])]; }
				if (!slots.by.empty())
				{
					const auto group = static_cast<std::size_t>(card.values[0]);
					if (of[0][group] == 1) { groups[groupCount++] = card.values[0]; }
					groupWorth[group] = std::max(groupWorth[group], highest(card));
				}
				mask |= card.mask;
				byBits = byBits && card.mask != 0;
			}
			std::vector<float>& worths = scratchWorths();
			if (!byBits)
			{
				worths.clear();
				for (const Scored& card : cards) { worths.insert(worths.end(), card.worths->begin(), card.worths->end()); }
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
					for (const Scored& card : cards) { deciding.push_back(highest(card)); }
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
				for (int wanted = static_cast<int>(cards.size()); wanted > 0 && holds; --wanted)
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
					if (part == RawPatterns::Part::Alike && many > static_cast<int>(cards.size())) { holds = false; }
					else if (part == RawPatterns::Part::Run) { longestRun = std::max(longestRun, many); }
				}
				if (!holds) { continue; }
				const std::optional<float> top = longestRun > 0 ? runTop(longestRun) : std::nullopt;
				if (longestRun > 0 && !top) { continue; }

				if (top)
				{
					deciding.clear();
					std::vector<Scored> rest = cards;
					for (int step = 0; step < longestRun; ++step)
					{
						const float worth = *top - static_cast<float>(step);
						deciding.push_back(worth);
						rest.erase(std::find_if(rest.begin(), rest.end(), [&](const Scored& card)
						{
							return std::find(card.worths->begin(), card.worths->end(), worth) != card.worths->end();
						}));
					}
					std::vector<float> others;
					for (const Scored& card : rest) { others.push_back(highest(card)); }
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

		// The best `best` of the cards (0: all of them): the pattern they
		// make, and a number ordering any two the same size by the pattern,
		// then by what decides between two of it, in base `base` (one more
		// than any worth).
		float bestOf(const RawPatterns& set, const Slots& slots, const std::vector<Scored>& cards, std::size_t best, float base, int& made)
		{
			const std::size_t size = best == 0 ? cards.size() : std::min(best, cards.size());
			// Room reused call after call: the odds call this thousands of times.
			thread_local std::vector<float> deciding;
			thread_local std::vector<Scored> some;
			thread_local std::vector<char> chosen;
			// What all the cards make is the most any `size` of them can
			// (every part asks for at least so many), so that is tried
			// first, and only when none of them makes it, every pattern.
			made = bestPattern(set, slots, cards, deciding);
			if (size == cards.size())
			{
				float strength = static_cast<float>(made);
				for (const float worth : deciding) { strength = strength * base + worth; }
				return strength;
			}
			float strongest = -1.0f;
			for (const bool onlyTheMost : { true, false })
			{
				chosen.assign(cards.size(), 0);
				std::fill(chosen.begin(), chosen.begin() + static_cast<std::ptrdiff_t>(size), char{ 1 });
				int bestMade = 0;
				do
				{
					some.clear();
					for (std::size_t i = 0; i < cards.size(); ++i)
					{
						if (chosen[i]) { some.push_back(cards[i]); }
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
			if (holder.holder.score.empty()) { continue; }
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
			holder.oddsRead = read;
		}
	}

	void Game::scoreHolder(Object& holder)
	{
		const auto set = std::find_if(patterns.begin(), patterns.end(), [&](const RawPatterns& each) { return each.name == holder.holder.score; });
		if (set == patterns.end()) { return; }
		const Slots slots = slotsOf(*set);
		const auto scored = [&](const Object& card)
		{
			std::uint64_t bits = 0;
			for (const float worth : card.card.worths)
			{
				if (worth < 0.0f || worth > 63.0f || worth != std::floor(worth)) { bits = 0; break; }
				bits |= std::uint64_t{ 1 } << static_cast<int>(worth);
			}
			Scored one{ {}, &card.card.worths, bits };
			for (std::size_t s = 0; s < slots.by.size(); ++s) { one.values[s] = static_cast<int>(std::lround(valueOf(card, slots.by[s]))); }
			return one;
		};
		float base = 1.0f;
		for (const Object& object : objects)
		{
			if (!object.held) { continue; }
			// The values compared are counted in room for 0 to 63 (bestPattern).
			const Scored one = scored(object);
			if (std::any_of(one.values.begin(), one.values.begin() + static_cast<std::ptrdiff_t>(slots.by.size()), [](int value) { return value < 0 || value >= 64; })) { return; }
			base = std::max(base, highest(one) + 1.0f);
		}

		std::vector<Scored> own;
		std::vector<int> seen;
		const auto add = [&](const Object& from)
		{
			for (const int number : from.holder.cards)
			{
				if (const Object* card = cardAt(number))
				{
					own.push_back(scored(*card));
					seen.push_back(number);
				}
			}
		};
		add(holder);
		const std::size_t mine = own.size();
		Object* shared = holder.holder.scoreWith.empty() ? nullptr : holderNamed(holder.holder.scoreWith);
		if (shared) { add(*shared); }
		const std::size_t best = static_cast<std::size_t>(holder.holder.scoreBest);

		int made = 0;
		holder.strength = own.empty() ? 0.0f : bestOf(*set, slots, own, best, base, made);
		holder.pattern = static_cast<float>(made);

		// The odds: as many unseen cards as this holder holds, against it,
		// the shared holder made up to full from the unseen too; the same
		// each time for the same cards (its own generator, seeded by them).
		holder.odds = 0.0f;
		if (mine > 0 && holder.oddsRead)
		{
			std::vector<Scored> unseen;
			for (const Object& object : objects)
			{
				if (object.held && std::find(seen.begin(), seen.end(), object.cardNumber) == seen.end()) { unseen.push_back(scored(object)); }
			}
			const std::size_t board = own.size() - mine;
			const std::size_t full = static_cast<std::size_t>(holder.holder.scoreFull);
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
				holder.odds = std::round(won * 100.0f / static_cast<float>(trials));
			}
		}
		refreshBoundTexts(holder.name, "pattern", holder.pattern);
		refreshBoundTexts(holder.name, "strength", holder.strength);
		refreshBoundTexts(holder.name, "odds", holder.odds);
	}

	void Game::refreshCall(Object& holder)
	{
		const Object* top = holder.holder.cards.empty() ? nullptr : cardAt(holder.holder.cards.back());
		holder.calling = !holder.holder.takes.wildRules.empty() && top && isWild(holder.holder.takes, *top) && holder.called == 0.0f ? 1.0f : 0.0f;
		refreshBoundTexts(holder.name, "calling", holder.calling);
		refreshBoundTexts(holder.name, "called", holder.called);
	}

	int Game::takeTop(Object& holder)
	{
		if (holder.holder.cards.empty()) { return 0; }
		const int number = holder.holder.cards.back();
		holder.holder.cards.pop_back();
		refreshHolder(holder);
		return number;
	}

	void Game::showTop(Object& holder)
	{
		if (holder.holder.topSprite.empty() || holder.holder.cards.empty()) { return; }
		if (Object* top = cardAt(holder.holder.cards.back()); top && top->lookName() != holder.holder.topSprite) { showHeld(*top, holder.holder.topSprite); refreshHolder(holder); }
	}

	void Game::sendStarts()
	{
		// Numbered, every deck in turn.
		int number = 0;
		for (Object& object : objects)
		{
			if (object.held) { object.cardNumber = ++number; }
		}
		cardIndex.clear();
		if (number == 0) { return; }

		for (Object& object : objects)
		{
			if (object.holder.is) { object.holder.cards.clear(); object.holder.broken.clear(); object.called = 0.0f; refreshHolder(object); }
			if (object.held) { object.laidBy = 0; }
			if (object.hand.is)
			{
				object.hand.held.clear();
				object.hand.from.clear();
				object.hand.holder.clear();
				object.hand.at = -1;
			}
		}

		// Each deck shuffled into its holder.
		std::map<std::string, std::vector<int>> starts;
		std::vector<std::string> order;
		for (Object& object : objects)
		{
			if (!object.held) { continue; }
			if (!starts.count(object.card.startHolder)) { order.push_back(object.card.startHolder); }
			starts[object.card.startHolder].push_back(object.cardNumber);
		}
		for (const std::string& name : order)
		{
			Object* holder = holderNamed(name);
			if (!holder) { throw std::runtime_error("<starts> > <send into=\"" + name + "\">: there is no holder called '" + name + "'"); }
			std::vector<int>& cards = starts[name];
			for (std::size_t i = cards.size(); i > 1; --i)
			{
				const std::size_t j = std::min(i - 1, static_cast<std::size_t>(randomBetween(0.0f, static_cast<float>(i))));
				std::swap(cards[i - 1], cards[j]);
			}
			// Stacked (tests): those cards last, so on top, the first last of all.
			for (auto each = stackedStart.rbegin(); each != stackedStart.rend(); ++each)
			{
				const auto at = std::find(cards.begin(), cards.end(), *each);
				if (at == cards.end()) { continue; }
				cards.erase(at);
				cards.push_back(*each);
			}
			for (const int card : cards)
			{
				if (Object* each = cardAt(card)) { turnCard(*each, false); }
				putInHolder(*holder, card);
			}

			// Dealt in rounds, a card to each holder that wants one more.
			std::vector<std::pair<Object*, int>> wanting;
			std::map<std::string, int> inGroup;
			for (Object& object : objects)
			{
				if (!object.holder.is || &object == holder) { continue; }
				int count = object.holder.startCount;
				if (!object.groupName.empty())
				{
					count += object.holder.startMore * inGroup[object.groupName];
					++inGroup[object.groupName];
				}
				if (count > 0) { wanting.emplace_back(&object, count); }
			}
			for (int round = 1; !wanting.empty() && !holder->holder.cards.empty(); ++round)
			{
				bool any = false;
				for (auto& [each, count] : wanting)
				{
					if (count < round || holder->holder.cards.empty()) { continue; }
					putInHolder(*each, takeTop(*holder));
					any = true;
				}
				if (!any) { break; }
			}
			for (auto& [each, count] : wanting) { showTop(*each); }
		}

		stackedStart.clear();

		// A wild card turned up to start: its own suit is the one called.
		for (Object& object : objects)
		{
			if (object.holder.is && object.calling > 0.0f)
			{
				const Object* top = cardAt(object.holder.cards.back());
				object.called = valueOf(*top, object.holder.takes.wildVariable);
				refreshCall(object);
			}
		}

		// Each hand at its own holder, or else the first holder.
		for (Object& object : objects)
		{
			if (!object.hand.is) { continue; }
			Object* own = object.hand.own.empty() ? nullptr : holderNamed(object.hand.own);
			for (Object& holder : objects)
			{
				if (!holder.holder.is || (own && &holder != own)) { continue; }
				object.hand.holder = holder.name;
				object.hand.at = static_cast<int>(holder.holder.cards.size()) - 1;
				break;
			}
		}
		layOutCards();
	}

	// Where each card of a holder goes: its place, then each card on from the
	// one below by its <spread>, or by <spread sprite> when it shows that look.
	Vector2f Game::cardPlace(const Object& holder, std::size_t index)
	{
		const std::size_t count = holder.holder.cards.size();
		const auto step = [&](std::size_t i)
		{
			const Object* card = cardAt(holder.holder.cards[i]);
			const bool looked = card && !holder.holder.spreadSprite.empty() && card->lookName() == holder.holder.spreadSprite;
			return looked ? holder.holder.lookSpread : holder.holder.spread;
		};
		// A fan never wider than its <width>: the steps across shrink.
		float across = 1.0f;
		if (holder.holder.spreadWidth > 0.0f && count > 1)
		{
			float wide = 0.0f;
			for (std::size_t i = 0; i + 1 < count; ++i) { wide += std::fabs(step(i).x); }
			const Object* first = cardAt(holder.holder.cards.front());
			const float room = holder.holder.spreadWidth - (first ? sizeOf(*first).x : 0.0f);
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

	void Game::layOutCards()
	{
		int order = 1;
		for (Object& holder : objects)
		{
			if (!holder.holder.is) { continue; }
			for (std::size_t i = 0; i < holder.holder.cards.size(); ++i)
			{
				Object* card = cardAt(holder.holder.cards[i]);
				if (!card) { continue; }
				card->position = cardPlace(holder, i);
				card->drawOrder = order++;
			}
		}

		for (Object& hand : objects)
		{
			if (!hand.hand.is) { continue; }
			Object* holder = hand.hand.holder.empty() ? nullptr : holderNamed(hand.hand.holder);
			// A hand pointing into its own holder, emptied then dealt to again:
			// at its first card.
			if (holder && hand.hand.at < 0 && !holder->holder.cards.empty() && !hand.hand.own.empty() && holderNamed(hand.hand.own) == holder) { hand.hand.at = 0; }
			Vector2f pointed = holder ? holder->position : hand.position;
			Vector2f size = holder ? sizeOf(*holder) : Vector2f{};
			if (holder && hand.hand.at >= 0 && static_cast<std::size_t>(hand.hand.at) < holder->holder.cards.size())
			{
				pointed = cardPlace(*holder, static_cast<std::size_t>(hand.hand.at));
				if (Object* card = cardAt(holder->holder.cards[static_cast<std::size_t>(hand.hand.at)]))
				{
					size = sizeOf(*card);
					// A holder with a <lift>: a face-up card pointed at is lifted
					// out of the fan, so all of it shows.
					if (card->faceUp && holder->holder.lift != 0.0f)
					{
						pointed.y -= holder->holder.lift;
						card->position = pointed;
					}
				}
			}
			// The hand around what it points at.
			const Vector2f own = sizeOf(hand);
			hand.position = pointed + (size - own) * 0.5f;

			// What it holds: following the pointer, or laid as it would be
			// on the holder pointed at, a little off it.
			Vector2f at = pointed + Vector2f{ 6.0f, 6.0f };
			if (holder && !holder->holder.cards.empty()) { at = cardPlace(*holder, holder->holder.cards.size() - 1) + holder->holder.spread + Vector2f{ 6.0f, 6.0f }; }
			if (hand.hand.mouse && hand.pointer.x >= 0.0f && hand.pointer.y >= 0.0f) { at = hand.pointer - hand.hand.grab; }
			for (const int number : hand.hand.held)
			{
				Object* card = cardAt(number);
				if (!card) { continue; }
				card->position = at;
				card->drawOrder = 100000 + order++;
				at += holder ? holder->holder.spread : Vector2f{ 0.0f, 20.0f };
				if (holder && holder->holder.spread.x == 0.0f && holder->holder.spread.y == 0.0f) { at += Vector2f{ 0.0f, 20.0f }; }
			}
			hand.drawOrder = 200000;
		}
	}

	// Whether `cards` (the lowest first) may go on the holder, by its <takes>.
	bool Game::holderTakes(Object& holder, const std::vector<int>& cards, const Object* from)
	{
		const HolderRule& rule = holder.holder.takes;
		if (!rule.given || cards.empty()) { return false; }
		// <total variable>N: what is laid together, that variable added up, is N.
		for (const HolderRule::VarRule& each : rule.rules)
		{
			if (each.kind != HolderRule::VarRule::Kind::Total) { continue; }
			float total = 0.0f;
			for (const int number : cards)
			{
				const Object* card = cardAt(number);
				if (!card) { return false; }
				const auto value = card->variable.find(each.variable);
				total += value == card->variable.end() ? 0.0f : value->second;
			}
			if (total != each.amount) { return false; }
		}
		if (rule.one && cards.size() > 1) { return false; }
		const Object* first = cardAt(cards.front());
		if (!first) { return false; }
		// A trick: what the hand laying holds (its own holder) decides.
		if (rule.follow)
		{
			const auto holds = [&](const auto& test)
			{
				if (!from) { return false; }
				for (const int number : from->holder.cards)
				{
					const Object* card = cardAt(number);
					if (card && number != first->cardNumber && test(*card)) { return true; }
				}
				return false;
			};
			if (holder.holder.cards.empty())
			{
				const auto unbroken = [&](const Object& card)
				{
					return std::find(rule.breaks.begin(), rule.breaks.end(), followedName(rule, card)) != rule.breaks.end()
						&& std::find(holder.holder.broken.begin(), holder.holder.broken.end(), followed(rule, card)) == holder.holder.broken.end();
				};
				// A suit not yet broken leads only when the hand holds nothing else.
				if (unbroken(*first) && holds([&](const Object& card) { return !unbroken(card); })) { return false; }
			}
			else if (const Object* lead = cardAt(holder.holder.cards.front()); lead && followed(rule, *first) != followed(rule, *lead))
			{
				if (holds([&](const Object& card) { return followed(rule, card) == followed(rule, *lead); })) { return false; }
			}
		}
		if (holder.holder.cards.empty())
		{
			return acceptsBy(rule.emptyRules, *first, nullptr);
		}
		// Waiting for a suit to be called: nothing goes on it.
		if (holder.calling > 0.0f) { return false; }
		if (isWild(rule, *first)) { return true; }
		const Object* top = cardAt(holder.holder.cards.back());
		if (!top || !top->faceUp) { return false; }
		// <any>: one of them, a value called standing for the top's own.
		if (!rule.anyRules.empty())
		{
			const float called = rule.wildVariable.empty() ? 0.0f : holder.called;
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
		using Kind = HolderRule::VarRule::Kind;
		const auto valueOf = [](const Object& of, const std::string& name)
		{
			const auto found = of.variable.find(name);
			return found == of.variable.end() ? 0.0f : found->second;
		};
		for (const HolderRule::VarRule& each : rules)
		{
			if (each.kind == Kind::Total) { continue; }   // what is laid together, added up (holderTakes)
			const float mine = valueOf(next, each.variable);
			const std::string& named = each.topVariable.empty() ? each.variable : each.topVariable;
			const float theirs = !below ? 0.0f : value > 0.0f && named == called ? value : valueOf(*below, named);
			const float against = each.top ? theirs + each.amount : each.amount;
			bool holds = true;
			switch (each.kind)
			{
			case Kind::Equal: holds = mine == against; break;
			case Kind::Unequal: holds = mine != against; break;
			case Kind::Above: holds = mine > against; break;
			case Kind::Below: holds = mine < against; break;
			case Kind::AtLeast: holds = mine >= against; break;
			case Kind::AtMost: holds = mine <= against; break;
			case Kind::Total: break;
			}
			if (!holds) { return false; }
		}
		return true;
	}

	// Whether a card of a holder lies under another holder's card drawn over it.
	bool Game::isCovered(const Object& card, const Object& own)
	{
		const Vector2f size = sizeOf(card);
		for (const Object& holder : objects)
		{
			if (!holder.holder.is || &holder == &own || !isShown(holder)) { continue; }
			for (const int number : holder.holder.cards)
			{
				const Object* other = cardAt(number);
				if (!other || other == &card || other->drawOrder <= card.drawOrder) { continue; }
				const Vector2f otherSize = sizeOf(*other);
				if (other->position.x < card.position.x + size.x && card.position.x < other->position.x + otherSize.x
					&& other->position.y < card.position.y + size.y && card.position.y < other->position.y + otherSize.y)
				{
					return true;
				}
			}
		}
		return false;
	}

	bool Game::takeCards(Object& hand)
	{
		if (!hand.hand.is || (hand.hand.mouse && !hand.hand.over)) { return false; }
		Object* holder = holderNamed(hand.hand.holder);
		if (!holder || holder->holder.pickedSends > 0 || holder->holder.cards.empty()) { return false; }
		// Covering: a top card that a card of another holder, drawn over it,
		// overlaps stays where it is.
		if (const Object* top = cardAt(holder->holder.cards.back()); top && isCovered(*top, *holder)) { return false; }
		// Holding already: the top card of another holder too, while the hand
		// <holds> cards from that many holders.
		if (!hand.hand.held.empty())
		{
			const auto taken = [&](const std::string& name) { return name == hand.hand.from || std::any_of(hand.hand.also.begin(), hand.hand.also.end(), [&](const auto& each) { return each.second == name; }); };
			if (static_cast<int>(hand.hand.also.size()) + 1 >= hand.hand.holds || taken(holder->name)) { return false; }
			const int number = holder->holder.cards.back();
			const Object* top = cardAt(number);
			if (!top || !top->faceUp) { return false; }
			holder->holder.cards.pop_back();
			refreshHolder(*holder);
			hand.hand.held.push_back(number);
			hand.hand.also.emplace_back(number, holder->name);
			layOutCards();
			return true;
		}
		const int count = static_cast<int>(holder->holder.cards.size());
		const int at = hand.hand.at < 0 ? count - 1 : std::min(hand.hand.at, count - 1);
		// A hand with a holder of its own takes from it alone, any one card,
		// face up or down (it knows its own cards).
		if (!hand.hand.own.empty())
		{
			if (holder != holderNamed(hand.hand.own)) { return false; }
			hand.hand.held = { holder->holder.cards[static_cast<std::size_t>(at)] };
			holder->holder.cards.erase(holder->holder.cards.begin() + at);
			refreshHolder(*holder);
			hand.hand.from = holder->name;
			hand.hand.at = std::min(at, static_cast<int>(holder->holder.cards.size()) - 1);
			if (hand.hand.mouse && hand.pointer.x >= 0.0f && hand.pointer.y >= 0.0f)
			{
				hand.hand.grab = hand.pointer - cardPlace(*holder, static_cast<std::size_t>(std::max(0, hand.hand.at)));
			}
			layOutCards();
			return true;
		}
		if (at < count - 1 && !holder->holder.carry) { return false; }
		// Face up, and each on the one below it by the holder's own rule.
		for (int i = at; i < count; ++i)
		{
			const Object* card = cardAt(holder->holder.cards[static_cast<std::size_t>(i)]);
			if (!card || !card->faceUp) { return false; }
			if (i > at && !follows(holder->holder.takes, *cardAt(holder->holder.cards[static_cast<std::size_t>(i - 1)]), *card)) { return false; }
		}
		hand.hand.held.assign(holder->holder.cards.begin() + at, holder->holder.cards.end());
		holder->holder.cards.resize(static_cast<std::size_t>(at));
		refreshHolder(*holder);
		hand.hand.from = holder->name;
		hand.hand.at = at - 1;
		// Held where the pointer took them.
		if (hand.hand.mouse && hand.pointer.x >= 0.0f && hand.pointer.y >= 0.0f)
		{
			hand.hand.grab = hand.pointer - cardPlace(*holder, static_cast<std::size_t>(at));
		}
		layOutCards();
		return true;
	}

	bool Game::pickCards(Object& hand)
	{
		// From a holder with <picked>: it sends instead, and nothing is
		// picked up, so the picker's verbs after <pick /> do not run.
		const Object* holder = hand.hand.is ? holderNamed(hand.hand.holder) : nullptr;
		if (holder && holder->holder.pickedSends > 0)
		{
			const std::string sound = holder->holder.pickedSound;
			if (sendPicked(hand, "", "") && !sound.empty()) { requestSound(sound); }
			return false;
		}
		return takeCards(hand);
	}

	// The cards held go on the holder, the top one pointed at, and the holder
	// they came from shows its top.
	void Game::layHeld(Object& hand, Object& holder)
	{
		for (const int number : hand.hand.held)
		{
			if (Object* card = cardAt(number)) { card->laidBy = hand.hand.seat; }
			putInHolder(holder, number);
		}
		hand.hand.held.clear();
		if (Object* from = holderNamed(hand.hand.from); from && from != &holder) { showTop(*from); }
		hand.hand.from.clear();
		for (const auto& [number, name] : hand.hand.also)
		{
			if (Object* from = holderNamed(name); from && from != &holder) { showTop(*from); }
		}
		hand.hand.also.clear();
		// A hand with a holder of its own stays pointing into it.
		if (Object* own = hand.hand.own.empty() ? nullptr : holderNamed(hand.hand.own))
		{
			hand.hand.holder = own->name;
			hand.hand.at = std::min(std::max(hand.hand.at, 0), static_cast<int>(own->holder.cards.size()) - 1);
		}
		else
		{
			hand.hand.holder = holder.name;
			hand.hand.at = static_cast<int>(holder.holder.cards.size()) - 1;
		}
		layOutCards();
	}

	bool Game::layCards(Object& hand, const std::string& onto)
	{
		if (!hand.hand.is || hand.hand.held.empty()) { return false; }
		std::vector<Object*> choices;
		if (onto.empty())
		{
			// Let go over nothing: laid nowhere.
			if (hand.hand.mouse && !hand.hand.over) { return false; }
			if (Object* holder = holderNamed(hand.hand.holder)) { choices.push_back(holder); }
		}
		else
		{
			const std::string full = fullName(onto);
			for (Object& object : objects)
			{
				if (object.holder.is && object.named(full) && isShown(object)) { choices.push_back(&object); }
			}
		}
		for (Object* holder : choices)
		{
			// Back where they came from: put back, always allowed.
			if (holder->name == hand.hand.from && onto.empty())
			{
				layHeld(hand, *holder);
				return true;
			}
			if (holderTakes(*holder, hand.hand.held, hand.hand.own.empty() ? nullptr : holderNamed(hand.hand.own)))
			{
				layHeld(hand, *holder);
				return true;
			}
		}
		return false;
	}

	bool Game::dropCards(Object& hand)
	{
		if (!hand.hand.is || hand.hand.held.empty()) { return false; }
		Object* from = holderNamed(hand.hand.from);
		if (!from) { return false; }
		const std::string pointed = hand.hand.holder;
		const int at = hand.hand.at;
		const int card = hand.hand.held.front();
		// Cards taken from other holders go back to theirs first.
		for (const auto& [number, name] : hand.hand.also)
		{
			hand.hand.held.erase(std::find(hand.hand.held.begin(), hand.hand.held.end(), number));
			if (Object* back = holderNamed(name)) { putInHolder(*back, number); }
		}
		hand.hand.also.clear();
		layHeld(hand, *from);
		// Back in a hand's own holder: pointing at it again.
		if (!hand.hand.own.empty() && from == holderNamed(hand.hand.own))
		{
			const auto found = std::find(from->holder.cards.begin(), from->holder.cards.end(), card);
			hand.hand.holder = from->name;
			hand.hand.at = static_cast<int>(found - from->holder.cards.begin());
			layOutCards();
			return true;
		}
		// Put back, the hand stays where it was.
		hand.hand.holder = pointed;
		hand.hand.at = at;
		if (Object* holder = holderNamed(pointed)) { hand.hand.at = std::min(at < 0 ? static_cast<int>(holder->holder.cards.size()) - 1 : at, static_cast<int>(holder->holder.cards.size()) - 1); }
		layOutCards();
		return true;
	}

	bool Game::sendPicked(Object& hand, const std::string& fromHolder, const std::string& faces)
	{
		if (!hand.hand.is || !hand.hand.held.empty() || (fromHolder.empty() && hand.hand.mouse && !hand.hand.over)) { return false; }
		// From the holder named, or else the one pointed at.
		Object* holder = holderNamed(fromHolder.empty() ? hand.hand.holder : fromHolder);
		if (!holder || holder->holder.pickedSends <= 0) { return false; }
		// Into the holder it names, or else the hand's own.
		const std::string& intoName = holder->holder.pickedInto.empty() ? hand.hand.own : holder->holder.pickedInto;
		Object* into = intoName.empty() ? nullptr : holderNamed(intoName);
		if (!into) { return false; }
		if (!holder->holder.cards.empty())
		{
			for (int i = 0; i < holder->holder.pickedSends && !holder->holder.cards.empty(); ++i)
			{
				const int number = takeTop(*holder);
				putInHolder(*into, number);
				if (Object* card = cardAt(number); card && !faces.empty())
				{
					turnCard(*card, faces == "up");
					refreshHolder(*into);
				}
			}
		}
		else
		{
			// Empty: the other holder turned back over onto it (or shuffled),
			// but for the top cards it leaves.
			Object* from = holder->holder.emptyFrom.empty() ? nullptr : holderNamed(holder->holder.emptyFrom);
			const std::size_t leave = static_cast<std::size_t>(holder->holder.emptyLeave);
			if (!from || from->holder.cards.size() <= leave) { return false; }
			const auto end = from->holder.cards.end() - static_cast<std::ptrdiff_t>(leave);
			std::vector<int> cards(from->holder.cards.begin(), end);
			from->holder.cards.erase(from->holder.cards.begin(), end);
			refreshHolder(*from);
			if (holder->holder.emptyRandom)
			{
				for (std::size_t i = cards.size(); i > 1; --i)
				{
					const std::size_t j = std::min(i - 1, static_cast<std::size_t>(randomBetween(0.0f, static_cast<float>(i))));
					std::swap(cards[i - 1], cards[j]);
				}
			}
			else
			{
				// Turned over: the top first.
				std::reverse(cards.begin(), cards.end());
			}
			for (const int card : cards) { putInHolder(*holder, card); }
		}
		// Pointing at the stock, still pointing at its top.
		if (fromHolder.empty()) { hand.hand.at = static_cast<int>(holder->holder.cards.size()) - 1; }
		layOutCards();
		return true;
	}

	namespace
	{
		// Which way up a card showing that look is: its face (its first
		// look) up; another of its looks down; none of its looks, nothing.
		std::optional<bool> faceFor(const Object& card, const std::string& sprite)
		{
			for (std::size_t i = 0; i < card.looks.size(); ++i)
			{
				if (card.looks[i].name == sprite) { return i == 0; }
			}
			return std::nullopt;
		}
	}

	bool Game::sendHeld(const std::string& fromName, const std::string& intoName, int count, bool all, const std::string& sprite)
	{
		Object* into = holderNamed(intoName);
		if (!into)
		{
			// A group's: each member's own holder in turn, a card a round.
			std::vector<std::pair<int, std::string>> owners;
			for (const Object& object : objects)
			{
				if (object.hand.is && !object.hand.own.empty() && (object.inGroup(intoName) || object.inGroup(fullName(intoName)))) { owners.emplace_back(object.hand.seat, object.hand.own); }
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
		if (Object* from = holderNamed(fromName))
		{
			if (from == into || from->holder.cards.empty()) { return false; }
			const std::size_t many = all ? from->holder.cards.size() : std::min(from->holder.cards.size(), static_cast<std::size_t>(count));
			// The top ones, the topmost first.
			for (std::size_t i = 0; i < many; ++i) { sent.push_back(takeTop(*from)); }
		}
		else
		{
			// A group's name: every member, wherever it is.
			if (!all) { return false; }
			for (Object& object : objects)
			{
				if (object.held && (object.inGroup(fromName) || object.inGroup(fullName(fromName)))) { sent.push_back(object.cardNumber); }
			}
			if (sent.empty()) { return false; }
			for (Object& object : objects)
			{
				if (object.holder.is)
				{
					auto& cards = object.holder.cards;
					const auto gone = std::remove_if(cards.begin(), cards.end(), [&](int number) { return std::find(sent.begin(), sent.end(), number) != sent.end(); });
					if (gone != cards.end())
					{
						cards.erase(gone, cards.end());
						object.holder.broken.clear();
						object.called = 0.0f;
						refreshHolder(object);
					}
				}
				if (object.hand.is)
				{
					object.hand.held.clear();
					object.hand.from.clear();
				}
			}
			std::sort(sent.begin(), sent.end());
		}
		for (const int number : sent)
		{
			Object* card = cardAt(number);
			if (card) { card->laidBy = 0; }
			putInHolder(*into, number);
			if (card && !sprite.empty())
			{
				if (const auto up = faceFor(*card, sprite)) { turnCard(*card, *up); }
			}
		}
		refreshHolder(*into);
		// A hand pointing where cards were: at its own, or at that holder.
		for (Object& object : objects)
		{
			if (!object.hand.is) { continue; }
			Object* pointed = holderNamed(object.hand.holder);
			if (pointed && object.hand.at >= static_cast<int>(pointed->holder.cards.size())) { object.hand.at = static_cast<int>(pointed->holder.cards.size()) - 1; }
		}
		layOutCards();
		return true;
	}

	bool Game::orderHeld(const std::string& name)
	{
		Object* holder = holderNamed(name);
		if (!holder) { return false; }
		std::vector<int>& cards = holder->holder.cards;
		for (std::size_t i = cards.size(); i > 1; --i)
		{
			const std::size_t j = std::min(i - 1, static_cast<std::size_t>(randomBetween(0.0f, static_cast<float>(i))));
			std::swap(cards[i - 1], cards[j]);
		}
		// Stacked (tests): those cards last, so on top, the first last of all.
		for (auto each = stackedStart.rbegin(); each != stackedStart.rend(); ++each)
		{
			const auto at = std::find(cards.begin(), cards.end(), *each);
			if (at == cards.end()) { continue; }
			cards.erase(at);
			cards.push_back(*each);
		}
		stackedStart.clear();
		refreshHolder(*holder);
		layOutCards();
		return true;
	}

	bool Game::turnHeld(Object& holder, const std::string& sprite)
	{
		for (const int number : holder.holder.cards)
		{
			Object* card = cardAt(number);
			if (!card) { continue; }
			const auto up = faceFor(*card, sprite);
			if (!up) { return false; }
			if (card->faceUp != *up) { turnCard(*card, *up); }
		}
		refreshHolder(holder);
		layOutCards();
		return true;
	}

	// Every holder in play, and its middle and size as drawn.
	std::vector<Object*> Game::shownHolders()
	{
		std::vector<Object*> holders;
		for (Object& object : objects)
		{
			if (object.holder.is && isShown(object)) { holders.push_back(&object); }
		}
		return holders;
	}

	bool Game::hopHand(Object& hand, Direction direction)
	{
		Object* holder = holderNamed(hand.hand.holder);
		const std::vector<Object*> holders = shownHolders();
		if (holders.empty()) { return false; }
		if (!holder) { holder = holders.front(); }
		const int count = static_cast<int>(holder->holder.cards.size());
		const bool own = !hand.hand.own.empty() && holder == holderNamed(hand.hand.own);
		const auto takeable = [&](int index)
		{
			if (index < 0 || index >= count) { return false; }
			const Object* card = cardAt(holder->holder.cards[static_cast<std::size_t>(index)]);
			return card && (card->faceUp || own);
		};
		const auto pointAt = [&](Object& to)
		{
			hand.hand.holder = to.name;
			hand.hand.at = static_cast<int>(to.holder.cards.size()) - 1;
			hand.hand.mouse = false;
			layOutCards();
			return true;
		};

		hand.facing = direction;
		// A holder fanned across: left and right go through its cards first
		// (any of a hand's own, else a run's face-up ones).
		const bool fannedAcross = std::fabs(holder->holder.spread.x) > std::fabs(holder->holder.spread.y);
		if (fannedAcross && (direction == Direction::Left || direction == Direction::Right) && (own || holder->holder.carry))
		{
			const int at = hand.hand.at < 0 ? count - 1 : hand.hand.at;
			const int next = at + ((direction == Direction::Right) == (holder->holder.spread.x > 0.0f) ? 1 : -1);
			if (takeable(next) && takeable(at))
			{
				hand.hand.at = next;
				hand.hand.mouse = false;
				layOutCards();
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

		// Up and down: through a run's face-up cards, then to the row of
		// holders above or below, the holder nearest across.
		const bool up = direction == Direction::Up;
		const int at = hand.hand.at < 0 ? count - 1 : hand.hand.at;
		if (!fannedAcross && holder->holder.carry && up && takeable(at - 1) && takeable(at))
		{
			hand.hand.at = at - 1;
			hand.hand.mouse = false;
			layOutCards();
			return true;
		}
		if (!fannedAcross && holder->holder.carry && !up && at < count - 1)
		{
			hand.hand.at = at + 1;
			hand.hand.mouse = false;
			layOutCards();
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
		(horizontal ? hand.pointer.x : hand.pointer.y) = at;
		if (hand.pointer.x < 0.0f || hand.pointer.y < 0.0f) { return; }
		hand.hand.mouse = true;
		hand.hand.over = false;
		const Vector2f point = hand.pointer;
		const auto inside = [&](Vector2f where, Vector2f size)
		{
			return point.x >= where.x && point.y >= where.y && point.x < where.x + size.x && point.y < where.y + size.y;
		};
		// The top card under the pointer (of holders overlapping, the one
		// drawn over), else the holder's place.
		Object* found = nullptr;
		int foundAt = -1;
		int foundOrder = -1;
		for (Object* holder : shownHolders())
		{
			bool hit = false;
			int index = -1;
			for (int i = static_cast<int>(holder->holder.cards.size()) - 1; i >= 0 && !hit; --i)
			{
				const Object* card = cardAt(holder->holder.cards[static_cast<std::size_t>(i)]);
				if (card && inside(cardPlace(*holder, static_cast<std::size_t>(i)), sizeOf(*card)))
				{
					hit = true;
					index = i;
				}
			}
			if (!hit && inside(holder->position, sizeOf(*holder))) { hit = true; }
			if (!hit) { continue; }
			const Object* card = index >= 0 ? cardAt(holder->holder.cards[static_cast<std::size_t>(index)]) : nullptr;
			const int order = card ? card->drawOrder : 0;
			if (found && order <= foundOrder) { continue; }
			found = holder;
			foundAt = index;
			foundOrder = order;
		}
		if (found)
		{
			// Holding: the holder, its top card.
			if (!hand.hand.held.empty() && hand.hand.holds <= 1) { foundAt = static_cast<int>(found->holder.cards.size()) - 1; }
			hand.hand.holder = found->name;
			hand.hand.at = foundAt;
			hand.hand.over = true;
		}
		layOutCards();
	}

	bool Game::callValue(Object& hand, const std::string& name)
	{
		if (!hand.hand.is || !hand.hand.held.empty() || hand.hand.own.empty() || (hand.hand.mouse && !hand.hand.over)) { return false; }
		Object* holder = holderNamed(name);
		Object* own = holderNamed(hand.hand.own);
		if (!holder || !own || holder->calling <= 0.0f || holderNamed(hand.hand.holder) != own) { return false; }
		const int count = static_cast<int>(own->holder.cards.size());
		if (count == 0) { return false; }
		const int at = hand.hand.at < 0 ? count - 1 : std::min(hand.hand.at, count - 1);
		const Object* card = cardAt(own->holder.cards[static_cast<std::size_t>(at)]);
		if (!card) { return false; }
		// The value named: the card's <wild variable=>.
		holder->called = valueOf(*card, holder->holder.takes.wildVariable);
		refreshCall(*holder);
		return true;
	}

	// A computer playing a card game, a step each time it is due: point the
	// hand at what it will do, then press. Calling, the suit it holds most
	// of; else the card that goes somewhere, a wild one last, from the suit
	// it holds most of and the highest; else a draw from the stock.
	int Game::playToTrick(Object& trick, const Object& hand, const Object& own, bool avoid)
	{
		int players = 0;
		for (const Object& object : objects)
		{
			if (object.hand.is && object.hand.seat > 0 && isShown(object)) { ++players; }
		}
		const bool last = static_cast<int>(trick.holder.cards.size()) + 1 >= std::max(players, 2);
		const Object* winning = trickWinning(trick);
		const auto worth = [](const Object& card) { return card.card.worths.empty() ? 0.0f : card.card.worths.back(); };

		std::vector<std::pair<int, const Object*>> legal;
		for (int i = 0; i < static_cast<int>(own.holder.cards.size()); ++i)
		{
			const int number = own.holder.cards[static_cast<std::size_t>(i)];
			const Object* card = cardAt(number);
			if (card && holderTakes(trick, { number }, &own)) { legal.emplace_back(i, card); }
		}
		if (legal.empty()) { return -1; }
		const Object* led = trick.holder.cards.empty() ? nullptr : cardAt(trick.holder.cards.front());
		const bool following = led && std::any_of(legal.begin(), legal.end(), [&](const auto& each) { return followed(trick.holder.takes, *each.second) == followed(trick.holder.takes, *led); });
		const bool ducking = following && std::any_of(legal.begin(), legal.end(), [&](const auto& each) { return worth(*each.second) < worth(*winning); });
		const HolderRule& rule = trick.holder.takes;
		float top = 0.0f;
		for (const Object& object : objects)
		{
			if (object.held && !object.card.worths.empty()) { top = std::max(top, object.card.worths.back()); }
		}
		// A partner (another hand on its <team>) winning safely: it is the
		// last card, or the partner's is the highest or next of its suit.
		bool partnerWins = false;
		if (winning && hand.hand.team > 0 && winning->laidBy != hand.hand.seat)
		{
			for (const Object& other : objects)
			{
				if (other.hand.is && other.hand.seat == winning->laidBy && other.hand.team == hand.hand.team) { partnerWins = last || worth(*winning) >= top - 1.0f || isTrump(rule, *winning); }
			}
		}

		// How much each card is wanted: the highest is played.
		const auto wanted = [&](const Object& card)
		{
			const float value = worth(card);
			const float points = cardPoints(trick, card);
			if (!winning)                                                                  // leading
			{
				if (avoid) { return -(points * 100.0f + value); }
				return value >= top ? 100.0f + value : -value - (isTrump(rule, card) ? 50.0f : 0.0f);  // an ace, else the lowest side card
			}
			if (!following && avoid) { return points * 100.0f + value; }                  // none of the suit led: shed
			if (avoid)
			{
				if (ducking) { return value < worth(*winning) ? value : -1000.0f - value; }  // the highest that still loses
				return last ? value : -value;                                               // taking it anyway
			}
			const float spare = isTrump(rule, card) ? 50.0f : 0.0f;                           // a trump kept back
			if (partnerWins) { return -value - spare; }                                       // the lowest: the partner has it
			return beatsInTrick(rule, card, *winning) ? 1000.0f - value : -value - spare;     // the lowest that wins, else the lowest
		};
		int best = legal.front().first;
		float most = wanted(*legal.front().second);
		for (const auto& [i, card] : legal)
		{
			const float each = wanted(*card);
			if (each > most)
			{
				most = each;
				best = i;
			}
		}
		return best;
	}

	std::optional<KeyCode> Game::thinkHeld(const CmdThink& think, Object& hand)
	{
		Object* own = hand.hand.own.empty() ? nullptr : holderNamed(hand.hand.own);
		if (!own || !hand.hand.held.empty()) { return std::nullopt; }
		if (cardWait > 0)
		{
			--cardWait;
			return std::nullopt;
		}
		constexpr int pace = 30;

		// How many of its own cards share each value of the variable a wild
		// card calls (none called: all alike).
		std::string calledBy;
		for (const Object* each : shownHolders())
		{
			if (calledBy.empty()) { calledBy = each->holder.takes.wildVariable; }
		}
		const auto kindOf = [&](const Object& card) { return calledBy.empty() ? 0 : static_cast<int>(std::lround(valueOf(card, calledBy))); };
		std::map<int, int> ofKind;
		for (const int number : own->holder.cards)
		{
			if (const Object* card = cardAt(number)) { ++ofKind[kindOf(*card)]; }
		}
		std::vector<Object*> targets;
		Object* stock = nullptr;
		for (Object* holder : shownHolders())
		{
			if (holder == own) { continue; }
			if (holder->holder.takes.given) { targets.push_back(holder); }
			if (!stock && holder->holder.pickedSends > 0) { stock = holder; }
		}

		Object* aim = nullptr;
		int index = -1;
		const bool calling = std::any_of(targets.begin(), targets.end(), [](const Object* holder) { return holder->calling > 0.0f; });
		const auto trick = std::find_if(targets.begin(), targets.end(), [](const Object* holder) { return holder->holder.takes.follow; });
		if (!own->holder.melds.empty())
		{
			// Melds: holding no more than its best, it draws, the face-up top
			// of a holder when that leaves less deadwood, else the face-down
			// one; holding more, it lets go of the card leaving the least
			// (the one worth most, of two alike).
			const int best = own->holder.meldsBest > 0 ? own->holder.meldsBest : static_cast<int>(own->holder.cards.size());
			if (static_cast<int>(own->holder.cards.size()) <= best)
			{
				Object* up = nullptr;
				Object* down = nullptr;
				for (Object* holder : shownHolders())
				{
					if (holder == own || holder->holder.pickedSends <= 0 || holder->holder.cards.empty()) { continue; }
					const Object* top = cardAt(holder->holder.cards.back());
					if (top && top->faceUp) { up = up ? up : holder; }
					else { down = down ? down : holder; }
				}
				aim = down ? down : up;
				if (up && down)
				{
					std::vector<int> with = own->holder.cards;
					with.push_back(up->holder.cards.back());
					if (meldDeadwood(*own, with, best, nullptr) < meldDeadwood(*own, own->holder.cards, best, nullptr)) { aim = up; }
				}
				if (aim) { index = static_cast<int>(aim->holder.cards.size()) - 1; }
			}
			else
			{
				float least = 0.0f;
				float worth = 0.0f;
				for (int i = 0; i < static_cast<int>(own->holder.cards.size()); ++i)
				{
					std::vector<int> rest = own->holder.cards;
					rest.erase(rest.begin() + i);
					const float left = meldDeadwood(*own, rest, 0, nullptr);
					const float points = cardAt(own->holder.cards[static_cast<std::size_t>(i)]) ? cardPoints(*own, *cardAt(own->holder.cards[static_cast<std::size_t>(i)])) : 0.0f;
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
			// A trick: of the cards it takes, the one that best keeps out
			// of it (<avoid />) or takes it.
			index = playToTrick(**trick, hand, *own, think.avoid);
			if (index >= 0) { aim = own; }
		}
		else if (calling)
		{
			int most = 0;
			for (int i = 0; i < static_cast<int>(own->holder.cards.size()); ++i)
			{
				const Object* card = cardAt(own->holder.cards[static_cast<std::size_t>(i)]);
				if (card && ofKind[kindOf(*card)] > most)
				{
					most = ofKind[kindOf(*card)];
					aim = own;
					index = i;
				}
			}
		}
		else
		{
			float best = 0.0f;
			for (int i = 0; i < static_cast<int>(own->holder.cards.size()); ++i)
			{
				const int number = own->holder.cards[static_cast<std::size_t>(i)];
				const Object* card = cardAt(number);
				if (!card) { continue; }
				for (Object* holder : targets)
				{
					if (!holderTakes(*holder, { number }, own)) { continue; }
					const float worth = isWild(holder->holder.takes, *card) ? 1.0f : 100.0f + static_cast<float>(ofKind[kindOf(*card)] * 20) + (card->card.worths.empty() ? 0.0f : card->card.worths.back());
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
				index = static_cast<int>(stock->holder.cards.size()) - 1;
			}
		}
		if (!aim) { return std::nullopt; }

		cardWait = pace;
		if (hand.hand.holder != aim->name || hand.hand.at != index)
		{
			hand.hand.holder = aim->name;
			hand.hand.at = index;
			hand.hand.mouse = false;
			layOutCards();
			return std::nullopt;
		}
		return think.button;
	}
}
