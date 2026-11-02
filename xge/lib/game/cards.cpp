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
#include <utility>

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

		// The card's shape: `fill` inside a one-pixel `edge`, its corners
		// rounded (cut by one pixel on a small card, two on a big one).
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
			const bool round = width >= 32 && height >= 32;
			for (const auto& [x, y] : { std::pair{ 0, 0 }, std::pair{ width - 1, 0 }, std::pair{ 0, height - 1 }, std::pair{ width - 1, height - 1 } })
			{
				card.set(x, y, '.');
				if (!round) { continue; }
				const int inX = x == 0 ? 1 : x - 1;
				const int inY = y == 0 ? 1 : y - 1;
				card.set(inX, y, '.');
				card.set(x, inY, '.');
				card.set(inX, inY, edge);
			}
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

		Canvas fromRows(const std::vector<std::string>& rows)
		{
			Canvas made(rows.empty() ? 0 : static_cast<int>(rows.front().size()), static_cast<int>(rows.size()), '.');
			made.rows = rows;
			return made;
		}

		// A picture's rows, and with `half` the same turned round below.
		Canvas whole(const std::vector<std::string>& rows, bool half)
		{
			const Canvas top = fromRows(rows);
			if (!half) { return top; }
			Canvas made(top.width, top.height * 2, '.');
			made.paste(top, 0, 0);
			made.paste(top.turned(), 0, top.height);
			return made;
		}

		// Each pixel of a picture `grow` by `grow`.
		Canvas grown(const Canvas& small, int grow)
		{
			Canvas made(small.width * grow, small.height * grow, '.');
			made.paste(small, 0, 0, grow);
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
		const auto sameWidth = [&](const std::vector<std::string>& rows, const std::string& what)
		{
			for (const std::string& row : rows)
			{
				if (row.size() != rows.front().size()) { throw std::runtime_error(where + ": " + what + " has rows of different lengths"); }
			}
		};

		// Each rank's corner index, and the widest: the corners are that
		// wide, each index and small pip in the middle of it.
		std::vector<Canvas> indexes;
		int widest = 0;
		for (const RawDeck::Rank& rank : deck.ranks)
		{
			const std::string what = "<rank name=\"" + rank.name + "\">";
			if (rank.index.empty())
			{
				for (const char c : rank.name)
				{
					if (!font().count(c)) { throw std::runtime_error(where + ": " + what + " has '" + std::string(1, c) + "' and no <index>; a rank's name is drawn with " + cardFontCharacters()); }
				}
				indexes.push_back(label(rank.name, '*'));
			}
			else
			{
				sameWidth(rank.index, what + " <index>");
				indexes.push_back(fromRows(rank.index));
			}
			sameWidth(rank.pips, what + " <pips>");
			sameWidth(rank.picture, what + " <picture>");
			for (const std::string& row : rank.pips)
			{
				for (const char c : row)
				{
					if (c != '.' && c != '*' && c != 'v' && c != '@') { throw std::runtime_error(where + ": " + what + " <pips> has '" + std::string(1, c) + "'; a pip is '*', 'v' (turned round) or '@' (the large pip), '.' none"); }
				}
			}
			widest = std::max(widest, indexes.back().width);
		}

		// The back, shared: the card's shape, a frame inside its edge and
		// the pattern (or a lattice of diamonds) inside that.
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
		sameWidth(deck.pattern, "<back> <pattern>");
		const int tileWide = deck.pattern.empty() ? 0 : static_cast<int>(deck.pattern.front().size());
		const int tileHigh = static_cast<int>(deck.pattern.size());
		for (int y = 4; y < deck.height - 4; ++y)
		{
			for (int x = 4; x < deck.width - 4; ++x)
			{
				const bool ink = deck.pattern.empty() ? ((x + y) % 4 == 0 || (x - y + 64) % 4 == 0)
					: deck.pattern[static_cast<std::size_t>((y - 4) % tileHigh)][static_cast<std::size_t>((x - 4) % tileWide)] == '*';
				if (ink) { back.set(x, y, '#'); }
			}
		}
		const RawSprite backSprite = bitmapOf("back", back, deck.backEdge, { { '@', deck.backColor }, { '#', deck.backEdge } }, deck.scale);

		int number = 0;
		for (int pack = 0; pack < std::max(1, deck.copies); ++pack)
		{
			for (std::size_t s = 0; s < deck.suits.size(); ++s)
			{
				const RawDeck::Suit& suit = deck.suits[s];
				const Canvas pip = fromRows(suit.rows);
				const Canvas small = suit.index.empty() ? pip : fromRows(suit.index);
				std::vector<std::pair<char, std::string>> colors{ { '@', deck.faceColor }, { '#', deck.faceEdge } };
				colors.insert(colors.end(), suit.colors.begin(), suit.colors.end());
				colors.insert(colors.end(), deck.colors.begin(), deck.colors.end());
				for (std::size_t r = 0; r < deck.ranks.size(); ++r)
				{
					const RawDeck::Rank& rank = deck.ranks[r];
					const std::string what = "<rank name=\"" + rank.name + "\">";
					// The face: in the top left corner the rank's index with
					// the small pip under it (seen where cards overlap), the
					// same turned round in the bottom right, and between them
					// the pips laid out, the picture, or one big pip.
					Canvas face = blank(deck.width, deck.height, '@', '#');
					const Canvas& index = indexes[r];
					const int gap = deck.width >= 32 ? 2 : 1;
					const int edge = deck.width >= 32 ? 3 : 2;
					Canvas corner(std::max(widest, small.width), index.height + gap + small.height, '.');
					corner.paste(index, (corner.width - index.width) / 2, 0);
					corner.paste(small, (corner.width - small.width) / 2, index.height + gap);
					if (corner.width * 2 + 2 * edge > deck.width || corner.height + 2 * edge > deck.height)
					{
						throw std::runtime_error(where + ": a face of " + std::to_string(deck.width) + " by " + std::to_string(deck.height) + " is too small for the index of " + what + " and the pip of " + suit.name);
					}
					face.paste(corner, edge, edge);
					face.paste(corner.turned(), deck.width - edge - corner.width, deck.height - edge - corner.height);

					// What lies between the corners.
					const int left = edge + corner.width + gap;
					const int right = deck.width - left;
					if (!rank.picture.empty())
					{
						const Canvas picture = whole(rank.picture, rank.half);
						for (const std::string& row : picture.rows)
						{
							for (const char c : row)
							{
								if (c != '.' && c != '*' && std::none_of(colors.begin(), colors.end(), [&](const auto& each) { return each.first == c; }))
								{
									throw std::runtime_error(where + ": " + what + " <picture> has '" + std::string(1, c) + "', which no <color char> of the deck or of " + suit.name + " names");
								}
							}
						}
						face.paste(picture, (deck.width - picture.width) / 2, (deck.height - picture.height) / 2);
					}
					else if (!rank.pips.empty())
					{
						Canvas large = suit.large.empty() ? grown(pip, 2) : fromRows(suit.large);
						const Canvas upside = pip.turned();
						const int rows = static_cast<int>(rank.pips.size());
						const int columns = static_cast<int>(rank.pips.front().size());
						const float top = static_cast<float>(edge + 2);
						const float wide = static_cast<float>(right - left - pip.width);
						const float high = static_cast<float>(deck.height) - 2.0f * top - static_cast<float>(pip.height);
						if (wide < 0.0f || high < 0.0f) { throw std::runtime_error(where + ": a face of " + std::to_string(deck.width) + " by " + std::to_string(deck.height) + " has no room for the pips of " + suit.name + " between its corners"); }
						for (int row = 0; row < rows; ++row)
						{
							for (int column = 0; column < columns; ++column)
							{
								const char c = rank.pips[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)];
								if (c == '.') { continue; }
								const Canvas& drawn = c == '@' ? large : (c == 'v' ? upside : pip);
								// The grid's points spread from corner to corner of
								// the room for a pip; one row or column is the middle.
								const float x = columns == 1 ? static_cast<float>(deck.width) * 0.5f
									: static_cast<float>(left) + static_cast<float>(pip.width) * 0.5f + wide * static_cast<float>(column) / static_cast<float>(columns - 1);
								const float y = rows == 1 ? static_cast<float>(deck.height) * 0.5f
									: top + static_cast<float>(pip.height) * 0.5f + high * static_cast<float>(row) / static_cast<float>(rows - 1);
								face.paste(drawn, static_cast<int>(std::lround(x - static_cast<float>(drawn.width) * 0.5f)), static_cast<int>(std::lround(y - static_cast<float>(drawn.height) * 0.5f)));
							}
						}
					}
					else
					{
						const int roomHigh = deck.height - 2 * (edge + corner.height) - 2;
						int grow = 3;
						while (grow > 1 && (pip.width * grow > right - left || pip.height * grow > roomHigh)) { --grow; }
						if (pip.height * grow <= roomHigh && pip.width * grow <= deck.width - 4)
						{
							face.paste(pip, (deck.width - pip.width * grow) / 2, (deck.height - pip.height * grow) / 2, grow);
						}
					}

					RawObject card;
					++number;
					card.name = deck.name + "." + std::to_string(number);
					card.groupName = deck.name;
					card.isCard = true;
					card.card.deck = deck.name;
					card.card.suit = static_cast<int>(s) + 1;
					card.card.rank = static_cast<int>(r) + 1;
					card.card.suitName = suit.name;
					card.card.rankName = rank.name;
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
		auto at = pile.pile.cards.end();
		if (pile.pile.sorted)
		{
			// Suit by suit, each in rank order.
			const auto before = [&](int other)
			{
				const Object* each = cardAt(other);
				return each && std::make_pair(card->card.suit, card->card.rank) < std::make_pair(each->card.suit, each->card.rank);
			};
			at = std::find_if(pile.pile.cards.begin(), pile.pile.cards.end(), before);
		}
		pile.pile.cards.insert(at, number);
		pile.cardCount = static_cast<float>(pile.pile.cards.size());
		// A card laid answers any call.
		pile.called = 0.0f;
		refreshCall(pile);
	}

	bool Game::isWild(const PileRule& rule, const Object& card)
	{
		return std::find(rule.wildRanks.begin(), rule.wildRanks.end(), card.card.rankName) != rule.wildRanks.end();
	}

	void Game::refreshCall(Object& pile)
	{
		const Object* top = pile.pile.cards.empty() ? nullptr : cardAt(pile.pile.cards.back());
		pile.calling = !pile.pile.takes.wildRanks.empty() && top && isWild(pile.pile.takes, *top) && pile.called == 0.0f ? 1.0f : 0.0f;
	}

	int Game::takeTop(Object& pile)
	{
		if (pile.pile.cards.empty()) { return 0; }
		const int number = pile.pile.cards.back();
		pile.pile.cards.pop_back();
		pile.cardCount = static_cast<float>(pile.pile.cards.size());
		refreshCall(pile);
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
			if (object.pile.is) { object.pile.cards.clear(); object.cardCount = 0.0f; object.called = 0.0f; object.calling = 0.0f; }
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

		// A wild card turned up to start: its own suit is the one called.
		for (Object& object : objects)
		{
			if (object.pile.is && object.calling > 0.0f)
			{
				object.called = static_cast<float>(cardAt(object.pile.cards.back())->card.suit);
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

	// How far a hand lifts the card it points at in its own pile.
	constexpr float liftCard = 16.0f;

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
				if (Object* card = cardAt(pile->pile.cards[static_cast<std::size_t>(hand.hand.at)]))
				{
					size = sizeOf(*card);
					// In a hand's own pile, a face-up card pointed at is
					// lifted out of the fan, so all of it shows.
					if (card->faceUp && !hand.hand.own.empty() && pile == pileNamed(hand.hand.own))
					{
						pointed.y -= liftCard;
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
		return follows(rule, *top, *first);
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
		if (!pile || pile->pile.deals > 0 || pile->pile.cards.empty()) { return false; }
		const int count = static_cast<int>(pile->pile.cards.size());
		const int at = hand.hand.at < 0 ? count - 1 : std::min(hand.hand.at, count - 1);
		// A hand with a pile of its own takes from it alone, any one card,
		// face up or down (it knows its own cards).
		if (!hand.hand.own.empty())
		{
			if (pile != pileNamed(hand.hand.own)) { return false; }
			hand.hand.held = { pile->pile.cards[static_cast<std::size_t>(at)] };
			pile->pile.cards.erase(pile->pile.cards.begin() + at);
			pile->cardCount = static_cast<float>(pile->pile.cards.size());
			hand.hand.from = pile->name;
			hand.hand.at = std::min(at, static_cast<int>(pile->pile.cards.size()) - 1);
			if (hand.hand.mouse && hand.pointer.x >= 0.0f && hand.pointer.y >= 0.0f)
			{
				hand.hand.grab = hand.pointer - cardPlace(*pile, static_cast<std::size_t>(std::max(0, hand.hand.at)));
			}
			layOutCards();
			return true;
		}
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
		const int card = hand.hand.held.front();
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

	bool Game::dealCards(Object& hand)
	{
		if (!hand.hand.is || !hand.hand.held.empty() || (hand.hand.mouse && !hand.hand.over)) { return false; }
		Object* pile = pileNamed(hand.hand.pile);
		if (!pile || pile->pile.deals <= 0) { return false; }
		// Into the pile it names, or else the hand's own.
		const std::string& intoName = pile->pile.dealsInto.empty() ? hand.hand.own : pile->pile.dealsInto;
		Object* into = intoName.empty() ? nullptr : pileNamed(intoName);
		if (!into) { return false; }
		if (!pile->pile.cards.empty())
		{
			for (int i = 0; i < pile->pile.deals && !pile->pile.cards.empty(); ++i) { putOnPile(*into, takeTop(*pile)); }
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
			from->cardCount = static_cast<float>(from->pile.cards.size());
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
		if (fannedAcross && (direction == Direction::Left || direction == Direction::Right) && (own || pile->pile.run))
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
		if (!fannedAcross && pile->pile.run && up && takeable(at - 1) && takeable(at))
		{
			hand.hand.at = at - 1;
			hand.hand.mouse = false;
			layOutCards();
			return true;
		}
		if (!fannedAcross && pile->pile.run && !up && at < count - 1)
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
		pile->called = static_cast<float>(card->card.suit);
		refreshCall(*pile);
		return true;
	}

	// A computer playing a card game, a step each time it is due: point the
	// hand at what it will do, then press. Calling, the suit it holds most
	// of; else the card that goes somewhere, a wild one last, from the suit
	// it holds most of and the highest; else a draw from the stock.
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
		if (calling)
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
					if (!pileTakes(*pile, { number })) { continue; }
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
