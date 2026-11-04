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
	for (const char* name : { "1", "2", "3", "4", "5", "6", "7", "F", "C", "R" }) { deck.ranks.push_back({ name, {}, {}, {}, false, {} }); }
	std::vector<RawObject> cards;
	addDeck(cards, deck);
	REQUIRE(cards.size() == 40);
	CHECK(cards.back().name == "carte.40");
	CHECK(cards.back().card.rankName == "R");

	// Two pips on a grid of one column and three rows: the top one upright
	// in the top middle, the bottom one turned round in the bottom middle.
	RawDeck two = deck;
	two.suits = { { "arrows", "color.black", { "*", "*", "." }, {}, {}, {} } };
	two.ranks = { { "2", {}, { "*", ".", "v" }, {}, false, {} } };
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
	court.ranks = { { "K", {}, {}, { "*..", "..." }, true, {} } };
	cards.clear();
	addDeck(cards, court);
	const std::vector<std::string>& king = cards.front().looks.front().bitmapRows;
	CHECK(king[44][30] == '*');
	CHECK(king[47][32] == '*');

	// A picture's character no color names stops the load.
	court.ranks = { { "K", {}, {}, { "*.q" }, false, {} } };
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

namespace
{
	// Blackjack, at the table, betting.
	struct Twentyone : Played
	{
		Twentyone() : Played("descriptions/blackjack.xml")
		{
			tap(KeyCode::Space);
			frames(2);
		}

		Object& pile(const std::string& name) { return get(name); }
		float table(const std::string& name) { return get("table").variable.at(name); }

		int cardOf(const std::string& rank, const std::string& suit)
		{
			for (Object& object : game.getCurrentObjects())
			{
				if (object.isCard && object.card.suitName == suit && object.card.rankName == rank) { return object.cardNumber; }
			}
			return 0;
		}

		// The stock stacked so these come off it in this order (as "A hearts").
		void stack(const std::vector<std::string>& next)
		{
			auto& cards = pile("stock").pile.cards;
			for (auto each = next.rbegin(); each != next.rend(); ++each)
			{
				const auto space = each->find(' ');
				const int number = cardOf(each->substr(0, space), each->substr(space + 1));
				cards.erase(std::find(cards.begin(), cards.end(), number));
				cards.push_back(number);
			}
		}

		void press(KeyCode key)
		{
			tap(key);
			frames(1);
		}

		// Frames until the hand is over.
		void untilDone()
		{
			for (int i = 0; i < 2000 && table("phase") != 4.0f && state() == "playing"; ++i) { frames(1); }
		}
	};
}

TEST_CASE("blackjack.xml: points count face-up cards, an ace 11 while within 21, and the dealer's second card is hidden", "[cards][blackjack]")
{
	Twentyone play;
	// You: A, 6; the dealer: 9 and a hidden K.
	play.stack({ "A hearts", "9 spades", "6 clubs", "K diamonds" });
	play.press(KeyCode::Space);
	CHECK(play.table("bank") == 190.0f);
	CHECK(play.pile("player").points == 17.0f);
	CHECK(play.pile("dealer").points == 9.0f);
	CHECK(play.pile("dealer").cardCount == 2.0f);
	CHECK(play.get("dealerpoints").picture.text == "dealer 9");

	// A 9: the ace counts 1 now.
	play.stack({ "9 hearts" });
	play.press(KeyCode::W);
	CHECK(play.pile("player").points == 16.0f);
	CHECK(play.get("yourpoints").picture.text == "you 16");
}

TEST_CASE("blackjack.xml: the dealer draws to 17, and the nearer to 21 wins", "[cards][blackjack]")
{
	Twentyone play;
	// You: 10, 9 (19); the dealer: 6, 5, then 3 (14), 4 (18).
	play.stack({ "10 hearts", "6 spades", "9 clubs", "5 diamonds", "3 hearts", "4 hearts" });
	play.press(KeyCode::Space);
	play.press(KeyCode::S);
	play.untilDone();
	CHECK(play.pile("dealer").pile.cards.size() == 4);
	CHECK(play.pile("dealer").points == 18.0f);
	CHECK(play.get("result").lookName() == "win");
	CHECK(play.table("bank") == 210.0f);

	// The next hand clears the table.
	play.press(KeyCode::Space);
	CHECK(play.pile("player").pile.cards.empty());
	CHECK(play.pile("stock").pile.cards.size() == 52);
	CHECK(play.table("phase") == 0.0f);
}

TEST_CASE("blackjack.xml: over 21 is bust; blackjack pays three to two", "[cards][blackjack]")
{
	Twentyone play;
	play.stack({ "10 hearts", "6 spades", "9 clubs", "5 diamonds", "K clubs" });
	play.press(KeyCode::Space);
	play.press(KeyCode::W);
	play.frames(2);
	CHECK(play.pile("player").points == 29.0f);
	CHECK(play.get("result").lookName() == "bust");
	CHECK(play.table("bank") == 190.0f);
	// The dealer's hidden card shows.
	for (const int number : play.pile("dealer").pile.cards) { CHECK(play.game.cardAt(number)->faceUp); }

	play.press(KeyCode::Space);
	play.stack({ "A hearts", "6 spades", "K clubs", "5 diamonds" });
	play.press(KeyCode::Space);
	play.untilDone();
	CHECK(play.get("result").lookName() == "blackjack");
	CHECK(play.table("bank") == 205.0f);
}

TEST_CASE("blackjack.xml: the bet stays from 10 to 100 and within your chips", "[cards][blackjack]")
{
	Twentyone play;
	play.press(KeyCode::A);
	play.frames(1);
	CHECK(play.table("bet") == 10.0f);
	for (int i = 0; i < 15; ++i) { play.press(KeyCode::D); play.frames(1); }
	CHECK(play.table("bet") == 100.0f);
	play.get("table").variable["bank"] = 40.0f;
	play.frames(2);
	CHECK(play.table("bet") == 40.0f);
	play.get("table").variable["bank"] = 5.0f;
	play.frames(2);
	CHECK(play.state() == "broke");
}

namespace
{
	struct Holdem : Played
	{
		Holdem() : Played("descriptions/holdem.xml")
		{
			tap(KeyCode::Space);
			frames(2);
		}

		Object& pile(const std::string& name) { return get(name); }
		float table(const std::string& name) { return get("table").variable.at(name); }
		float chips(const std::string& who) { return get(who).variable.at("chips"); }

		int cardOf(const std::string& name)
		{
			const auto space = name.find(' ');
			for (Object& object : game.getCurrentObjects())
			{
				if (object.isCard && object.card.rankName == name.substr(0, space) && object.card.suitName == name.substr(space + 1)) { return object.cardNumber; }
			}
			return 0;
		}

		// The stock stacked so these come off it in this order: yours,
		// the computer's, yours, the computer's, then the board.
		void stack(const std::vector<std::string>& next)
		{
			auto& cards = pile("stock").pile.cards;
			for (auto each = next.rbegin(); each != next.rend(); ++each)
			{
				const int number = cardOf(*each);
				cards.erase(std::find(cards.begin(), cards.end(), number));
				cards.push_back(number);
			}
		}

		// Seven cards (as "A hearts"): two in yours, the rest on the board, scored.
		void lay(const std::vector<std::string>& seven)
		{
			auto& yours = pile("yours").pile.cards;
			auto& board = pile("board").pile.cards;
			yours.clear();
			board.clear();
			for (std::size_t i = 0; i < seven.size(); ++i) { (i < 2 ? yours : board).push_back(cardOf(seven[i])); }
			game.scorePile(pile("yours"));
		}

		void press(KeyCode key)
		{
			tap(key);
			frames(1);
		}

		// Space between hands shuffles; the stock stacked then, the cards come.
		void deal(const std::vector<std::string>& next)
		{
			press(KeyCode::Space);
			stack(next);
			for (int i = 0; i < 100 && table("street") != 0.0f; ++i) { frames(1); }
		}

		// Frames until it is your move, or the hand is over.
		void untilYou()
		{
			for (int i = 0; i < 600; ++i)
			{
				frames(1);
				const std::string look = get("you").lookName();
				if (look != "wait" && get("table").variable.at("turn") == 1.0f) { return; }
				if (table("street") >= 4.0f) { frames(2); return; }
			}
		}
	};
}

TEST_CASE("holdem.xml: the best five of seven: the kinds in order, a wheel the lowest straight", "[cards][holdem]")
{
	Holdem play;
	const auto kindOf = [&](const std::vector<std::string>& seven)
	{
		play.lay(seven);
		return play.pile("yours").pattern;
	};
	CHECK(kindOf({ "A hearts", "K clubs", "2 spades", "7 diamonds", "9 clubs", "J hearts", "4 spades" }) == 0.0f);
	CHECK(kindOf({ "A hearts", "A clubs", "2 spades", "7 diamonds", "9 clubs", "J hearts", "4 spades" }) == 1.0f);
	CHECK(kindOf({ "A hearts", "A clubs", "2 spades", "7 diamonds", "9 clubs", "2 hearts", "4 spades" }) == 2.0f);
	CHECK(kindOf({ "A hearts", "A clubs", "A spades", "7 diamonds", "9 clubs", "J hearts", "4 spades" }) == 3.0f);
	CHECK(kindOf({ "A hearts", "2 clubs", "3 spades", "4 diamonds", "5 clubs", "J hearts", "J spades" }) == 4.0f);
	CHECK(kindOf({ "A hearts", "2 hearts", "9 hearts", "4 hearts", "5 clubs", "J hearts", "J spades" }) == 5.0f);
	CHECK(kindOf({ "A hearts", "A clubs", "J diamonds", "4 diamonds", "5 clubs", "J hearts", "J spades" }) == 6.0f);
	CHECK(kindOf({ "J clubs", "A clubs", "J diamonds", "4 diamonds", "5 clubs", "J hearts", "J spades" }) == 7.0f);
	CHECK(kindOf({ "10 hearts", "K hearts", "Q hearts", "J hearts", "A hearts", "2 clubs", "3 spades" }) == 8.0f);
	CHECK(play.get("yourchips").picture.text == "you: 500 chips, a straight flush");

	// A wheel (ace to five) is under a six-high straight; a flush beats both.
	const float wheel = (play.lay({ "A hearts", "2 clubs", "3 spades", "4 diamonds", "5 clubs", "K hearts", "K spades" }), play.pile("yours").strength);
	const float sixHigh = (play.lay({ "6 hearts", "2 clubs", "3 spades", "4 diamonds", "5 clubs", "K hearts", "K spades" }), play.pile("yours").strength);
	const float flush = (play.lay({ "2 hearts", "7 hearts", "3 hearts", "4 diamonds", "5 clubs", "K hearts", "9 hearts" }), play.pile("yours").strength);
	CHECK(wheel < sixHigh);
	CHECK(sixHigh < flush);

	// Two pair: the higher pair decides, then the lower, then the fifth card.
	const float kingsSevens = (play.lay({ "K hearts", "K clubs", "7 spades", "7 diamonds", "2 clubs", "3 hearts", "4 spades" }), play.pile("yours").strength);
	const float kingsSixesAce = (play.lay({ "K hearts", "K clubs", "6 spades", "6 diamonds", "A clubs", "3 hearts", "4 spades" }), play.pile("yours").strength);
	const float kingsSevensAce = (play.lay({ "K hearts", "K clubs", "7 spades", "7 diamonds", "A clubs", "3 hearts", "4 spades" }), play.pile("yours").strength);
	CHECK(kingsSixesAce < kingsSevens);
	CHECK(kingsSevens < kingsSevensAce);
}

TEST_CASE("holdem.xml: the odds against one unseen hand: a pair of aces about 85, seven-two about 35", "[cards][holdem]")
{
	Holdem play;
	play.lay({ "A hearts", "A clubs" });
	CHECK(play.pile("yours").odds >= 80.0f);
	CHECK(play.pile("yours").odds <= 90.0f);
	play.lay({ "7 hearts", "2 clubs" });
	CHECK(play.pile("yours").odds >= 28.0f);
	CHECK(play.pile("yours").odds <= 40.0f);
	// The same cards, the same odds.
	const float once = play.pile("yours").odds;
	play.lay({ "7 hearts", "2 clubs" });
	CHECK(play.pile("yours").odds == once);
	// A made hand on the river: nothing left to come, a straight flush cannot lose.
	play.lay({ "10 hearts", "K hearts", "Q hearts", "J hearts", "A hearts", "2 clubs", "3 spades" });
	CHECK(play.pile("yours").odds == 100.0f);
}

TEST_CASE("holdem.xml: the deal antes five each; checked through, the better hand takes the pot at the showdown", "[cards][holdem]")
{
	Holdem play;
	play.deal({ "A hearts", "2 clubs", "K hearts", "7 diamonds", "Q hearts", "J hearts", "10 hearts", "3 spades", "4 clubs" });
	CHECK(play.table("pot") == 10.0f);
	CHECK(play.chips("you") == 495.0f);
	CHECK(play.pile("yours").pile.cards.size() == 2);
	CHECK(play.pile("theirs").pile.cards.size() == 2);
	for (const int number : play.pile("theirs").pile.cards) { CHECK_FALSE(play.game.cardAt(number)->faceUp); }
	CHECK(play.get("you").lookName() == "act");

	// You check each round; the computer, with little, checks too.
	for (int street = 0; street < 4 && play.table("street") < 4.0f; ++street)
	{
		play.press(KeyCode::Space);
		play.untilYou();
	}
	CHECK(play.pile("board").pile.cards.size() == 5);
	CHECK(play.pile("yours").pattern == 8.0f);
	CHECK(play.get("result").lookName() == "youwin");
	CHECK(play.chips("you") == 505.0f);
	CHECK(play.chips("cpu") == 495.0f);
	for (const int number : play.pile("theirs").pile.cards) { CHECK(play.game.cardAt(number)->faceUp); }
	CHECK(play.get("you").lookName() == "done");
}

TEST_CASE("holdem.xml: the same best five splits the pot", "[cards][holdem]")
{
	Holdem play;
	play.deal({ "2 hearts", "2 clubs", "3 hearts", "3 clubs", "A spades", "K spades", "Q spades", "J spades", "10 spades" });
	for (int street = 0; street < 4 && play.table("street") < 4.0f; ++street)
	{
		play.press(KeyCode::Space);
		play.untilYou();
	}
	CHECK(play.get("result").lookName() == "split");
	CHECK(play.chips("you") == 500.0f);
	CHECK(play.chips("cpu") == 500.0f);
}

TEST_CASE("holdem.xml: a bet the computer's odds do not pay for, it folds; you fold, it takes the pot", "[cards][holdem]")
{
	Holdem play;
	// Your aces against its seven-two; the flop gives you a third ace.
	play.deal({ "A hearts", "7 clubs", "A clubs", "2 diamonds", "A spades", "K spades", "9 hearts", "4 hearts", "5 spades" });
	play.press(KeyCode::Space);
	play.untilYou();
	REQUIRE(play.table("street") == 1.0f);
	play.press(KeyCode::W);
	CHECK(play.table("owed") == 10.0f);
	play.untilYou();
	CHECK(play.get("cpusays").lookName() == "fold");
	CHECK(play.get("result").lookName() == "cpufolds");
	CHECK(play.chips("you") + play.chips("cpu") == 1000.0f);
	CHECK(play.chips("you") > 500.0f);

	// The next hand the computer acts first; you fold to whatever it does.
	const float yoursBefore = play.chips("you");
	play.deal({});
	CHECK(play.table("first") == 2.0f);
	play.untilYou();
	play.press(KeyCode::X);
	play.frames(2);
	CHECK(play.get("result").lookName() == "youfold");
	CHECK(play.table("pot") == 0.0f);
	CHECK(play.chips("you") == yoursBefore - 5.0f);
	CHECK(play.chips("you") + play.chips("cpu") == 1000.0f);
}

TEST_CASE("holdem.xml: raises stop at three a round; out of chips between hands ends the match", "[cards][holdem]")
{
	Holdem play;
	play.deal({ "A hearts", "A clubs", "A diamonds", "A spades", "K spades", "K hearts", "9 hearts", "4 hearts", "5 spades" });
	for (int i = 0; i < 6 && play.table("street") == 0.0f; ++i)
	{
		if (play.get("you").lookName() == "act") { play.press(KeyCode::W); }
		else { play.press(KeyCode::Space); }
		play.untilYou();
	}
	CHECK(play.table("raises") <= 3.0f);
	CHECK(play.table("street") >= 1.0f);

	play.get("cpu").variable["chips"] = 0.0f;
	play.get("table").variable["street"] = 5.0f;
	play.frames(2);
	CHECK(play.state() == "over");
	CHECK(play.get("over.winner").lookName() == "you");
}

TEST_CASE("holdem.xml: hand after hand, you calling everything, every hand ends and no chip is lost", "[cards][holdem]")
{
	Holdem play;
	int hands = 0;
	for (; hands < 40 && play.state() == "playing"; ++hands)
	{
		play.deal({});
		for (int i = 0; i < 20 && play.table("street") < 4.0f; ++i)
		{
			play.untilYou();
			if (play.table("street") < 4.0f) { play.press(KeyCode::Space); }
		}
		play.frames(2);
		REQUIRE(play.table("street") == 5.0f);
		CHECK(play.table("pot") == 0.0f);
		CHECK(play.chips("you") + play.chips("cpu") == 1000.0f);
	}
	CHECK(hands > 0);
}

namespace
{
	struct Hearts : Played
	{
		Hearts() : Played("descriptions/hearts.xml")
		{
			tap(KeyCode::Space);
			frames(2);
		}

		Object& pile(const std::string& name) { return get(name); }
		float table(const std::string& name) { return get("table").variable.at(name); }
		float of(const std::string& hand, const std::string& name) { return get(hand).variable.at(name); }
		void set(const std::string& object, const std::string& name, float value) { game.setVariable(get(object).name, name, value); }

		int cardOf(const std::string& name)
		{
			const auto space = name.find(' ');
			for (Object& object : game.getCurrentObjects())
			{
				if (object.isCard && object.card.rankName == name.substr(0, space) && object.card.suitName == name.substr(space + 1)) { return object.cardNumber; }
			}
			return 0;
		}

		// A new deal from a stacked stock: thirteen each to you, west,
		// north and east, in that order, the rest of each suit by rank
		// filling up; `leader` leads the first trick.
		void deal(const std::vector<std::vector<std::string>>& hands, int leader)
		{
			set("table", "phase", 3.0f);
			set("table", "first", static_cast<float>(leader == 1 ? 4 : leader - 1));
			frames(2);
			tap(KeyCode::Space);
			frames(1);
			auto& cards = pile("stock").pile.cards;
			std::vector<std::string> order;
			for (const auto& hand : hands) { order.insert(order.end(), hand.begin(), hand.end()); }
			for (auto each = order.rbegin(); each != order.rend(); ++each)
			{
				const int number = cardOf(*each);
				cards.erase(std::find(cards.begin(), cards.end(), number));
				cards.push_back(number);
			}
			for (int i = 0; i < 100 && table("phase") != 0.0f; ++i) { frames(1); }
			frames(2);
		}

		// Your hand pointed at that card (as "Q spades"), and Space.
		bool play(const std::string& name)
		{
			const auto& mine = pile("mine").pile.cards;
			const auto at = std::find(mine.begin(), mine.end(), cardOf(name));
			if (at == mine.end()) { return false; }
			Object& hand = get("hand");
			hand.hand.pile = "playing.mine";
			hand.hand.at = static_cast<int>(at - mine.begin());
			tap(KeyCode::Space);
			frames(1);
			return std::find(pile("trick").pile.cards.begin(), pile("trick").pile.cards.end(), cardOf(name)) != pile("trick").pile.cards.end();
		}

		// Frames until it is your turn to play, or the hand is over.
		void untilYou()
		{
			for (int i = 0; i < 3000; ++i)
			{
				if (table("phase") == 3.0f || state() != "playing") { return; }
				if (table("phase") == 0.0f && table("turn") == 1.0f && get("hand").lookName() == "free") { return; }
				frames(1);
			}
		}
	};

	std::vector<std::string> suitOf(const std::string& suit, std::initializer_list<const char*> ranks)
	{
		std::vector<std::string> cards;
		for (const char* rank : ranks) { cards.push_back(std::string(rank) + " " + suit); }
		return cards;
	}

	std::vector<std::string> joined(std::vector<std::string> a, const std::vector<std::string>& b)
	{
		a.insert(a.end(), b.begin(), b.end());
		return a;
	}
}

TEST_CASE("hearts.xml: the deal: thirteen each, yours face up in suit order, the ace last", "[cards][hearts]")
{
	Hearts play;
	for (const char* name : { "mine", "west", "north", "east" }) { CHECK(play.pile(name).pile.cards.size() == 13); }
	CHECK(play.pile("stock").pile.cards.empty());
	const auto& mine = play.pile("mine").pile.cards;
	for (std::size_t i = 1; i < mine.size(); ++i)
	{
		const Object& before = *play.game.cardAt(mine[i - 1]);
		const Object& after = *play.game.cardAt(mine[i]);
		CHECK(std::make_pair(before.card.suit, before.card.worths.back()) < std::make_pair(after.card.suit, after.card.worths.back()));
		CHECK(after.faceUp);
	}
	CHECK(play.table("turn") == 1.0f);
	CHECK(play.get("hand").lookName() == "free");
}

TEST_CASE("hearts.xml: the suit led is followed; hearts lead only once broken, or when nothing else is held", "[cards][hearts]")
{
	Hearts play;
	// You: the 2 of clubs, a heart and eleven spades; west the other clubs and a diamond;
	// north every heart but yours; east the queen of spades and the diamonds.
	play.deal({
		joined(joined(suitOf("clubs", { "2" }), suitOf("hearts", { "A" })), suitOf("spades", { "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "K" })),
		joined(suitOf("clubs", { "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A" }), suitOf("diamonds", { "2" })),
		joined(suitOf("hearts", { "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K" }), suitOf("spades", { "A" })),
		joined(suitOf("spades", { "Q" }), suitOf("diamonds", { "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A" })),
	}, 1);
	REQUIRE(play.table("turn") == 1.0f);
	// Hearts are not broken: no heart leads.
	CHECK_FALSE(play.play("A hearts"));
	CHECK(play.pile("mine").pile.cards.size() == 13);
	CHECK(play.play("2 clubs"));
	CHECK(play.pile("trick").lead == 3.0f);

	// West follows low; north and east, with no clubs, shed their worst:
	// north its king of hearts, east the queen of spades.
	play.untilYou();
	CHECK(play.of("westhand", "got") == 14.0f);
	CHECK(play.table("tricks") == 1.0f);
	// A heart has been laid: hearts are broken.
	const auto& broken = play.pile("trick").pile.broken;
	CHECK(std::find(broken.begin(), broken.end(), play.game.cardAt(play.cardOf("A hearts"))->card.suit) != broken.end());
	// West led again (its diamond), north shed another heart, east followed;
	// you have no diamond, so anything goes.
	CHECK(play.pile("trick").pile.cards.size() == 3);
	CHECK(play.play("A hearts"));
}

TEST_CASE("hearts.xml: a card not of the suit led is refused while you hold one", "[cards][hearts]")
{
	Hearts play;
	// West leads its 2 of clubs; you hold a club and must play it.
	play.deal({
		joined(suitOf("clubs", { "K" }), suitOf("diamonds", { "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K" })),
		joined(suitOf("clubs", { "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "A" }), suitOf("diamonds", { "A" })),
		suitOf("hearts", { "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A" }),
		suitOf("spades", { "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A" }),
	}, 2);
	play.untilYou();
	REQUIRE(play.pile("trick").pile.cards.size() == 3);
	CHECK(play.pile("trick").lead == 3.0f);
	CHECK_FALSE(play.play("5 diamonds"));
	CHECK(play.get("hand").lookName() == "free");
	CHECK(play.play("K clubs"));
	// Yours is the highest club: you take it, with north's heart and east's queen of spades.
	play.untilYou();
	CHECK(play.of("hand", "got") == 14.0f);
	CHECK(play.pile("stock").pile.cards.size() == 4);
	CHECK(play.table("turn") == 1.0f);
}

TEST_CASE("hearts.xml: holding nothing but hearts, one leads before they are broken", "[cards][hearts]")
{
	Hearts play;
	play.deal({
		suitOf("hearts", { "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A" }),
		suitOf("clubs", { "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A" }),
		suitOf("diamonds", { "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A" }),
		suitOf("spades", { "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A" }),
	}, 1);
	CHECK(play.play("2 hearts"));
	// Nobody else has a heart: you take your own, and east sheds the queen of spades on it.
	play.untilYou();
	CHECK(play.of("hand", "got") == 14.0f);
}

TEST_CASE("hearts.xml: the hand's points to the scores; all 26 to one shoots the moon", "[cards][hearts]")
{
	Hearts play;
	play.set("table", "tricks", 13.0f);
	play.set("hand", "got", 3.0f);
	play.set("westhand", "got", 13.0f);
	play.set("northhand", "got", 10.0f);
	play.frames(2);
	CHECK(play.of("hand", "score") == 3.0f);
	CHECK(play.of("westhand", "score") == 13.0f);
	CHECK(play.of("northhand", "score") == 10.0f);
	CHECK(play.table("phase") == 3.0f);
	CHECK(play.get("hand").lookName() == "done");

	play.set("hand", "got", 0.0f);
	play.set("westhand", "got", 0.0f);
	play.set("northhand", "got", 26.0f);
	play.set("table", "phase", 0.0f);
	play.frames(2);
	CHECK(play.of("hand", "score") == 29.0f);
	CHECK(play.of("westhand", "score") == 39.0f);
	CHECK(play.of("northhand", "score") == 10.0f);
	CHECK(play.of("easthand", "score") == 26.0f);
	CHECK(play.get("message").lookName() == "moon");
}

TEST_CASE("hearts.xml: at 100 after a hand the lowest score wins", "[cards][hearts]")
{
	Hearts play;
	play.set("hand", "score", 40.0f);
	play.set("westhand", "score", 104.0f);
	play.set("northhand", "score", 35.0f);
	play.set("easthand", "score", 60.0f);
	play.set("table", "phase", 3.0f);
	play.frames(2);
	CHECK(play.state() == "over");
	CHECK(play.get("over.winner").lookName() == "north");
}

TEST_CASE("hearts.xml: a whole hand, you playing the first card that goes: 26 points are taken", "[cards][hearts]")
{
	Hearts play;
	for (int trick = 0; trick < 13 && play.table("phase") != 3.0f; ++trick)
	{
		play.untilYou();
		if (play.table("phase") == 3.0f) { break; }
		bool played = false;
		const std::vector<int> mine = play.pile("mine").pile.cards;
		for (const int number : mine)
		{
			const Object& card = *play.game.cardAt(number);
			if (play.play(card.card.rankName + " " + card.card.suitName))
			{
				played = true;
				break;
			}
		}
		REQUIRE(played);
		play.frames(2);
	}
	play.untilYou();
	REQUIRE(play.table("phase") == 3.0f);
	const float total = play.of("hand", "score") + play.of("westhand", "score") + play.of("northhand", "score") + play.of("easthand", "score");
	CHECK((total == 26.0f || total == 78.0f));
	CHECK(play.pile("stock").pile.cards.size() == 52);

	// Space deals again; the next to lead is west.
	play.frames(2);
	play.tap(KeyCode::Space);
	for (int i = 0; i < 100 && play.table("phase") != 0.0f; ++i) { play.frames(1); }
	play.frames(1);
	CHECK(play.pile("mine").pile.cards.size() == 13);
	CHECK(play.table("turn") == 2.0f);
}
