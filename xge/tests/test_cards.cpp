// test_cards.cpp
// XML Game Engine
// author: beefviper
// date: Nov 1, 2026
//
// Catch2 tests for cards: a <deck> made from suits and ranks, piles and
// their rules, the deal, and a hand that takes, lays and puts back
// (klondike.xml).

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
