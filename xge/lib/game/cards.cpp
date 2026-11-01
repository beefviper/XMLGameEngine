// cards.cpp
// XML Game Engine
// author: beefviper
// date: Nov 1, 2026
//
// Cards: a <deck>'s card objects (addDeck), and the game's piles and hands
// (Game): the deal at the start, a hand pointing at piles and taking and
// laying cards by each pile's rules, and where every card is drawn.

#include "game/cards.h"
#include "game/game.h"
#include "game/random.h"
#include "play/verbs.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <stdexcept>

namespace xge
{
	namespace
	{
		// A small pixel font for the rank labels, three wide and five high.
		const std::map<char, std::array<const char*, 5>>& font()
		{
			static const std::map<char, std::array<const char*, 5>> glyphs{
				{ '0', { "###", "#.#", "#.#", "#.#", "###" } }, { '1', { ".#.", "##.", ".#.", ".#.", "###" } },
				{ '2', { "###", "..#", "###", "#..", "###" } }, { '3', { "###", "..#", ".##", "..#", "###" } },
				{ '4', { "#.#", "#.#", "###", "..#", "..#" } }, { '5', { "###", "#..", "###", "..#", "###" } },
				{ '6', { "###", "#..", "###", "#.#", "###" } }, { '7', { "###", "..#", ".#.", ".#.", ".#." } },
				{ '8', { "###", "#.#", "###", "#.#", "###" } }, { '9', { "###", "#.#", "###", "..#", "###" } },
				{ 'A', { ".#.", "#.#", "###", "#.#", "#.#" } }, { 'B', { "##.", "#.#", "##.", "#.#", "##." } },
				{ 'C', { ".##", "#..", "#..", "#..", ".##" } }, { 'D', { "##.", "#.#", "#.#", "#.#", "##." } },
				{ 'E', { "###", "#..", "##.", "#..", "###" } }, { 'F', { "###", "#..", "##.", "#..", "#.." } },
				{ 'G', { ".##", "#..", "#.#", "#.#", ".##" } }, { 'H', { "#.#", "#.#", "###", "#.#", "#.#" } },
				{ 'I', { "###", ".#.", ".#.", ".#.", "###" } }, { 'J', { "..#", "..#", "..#", "#.#", ".#." } },
				{ 'K', { "#.#", "#.#", "##.", "#.#", "#.#" } }, { 'L', { "#..", "#..", "#..", "#..", "###" } },
				{ 'M', { "#.#", "###", "###", "#.#", "#.#" } }, { 'N', { "##.", "#.#", "#.#", "#.#", "#.#" } },
				{ 'O', { ".#.", "#.#", "#.#", "#.#", ".#." } }, { 'P', { "##.", "#.#", "##.", "#..", "#.." } },
				{ 'Q', { ".#.", "#.#", "#.#", "##.", ".##" } }, { 'R', { "##.", "#.#", "##.", "#.#", "#.#" } },
				{ 'S', { ".##", "#..", ".#.", "..#", "##." } }, { 'T', { "###", ".#.", ".#.", ".#.", ".#." } },
				{ 'U', { "#.#", "#.#", "#.#", "#.#", "###" } }, { 'V', { "#.#", "#.#", "#.#", "#.#", ".#." } },
				{ 'W', { "#.#", "#.#", "###", "###", "#.#" } }, { 'X', { "#.#", "#.#", ".#.", "#.#", "#.#" } },
				{ 'Y', { "#.#", "#.#", ".#.", ".#.", ".#." } }, { 'Z', { "###", "..#", ".#.", "#..", "###" } },
			};
			return glyphs;
		}

		// A picture being built: rows of characters, '.' clear.
		struct Canvas
		{
			int width{};
			int height{};
			std::vector<std::string> rows;

			Canvas(int w, int h, char fill) : width(w), height(h), rows(static_cast<std::size_t>(h), std::string(static_cast<std::size_t>(w), fill)) {}

			void set(int x, int y, char c)
			{
				if (x >= 0 && y >= 0 && x < width && y < height) { rows[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] = c; }
			}

			// Another picture's characters (not its clear ones) at x, y, each
			// pixel `grow` by `grow`.
			void paste(const Canvas& other, int x, int y, int grow = 1)
			{
				for (int r = 0; r < other.height; ++r)
				{
					for (int c = 0; c < other.width; ++c)
					{
						const char each = other.rows[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];
						if (each == '.') { continue; }
						for (int dy = 0; dy < grow; ++dy)
						{
							for (int dx = 0; dx < grow; ++dx) { set(x + c * grow + dx, y + r * grow + dy, each); }
						}
					}
				}
			}

			Canvas turned() const
			{
				Canvas half(width, height, '.');
				for (int r = 0; r < height; ++r)
				{
					for (int c = 0; c < width; ++c) { half.set(width - 1 - c, height - 1 - r, rows[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)]); }
				}
				return half;
			}
		};

		// The card's shape: `fill` inside a one-pixel `edge`, its corners cut.
		Canvas blank(int width, int height, char fill, char edge)
		{
			Canvas card(width, height, fill);
			for (int x = 0; x < width; ++x)
			{
				card.set(x, 0, edge);
				card.set(x, height - 1, edge);
			}
			for (int y = 0; y < height; ++y)
			{
				card.set(0, y, edge);
				card.set(width - 1, y, edge);
			}
			card.set(0, 0, '.');
			card.set(width - 1, 0, '.');
			card.set(0, height - 1, '.');
			card.set(width - 1, height - 1, '.');
			return card;
		}

		Canvas label(const std::string& text, char ink)
		{
			const int width = text.empty() ? 0 : static_cast<int>(text.size()) * 4 - 1;
			Canvas made(std::max(width, 1), 5, '.');
			for (std::size_t i = 0; i < text.size(); ++i)
			{
				const auto glyph = font().find(text[i]);
				for (int r = 0; r < 5; ++r)
				{
					for (int c = 0; c < 3; ++c)
					{
						if (glyph->second[static_cast<std::size_t>(r)][c] == '#') { made.set(static_cast<int>(i) * 4 + c, r, ink); }
					}
				}
			}
			return made;
		}

		Canvas pip(const RawDeck::Suit& suit)
		{
			Canvas made(static_cast<int>(suit.rows.front().size()), static_cast<int>(suit.rows.size()), '.');
			made.rows = suit.rows;
			return made;
		}

		RawSprite bitmapOf(const std::string& name, const Canvas& canvas, const std::string& color, std::vector<std::pair<char, std::string>> colors, int scale)
		{
			RawSprite sprite;
			sprite.kind = "bitmap";
			sprite.name = name;
			sprite.bitmapRows = canvas.rows;
			sprite.bitmapColors = std::move(colors);
			sprite.color = color;
			sprite.scale = RawValue::expression(std::to_string(scale));
			sprite.hasScale = true;
			return sprite;
		}
	}

	const std::string& cardFontCharacters()
	{
		static const std::string all = []
		{
			std::string chars;
			for (const auto& [c, glyph] : font()) { chars += c; }
			return chars;
		}();
		return all;
	}

	void addDeck(std::vector<RawObject>& out, const RawDeck& deck)
	{
		const std::string where = "<deck name=\"" + deck.name + "\">";
		if (deck.suits.empty() || deck.ranks.empty()) { throw std::runtime_error(where + " needs <suit>s and <rank>s"); }
		std::size_t widest = 0;
		for (const std::string& rank : deck.ranks)
		{
			for (const char c : rank)
			{
				if (!font().count(c)) { throw std::runtime_error(where + ": <rank>" + rank + "</rank> has '" + std::string(1, c) + "'; a rank's label is drawn with " + cardFontCharacters()); }
			}
			widest = std::max(widest, rank.size());
		}

		// The back, shared: the card's shape, a frame inside its edge and a
		// pattern of diamonds inside that.
		Canvas back = blank(deck.width, deck.height, '@', '#');
		for (int x = 2; x < deck.width - 2; ++x)
		{
			back.set(x, 2, '#');
			back.set(x, deck.height - 3, '#');
		}
		for (int y = 2; y < deck.height - 2; ++y)
		{
			back.set(2, y, '#');
			back.set(deck.width - 3, y, '#');
		}
		for (int y = 4; y < deck.height - 4; ++y)
		{
			for (int x = 4; x < deck.width - 4; ++x)
			{
				if ((x + y) % 4 == 0 || (x - y + 64) % 4 == 0) { back.set(x, y, '#'); }
			}
		}
		const RawSprite backSprite = bitmapOf("back", back, deck.backEdge, { { '@', deck.backColor }, { '#', deck.backEdge } }, deck.scale);

		int number = 0;
		for (int pack = 0; pack < std::max(1, deck.copies); ++pack)
		{
			for (std::size_t s = 0; s < deck.suits.size(); ++s)
			{
				const RawDeck::Suit& suit = deck.suits[s];
				const Canvas small = pip(suit);
				for (std::size_t r = 0; r < deck.ranks.size(); ++r)
				{
					// The face: in the top left corner the rank and the pip
					// beside it (seen where cards overlap), the same turned
					// round in the bottom right, and a big pip in the middle.
					Canvas face = blank(deck.width, deck.height, '@', '#');
					const Canvas index = label(deck.ranks[r], '*');
					const int stripHeight = std::max(5, small.height);
					Canvas corner(index.width + 1 + small.width, stripHeight, '.');
					corner.paste(index, 0, 0);
					corner.paste(small, index.width + 1, 0);
					if (corner.width > deck.width - 4 || 2 * stripHeight + 4 > deck.height)
					{
						throw std::runtime_error(where + ": a face of " + std::to_string(deck.width) + " by " + std::to_string(deck.height) + " is too small for the rank '" + deck.ranks[r] + "' and the pip of " + suit.name);
					}
					face.paste(corner, 2, 2);
					face.paste(corner.turned(), deck.width - 2 - corner.width, deck.height - 2 - stripHeight);
					const int roomHigh = deck.height - 2 * (2 + stripHeight) - 2;
					int grow = 3;
					while (grow > 1 && (small.width * grow > deck.width - 4 || small.height * grow > roomHigh)) { --grow; }
					if (small.height * grow <= roomHigh)
					{
						face.paste(small, (deck.width - small.width * grow) / 2, (deck.height - small.height * grow) / 2, grow);
					}

					std::vector<std::pair<char, std::string>> colors{ { '@', deck.faceColor }, { '#', deck.faceEdge } };
					colors.insert(colors.end(), suit.colors.begin(), suit.colors.end());

					RawObject card;
					++number;
					card.name = deck.name + "." + std::to_string(number);
					card.groupName = deck.name;
					card.isCard = true;
					card.card.deck = deck.name;
					card.card.suit = static_cast<int>(s) + 1;
					card.card.rank = static_cast<int>(r) + 1;
					card.card.suitName = suit.name;
					card.card.rankName = deck.ranks[r];
					card.card.color = suit.color;
					card.card.startPile = deck.pile;
					card.looks = { bitmapOf("face", face, suit.color, colors, deck.scale), backSprite };
					card.sprite = card.looks.front();
					card.rawPosition = { RawValue::expression("0"), RawValue::expression("0") };
					card.rawVelocity = { RawValue::expression("0"), RawValue::expression("0") };
					card.variable["suit"] = RawValue::expression(std::to_string(card.card.suit));
					card.variable["rank"] = RawValue::expression(std::to_string(card.card.rank));
					out.push_back(std::move(card));
				}
			}
		}
	}

	// ===================================================================
	// The game's piles and hands
	// ===================================================================

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
		pile.pile.cards.push_back(number);
		pile.cardCount = static_cast<float>(pile.pile.cards.size());
	}

	int Game::takeTop(Object& pile)
	{
		if (pile.pile.cards.empty()) { return 0; }
		const int number = pile.pile.cards.back();
		pile.pile.cards.pop_back();
		pile.cardCount = static_cast<float>(pile.pile.cards.size());
		return number;
	}

	void Game::showTop(Object& pile)
	{
		if (!pile.pile.showTop || pile.pile.cards.empty()) { return; }
		if (Object* top = cardAt(pile.pile.cards.back()); top && !top->faceUp) { turnCard(*top, true); }
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
			if (object.pile.is) { object.pile.cards.clear(); object.cardCount = 0.0f; }
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

		// Each hand at the first pile.
		for (Object& object : objects)
		{
			if (!object.hand.is) { continue; }
			for (Object& pile : objects)
			{
				if (!pile.pile.is) { continue; }
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
		Vector2f at = pile.position;
		for (std::size_t i = 0; i < index && i < pile.pile.cards.size(); ++i)
		{
			const Object* card = cardAt(pile.pile.cards[i]);
			at += card && card->faceUp ? pile.pile.fan : pile.pile.hidden;
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
			Vector2f pointed = pile ? pile->position : hand.position;
			Vector2f size = pile ? sizeOf(*pile) : Vector2f{};
			if (pile && hand.hand.at >= 0 && static_cast<std::size_t>(hand.hand.at) < pile->pile.cards.size())
			{
				pointed = cardPlace(*pile, static_cast<std::size_t>(hand.hand.at));
				if (const Object* card = cardAt(pile->pile.cards[static_cast<std::size_t>(hand.hand.at)])) { size = sizeOf(*card); }
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
	bool Game::pileTakes(Object& pile, const std::vector<int>& cards)
	{
		const PileRule& rule = pile.pile.takes;
		if (!rule.given || cards.empty()) { return false; }
		if (rule.one && cards.size() > 1) { return false; }
		const Object* first = cardAt(cards.front());
		if (!first) { return false; }
		if (pile.pile.cards.empty())
		{
			if (!rule.emptyRanks.empty() && std::find(rule.emptyRanks.begin(), rule.emptyRanks.end(), first->card.rankName) == rule.emptyRanks.end()) { return false; }
			if (!rule.emptySuits.empty() && std::find(rule.emptySuits.begin(), rule.emptySuits.end(), first->card.suitName) == rule.emptySuits.end()) { return false; }
			return true;
		}
		const Object* top = cardAt(pile.pile.cards.back());
		return top && top->faceUp && follows(rule, *top, *first);
	}

	// Whether `next` may lie on `below` by the rule (not counting <one />).
	bool Game::follows(const PileRule& rule, const Object& below, const Object& next)
	{
		if (rule.sameSuit && (next.card.suit != below.card.suit || next.card.deck != below.card.deck)) { return false; }
		if (rule.otherColor && next.card.color == below.card.color) { return false; }
		if (rule.up && next.card.rank != below.card.rank + 1) { return false; }
		if (rule.down && next.card.rank != below.card.rank - 1) { return false; }
		return true;
	}

	bool Game::takeCards(Object& hand)
	{
		if (!hand.hand.is || !hand.hand.held.empty() || (hand.hand.mouse && !hand.hand.over)) { return false; }
		Object* pile = pileNamed(hand.hand.pile);
		if (!pile || !pile->pile.dealsInto.empty() || pile->pile.cards.empty()) { return false; }
		const int count = static_cast<int>(pile->pile.cards.size());
		const int at = hand.hand.at < 0 ? count - 1 : std::min(hand.hand.at, count - 1);
		if (at < count - 1 && !pile->pile.run) { return false; }
		// Face up, and each on the one below it by the pile's own rule.
		for (int i = at; i < count; ++i)
		{
			const Object* card = cardAt(pile->pile.cards[static_cast<std::size_t>(i)]);
			if (!card || !card->faceUp) { return false; }
			if (i > at && !follows(pile->pile.takes, *cardAt(pile->pile.cards[static_cast<std::size_t>(i - 1)]), *card)) { return false; }
		}
		hand.hand.held.assign(pile->pile.cards.begin() + at, pile->pile.cards.end());
		pile->pile.cards.resize(static_cast<std::size_t>(at));
		pile->cardCount = static_cast<float>(pile->pile.cards.size());
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

	// The cards held go on the pile, the top one pointed at, and the pile
	// they came from shows its top.
	void Game::layHeld(Object& hand, Object& pile)
	{
		for (const int number : hand.hand.held) { putOnPile(pile, number); }
		hand.hand.held.clear();
		if (Object* from = pileNamed(hand.hand.from); from && from != &pile) { showTop(*from); }
		hand.hand.from.clear();
		hand.hand.pile = pile.name;
		hand.hand.at = static_cast<int>(pile.pile.cards.size()) - 1;
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
			if (pileTakes(*pile, hand.hand.held))
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
		layHeld(hand, *from);
		// Put back, the hand stays where it was.
		hand.hand.pile = pointed;
		hand.hand.at = at;
		if (Object* pile = pileNamed(pointed)) { hand.hand.at = std::min(at < 0 ? static_cast<int>(pile->pile.cards.size()) - 1 : at, static_cast<int>(pile->pile.cards.size()) - 1); }
		layOutCards();
		return true;
	}

	bool Game::dealCards(Object& hand)
	{
		if (!hand.hand.is || !hand.hand.held.empty() || (hand.hand.mouse && !hand.hand.over)) { return false; }
		Object* pile = pileNamed(hand.hand.pile);
		if (!pile || pile->pile.dealsInto.empty()) { return false; }
		Object* into = pileNamed(pile->pile.dealsInto);
		if (!into) { return false; }
		if (!pile->pile.cards.empty())
		{
			for (int i = 0; i < std::max(1, pile->pile.deals) && !pile->pile.cards.empty(); ++i) { putOnPile(*into, takeTop(*pile)); }
		}
		else
		{
			// Empty: the other pile turned back over onto it.
			Object* from = pile->pile.refillFrom.empty() ? nullptr : pileNamed(pile->pile.refillFrom);
			if (!from || from->pile.cards.empty()) { return false; }
			while (!from->pile.cards.empty()) { putOnPile(*pile, takeTop(*from)); }
		}
		hand.hand.at = static_cast<int>(pile->pile.cards.size()) - 1;
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
		const auto takeable = [&](int index)
		{
			if (index < 0 || index >= count) { return false; }
			const Object* card = cardAt(pile->pile.cards[static_cast<std::size_t>(index)]);
			return card && card->faceUp;
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
		if (pile->pile.run && up && takeable(at - 1) && takeable(at))
		{
			hand.hand.at = at - 1;
			hand.hand.mouse = false;
			layOutCards();
			return true;
		}
		if (pile->pile.run && !up && at < count - 1)
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
		// The top card under the pointer, else the pile's place; holding
		// cards, the pile under the held cards' middle counts too.
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
			// Holding: the pile, its top card.
			if (!hand.hand.held.empty()) { index = static_cast<int>(pile->pile.cards.size()) - 1; }
			hand.hand.pile = pile->name;
			hand.hand.at = index;
			hand.hand.over = true;
			break;
		}
		layOutCards();
	}
}
