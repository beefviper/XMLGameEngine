// cards.cpp
// XML Game Engine
// author: beefviper
// date: Nov 1, 2026
//
// Cards: the members of a held class and the holders that hold them
// (settleHolders), and the game's piles and hands (Game): the deal at the
// start, a picker pointing at holders and picking and placing cards by
// each holder's rules, and where every card is drawn.

#include "game/cards.h"
#include "game/data.h"
#include "game/game.h"
#include "game/random.h"
#include "play/verbs.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <map>
#include <optional>
#include <random>
#include <stdexcept>
#include <utility>

namespace xge
{
	// ===================================================================
	// The game's piles and hands
	// ===================================================================

	void settleHolders(GameData& data)
	{
		std::vector<RawObject>& objects = data.objects;

		// The classes held, and their members: cards.
		std::vector<std::string> held;
		for (const RawObject& object : objects)
		{
			if (!object.pile.holdsClass.empty() && std::find(held.begin(), held.end(), object.pile.holdsClass) == held.end()) { held.push_back(object.pile.holdsClass); }
		}
		if (held.empty()) { return; }

		std::map<std::string, const RawObject*> sample;  // a member of each class held
		for (RawObject& object : objects)
		{
			if (object.objClass.empty() || std::find(held.begin(), held.end(), object.objClass) == held.end()) { continue; }
			const std::string where = "object '" + object.name + "' (of class " + object.objClass + ", which a <holds> holds)";
			if (object.groupName.empty() || object.cell.column < 1) { throw std::runtime_error(where + ": is a member of a <group> laid out in <columns> and <rows>, its suit its row and its rank its column"); }
			if (object.looks.size() < 2) { throw std::runtime_error(where + ": has two looks, its face first and its back"); }
			object.isCard = true;
			object.card.deck = object.groupName;
			object.card.suit = object.cell.row;
			object.card.rank = object.cell.column;
			object.card.suitName = std::to_string(object.cell.row);
			object.card.rankName = std::to_string(object.cell.column);
			if (const auto color = object.variable.find("color"); color != object.variable.end()) { object.card.color = color->second.text; }
			float worth = static_cast<float>(object.card.rank);
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
			object.variable["suit"] = RawValue::expression(std::to_string(object.card.suit));
			object.variable["rank"] = RawValue::expression(std::to_string(object.card.rank));
			object.variable["worth"] = RawValue::expression(formatDisplayNumber(worth));
			object.sprite = object.looks.front();
			sample.emplace(object.objClass, &object);
		}

		// Each holder: which way up its looks turn what is laid there.
		for (RawObject& object : objects)
		{
			RawObject::RawPile& pile = object.pile;
			if (pile.holdsClass.empty()) { continue; }
			const std::string where = "object '" + object.name + "' > <holds class=\"" + pile.holdsClass + "\">";
			const auto member = sample.find(pile.holdsClass);
			if (member == sample.end()) { throw std::runtime_error(where + ": nothing is of class " + pile.holdsClass); }
			const std::vector<RawSprite>& looks = member->second->looks;
			const std::string face = looks[0].name;
			const auto known = [&](const std::string& look, const std::string& part)
			{
				if (std::none_of(looks.begin(), looks.end(), [&](const RawSprite& each) { return each.name == look; }))
				{
					throw std::runtime_error(where + ": " + part + " sprite=\"" + look + "\", a look the members of class " + pile.holdsClass + " have not (they have '" + face + "' and '" + looks[1].name + "')");
				}
			};
			if (!pile.holdsSprite.empty()) { known(pile.holdsSprite, "its"); pile.faces = pile.holdsSprite == face ? "up" : "down"; }
			// <counts variable>: a variable of its own, kept to the total.
			if (!pile.counts.empty() && !object.variable.count(pile.counts)) { object.variable[pile.counts] = RawValue::expression("0"); }
			if (!pile.spreadSprite.empty())
			{
				known(pile.spreadSprite, "<spread>");
				// The engine's hidden fan is for the face-down ones: a spread
				// for the face is the fan, the plain one then the hidden.
				if (pile.spreadSprite == face)
				{
					std::swap(pile.fanX, pile.hiddenX);
					std::swap(pile.fanY, pile.hiddenY);
				}
			}
			if (!pile.topSprite.empty())
			{
				known(pile.topSprite, "<top>");
				if (pile.topSprite != face) { throw std::runtime_error(where + " > <top>: turns the top one to its face, '" + face + "'"); }
				pile.showTop = true;
			}
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
		const auto pileFor = [&](const std::string& name, const std::string& state) -> std::vector<RawObject*>
		{
			std::vector<RawObject*> found;
			for (RawObject& object : objects)
			{
				if (object.pile.is && object.ownerState == state && (object.named(state + "." + name) || object.named(name))) { found.push_back(&object); }
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
						if (object.isCard && object.ownerState == state.name && (object.named(state.name + "." + step.from) || object.named(step.from))) { object.card.startPile = step.into; any = true; }
					}
					if (!any) { throw std::runtime_error(where + ": from=\"" + step.from + "\" names no held members of this state"); }
					if (pileFor(step.into, state.name).size() != 1) { throw std::runtime_error(where + ": into=\"" + step.into + "\" names no one holder of this state"); }
					start = step.into;
					continue;
				}
				if (start.empty() || step.from != start)
				{
					throw std::runtime_error(where + ": for now a start sends <all /> of a group into one holder first, then orders and deals from that holder");
				}
				if (step.verb == "order") { continue; }   // the cards are always shuffled where they start
				std::vector<RawObject*> into = pileFor(step.into, state.name);
				// A group's: each member's own holder.
				if (into.empty())
				{
					for (const RawObject& object : objects)
					{
						if (object.ownerState != state.name || object.handPile.empty() || (object.groupName != step.into && object.groupName != state.name + "." + step.into)) { continue; }
						for (RawObject* own : pileFor(object.handPile, state.name)) { into.push_back(own); }
					}
				}
				if (into.empty()) { throw std::runtime_error(where + ": into=\"" + step.into + "\" names no holder of this state"); }
				for (RawObject* each : into)
				{
					each->pile.dealt = RawValue::expression(std::to_string(step.count));
					if (step.more != 0) { each->pile.dealtMore = RawValue::expression(std::to_string(step.more)); }
				}
			}
		}
		for (const RawObject& object : objects)
		{
			if (object.isCard && object.card.startPile.empty())
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

	Object* Game::pileNamed(const std::string& name)
	{
		const std::string full = fullName(name);
		for (Object& object : objects)
		{
			if (object.pile.is && object.name == full) { return &object; }
		}
		return nullptr;
	}

	void Game::turnCard(Object& card, bool up)
	{
		card.faceUp = up;
		const std::size_t look = up ? 0 : 1;
		if (card.look != look && look < card.looks.size()) { card.showLook(look); }
	}

	void Game::putOnPile(Object& pile, int number)
	{
		Object* card = cardAt(number);
		if (!card) { return; }
		if (pile.pile.faces == "up") { turnCard(*card, true); }
		else if (pile.pile.faces == "down") { turnCard(*card, false); }
		auto at = pile.pile.cards.end();
		if (!pile.pile.orderBy.empty())
		{
			// <order>: by each variable named, the first first.
			const auto order = [&](const Object& of)
			{
				std::vector<float> values;
				for (const std::string& name : pile.pile.orderBy)
				{
					const auto found = of.variable.find(name);
					values.push_back(found == of.variable.end() ? 0.0f : found->second);
				}
				return values;
			};
			const std::vector<float> mine = order(*card);
			at = std::find_if(pile.pile.cards.begin(), pile.pile.cards.end(), [&](int other)
				{
					const Object* each = cardAt(other);
					return each && mine < order(*each);
				});
		}
		else if (pile.pile.sorted)
		{
			// Suit by suit, each by its highest worth (in rank order,
			// unless a worth puts a card elsewhere: an ace worth 14 last).
			const auto order = [](const Object& of) { return std::make_tuple(of.card.suit, of.card.worths.empty() ? 0.0f : of.card.worths.back(), of.card.rank); };
			const auto before = [&](int other)
			{
				const Object* each = cardAt(other);
				return each && order(*card) < order(*each);
			};
			at = std::find_if(pile.pile.cards.begin(), pile.pile.cards.end(), before);
		}
		pile.pile.cards.insert(at, number);
		// A suit that <breaks>, broken.
		const std::vector<std::string>& breaks = pile.pile.takes.breakSuits;
		if (std::find(breaks.begin(), breaks.end(), card->card.suitName) != breaks.end()
			&& std::find(pile.pile.broken.begin(), pile.pile.broken.end(), card->card.suit) == pile.pile.broken.end())
		{
			pile.pile.broken.push_back(card->card.suit);
		}
		// A card laid answers any call.
		pile.called = 0.0f;
		refreshPile(pile);
	}

	float Game::cardPoints(const Object& pile, const Object& card)
	{
		if (!pile.pile.counts.empty() && pile.pile.counts != "worth")
		{
			const auto value = card.variable.find(pile.pile.counts);
			return value == card.variable.end() ? 0.0f : value->second;
		}
		return card.card.worths.empty() ? static_cast<float>(card.card.rank) : card.card.worths.front();
	}

	// Whether these cards, every one of them, make the pattern: each part
	// holds for all of them (<same>: one rank, <run>: worths in a row, a
	// card counting any of its worths, <suited>: one suit), and there are
	// at least as many as its number.
	bool Game::meldFits(const std::vector<const Object*>& group, const RawPatterns::Pattern& pattern)
	{
		const std::size_t n = group.size();
		for (const auto& [part, count] : pattern.parts)
		{
			if (n < static_cast<std::size_t>(count)) { return false; }
			if (part == RawPatterns::Part::Same || part == RawPatterns::Part::Suited)
			{
				for (const Object* card : group)
				{
					if (part == RawPatterns::Part::Same ? card->card.rank != group.front()->card.rank : card->card.suit != group.front()->card.suit) { return false; }
				}
				continue;
			}
			// A run: some start from which each worth in turn is a different card's.
			bool found = false;
			for (const Object* starter : group)
			{
				// By worth, a card counting any of its worths; by rank, its column.
				const auto valuesOf = [&](const Object& card) { return pattern.runBy == "rank" ? std::vector<float>{ static_cast<float>(card.card.rank) } : card.card.worths; };
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
	// of them) laid out in groups each making one of the pile's <melds>
	// patterns; `melds`, when asked, gets the groups (indexes into
	// `numbers`). Each card counts its points by the pile's <points>.
	float Game::meldDeadwood(const Object& pile, const std::vector<int>& numbers, int best, std::vector<std::vector<int>>* melds)
	{
		std::vector<const Object*> cards;
		for (const int number : numbers)
		{
			if (const Object* card = cardAt(number)) { cards.push_back(card); }
		}
		const std::size_t n = cards.size();
		std::vector<float> value(n);
		for (std::size_t i = 0; i < n; ++i) { value[i] = cardPoints(pile, *cards[i]); }
		const auto set = std::find_if(patterns.begin(), patterns.end(), [&](const RawPatterns& each) { return each.name == pile.pile.melds; });
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

	// A pile with <melds>: its groups first, each lowest card first, the
	// rest after them in suit and rank order; `deadwood` from its best.
	void Game::layOutMelds(Object& pile)
	{
		std::vector<std::vector<int>> melds;
		const std::vector<int> cards = pile.pile.cards;
		pile.deadwood = meldDeadwood(pile, cards, pile.pile.meldsBest, nullptr);
		meldDeadwood(pile, cards, 0, &melds);
		std::vector<int> order;
		std::vector<bool> placed(cards.size(), false);
		const auto byWorth = [&](int a, int b)
		{
			const Object& x = *cardAt(cards[static_cast<std::size_t>(a)]);
			const Object& y = *cardAt(cards[static_cast<std::size_t>(b)]);
			return std::make_tuple(x.card.suit, x.card.rank) < std::make_tuple(y.card.suit, y.card.rank);
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
		pile.pile.cards = order;
		refreshBoundTexts(pile.name, "deadwood", pile.deadwood);
	}

	bool Game::isTrump(const PileRule& rule, const Object& card)
	{
		return std::find(rule.trumpSuits.begin(), rule.trumpSuits.end(), card.card.suitName) != rule.trumpSuits.end();
	}

	// Whether `card` laid now takes the trick from `best`: a trump over
	// any other card, else a higher card of the same suit.
	bool Game::beatsInTrick(const PileRule& rule, const Object& card, const Object& best)
	{
		const bool trump = isTrump(rule, card);
		if (trump != isTrump(rule, best)) { return trump; }
		return card.card.suit == best.card.suit && card.card.worths.back() > best.card.worths.back();
	}

	const Object* Game::trickWinning(const Object& pile)
	{
		if (pile.pile.cards.empty()) { return nullptr; }
		const Object* best = cardAt(pile.pile.cards.front());
		for (const int number : pile.pile.cards)
		{
			const Object* card = cardAt(number);
			if (card && best && beatsInTrick(pile.pile.takes, *card, *best)) { best = card; }
		}
		return best;
	}

	// How many tricks a hand's cards should take, as a player reckons it
	// to bid: each side suit's ace, a king with another card of its suit,
	// half a queen with two more and a higher card; each trump honour with
	// as many trumps under it as there are above it, and each trump past
	// three; with trumps to spare, a missing suit one and a single card a
	// half. Rounded to the nearest.
	float Game::reckonTricks(const Object& own, const PileRule& trick)
	{
		float top = 0.0f;
		std::vector<int> suits;
		for (const Object& object : objects)
		{
			if (!object.isCard || object.card.worths.empty()) { continue; }
			top = std::max(top, object.card.worths.back());
			if (std::find(suits.begin(), suits.end(), object.card.suit) == suits.end()) { suits.push_back(object.card.suit); }
		}
		std::map<int, std::vector<float>> held;
		std::map<int, bool> trumpSuit;
		int trumps = 0;
		for (const int number : own.pile.cards)
		{
			const Object* card = cardAt(number);
			if (!card || card->card.worths.empty()) { continue; }
			held[card->card.suit].push_back(card->card.worths.back());
			if (isTrump(trick, *card))
			{
				trumpSuit[card->card.suit] = true;
				++trumps;
			}
		}
		float total = 0.0f;
		int honours = 0;
		for (auto& [suit, worths] : held)
		{
			std::sort(worths.rbegin(), worths.rend());
			const std::size_t length = worths.size();
			const auto has = [&](float worth) { return std::find(worths.begin(), worths.end(), worth) != worths.end(); };
			if (trumpSuit[suit])
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
		if (!trick.trumpSuits.empty())
		{
			float ruffs = 0.0f;
			for (const int suit : suits)
			{
				if (trumpSuit[suit]) { continue; }
				const auto it = held.find(suit);
				const std::size_t length = it == held.end() ? 0 : it->second.size();
				if (length == 0) { ruffs += 1.0f; }
				else if (length == 1) { ruffs += 0.5f; }
			}
			const int spare = trumps - honours;
			total += std::min(ruffs, static_cast<float>(std::max(spare, 0)));
		}
		return std::floor(total + 0.5f);
	}

	const Object* Game::trickPile()
	{
		const auto found = std::find_if(objects.begin(), objects.end(), [](const Object& object) { return object.pile.is && object.pile.takes.follow; });
		return found == objects.end() ? nullptr : &*found;
	}

	bool Game::isWild(const PileRule& rule, const Object& card)
	{
		return std::find(rule.wildRanks.begin(), rule.wildRanks.end(), card.card.rankName) != rule.wildRanks.end()
			|| (!rule.wildRules.empty() && acceptsBy(rule.wildRules, card, nullptr));
	}

	void Game::refreshPile(Object& pile)
	{
		pile.cardCount = static_cast<float>(pile.pile.cards.size());
		refreshCall(pile);

		// Points: each face-up card at its lowest worth, then each that may
		// count higher raised as far as the total stays within the limit.
		float total = 0.0f;
		std::vector<const Object*> shown;
		// <counts variable>: a variable of each other than its worth, as it is.
		const std::string& counted = pile.pile.counts;
		const bool byVariable = !counted.empty() && counted != "worth";
		for (const int number : pile.pile.cards)
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
				if (more > 0.0f && (pile.pile.pointsMost <= 0.0f || total + more <= pile.pile.pointsMost))
				{
					total += more;
					break;
				}
			}
		}
		pile.points = total;
		if (!counted.empty()) { setVariable(pile.name, counted, total); }
		refreshBoundTexts(pile.name, "cards", pile.cardCount);
		refreshBoundTexts(pile.name, "held", pile.cardCount);
		refreshBoundTexts(pile.name, "points", pile.points);

		// A pile with <melds>: laid out in its groups, and its deadwood.
		if (!pile.pile.melds.empty() && std::all_of(pile.pile.cards.begin(), pile.pile.cards.end(), [&](int number) { return cardAt(number) != nullptr; })) { layOutMelds(pile); }

		// A hand's own pile in a game with a trick: the tricks it should take.
		if (const Object* trick = trickPile(); trick && trick != &pile)
		{
			const bool owned = std::any_of(objects.begin(), objects.end(), [&](const Object& hand) { return hand.hand.is && !hand.hand.own.empty() && pileNamed(hand.hand.own) == &pile; });
			if (owned)
			{
				pile.tricks = reckonTricks(pile, trick->pile.takes);
				refreshBoundTexts(pile.name, "tricks", pile.tricks);
			}
		}

		// A trick: the suit led, and who laid the card winning it.
		if (pile.pile.takes.follow)
		{
			const Object* lead = pile.pile.cards.empty() ? nullptr : cardAt(pile.pile.cards.front());
			const Object* best = trickWinning(pile);
			pile.lead = lead ? static_cast<float>(lead->card.suit) : 0.0f;
			pile.winner = best ? static_cast<float>(best->laidBy) : 0.0f;
			refreshBoundTexts(pile.name, "lead", pile.lead);
			refreshBoundTexts(pile.name, "winner", pile.winner);
		}

		// Scored piles, this one and those that score its cards with theirs,
		// are scored again before the conditions are asked (scoreDuePiles):
		// once, however many cards came and went this frame.
		if (!pile.pile.score.empty()) { pile.pile.scoreDue = scoresDue = true; }
		for (Object& other : objects)
		{
			if (&other != &pile && other.pile.is && !other.pile.score.empty() && !other.pile.scoreWith.empty() && pileNamed(other.pile.scoreWith) == &pile)
			{
				other.pile.scoreDue = scoresDue = true;
			}
		}
	}

	void Game::scoreDuePiles()
	{
		if (!scoresDue) { return; }
		scoresDue = false;
		for (Object& object : objects)
		{
			if (object.pile.is && object.pile.scoreDue)
			{
				object.pile.scoreDue = false;
				scorePile(object);
			}
		}
	}

	namespace
	{
		// A card as patterns see it: its rank and suit (numbered from 1)
		// and the worths it may count for, lowest first.
		struct Scored
		{
			int rank;
			int suit;
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
		// worths from its top down, else the cards in groups of a rank,
		// bigger groups first, each group by worth; then the rest by worth.
		// With `only` (from 1), that pattern alone is tried: -1 when the
		// cards do not make it.
		int bestPattern(const RawPatterns& set, const std::vector<Scored>& cards, std::vector<float>& deciding, int only = -1)
		{
			// Counted in place (ranks and suits are under 64; scorePile
			// sees to that), as this runs thousands of times for the odds.
			std::array<int, 64> ofRank{};
			std::array<int, 64> ofSuit{};
			std::array<float, 64> rankWorth{};
			std::array<int, 64> ranks{};
			std::size_t rankCount = 0;
			int mostSuited = 0;
			std::uint64_t mask = 0;
			bool byBits = true;
			for (const Scored& card : cards)
			{
				const auto rank = static_cast<std::size_t>(card.rank);
				if (ofRank[rank]++ == 0) { ranks[rankCount++] = card.rank; }
				mostSuited = std::max(mostSuited, ++ofSuit[static_cast<std::size_t>(card.suit)]);
				rankWorth[rank] = highest(card);
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
			// Ranks by how many, then by worth, highest first.
			const auto byGroup = [&](int a, int b)
			{
				const auto ia = static_cast<std::size_t>(a);
				const auto ib = static_cast<std::size_t>(b);
				return ofRank[ia] != ofRank[ib] ? ofRank[ia] > ofRank[ib] : rankWorth[ia] > rankWorth[ib];
			};
			std::sort(ranks.begin(), ranks.begin() + static_cast<std::ptrdiff_t>(rankCount), byGroup);
			const auto inGroups = [&]()
			{
				deciding.clear();
				for (std::size_t i = 0; i < rankCount; ++i)
				{
					const auto rank = static_cast<std::size_t>(ranks[i]);
					deciding.insert(deciding.end(), static_cast<std::size_t>(ofRank[rank]), rankWorth[rank]);
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
				int longestRun = 0;
				bool holds = true;
				// Each <same> a rank of its own, the biggest first: taken
				// in that order, the ranks by group size serve them best.
				std::uint64_t used = 0;
				for (int wanted = static_cast<int>(cards.size()); wanted > 0 && holds; --wanted)
				{
					for (const auto& [part, many] : pattern.parts)
					{
						if (part != RawPatterns::Part::Same || many != wanted) { continue; }
						bool found = false;
						for (std::size_t i = 0; i < rankCount && !found; ++i)
						{
							const auto rank = static_cast<std::size_t>(ranks[i]);
							if (!(used & (std::uint64_t{ 1 } << rank)) && ofRank[rank] >= many)
							{
								used |= std::uint64_t{ 1 } << rank;
								found = true;
							}
						}
						if (!found) { holds = false; break; }
					}
				}
				for (const auto& [part, many] : pattern.parts)
				{
					if (part == RawPatterns::Part::Same && many > static_cast<int>(cards.size())) { holds = false; }
					else if (part == RawPatterns::Part::Suited) { holds = holds && mostSuited >= many; }
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
		float bestOf(const RawPatterns& set, const std::vector<Scored>& cards, std::size_t best, float base, int& made)
		{
			const std::size_t size = best == 0 ? cards.size() : std::min(best, cards.size());
			std::vector<float> deciding;
			// What all the cards make is the most any `size` of them can
			// (every part asks for at least so many), so that is tried
			// first, and only when none of them makes it, every pattern.
			made = bestPattern(set, cards, deciding);
			if (size == cards.size())
			{
				float strength = static_cast<float>(made);
				for (const float worth : deciding) { strength = strength * base + worth; }
				return strength;
			}
			std::vector<Scored> some;
			float strongest = -1.0f;
			for (const bool onlyTheMost : { true, false })
			{
				std::vector<bool> chosen(cards.size(), false);
				std::fill(chosen.begin(), chosen.begin() + static_cast<std::ptrdiff_t>(size), true);
				int bestMade = 0;
				do
				{
					some.clear();
					for (std::size_t i = 0; i < cards.size(); ++i)
					{
						if (chosen[i]) { some.push_back(cards[i]); }
					}
					const int pattern = bestPattern(set, some, deciding, onlyTheMost ? made : -1);
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

	void Game::scorePile(Object& pile)
	{
		const auto set = std::find_if(patterns.begin(), patterns.end(), [&](const RawPatterns& each) { return each.name == pile.pile.score; });
		if (set == patterns.end()) { return; }
		const auto scored = [](const Object& card)
		{
			std::uint64_t bits = 0;
			for (const float worth : card.card.worths)
			{
				if (worth < 0.0f || worth > 63.0f || worth != std::floor(worth)) { bits = 0; break; }
				bits |= std::uint64_t{ 1 } << static_cast<int>(worth);
			}
			return Scored{ card.card.rank, card.card.suit, &card.card.worths, bits };
		};
		float base = 1.0f;
		for (const Object& object : objects)
		{
			if (!object.isCard) { continue; }
			// Ranks and suits are counted in room for 63 (bestPattern).
			if (object.card.rank >= 64 || object.card.suit >= 64) { return; }
			base = std::max(base, highest(scored(object)) + 1.0f);
		}

		std::vector<Scored> own;
		std::vector<int> seen;
		const auto add = [&](const Object& from)
		{
			for (const int number : from.pile.cards)
			{
				if (const Object* card = cardAt(number))
				{
					own.push_back(scored(*card));
					seen.push_back(number);
				}
			}
		};
		add(pile);
		const std::size_t mine = own.size();
		Object* shared = pile.pile.scoreWith.empty() ? nullptr : pileNamed(pile.pile.scoreWith);
		if (shared) { add(*shared); }
		const std::size_t best = static_cast<std::size_t>(pile.pile.scoreBest);

		int made = 0;
		pile.strength = own.empty() ? 0.0f : bestOf(*set, own, best, base, made);
		pile.pattern = static_cast<float>(made);

		// The odds: as many unseen cards as this pile holds, against it,
		// the shared pile made up to full from the unseen too; the same
		// each time for the same cards (its own generator, seeded by them).
		pile.odds = 0.0f;
		if (mine > 0)
		{
			std::vector<Scored> unseen;
			for (const Object& object : objects)
			{
				if (object.isCard && std::find(seen.begin(), seen.end(), object.cardNumber) == seen.end()) { unseen.push_back(scored(object)); }
			}
			const std::size_t board = own.size() - mine;
			const std::size_t full = static_cast<std::size_t>(pile.pile.scoreFull);
			const std::size_t more = full > board ? full - board : 0;
			if (unseen.size() >= more + mine)
			{
				std::uint64_t seed = 1469598103934665603ull;
				for (const int number : seen) { seed = (seed ^ static_cast<std::uint64_t>(static_cast<unsigned>(number))) * std::uint64_t{ 1099511628211u }; }
				std::mt19937 random(static_cast<std::mt19937::result_type>(seed));
				constexpr int trials = 1000;
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
					const float us = bestOf(*set, ours, best, base, ignored);
					const float them = bestOf(*set, theirs, best, base, ignored);
					won += us > them ? 1.0f : us == them ? 0.5f : 0.0f;
				}
				pile.odds = std::round(won * 100.0f / static_cast<float>(trials));
			}
		}
		refreshBoundTexts(pile.name, "pattern", pile.pattern);
		refreshBoundTexts(pile.name, "strength", pile.strength);
		refreshBoundTexts(pile.name, "odds", pile.odds);
	}

	void Game::refreshCall(Object& pile)
	{
		const Object* top = pile.pile.cards.empty() ? nullptr : cardAt(pile.pile.cards.back());
		pile.calling = (!pile.pile.takes.wildRanks.empty() || !pile.pile.takes.wildRules.empty()) && top && isWild(pile.pile.takes, *top) && pile.called == 0.0f ? 1.0f : 0.0f;
		refreshBoundTexts(pile.name, "calling", pile.calling);
		refreshBoundTexts(pile.name, "called", pile.called);
	}

	int Game::takeTop(Object& pile)
	{
		if (pile.pile.cards.empty()) { return 0; }
		const int number = pile.pile.cards.back();
		pile.pile.cards.pop_back();
		refreshPile(pile);
		return number;
	}

	void Game::showTop(Object& pile)
	{
		if (!pile.pile.showTop || pile.pile.cards.empty()) { return; }
		if (Object* top = cardAt(pile.pile.cards.back()); top && !top->faceUp) { turnCard(*top, true); refreshPile(pile); }
	}

	void Game::dealDecks()
	{
		// Numbered, every deck in turn.
		int number = 0;
		for (Object& object : objects)
		{
			if (object.isCard) { object.cardNumber = ++number; }
		}
		cardIndex.clear();
		if (number == 0) { return; }

		for (Object& object : objects)
		{
			if (object.pile.is) { object.pile.cards.clear(); object.pile.broken.clear(); object.called = 0.0f; refreshPile(object); }
			if (object.isCard) { object.laidBy = 0; }
			if (object.hand.is)
			{
				object.hand.held.clear();
				object.hand.from.clear();
				object.hand.pile.clear();
				object.hand.at = -1;
			}
		}

		// Each deck shuffled into its pile.
		std::map<std::string, std::vector<int>> starts;
		std::vector<std::string> order;
		for (Object& object : objects)
		{
			if (!object.isCard) { continue; }
			if (!starts.count(object.card.startPile)) { order.push_back(object.card.startPile); }
			starts[object.card.startPile].push_back(object.cardNumber);
		}
		for (const std::string& name : order)
		{
			Object* pile = pileNamed(name);
			if (!pile) { throw std::runtime_error("<deck pile=\"" + name + "\">: there is no <pile> called '" + name + "'"); }
			std::vector<int>& cards = starts[name];
			for (std::size_t i = cards.size(); i > 1; --i)
			{
				const std::size_t j = std::min(i - 1, static_cast<std::size_t>(randomBetween(0.0f, static_cast<float>(i))));
				std::swap(cards[i - 1], cards[j]);
			}
			// Stacked (tests): those cards last, so on top, the first last of all.
			for (auto each = stackedDeal.rbegin(); each != stackedDeal.rend(); ++each)
			{
				const auto at = std::find(cards.begin(), cards.end(), *each);
				if (at == cards.end()) { continue; }
				cards.erase(at);
				cards.push_back(*each);
			}
			for (const int card : cards)
			{
				if (Object* each = cardAt(card)) { turnCard(*each, false); }
				putOnPile(*pile, card);
			}

			// Dealt in rounds, a card to each pile that wants one more.
			std::vector<std::pair<Object*, int>> wanting;
			std::map<std::string, int> inGroup;
			for (Object& object : objects)
			{
				if (!object.pile.is || &object == pile) { continue; }
				int dealt = object.pile.dealt;
				if (!object.groupName.empty())
				{
					dealt += object.pile.dealtMore * inGroup[object.groupName];
					++inGroup[object.groupName];
				}
				if (dealt > 0) { wanting.emplace_back(&object, dealt); }
			}
			for (int round = 1; !wanting.empty() && !pile->pile.cards.empty(); ++round)
			{
				bool any = false;
				for (auto& [each, count] : wanting)
				{
					if (count < round || pile->pile.cards.empty()) { continue; }
					putOnPile(*each, takeTop(*pile));
					any = true;
				}
				if (!any) { break; }
			}
			for (auto& [each, count] : wanting) { showTop(*each); }
		}

		stackedDeal.clear();

		// A wild card turned up to start: its own suit is the one called.
		for (Object& object : objects)
		{
			if (object.pile.is && object.calling > 0.0f)
			{
				const Object* top = cardAt(object.pile.cards.back());
				const std::string& named = object.pile.takes.wildVariable;
				const auto value = named.empty() ? top->variable.end() : top->variable.find(named);
				object.called = value == top->variable.end() ? static_cast<float>(top->card.suit) : value->second;
				refreshCall(object);
			}
		}

		// Each hand at its own pile, or else the first pile.
		for (Object& object : objects)
		{
			if (!object.hand.is) { continue; }
			Object* own = object.hand.own.empty() ? nullptr : pileNamed(object.hand.own);
			for (Object& pile : objects)
			{
				if (!pile.pile.is || (own && &pile != own)) { continue; }
				object.hand.pile = pile.name;
				object.hand.at = static_cast<int>(pile.pile.cards.size()) - 1;
				break;
			}
		}
		layOutCards();
	}

	// Where each card of a pile goes: its place, then each card on from the
	// one below by the fan (face up) or the hidden fan (face down).
	Vector2f Game::cardPlace(const Object& pile, std::size_t index)
	{
		const std::size_t count = pile.pile.cards.size();
		const auto step = [&](std::size_t i)
		{
			const Object* card = cardAt(pile.pile.cards[i]);
			return card && card->faceUp ? pile.pile.fan : pile.pile.hidden;
		};
		// A fan never wider than its <width>: the steps across shrink.
		float across = 1.0f;
		if (pile.pile.fanWidth > 0.0f && count > 1)
		{
			float wide = 0.0f;
			for (std::size_t i = 0; i + 1 < count; ++i) { wide += std::fabs(step(i).x); }
			const Object* first = cardAt(pile.pile.cards.front());
			const float room = pile.pile.fanWidth - (first ? sizeOf(*first).x : 0.0f);
			if (wide > room && wide > 0.0f) { across = std::max(0.0f, room) / wide; }
		}
		Vector2f at = pile.position;
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
		for (Object& pile : objects)
		{
			if (!pile.pile.is) { continue; }
			for (std::size_t i = 0; i < pile.pile.cards.size(); ++i)
			{
				Object* card = cardAt(pile.pile.cards[i]);
				if (!card) { continue; }
				card->position = cardPlace(pile, i);
				card->drawOrder = order++;
			}
		}

		for (Object& hand : objects)
		{
			if (!hand.hand.is) { continue; }
			Object* pile = hand.hand.pile.empty() ? nullptr : pileNamed(hand.hand.pile);
			// A hand pointing into its own pile, emptied then dealt to again:
			// at its first card.
			if (pile && hand.hand.at < 0 && !pile->pile.cards.empty() && !hand.hand.own.empty() && pileNamed(hand.hand.own) == pile) { hand.hand.at = 0; }
			Vector2f pointed = pile ? pile->position : hand.position;
			Vector2f size = pile ? sizeOf(*pile) : Vector2f{};
			if (pile && hand.hand.at >= 0 && static_cast<std::size_t>(hand.hand.at) < pile->pile.cards.size())
			{
				pointed = cardPlace(*pile, static_cast<std::size_t>(hand.hand.at));
				if (Object* card = cardAt(pile->pile.cards[static_cast<std::size_t>(hand.hand.at)]))
				{
					size = sizeOf(*card);
					// A pile with a <lift>: a face-up card pointed at is lifted
					// out of the fan, so all of it shows.
					if (card->faceUp && pile->pile.lift != 0.0f)
					{
						pointed.y -= pile->pile.lift;
						card->position = pointed;
					}
				}
			}
			// The hand around what it points at.
			const Vector2f own = sizeOf(hand);
			hand.position = pointed + (size - own) * 0.5f;

			// What it holds: following the pointer, or laid as it would be
			// on the pile pointed at, a little off it.
			Vector2f at = pointed + Vector2f{ 6.0f, 6.0f };
			if (pile && !pile->pile.cards.empty()) { at = cardPlace(*pile, pile->pile.cards.size() - 1) + pile->pile.fan + Vector2f{ 6.0f, 6.0f }; }
			if (hand.hand.mouse && hand.pointer.x >= 0.0f && hand.pointer.y >= 0.0f) { at = hand.pointer - hand.hand.grab; }
			for (const int number : hand.hand.held)
			{
				Object* card = cardAt(number);
				if (!card) { continue; }
				card->position = at;
				card->drawOrder = 100000 + order++;
				at += pile ? pile->pile.fan : Vector2f{ 0.0f, 20.0f };
				if (pile && pile->pile.fan.x == 0.0f && pile->pile.fan.y == 0.0f) { at += Vector2f{ 0.0f, 20.0f }; }
			}
			hand.drawOrder = 200000;
		}
	}

	// Whether `cards` (the lowest first) may go on the pile, by its <takes>.
	bool Game::pileTakes(Object& pile, const std::vector<int>& cards, const Object* holder)
	{
		const PileRule& rule = pile.pile.takes;
		if (!rule.given || cards.empty()) { return false; }
		// <sum>: the cards laid, added up by their worths.
		if (rule.sum > 0)
		{
			float total = 0.0f;
			for (const int number : cards)
			{
				const Object* card = cardAt(number);
				if (!card) { return false; }
				total += card->card.worths.empty() ? static_cast<float>(card->card.rank) : card->card.worths.front();
			}
			return total == static_cast<float>(rule.sum);
		}
		// <total variable>N: what is laid together, that variable added up, is N.
		for (const PileRule::VarRule& each : rule.rules)
		{
			if (each.kind != PileRule::VarRule::Kind::Total) { continue; }
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
		// A trick: what the hand laying holds (its own pile) decides.
		if (rule.follow)
		{
			const auto holds = [&](const auto& test)
			{
				if (!holder) { return false; }
				for (const int number : holder->pile.cards)
				{
					const Object* card = cardAt(number);
					if (card && number != first->cardNumber && test(*card)) { return true; }
				}
				return false;
			};
			if (pile.pile.cards.empty())
			{
				const auto unbroken = [&](const Object& card)
				{
					return std::find(rule.breakSuits.begin(), rule.breakSuits.end(), card.card.suitName) != rule.breakSuits.end()
						&& std::find(pile.pile.broken.begin(), pile.pile.broken.end(), card.card.suit) == pile.pile.broken.end();
				};
				// A suit not yet broken leads only when the hand holds nothing else.
				if (unbroken(*first) && holds([&](const Object& card) { return !unbroken(card); })) { return false; }
			}
			else if (const Object* lead = cardAt(pile.pile.cards.front()); lead && first->card.suit != lead->card.suit)
			{
				if (holds([&](const Object& card) { return card.card.suit == lead->card.suit; })) { return false; }
			}
		}
		if (pile.pile.cards.empty())
		{
			if (!acceptsBy(rule.emptyRules, *first, nullptr)) { return false; }
			if (!rule.emptyRanks.empty() && std::find(rule.emptyRanks.begin(), rule.emptyRanks.end(), first->card.rankName) == rule.emptyRanks.end()) { return false; }
			if (!rule.emptySuits.empty() && std::find(rule.emptySuits.begin(), rule.emptySuits.end(), first->card.suitName) == rule.emptySuits.end()) { return false; }
			return true;
		}
		// Waiting for a suit to be called: nothing goes on it.
		if (pile.calling > 0.0f) { return false; }
		if (isWild(rule, *first)) { return true; }
		const Object* top = cardAt(pile.pile.cards.back());
		if (!top || !top->faceUp) { return false; }
		if (rule.matches)
		{
			const int suit = pile.called > 0.0f ? static_cast<int>(pile.called) : top->card.suit;
			if (first->card.rank != top->card.rank && first->card.suit != suit) { return false; }
		}
		// <any>: one of them, a value called standing for the top's own.
		if (!rule.anyRules.empty())
		{
			const float called = rule.wildVariable.empty() ? 0.0f : pile.called;
			const bool any = std::any_of(rule.anyRules.begin(), rule.anyRules.end(), [&](const PileRule::VarRule& each) { return acceptsBy({ each }, *first, top, rule.wildVariable, called); });
			if (!any) { return false; }
		}
		return follows(rule, *top, *first);
	}

	// Whether `next` may lie on `below` by the rule (not counting <one />).
	bool Game::follows(const PileRule& rule, const Object& below, const Object& next)
	{
		if (rule.sameSuit && (next.card.suit != below.card.suit || next.card.deck != below.card.deck)) { return false; }
		if (rule.otherColor && next.card.color == below.card.color) { return false; }
		if (rule.up && next.card.rank != below.card.rank + 1) { return false; }
		if (rule.down && next.card.rank != below.card.rank - 1) { return false; }
		return acceptsBy(rule.rules, next, &below);
	}

	// Whether `next` keeps an <accepts>'s rules on its variables, against
	// the one under it (`below`; none for the first on an empty holder).
	bool Game::acceptsBy(const std::vector<PileRule::VarRule>& rules, const Object& next, const Object* below, const std::string& called, float value)
	{
		using Kind = PileRule::VarRule::Kind;
		const auto valueOf = [](const Object& of, const std::string& name)
		{
			const auto found = of.variable.find(name);
			return found == of.variable.end() ? 0.0f : found->second;
		};
		for (const PileRule::VarRule& each : rules)
		{
			if (each.kind == Kind::Total) { continue; }   // what is laid together, added up (pileTakes)
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

	// Whether a card of a pile lies under another pile's card drawn over it.
	bool Game::isCovered(const Object& card, const Object& own)
	{
		const Vector2f size = sizeOf(card);
		for (const Object& pile : objects)
		{
			if (!pile.pile.is || &pile == &own || !isShown(pile)) { continue; }
			for (const int number : pile.pile.cards)
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
		Object* pile = pileNamed(hand.hand.pile);
		if (!pile || pile->pile.deals > 0 || pile->pile.cards.empty()) { return false; }
		// Covering: a top card that a card of another pile, drawn over it,
		// overlaps stays where it is.
		if (const Object* top = cardAt(pile->pile.cards.back()); top && isCovered(*top, *pile)) { return false; }
		// Holding already: the top card of another pile too, while the hand
		// <holds> cards from that many piles.
		if (!hand.hand.held.empty())
		{
			const auto taken = [&](const std::string& name) { return name == hand.hand.from || std::any_of(hand.hand.also.begin(), hand.hand.also.end(), [&](const auto& each) { return each.second == name; }); };
			if (static_cast<int>(hand.hand.also.size()) + 1 >= hand.hand.holds || taken(pile->name)) { return false; }
			const int number = pile->pile.cards.back();
			const Object* top = cardAt(number);
			if (!top || !top->faceUp) { return false; }
			pile->pile.cards.pop_back();
			refreshPile(*pile);
			hand.hand.held.push_back(number);
			hand.hand.also.emplace_back(number, pile->name);
			layOutCards();
			return true;
		}
		const int count = static_cast<int>(pile->pile.cards.size());
		const int at = hand.hand.at < 0 ? count - 1 : std::min(hand.hand.at, count - 1);
		// A hand with a pile of its own takes from it alone, any one card,
		// face up or down (it knows its own cards).
		if (!hand.hand.own.empty())
		{
			if (pile != pileNamed(hand.hand.own)) { return false; }
			hand.hand.held = { pile->pile.cards[static_cast<std::size_t>(at)] };
			pile->pile.cards.erase(pile->pile.cards.begin() + at);
			refreshPile(*pile);
			hand.hand.from = pile->name;
			hand.hand.at = std::min(at, static_cast<int>(pile->pile.cards.size()) - 1);
			if (hand.hand.mouse && hand.pointer.x >= 0.0f && hand.pointer.y >= 0.0f)
			{
				hand.hand.grab = hand.pointer - cardPlace(*pile, static_cast<std::size_t>(std::max(0, hand.hand.at)));
			}
			layOutCards();
			return true;
		}
		if (at < count - 1 && !pile->pile.carry) { return false; }
		// Face up, and each on the one below it by the pile's own rule.
		for (int i = at; i < count; ++i)
		{
			const Object* card = cardAt(pile->pile.cards[static_cast<std::size_t>(i)]);
			if (!card || !card->faceUp) { return false; }
			if (i > at && !follows(pile->pile.takes, *cardAt(pile->pile.cards[static_cast<std::size_t>(i - 1)]), *card)) { return false; }
		}
		hand.hand.held.assign(pile->pile.cards.begin() + at, pile->pile.cards.end());
		pile->pile.cards.resize(static_cast<std::size_t>(at));
		refreshPile(*pile);
		hand.hand.from = pile->name;
		hand.hand.at = at - 1;
		// Held where the pointer took them.
		if (hand.hand.mouse && hand.pointer.x >= 0.0f && hand.pointer.y >= 0.0f)
		{
			hand.hand.grab = hand.pointer - cardPlace(*pile, static_cast<std::size_t>(at));
		}
		layOutCards();
		return true;
	}

	bool Game::pickCards(Object& hand)
	{
		// From a holder with <picked>: it sends instead, and nothing is
		// picked up, so the picker's verbs after <pick /> do not run.
		const Object* pile = hand.hand.is ? pileNamed(hand.hand.pile) : nullptr;
		if (pile && pile->pile.deals > 0)
		{
			const std::string sound = pile->pile.pickedSound;
			if (dealCards(hand, "", "") && !sound.empty()) { requestSound(sound); }
			return false;
		}
		return takeCards(hand);
	}

	// The cards held go on the pile, the top one pointed at, and the pile
	// they came from shows its top.
	void Game::layHeld(Object& hand, Object& pile)
	{
		for (const int number : hand.hand.held)
		{
			if (Object* card = cardAt(number)) { card->laidBy = hand.hand.seat; }
			putOnPile(pile, number);
		}
		hand.hand.held.clear();
		if (Object* from = pileNamed(hand.hand.from); from && from != &pile) { showTop(*from); }
		hand.hand.from.clear();
		for (const auto& [number, name] : hand.hand.also)
		{
			if (Object* from = pileNamed(name); from && from != &pile) { showTop(*from); }
		}
		hand.hand.also.clear();
		// A hand with a pile of its own stays pointing into it.
		if (Object* own = hand.hand.own.empty() ? nullptr : pileNamed(hand.hand.own))
		{
			hand.hand.pile = own->name;
			hand.hand.at = std::min(std::max(hand.hand.at, 0), static_cast<int>(own->pile.cards.size()) - 1);
		}
		else
		{
			hand.hand.pile = pile.name;
			hand.hand.at = static_cast<int>(pile.pile.cards.size()) - 1;
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
			if (Object* pile = pileNamed(hand.hand.pile)) { choices.push_back(pile); }
		}
		else
		{
			const std::string full = fullName(onto);
			for (Object& object : objects)
			{
				if (object.pile.is && object.named(full) && isShown(object)) { choices.push_back(&object); }
			}
		}
		for (Object* pile : choices)
		{
			// Back where they came from: put back, always allowed.
			if (pile->name == hand.hand.from && onto.empty())
			{
				layHeld(hand, *pile);
				return true;
			}
			if (pileTakes(*pile, hand.hand.held, hand.hand.own.empty() ? nullptr : pileNamed(hand.hand.own)))
			{
				layHeld(hand, *pile);
				return true;
			}
		}
		return false;
	}

	bool Game::dropCards(Object& hand)
	{
		if (!hand.hand.is || hand.hand.held.empty()) { return false; }
		Object* from = pileNamed(hand.hand.from);
		if (!from) { return false; }
		const std::string pointed = hand.hand.pile;
		const int at = hand.hand.at;
		const int card = hand.hand.held.front();
		// Cards taken from other piles go back to theirs first.
		for (const auto& [number, name] : hand.hand.also)
		{
			hand.hand.held.erase(std::find(hand.hand.held.begin(), hand.hand.held.end(), number));
			if (Object* back = pileNamed(name)) { putOnPile(*back, number); }
		}
		hand.hand.also.clear();
		layHeld(hand, *from);
		// Back in a hand's own pile: pointing at it again.
		if (!hand.hand.own.empty() && from == pileNamed(hand.hand.own))
		{
			const auto found = std::find(from->pile.cards.begin(), from->pile.cards.end(), card);
			hand.hand.pile = from->name;
			hand.hand.at = static_cast<int>(found - from->pile.cards.begin());
			layOutCards();
			return true;
		}
		// Put back, the hand stays where it was.
		hand.hand.pile = pointed;
		hand.hand.at = at;
		if (Object* pile = pileNamed(pointed)) { hand.hand.at = std::min(at < 0 ? static_cast<int>(pile->pile.cards.size()) - 1 : at, static_cast<int>(pile->pile.cards.size()) - 1); }
		layOutCards();
		return true;
	}

	bool Game::dealCards(Object& hand, const std::string& fromPile, const std::string& faces)
	{
		if (!hand.hand.is || !hand.hand.held.empty() || (fromPile.empty() && hand.hand.mouse && !hand.hand.over)) { return false; }
		// From the pile named, or else the one pointed at.
		Object* pile = pileNamed(fromPile.empty() ? hand.hand.pile : fromPile);
		if (!pile || pile->pile.deals <= 0) { return false; }
		// Into the pile it names, or else the hand's own.
		const std::string& intoName = pile->pile.dealsInto.empty() ? hand.hand.own : pile->pile.dealsInto;
		Object* into = intoName.empty() ? nullptr : pileNamed(intoName);
		if (!into) { return false; }
		if (!pile->pile.cards.empty())
		{
			for (int i = 0; i < pile->pile.deals && !pile->pile.cards.empty(); ++i)
			{
				const int number = takeTop(*pile);
				putOnPile(*into, number);
				if (Object* card = cardAt(number); card && !faces.empty())
				{
					turnCard(*card, faces == "up");
					refreshPile(*into);
				}
			}
		}
		else
		{
			// Empty: the other pile turned back over onto it (or shuffled),
			// but for the top cards it leaves.
			Object* from = pile->pile.refillFrom.empty() ? nullptr : pileNamed(pile->pile.refillFrom);
			const std::size_t leave = static_cast<std::size_t>(pile->pile.refillLeave);
			if (!from || from->pile.cards.size() <= leave) { return false; }
			const auto end = from->pile.cards.end() - static_cast<std::ptrdiff_t>(leave);
			std::vector<int> cards(from->pile.cards.begin(), end);
			from->pile.cards.erase(from->pile.cards.begin(), end);
			refreshPile(*from);
			if (pile->pile.refillShuffle)
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
			for (const int card : cards) { putOnPile(*pile, card); }
		}
		// Pointing at the stock, still pointing at its top.
		if (fromPile.empty()) { hand.hand.at = static_cast<int>(pile->pile.cards.size()) - 1; }
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
		Object* into = pileNamed(intoName);
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
		if (Object* from = pileNamed(fromName))
		{
			if (from == into || from->pile.cards.empty()) { return false; }
			const std::size_t many = all ? from->pile.cards.size() : std::min(from->pile.cards.size(), static_cast<std::size_t>(count));
			// The top ones, the topmost first.
			for (std::size_t i = 0; i < many; ++i) { sent.push_back(takeTop(*from)); }
		}
		else
		{
			// A group's name: every member, wherever it is.
			if (!all) { return false; }
			for (Object& object : objects)
			{
				if (object.isCard && (object.inGroup(fromName) || object.card.deck == fromName || object.inGroup(fullName(fromName)))) { sent.push_back(object.cardNumber); }
			}
			if (sent.empty()) { return false; }
			for (Object& object : objects)
			{
				if (object.pile.is)
				{
					auto& cards = object.pile.cards;
					const auto gone = std::remove_if(cards.begin(), cards.end(), [&](int number) { return std::find(sent.begin(), sent.end(), number) != sent.end(); });
					if (gone != cards.end())
					{
						cards.erase(gone, cards.end());
						object.pile.broken.clear();
						object.called = 0.0f;
						refreshPile(object);
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
			putOnPile(*into, number);
			if (card && !sprite.empty())
			{
				if (const auto up = faceFor(*card, sprite)) { turnCard(*card, *up); }
			}
		}
		refreshPile(*into);
		// A hand pointing where cards were: at its own, or at that holder.
		for (Object& object : objects)
		{
			if (!object.hand.is) { continue; }
			Object* pointed = pileNamed(object.hand.pile);
			if (pointed && object.hand.at >= static_cast<int>(pointed->pile.cards.size())) { object.hand.at = static_cast<int>(pointed->pile.cards.size()) - 1; }
		}
		layOutCards();
		return true;
	}

	bool Game::orderHeld(const std::string& name)
	{
		Object* holder = pileNamed(name);
		if (!holder) { return false; }
		std::vector<int>& cards = holder->pile.cards;
		for (std::size_t i = cards.size(); i > 1; --i)
		{
			const std::size_t j = std::min(i - 1, static_cast<std::size_t>(randomBetween(0.0f, static_cast<float>(i))));
			std::swap(cards[i - 1], cards[j]);
		}
		// Stacked (tests): those cards last, so on top, the first last of all.
		for (auto each = stackedDeal.rbegin(); each != stackedDeal.rend(); ++each)
		{
			const auto at = std::find(cards.begin(), cards.end(), *each);
			if (at == cards.end()) { continue; }
			cards.erase(at);
			cards.push_back(*each);
		}
		stackedDeal.clear();
		refreshPile(*holder);
		layOutCards();
		return true;
	}

	bool Game::turnHeld(Object& holder, const std::string& sprite)
	{
		for (const int number : holder.pile.cards)
		{
			Object* card = cardAt(number);
			if (!card) { continue; }
			const auto up = faceFor(*card, sprite);
			if (!up) { return false; }
			if (card->faceUp != *up) { turnCard(*card, *up); }
		}
		refreshPile(holder);
		layOutCards();
		return true;
	}

	// Every pile in play, and its middle and size as drawn.
	std::vector<Object*> Game::shownPiles()
	{
		std::vector<Object*> piles;
		for (Object& object : objects)
		{
			if (object.pile.is && isShown(object)) { piles.push_back(&object); }
		}
		return piles;
	}

	bool Game::hopHand(Object& hand, Direction direction)
	{
		Object* pile = pileNamed(hand.hand.pile);
		const std::vector<Object*> piles = shownPiles();
		if (piles.empty()) { return false; }
		if (!pile) { pile = piles.front(); }
		const int count = static_cast<int>(pile->pile.cards.size());
		const bool own = !hand.hand.own.empty() && pile == pileNamed(hand.hand.own);
		const auto takeable = [&](int index)
		{
			if (index < 0 || index >= count) { return false; }
			const Object* card = cardAt(pile->pile.cards[static_cast<std::size_t>(index)]);
			return card && (card->faceUp || own);
		};
		const auto pointAt = [&](Object& to)
		{
			hand.hand.pile = to.name;
			hand.hand.at = static_cast<int>(to.pile.cards.size()) - 1;
			hand.hand.mouse = false;
			layOutCards();
			return true;
		};

		hand.facing = direction;
		// A pile fanned across: left and right go through its cards first
		// (any of a hand's own, else a run's face-up ones).
		const bool fannedAcross = std::fabs(pile->pile.fan.x) > std::fabs(pile->pile.fan.y);
		if (fannedAcross && (direction == Direction::Left || direction == Direction::Right) && (own || pile->pile.carry))
		{
			const int at = hand.hand.at < 0 ? count - 1 : hand.hand.at;
			const int next = at + ((direction == Direction::Right) == (pile->pile.fan.x > 0.0f) ? 1 : -1);
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
			// The next pile in the row, at the same height.
			const float ahead = direction == Direction::Right ? 1.0f : -1.0f;
			Object* best = nullptr;
			for (Object* each : piles)
			{
				if (each == pile || std::fabs(each->position.y - pile->position.y) > 0.5f) { continue; }
				const float along = (each->position.x - pile->position.x) * ahead;
				if (along <= 0.0f) { continue; }
				if (!best || along < (best->position.x - pile->position.x) * ahead) { best = each; }
			}
			return best && pointAt(*best);
		}

		// Up and down: through a run's face-up cards, then to the row of
		// piles above or below, the pile nearest across.
		const bool up = direction == Direction::Up;
		const int at = hand.hand.at < 0 ? count - 1 : hand.hand.at;
		if (!fannedAcross && pile->pile.carry && up && takeable(at - 1) && takeable(at))
		{
			hand.hand.at = at - 1;
			hand.hand.mouse = false;
			layOutCards();
			return true;
		}
		if (!fannedAcross && pile->pile.carry && !up && at < count - 1)
		{
			hand.hand.at = at + 1;
			hand.hand.mouse = false;
			layOutCards();
			return true;
		}
		Object* best = nullptr;
		float bestRow = 0.0f;
		float bestAcross = 0.0f;
		for (Object* each : piles)
		{
			const float row = (each->position.y - pile->position.y) * (up ? -1.0f : 1.0f);
			if (row <= 0.5f) { continue; }
			const float across = std::fabs(each->position.x - pile->position.x);
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
		// The top card under the pointer (of piles overlapping, the one
		// drawn over), else the pile's place.
		Object* found = nullptr;
		int foundAt = -1;
		int foundOrder = -1;
		for (Object* pile : shownPiles())
		{
			bool hit = false;
			int index = -1;
			for (int i = static_cast<int>(pile->pile.cards.size()) - 1; i >= 0 && !hit; --i)
			{
				const Object* card = cardAt(pile->pile.cards[static_cast<std::size_t>(i)]);
				if (card && inside(cardPlace(*pile, static_cast<std::size_t>(i)), sizeOf(*card)))
				{
					hit = true;
					index = i;
				}
			}
			if (!hit && inside(pile->position, sizeOf(*pile))) { hit = true; }
			if (!hit) { continue; }
			const Object* card = index >= 0 ? cardAt(pile->pile.cards[static_cast<std::size_t>(index)]) : nullptr;
			const int order = card ? card->drawOrder : 0;
			if (found && order <= foundOrder) { continue; }
			found = pile;
			foundAt = index;
			foundOrder = order;
		}
		if (found)
		{
			// Holding: the pile, its top card.
			if (!hand.hand.held.empty() && hand.hand.holds <= 1) { foundAt = static_cast<int>(found->pile.cards.size()) - 1; }
			hand.hand.pile = found->name;
			hand.hand.at = foundAt;
			hand.hand.over = true;
		}
		layOutCards();
	}

	bool Game::callSuit(Object& hand, const std::string& name)
	{
		if (!hand.hand.is || !hand.hand.held.empty() || hand.hand.own.empty() || (hand.hand.mouse && !hand.hand.over)) { return false; }
		Object* pile = pileNamed(name);
		Object* own = pileNamed(hand.hand.own);
		if (!pile || !own || pile->calling <= 0.0f || pileNamed(hand.hand.pile) != own) { return false; }
		const int count = static_cast<int>(own->pile.cards.size());
		if (count == 0) { return false; }
		const int at = hand.hand.at < 0 ? count - 1 : std::min(hand.hand.at, count - 1);
		const Object* card = cardAt(own->pile.cards[static_cast<std::size_t>(at)]);
		if (!card) { return false; }
		// The value named: the card's suit, or its <wild variable=>.
		const std::string& named = pile->pile.takes.wildVariable;
		const auto value = named.empty() ? card->variable.end() : card->variable.find(named);
		pile->called = value == card->variable.end() ? static_cast<float>(card->card.suit) : value->second;
		refreshCall(*pile);
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
		const bool last = static_cast<int>(trick.pile.cards.size()) + 1 >= std::max(players, 2);
		const Object* winning = trickWinning(trick);
		const auto worth = [](const Object& card) { return card.card.worths.empty() ? static_cast<float>(card.card.rank) : card.card.worths.back(); };

		std::vector<std::pair<int, const Object*>> legal;
		for (int i = 0; i < static_cast<int>(own.pile.cards.size()); ++i)
		{
			const int number = own.pile.cards[static_cast<std::size_t>(i)];
			const Object* card = cardAt(number);
			if (card && pileTakes(trick, { number }, &own)) { legal.emplace_back(i, card); }
		}
		if (legal.empty()) { return -1; }
		const Object* led = trick.pile.cards.empty() ? nullptr : cardAt(trick.pile.cards.front());
		const bool following = led && std::any_of(legal.begin(), legal.end(), [&](const auto& each) { return each.second->card.suit == led->card.suit; });
		const bool ducking = following && std::any_of(legal.begin(), legal.end(), [&](const auto& each) { return worth(*each.second) < worth(*winning); });
		const PileRule& rule = trick.pile.takes;
		float top = 0.0f;
		for (const Object& object : objects)
		{
			if (object.isCard && !object.card.worths.empty()) { top = std::max(top, object.card.worths.back()); }
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

	std::optional<KeyCode> Game::thinkCards(const CmdThink& think, Object& hand)
	{
		Object* own = hand.hand.own.empty() ? nullptr : pileNamed(hand.hand.own);
		if (!own || !hand.hand.held.empty()) { return std::nullopt; }
		if (cardWait > 0)
		{
			--cardWait;
			return std::nullopt;
		}
		constexpr int pace = 30;

		std::map<int, int> inSuit;
		for (const int number : own->pile.cards)
		{
			if (const Object* card = cardAt(number)) { ++inSuit[card->card.suit]; }
		}
		std::vector<Object*> targets;
		Object* stock = nullptr;
		for (Object* pile : shownPiles())
		{
			if (pile == own) { continue; }
			if (pile->pile.takes.given) { targets.push_back(pile); }
			if (!stock && pile->pile.deals > 0) { stock = pile; }
		}

		Object* aim = nullptr;
		int index = -1;
		const bool calling = std::any_of(targets.begin(), targets.end(), [](const Object* pile) { return pile->calling > 0.0f; });
		const auto trick = std::find_if(targets.begin(), targets.end(), [](const Object* pile) { return pile->pile.takes.follow; });
		if (!own->pile.melds.empty())
		{
			// Melds: holding no more than its best, it draws, the face-up top
			// of a pile when that leaves less deadwood, else the face-down
			// one; holding more, it lets go of the card leaving the least
			// (the one worth most, of two alike).
			const int best = own->pile.meldsBest > 0 ? own->pile.meldsBest : static_cast<int>(own->pile.cards.size());
			if (static_cast<int>(own->pile.cards.size()) <= best)
			{
				Object* up = nullptr;
				Object* down = nullptr;
				for (Object* pile : shownPiles())
				{
					if (pile == own || pile->pile.deals <= 0 || pile->pile.cards.empty()) { continue; }
					const Object* top = cardAt(pile->pile.cards.back());
					if (top && top->faceUp) { up = up ? up : pile; }
					else { down = down ? down : pile; }
				}
				aim = down ? down : up;
				if (up && down)
				{
					std::vector<int> with = own->pile.cards;
					with.push_back(up->pile.cards.back());
					if (meldDeadwood(*own, with, best, nullptr) < meldDeadwood(*own, own->pile.cards, best, nullptr)) { aim = up; }
				}
				if (aim) { index = static_cast<int>(aim->pile.cards.size()) - 1; }
			}
			else
			{
				float least = 0.0f;
				float worth = 0.0f;
				for (int i = 0; i < static_cast<int>(own->pile.cards.size()); ++i)
				{
					std::vector<int> rest = own->pile.cards;
					rest.erase(rest.begin() + i);
					const float left = meldDeadwood(*own, rest, 0, nullptr);
					const float points = cardAt(own->pile.cards[static_cast<std::size_t>(i)]) ? cardPoints(*own, *cardAt(own->pile.cards[static_cast<std::size_t>(i)])) : 0.0f;
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
			for (int i = 0; i < static_cast<int>(own->pile.cards.size()); ++i)
			{
				const Object* card = cardAt(own->pile.cards[static_cast<std::size_t>(i)]);
				if (card && inSuit[card->card.suit] > most)
				{
					most = inSuit[card->card.suit];
					aim = own;
					index = i;
				}
			}
		}
		else
		{
			float best = 0.0f;
			for (int i = 0; i < static_cast<int>(own->pile.cards.size()); ++i)
			{
				const int number = own->pile.cards[static_cast<std::size_t>(i)];
				const Object* card = cardAt(number);
				if (!card) { continue; }
				for (Object* pile : targets)
				{
					if (!pileTakes(*pile, { number }, own)) { continue; }
					const float worth = isWild(pile->pile.takes, *card) ? 1.0f : 100.0f + static_cast<float>(inSuit[card->card.suit] * 20 + card->card.rank);
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
				index = static_cast<int>(stock->pile.cards.size()) - 1;
			}
		}
		if (!aim) { return std::nullopt; }

		cardWait = pace;
		if (hand.hand.pile != aim->name || hand.hand.at != index)
		{
			hand.hand.pile = aim->name;
			hand.hand.at = index;
			hand.hand.mouse = false;
			layOutCards();
			return std::nullopt;
		}
		return think.button;
	}
}
