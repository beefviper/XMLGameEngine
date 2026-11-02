// test_cards.cpp
// XML Game Engine
// author: beefviper
// date: Nov 1, 2026
//
// Catch2 tests for cards: a <deck> made from suits and ranks, piles and
// their rules, the deal, and a hand that takes, lays and puts back
// (klondike.xml).

#include "game/cards.h"
#include "played.h"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace xge;
using played::Played;

namespace
{
	struct Table : Played
	{
		Table() : Played("descriptions/klondike.xml")
		{
			tap(KeyCode::Space);
			frames(2);
		}

		Object& pile(const std::string& name) { return get(name); }
		Object& hand() { return get("hand"); }
		Object& card(int number) { return *game.cardAt(number); }
		Object& top(const std::string& name) { return card(pile(name).pile.cards.back()); }
	};
}

TEST_CASE("klondike.xml: the deal: a staircase of columns, their tops face up, the rest in the stock", "[cards]")
{
	Table table;
	int total = 0;
	for (int column = 1; column <= 7; ++column)
	{
		Object& pile = table.pile("columns." + std::to_string(column) + ".1");
		REQUIRE(pile.pile.cards.size() == static_cast<std::size_t>(column));
		total += column;
		for (std::size_t i = 0; i < pile.pile.cards.size(); ++i)
		{
			CHECK(table.card(pile.pile.cards[i]).faceUp == (i + 1 == pile.pile.cards.size()));
		}
		CHECK(table.top("columns." + std::to_string(column) + ".1").lookName() == "face");
	}
	CHECK(total == 28);
	CHECK(table.pile("stock").pile.cards.size() == 24);
	CHECK(table.pile("stock").cardCount == 24.0f);
}

namespace
{
	// The card of that suit and rank.
	int cardOf(Table& table, const std::string& suit, const std::string& rank)
	{
		for (Object& object : table.game.getCurrentObjects())
		{
			if (object.isCard && object.card.suitName == suit && object.card.rankName == rank) { return object.cardNumber; }
		}
		return 0;
	}

	// Every pile emptied, then these cards (bottom first, face up) on each.
	void setUp(Table& table, const std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>>& piles)
	{
		for (Object& object : table.game.getCurrentObjects())
		{
			if (object.pile.is) { object.pile.cards.clear(); object.cardCount = 0.0f; }
		}
		for (const auto& [name, cards] : piles)
		{
			Object& pile = table.pile(name);
			for (const auto& [suit, rank] : cards)
			{
				const int number = cardOf(table, suit, rank);
				table.game.turnCard(table.card(number), true);
				pile.pile.cards.push_back(number);
			}
			pile.cardCount = static_cast<float>(pile.pile.cards.size());
		}
		table.game.layOutCards();
	}

	void pointAt(Table& table, const std::string& pile, int at = -1)
	{
		Object& hand = table.hand();
		hand.hand.pile = table.pile(pile).name;
		hand.hand.at = at < 0 ? static_cast<int>(table.pile(pile).pile.cards.size()) - 1 : at;
		hand.hand.mouse = false;
	}

	void space(Table& table)
	{
		table.tap(KeyCode::Space);
		table.frames(1);
	}

	std::string topOf(Table& table, const std::string& pile)
	{
		const Object& object = table.pile(pile);
		if (object.pile.cards.empty()) { return ""; }
		const Object& card = table.card(object.pile.cards.back());
		return card.card.rankName + " " + card.card.suitName;
	}
}

TEST_CASE("klondike.xml: the stock deals to the waste face up, and empty, takes the waste back face down", "[cards]")
{
	Table table;
	const int first = table.pile("stock").pile.cards.back();
	pointAt(table, "stock");
	space(table);
	CHECK(table.pile("stock").pile.cards.size() == 23);
	REQUIRE(table.pile("waste").pile.cards.size() == 1);
	CHECK(table.pile("waste").pile.cards.back() == first);
	CHECK(table.card(first).faceUp);
	CHECK(table.hand().lookName() == "free");
	for (int i = 0; i < 23; ++i) { space(table); }
	CHECK(table.pile("stock").pile.cards.empty());
	CHECK(table.pile("waste").pile.cards.size() == 24);
	space(table);
	CHECK(table.pile("waste").pile.cards.empty());
	REQUIRE(table.pile("stock").pile.cards.size() == 24);
	CHECK(table.pile("stock").pile.cards.back() == first);
	CHECK_FALSE(table.card(first).faceUp);
}

TEST_CASE("klondike.xml: a column takes a card one lower of the other colour, an empty one only a king", "[cards]")
{
	Table table;
	setUp(table, { { "waste", { { "hearts", "9" } } }, { "columns.1.1", { { "spades", "10" } } }, { "columns.2.1", { { "clubs", "10" } } } });
	pointAt(table, "waste");
	space(table);
	REQUIRE(table.hand().lookName() == "held");
	CHECK(table.pile("waste").pile.cards.empty());
	// Onto an empty column: refused, still held.
	pointAt(table, "columns.3.1");
	space(table);
	CHECK(table.hand().lookName() == "held");
	CHECK(table.pile("columns.3.1").pile.cards.empty());
	pointAt(table, "columns.1.1");
	space(table);
	CHECK(table.hand().lookName() == "free");
	CHECK(topOf(table, "columns.1.1") == "9 hearts");
	// A black 9 does not go on the black 10.
	setUp(table, { { "waste", { { "spades", "9" } } }, { "columns.2.1", { { "clubs", "10" } } } });
	pointAt(table, "waste");
	space(table);
	pointAt(table, "columns.2.1");
	space(table);
	CHECK(table.hand().lookName() == "held");
	// Put back.
	table.tap(KeyCode::X);
	table.frames(1);
	CHECK(table.hand().lookName() == "free");
	CHECK(topOf(table, "waste") == "9 spades");
	// A king on an empty column.
	setUp(table, { { "waste", { { "diamonds", "K" } } } });
	pointAt(table, "waste");
	space(table);
	pointAt(table, "columns.4.1");
	space(table);
	CHECK(topOf(table, "columns.4.1") == "K diamonds");
}

TEST_CASE("klondike.xml: a foundation takes an ace, then the same suit up, one card at a time", "[cards]")
{
	Table table;
	setUp(table, { { "waste", { { "hearts", "2" }, { "hearts", "A" } } }, { "columns.1.1", { { "clubs", "3" }, { "hearts", "2" } } } });
	// The ace home by F, then the 2 by F.
	pointAt(table, "waste");
	table.tap(KeyCode::F);
	table.frames(1);
	CHECK(topOf(table, "foundations.1.1") == "A hearts");
	CHECK(table.hand().lookName() == "free");
	pointAt(table, "waste");
	table.tap(KeyCode::F);
	table.frames(1);
	CHECK(topOf(table, "foundations.1.1") == "2 hearts");
	// The 3 of clubs does not go home: put back where it was.
	pointAt(table, "columns.1.1", 0);
	table.tap(KeyCode::F);
	table.frames(1);
	CHECK(table.pile("columns.1.1").pile.cards.size() == 2);
	CHECK(table.hand().hand.held.empty());
	CHECK(table.hand().lookName() == "free");
	// Nor a run of two onto a foundation.
	setUp(table, { { "foundations.2.1", { { "spades", "A" } } }, { "columns.1.1", { { "spades", "2" }, { "hearts", "A" } } } });
	pointAt(table, "columns.1.1", 0);
	table.tap(KeyCode::F);
	table.frames(1);
	CHECK(table.hand().hand.held.empty());
	CHECK(table.pile("columns.1.1").pile.cards.size() == 2);
	CHECK(table.pile("foundations.2.1").pile.cards.size() == 1);
}

TEST_CASE("klondike.xml: a run moves together, and the face-down card under it turns up", "[cards]")
{
	Table table;
	setUp(table, { { "columns.1.1", { { "clubs", "4" }, { "spades", "10" }, { "hearts", "9" }, { "clubs", "8" } } }, { "columns.2.1", { { "diamonds", "J" } } } });
	table.game.turnCard(table.card(table.pile("columns.1.1").pile.cards[0]), false);
	// The 10 and what is on it, onto the red jack.
	pointAt(table, "columns.1.1", 1);
	space(table);
	REQUIRE(table.hand().hand.held.size() == 3);
	pointAt(table, "columns.2.1");
	space(table);
	CHECK(table.pile("columns.2.1").pile.cards.size() == 4);
	CHECK(topOf(table, "columns.2.1") == "8 clubs");
	CHECK(table.pile("columns.1.1").pile.cards.size() == 1);
	CHECK(table.top("columns.1.1").faceUp);
	// A run that is not in order cannot be taken: a face-down card neither.
	setUp(table, { { "columns.1.1", { { "clubs", "4" }, { "spades", "10" } } } });
	table.game.turnCard(table.card(table.pile("columns.1.1").pile.cards[0]), false);
	pointAt(table, "columns.1.1", 0);
	table.tap(KeyCode::F);
	table.frames(1);
	CHECK(table.hand().hand.held.empty());
	CHECK(table.pile("columns.1.1").pile.cards.size() == 2);
}

TEST_CASE("klondike.xml: the hand goes along the rows and up and down a column's run", "[cards]")
{
	Table table;
	pointAt(table, "stock");
	table.tap(KeyCode::D);
	CHECK(table.hand().hand.pile == table.pile("waste").name);
	table.tap(KeyCode::D);
	CHECK(table.hand().hand.pile == table.pile("foundations.1.1").name);
	table.tap(KeyCode::S);
	CHECK(table.hand().hand.pile == table.pile("columns.4.1").name);
	setUp(table, { { "columns.4.1", { { "spades", "10" }, { "hearts", "9" } } } });
	pointAt(table, "columns.4.1");
	table.tap(KeyCode::W);
	CHECK(table.hand().hand.at == 0);
	table.tap(KeyCode::W);
	CHECK(table.hand().hand.pile == table.pile("foundations.1.1").name);
	table.tap(KeyCode::A);
	CHECK(table.hand().hand.pile == table.pile("waste").name);
	table.tap(KeyCode::A);
	CHECK(table.hand().hand.pile == table.pile("stock").name);
	table.tap(KeyCode::A);
	CHECK(table.hand().hand.pile == table.pile("stock").name);
}

TEST_CASE("klondike.xml: the mouse takes a card, drags it and lays it", "[cards][mouse]")
{
	Table table;
	setUp(table, { { "waste", { { "hearts", "9" } } }, { "columns.1.1", { { "spades", "10" } } } });
	const auto mouse = [&](float x, float y)
	{
		Controls controls;
		controls.pointerInside = true;
		controls.pointerX = x;
		controls.pointerY = y;
		table.engine.handleControls(controls);
		table.frames(1);
	};
	const Vector2f waste = table.pile("waste").position;
	mouse(waste.x + 20.0f, waste.y + 30.0f);
	table.engine.handleKeyPressed(KeyCode::MouseLeft);
	table.frames(1);
	REQUIRE(table.hand().hand.held.size() == 1);
	// The card follows the pointer, held where it was taken.
	mouse(300.0f, 400.0f);
	CHECK(table.card(table.hand().hand.held.front()).position.x == 280.0f);
	CHECK(table.card(table.hand().hand.held.front()).position.y == 370.0f);
	const Vector2f column = table.top("columns.1.1").position;
	mouse(column.x + 20.0f, column.y + 30.0f);
	table.engine.handleKeyReleased(KeyCode::MouseLeft);
	table.frames(1);
	CHECK(topOf(table, "columns.1.1") == "9 hearts");
	CHECK(table.hand().lookName() == "free");
	// Let go over nothing: back where it came from.
	const Vector2f nine = table.top("columns.1.1").position;
	mouse(nine.x + 20.0f, nine.y + 30.0f);
	table.engine.handleKeyPressed(KeyCode::MouseLeft);
	table.frames(1);
	mouse(20.0f, 580.0f);
	table.engine.handleKeyReleased(KeyCode::MouseLeft);
	table.frames(1);
	CHECK(topOf(table, "columns.1.1") == "9 hearts");
	CHECK(table.hand().hand.held.empty());
}

TEST_CASE("klondike.xml: a click takes, and sounds, only over a card", "[cards][mouse]")
{
	Table table;
	setUp(table, { { "waste", { { "hearts", "9" } } } });
	const auto mouse = [&](float x, float y, bool down, bool inside = true)
	{
		Controls controls;
		controls.pointerInside = inside;
		controls.pointerX = x;
		controls.pointerY = y;
		controls.mouse[0] = down;
		table.engine.handleControls(controls);
	};
	const Vector2f waste = table.pile("waste").position;
	mouse(waste.x + 20.0f, waste.y + 30.0f, false);
	table.frames(1);
	(void)table.game.takeSoundRequests();
	// On the green, away from every pile: nothing taken, no sound.
	mouse(20.0f, 580.0f, false);
	mouse(20.0f, 580.0f, true);
	CHECK(table.hand().hand.held.empty());
	CHECK(table.game.takeSoundRequests().empty());
	mouse(20.0f, 580.0f, false);
	table.frames(1);
	// Off the playfield (a bar beside the picture, the title bar), even
	// with the hand last over the card: no press.
	mouse(waste.x + 20.0f, waste.y + 30.0f, false);
	mouse(-30.0f, 300.0f, false);
	mouse(-30.0f, 300.0f, true);
	CHECK(table.hand().hand.held.empty());
	mouse(-30.0f, 300.0f, false);
	mouse(100.0f, -20.0f, true, false);
	CHECK(table.hand().hand.held.empty());
	// Held down off it and brought over the card: still no press.
	mouse(waste.x + 20.0f, waste.y + 30.0f, true);
	CHECK(table.hand().hand.held.empty());
	CHECK(table.game.takeSoundRequests().empty());
	mouse(waste.x + 20.0f, waste.y + 30.0f, false);
	// Pressed on the card: taken, with its sound.
	mouse(waste.x + 20.0f, waste.y + 30.0f, true);
	CHECK(table.hand().hand.held.size() == 1);
	CHECK(table.game.takeSoundRequests() == std::vector<std::string>{ "card" });
}

TEST_CASE("klondike.xml: all 52 home wins, and a new game deals again", "[cards]")
{
	Table table;
	std::vector<std::pair<std::string, std::string>> spades;
	for (const char* rank : { "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q" }) { spades.emplace_back("spades", rank); }
	std::vector<std::pair<std::string, std::string>> hearts;
	std::vector<std::pair<std::string, std::string>> clubs;
	std::vector<std::pair<std::string, std::string>> diamonds;
	for (const char* rank : { "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K" })
	{
		hearts.emplace_back("hearts", rank);
		clubs.emplace_back("clubs", rank);
		diamonds.emplace_back("diamonds", rank);
	}
	setUp(table, { { "foundations.1.1", spades }, { "foundations.2.1", hearts }, { "foundations.3.1", clubs }, { "foundations.4.1", diamonds }, { "waste", { { "spades", "K" } } } });
	table.frames(1);
	CHECK(table.state() == "playing");
	pointAt(table, "waste");
	table.tap(KeyCode::F);
	table.frames(2);
	CHECK(table.state() == "won");
	table.tap(KeyCode::Space);
	table.frames(2);
	CHECK(table.state() == "title");
	CHECK(table.pile("stock").pile.cards.size() == 24);
	CHECK(table.pile("columns.7.1").pile.cards.size() == 7);
	CHECK(table.pile("foundations.1.1").pile.cards.empty());
}

TEST_CASE("a deck of its own suits and ranks: an Italian forty, pips laid out, a double-ended picture", "[cards]")
{
	RawDeck deck;
	deck.name = "carte";
	deck.pile = "mazzo";
	for (const char* name : { "coins", "cups", "swords", "batons" })
	{
		deck.suits.push_back({ name, "color.red", { ".*.", "***", ".*." }, {}, {}, {} });
	}
	for (const char* name : { "1", "2", "3", "4", "5", "6", "7", "F", "C", "R" }) { deck.ranks.push_back({ name, {}, {}, {}, false }); }
	std::vector<RawObject> cards;
	addDeck(cards, deck);
	REQUIRE(cards.size() == 40);
	CHECK(cards.back().name == "carte.40");
	CHECK(cards.back().card.rankName == "R");

	// Two pips on a grid of one column and three rows: the top one upright
	// in the top middle, the bottom one turned round in the bottom middle.
	RawDeck two = deck;
	two.suits = { { "arrows", "color.black", { "*", "*", "." }, {}, {}, {} } };
	two.ranks = { { "2", {}, { "*", ".", "v" }, {}, false } };
	cards.clear();
	addDeck(cards, two);
	const std::vector<std::string>& rows = cards.front().looks.front().bitmapRows;
	REQUIRE(rows.size() == 93);
	CHECK(rows[5][31] == '*');
	CHECK(rows[6][31] == '*');
	CHECK(rows[7][31] == '@');
	CHECK(rows[85][31] == '@');
	CHECK(rows[86][31] == '*');
	CHECK(rows[87][31] == '*');

	// A picture's top half, and the bottom the same turned round.
	RawDeck court = two;
	court.ranks = { { "K", {}, {}, { "*..", "..." }, true } };
	cards.clear();
	addDeck(cards, court);
	const std::vector<std::string>& king = cards.front().looks.front().bitmapRows;
	CHECK(king[44][30] == '*');
	CHECK(king[47][32] == '*');

	// A picture's character no color names stops the load.
	court.ranks = { { "K", {}, {}, { "*.q" }, false } };
	cards.clear();
	CHECK_THROWS(addDeck(cards, court));
}

namespace
{
	// Crazy Eights, dealt: you against the computer.
	struct Eights : Played
	{
		Eights() : Played("descriptions/crazyeights.xml")
		{
			tap(KeyCode::Space);
			frames(2);
		}

		Object& pile(const std::string& name) { return get(name); }
		Object& hand() { return get("hand"); }
		Object& card(int number) { return *game.cardAt(number); }
		float turn() { return hand().variable.at("turn"); }

		int cardOf(const std::string& rank, const std::string& suit)
		{
			for (Object& object : game.getCurrentObjects())
			{
				if (object.isCard && object.card.suitName == suit && object.card.rankName == rank) { return object.cardNumber; }
			}
			return 0;
		}

		// Every pile emptied, then these cards (as "8 hearts", bottom first)
		// put on each by the pile's own way (face up or down, sorted).
		void setUp(const std::vector<std::pair<std::string, std::vector<std::string>>>& piles)
		{
			for (Object& object : game.getCurrentObjects())
			{
				if (object.pile.is) { object.pile.cards.clear(); object.cardCount = 0.0f; object.called = 0.0f; object.calling = 0.0f; }
			}
			for (const auto& [name, cards] : piles)
			{
				for (const std::string& each : cards)
				{
					const auto space = each.find(' ');
					game.putOnPile(pile(name), cardOf(each.substr(0, space), each.substr(space + 1)));
				}
			}
			hand().hand.pile = pile("mine").name;
			hand().hand.at = 0;
			hand().hand.mouse = false;
			game.layOutCards();
		}

		std::string name(int number) { return card(number).card.rankName + " " + card(number).card.suitName; }
		std::string top(const std::string& name) { return pile(name).pile.cards.empty() ? "" : this->name(pile(name).pile.cards.back()); }

		// Points your hand at that card of yours.
		void pointAt(const std::string& which)
		{
			const auto& cards = pile("mine").pile.cards;
			for (std::size_t i = 0; i < cards.size(); ++i)
			{
				if (name(cards[i]) == which) { hand().hand.pile = pile("mine").name; hand().hand.at = static_cast<int>(i); }
			}
		}

		void space()
		{
			tap(KeyCode::Space);
			frames(1);
		}

		// Frames until it is your turn again (or the game is over).
		void untilYourTurn()
		{
			for (int i = 0; i < 2000 && (turn() != 1.0f && state() == "playing"); ++i) { frames(1); }
		}
	};
}

TEST_CASE("crazyeights.xml: the deal: seven each, yours face up in order, one turned up, the rest the stock", "[cards][eights]")
{
	Eights table;
	const auto& mine = table.pile("mine").pile.cards;
	REQUIRE(mine.size() == 7);
	for (std::size_t i = 0; i < mine.size(); ++i)
	{
		CHECK(table.card(mine[i]).faceUp);
		if (i > 0) { CHECK(std::make_pair(table.card(mine[i - 1]).card.suit, table.card(mine[i - 1]).card.rank) < std::make_pair(table.card(mine[i]).card.suit, table.card(mine[i]).card.rank)); }
	}
	REQUIRE(table.pile("theirs").pile.cards.size() == 7);
	for (const int number : table.pile("theirs").pile.cards) { CHECK_FALSE(table.card(number).faceUp); }
	REQUIRE(table.pile("discard").pile.cards.size() == 1);
	CHECK(table.card(table.pile("discard").pile.cards.back()).faceUp);
	CHECK(table.pile("stock").pile.cards.size() == 37);
	CHECK(table.hand().hand.pile == table.pile("mine").name);
	CHECK(table.turn() == 1.0f);
}

TEST_CASE("crazyeights.xml: a card goes on the pile by suit or rank; one that does not goes back in your hand", "[cards][eights]")
{
	Eights table;
	table.setUp({ { "discard", { "5 hearts" } }, { "mine", { "K clubs", "5 spades", "9 hearts", "2 diamonds" } }, { "theirs", { "3 clubs", "4 clubs" } }, { "stock", { "6 clubs" } } });
	table.pointAt("K clubs");
	table.space();
	CHECK(table.top("discard") == "5 hearts");
	CHECK(table.pile("mine").pile.cards.size() == 4);
	CHECK(table.hand().hand.held.empty());
	CHECK(table.turn() == 1.0f);

	table.pointAt("5 spades");
	table.space();
	CHECK(table.top("discard") == "5 spades");
	CHECK(table.pile("mine").pile.cards.size() == 3);
	CHECK(table.turn() == 2.0f);
	// Your hand still points into your own cards.
	CHECK(table.hand().hand.pile == table.pile("mine").name);
}

TEST_CASE("crazyeights.xml: an eight goes on anything, you name a suit, and the computer follows it", "[cards][eights]")
{
	Eights table;
	table.setUp({ { "discard", { "5 hearts" } }, { "mine", { "8 diamonds", "2 spades", "K clubs" } }, { "theirs", { "Q hearts", "8 clubs", "3 spades", "J diamonds" } }, { "stock", { "6 clubs", "7 clubs" } } });
	table.pointAt("8 diamonds");
	table.space();
	table.frames(1);
	CHECK(table.top("discard") == "8 diamonds");
	CHECK(table.pile("discard").calling == 1.0f);
	CHECK(table.turn() == 3.0f);
	CHECK(table.hand().lookName() == "calling");

	// Spades: the suit of the card pointed at.
	table.pointAt("2 spades");
	table.space();
	CHECK(table.pile("discard").called == 1.0f);
	CHECK(table.pile("discard").calling == 0.0f);
	CHECK(table.turn() == 2.0f);
	CHECK(table.pile("mine").pile.cards.size() == 2);
	table.frames(1);
	CHECK(table.get("called").lookName() == "spades");

	// It plays its spade, not its eight.
	table.untilYourTurn();
	CHECK(table.top("discard") == "3 spades");
	CHECK(table.pile("theirs").pile.cards.size() == 3);
	CHECK(table.pile("discard").called == 0.0f);
	CHECK(table.hand().lookName() == "free");
}

TEST_CASE("crazyeights.xml: the computer draws until it can play, and names the suit it holds most of after its eight", "[cards][eights]")
{
	Eights table;
	table.setUp({ { "discard", { "5 hearts" } }, { "mine", { "2 spades", "K clubs" } }, { "theirs", { "Q spades" } }, { "stock", { "8 clubs", "4 diamonds", "9 diamonds" } } });
	// The computer's turn.
	table.hand().variable["turn"] = 2.0f;
	table.hand().showLook(2);
	table.get("cpuhand").showLook(1);
	table.untilYourTurn();
	// Drawn: the 9 and 4 of diamonds, then the 8 of clubs, which it plays and names diamonds.
	CHECK(table.top("discard") == "8 clubs");
	CHECK(table.pile("theirs").pile.cards.size() == 3);
	CHECK(table.pile("stock").pile.cards.empty());
	CHECK(table.pile("discard").called == 4.0f);
	CHECK(table.turn() == 1.0f);
}

TEST_CASE("crazyeights.xml: drawn from empty, the stock takes the discard pile but its top, face down", "[cards][eights]")
{
	Eights table;
	table.setUp({ { "discard", { "5 hearts", "6 hearts", "7 hearts" } }, { "mine", { "2 spades" } }, { "theirs", { "K clubs" } } });
	table.hand().hand.pile = table.pile("stock").name;
	table.hand().hand.at = -1;
	table.space();
	CHECK(table.top("discard") == "7 hearts");
	CHECK(table.pile("discard").pile.cards.size() == 1);
	REQUIRE(table.pile("stock").pile.cards.size() == 2);
	for (const int number : table.pile("stock").pile.cards) { CHECK_FALSE(table.card(number).faceUp); }
	// Then a draw into your hand, which stays your turn.
	table.space();
	CHECK(table.pile("mine").pile.cards.size() == 2);
	CHECK(table.pile("stock").pile.cards.size() == 1);
	CHECK(table.turn() == 1.0f);
}

TEST_CASE("crazyeights.xml: a long hand squeezes into its width; your last card out wins", "[cards][eights]")
{
	Eights table;
	std::vector<std::string> many;
	for (const char* rank : { "A", "2", "3", "4", "5", "6", "7", "9", "10", "J", "Q", "K" }) { many.push_back(std::string(rank) + " spades"); }
	for (const char* rank : { "A", "2", "3", "4", "5", "6", "7", "9" }) { many.push_back(std::string(rank) + " clubs"); }
	table.setUp({ { "discard", { "5 hearts" } }, { "mine", many }, { "theirs", { "K hearts" } } });
	const Object& last = table.card(table.pile("mine").pile.cards.back());
	CHECK(last.position.x + 96.0f <= 56.0f + 768.0f + 0.5f);
	CHECK(last.position.x + 96.0f >= 56.0f + 768.0f - 0.5f);

	table.setUp({ { "discard", { "5 hearts" } }, { "mine", { "9 hearts" } }, { "theirs", { "K clubs" } } });
	table.space();
	table.frames(2);
	CHECK(table.state() == "over");
	CHECK(table.get("over.winner").lookName() == "you");
}

TEST_CASE("crazyeights.xml: a whole game against the computer, playing the first card that goes, ends", "[cards][eights]")
{
	Eights table;
	int turns = 0;
	for (; turns < 300 && table.state() == "playing"; ++turns)
	{
		REQUIRE(table.turn() == 1.0f);
		// The first card of yours that goes, else a draw; after an eight, your first card's suit.
		bool played = false;
		for (std::size_t i = 0; i < table.pile("mine").pile.cards.size() && !played; ++i)
		{
			table.hand().hand.pile = table.pile("mine").name;
			table.hand().hand.at = static_cast<int>(i);
			table.space();
			played = table.turn() != 1.0f;
		}
		if (!played)
		{
			table.hand().hand.pile = table.pile("stock").name;
			table.hand().hand.at = -1;
			table.space();
			continue;
		}
		table.frames(1);
		if (table.turn() == 3.0f)
		{
			table.hand().hand.pile = table.pile("mine").name;
			table.hand().hand.at = 0;
			table.space();
		}
		table.untilYourTurn();
	}
	CHECK(table.state() == "over");
	CHECK(turns < 300);
	CHECK(table.pile("mine").pile.cards.size() + table.pile("theirs").pile.cards.size() + table.pile("stock").pile.cards.size() + table.pile("discard").pile.cards.size() == 52);
}
