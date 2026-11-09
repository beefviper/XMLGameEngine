// test_cards.cpp
// XML Game Engine
// author: beefviper
// date: Nov 1, 2026
//
// Catch2 tests for cards: a pack laid out as a group, holders and their
// rules, the deal, and a picker that picks, places and puts back, game by
// game.

#include "game/cards.h"
#include "played.h"
#include "scratch.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
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
	// Klondike's pack is a group: a card's suit is its row (spades,
	// hearts, clubs, diamonds) and its rank its column (A to K).
	const std::vector<std::string> klondikeSuits{ "spades", "hearts", "clubs", "diamonds" };
	const std::vector<std::string> klondikeRanks{ "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K" };

	int numberIn(const std::vector<std::string>& names, const std::string& name)
	{
		return static_cast<int>(std::find(names.begin(), names.end(), name) - names.begin()) + 1;
	}

	// The card of that suit and rank.
	int cardOf(Table& table, const std::string& suit, const std::string& rank)
	{
		for (Object& object : table.game.getCurrentObjects())
		{
			if (object.isCard && object.card.suit == numberIn(klondikeSuits, suit) && object.card.rank == numberIn(klondikeRanks, rank)) { return object.cardNumber; }
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
		return klondikeRanks[static_cast<std::size_t>(card.card.rank - 1)] + " " + klondikeSuits[static_cast<std::size_t>(card.card.suit - 1)];
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

namespace
{
	// A game of one state holding `objects`, with the card sheet as two
	// <sheets>: "cards", a grid of 13 by 5, and "loose", no grid but a
	// margin and gap that change nothing (cells by x and y).
	struct SheetGame
	{
		std::string path;
		explicit SheetGame(const std::string& objects, const std::string& sheets = "")
			: path(scratchName("descriptions/sheet_scratch.xml"))
		{
			std::ofstream out(path);
			out << "<game xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"../xgedef.xsd\">"
				"<window name=\"sheets\"><width>800</width><height>600</height><background>color.black</background></window>"
				"<variables><variable name=\"unused\">0</variable></variables>"
				"<sheets><sheet name=\"cards\"><path>assets/cards.png</path><columns>13</columns><rows>5</rows></sheet>"
				"<sheet name=\"sized\"><path>assets/cards.png</path><cell><width>192</width><height>280</height></cell></sheet>" << sheets << "</sheets>"
				"<states><state name=\"playing\"><objects>" << objects << "</objects><inputs><input><button>escape</button><pop /></input></inputs></state></states></game>";
		}
		~SheetGame() { std::error_code ignored; std::filesystem::remove(path, ignored); }
	};
}

TEST_CASE("a pack cut from a sheet: a face for each suit and rank, one back for all", "[cards][sheets]")
{
	Played hearts("descriptions/hearts.xml");
	hearts.tap(KeyCode::Space);
	hearts.frames(2);
	Object& ace = *hearts.game.cardAt(1);
	REQUIRE(ace.looks.size() == 2);
	const Bitmap& face = *ace.looks[0].bitmap;
	REQUIRE(face.width == 96);
	REQUIRE(face.height == 140);
	CHECK_FALSE(face.solidAt(0, 0));          // a rounded corner
	CHECK(face.solidAt(48, 70));               // the big spade
	CHECK(face.rgba[(70 * 96 + 48) * 4] < 40);
	CHECK(ace.looks[1].name == "back");
	CHECK(ace.looks[1].bitmap == hearts.game.cardAt(52)->looks[1].bitmap);   // one back, shared
	CHECK(ace.looks[0].bitmap != hearts.game.cardAt(14)->looks[0].bitmap);
}

TEST_CASE("sheets: a cell by column and row, by x and y, by cell size; runs of cells as frames and faces", "[cards][sheets]")
{
	const std::string at = "<position><x>0</x><y>0</y></position>";
	SheetGame sheet(
		// the ace of hearts three ways, at half the sheet's size
		"<object name=\"grid\"><sprite><cell sheet=\"cards\"><column>1</column><row>2</row><scale>0.5</scale></cell></sprite>" + at + "</object>"
		"<object name=\"part\"><sprite><cell sheet=\"cards\"><x>0</x><y>280</y><width>192</width><height>280</height><scale>0.5</scale></cell></sprite>" + at + "</object>"
		"<object name=\"sized\"><sprite><cell sheet=\"sized\"><row>2</row><scale>0.5</scale></cell></sprite>" + at + "</object>"
		// an animation: three cells of the second row, then one of the third
		"<object name=\"walker\"><animation><interval>0.1</interval>"
		"<cells sheet=\"cards\"><row>2</row><count>3</count><scale>0.5</scale></cells>"
		"<cell sheet=\"cards\"><row>3</row><scale>0.5</scale></cell></animation>" + at + "</object>"
		// a group whose only pictures are its animation's, a run down a column
		"<group name=\"flock\"><animation><interval>0.1</interval><cells sheet=\"cards\"><count>2</count><down /><scale>0.5</scale></cells></animation>" + at +
		"<member name=\"bird\" /></group>");
	Played play(sheet.path);
	Object& grid = play.get("grid");
	const auto drawn = [](const Object& object) -> const Bitmap& { return *object.bitmap; };
	REQUIRE(drawn(grid).width == 96);
	CHECK((drawn(grid).rgba == drawn(play.get("part")).rgba));
	CHECK((drawn(grid).rgba == drawn(play.get("sized")).rgba));

	Object& walker = play.get("walker");
	REQUIRE(walker.animationBitmaps.size() == 4);
	CHECK((walker.animationBitmaps[0]->rgba == drawn(grid).rgba));
	CHECK((walker.animationBitmaps[1]->rgba != drawn(grid).rgba));
	Object& bird = play.get("bird");
	REQUIRE(bird.animationBitmaps.size() == 2);
	CHECK((bird.animationBitmaps[1]->rgba == drawn(grid).rgba));   // column 1 down to row 2: the ace of hearts
}

TEST_CASE("klondike.xml: the pack is a group whose cells cut their own faces from the sheet", "[cards][sheets]")
{
	Table table;
	Played hearts("descriptions/hearts.xml");
	hearts.tap(KeyCode::Space);
	hearts.frames(2);
	// Card 22 is the 9 of hearts in both: row 2, column 9.
	Object& nine = table.card(cardOf(table, "hearts", "9"));
	CHECK(nine.cardNumber == 22);
	CHECK(nine.card.suit == 2);
	CHECK(nine.card.rank == 9);
	CHECK((nine.looks[0].bitmap->rgba == hearts.game.cardAt(22)->looks[0].bitmap->rgba));
	CHECK((nine.looks[1].bitmap->rgba == hearts.game.cardAt(22)->looks[1].bitmap->rgba));
	CHECK(nine.variable.at("color") == 2.0f);
	CHECK(table.card(cardOf(table, "clubs", "9")).variable.at("color") == 1.0f);
}

TEST_CASE("sheets: a cell off the grid and a sheet not there stop the load", "[cards][sheets]")
{
	const std::string at = "<position><x>0</x><y>0</y></position>";
	{
		SheetGame sheet("<object name=\"off\"><sprite><cell sheet=\"cards\"><column>14</column></cell></sprite>" + at + "</object>");
		CHECK_THROWS(Played(sheet.path));
	}
	{
		SheetGame sheet("<object name=\"none\"><sprite><cell sheet=\"nothing\" /></sprite>" + at + "</object>");
		CHECK_THROWS(Played(sheet.path));
	}
	{
		SheetGame sheet("<object name=\"lost\"><sprite><cell sheet=\"missing\" /></sprite>" + at + "</object>",
			"<sheet name=\"missing\"><path>assets/nothing.png</path><columns>1</columns><rows>1</rows></sheet>");
		CHECK_THROWS(Played(sheet.path));
	}
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
		Object& hand() { return get("you"); }
		Object& computer() { return get("computer"); }
		Object& card(int number) { return *game.cardAt(number); }
		// 1 your turn, 2 the computer's, 3 yours to name a suit, 4 its.
		float turn()
		{
			const bool yours = hand().variable.at("turn") == 1.0f;
			const bool calling = (yours ? hand() : computer()).lookName() == "calling";
			return (yours ? 1.0f : 2.0f) + (calling ? 2.0f : 0.0f);
		}
		// The computer's turn, as a pass would give it.
		void computersTurn()
		{
			game.setVariable(hand().name, "turn", 0.0f);
			showLookNamed(hand(), "waiting");
			game.setVariable(computer().name, "turn", 1.0f);
			computer().showLook(0);
		}

		int cardOf(const std::string& rank, const std::string& suit)
		{
			for (Object& object : game.getCurrentObjects())
			{
				if (object.isCard && object.card.suit == numberIn(klondikeSuits, suit) && object.card.rank == numberIn(klondikeRanks, rank)) { return object.cardNumber; }
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

		std::string name(int number) { return klondikeRanks[static_cast<std::size_t>(card(number).card.rank - 1)] + " " + klondikeSuits[static_cast<std::size_t>(card(number).card.suit - 1)]; }
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
	CHECK(table.hand().lookName() == "free");
	CHECK(table.computer().lookName() == "waiting");
	CHECK(table.get("whoseturn").textPieces.front().shown == "your turn: play a card that matches, or draw");
}

TEST_CASE("crazyeights.xml: keys reach only the player on its turn, and never the one the computer plays", "[cards][eights][players]")
{
	Eights table;
	table.setUp({ { "discard", { "5 hearts" } }, { "mine", { "K clubs", "5 spades" } }, { "theirs", { "3 clubs", "4 clubs" } }, { "stock", { "6 clubs" } } });
	table.computersTurn();
	table.computer().hand.pile = table.pile("theirs").name;
	table.computer().hand.at = 0;
	table.hand().hand.at = 1;
	// Space and the arrows: nobody's, the computer's cards stay where they are.
	table.engine.handleKeyPressed(KeyCode::Space);
	table.engine.handleKeyReleased(KeyCode::Space);
	table.engine.handleKeyPressed(KeyCode::D);
	table.engine.handleKeyReleased(KeyCode::D);
	CHECK(table.pile("theirs").pile.cards.size() == 2);
	CHECK(table.pile("mine").pile.cards.size() == 2);
	CHECK(table.hand().hand.at == 1);
	CHECK(table.computer().hand.at == 0);
	CHECK(table.get("whoseturn").textPieces.front().shown == "the computer is playing");
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
	CHECK(table.get("called").textPieces.front().shown == "spades");

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
	table.computersTurn();
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

		// A card of the pack by name ("A", "hearts"): its column and row.
		int cardOf(const std::string& rank, const std::string& suit)
		{
			const std::vector<std::string> suits{ "spades", "hearts", "clubs", "diamonds" };
			const std::vector<std::string> ranks{ "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K" };
			const int row = static_cast<int>(std::find(suits.begin(), suits.end(), suit) - suits.begin()) + 1;
			const int column = static_cast<int>(std::find(ranks.begin(), ranks.end(), rank) - ranks.begin()) + 1;
			for (Object& object : game.getCurrentObjects())
			{
				if (object.isCard && object.card.suit == row && object.card.rank == column) { return object.cardNumber; }
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
			for (int i = 0; i < 2000 && get("table").lookName() != "done" && state() == "playing"; ++i) { frames(1); }
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
	CHECK(play.get("table").lookName() == "betting");
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
	// A card of the pack (rows spades, hearts, clubs, diamonds; columns A
	// to K) by its name, "Q spades"; and a card's name.
	const std::vector<std::string> packSuits{ "spades", "hearts", "clubs", "diamonds" };
	const std::vector<std::string> packRanks{ "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K" };

	int cardNamed(Game& game, const std::string& name)
	{
		const auto space = name.find(' ');
		const auto numberOf = [](const std::vector<std::string>& names, const std::string& given) { return static_cast<int>(std::find(names.begin(), names.end(), given) - names.begin()) + 1; };
		const int column = numberOf(packRanks, name.substr(0, space));
		const int row = numberOf(packSuits, name.substr(space + 1));
		for (Object& object : game.getCurrentObjects())
		{
			if (object.isCard && object.card.rank == column && object.card.suit == row) { return object.cardNumber; }
		}
		return 0;
	}

	std::string nameOf(const Object& card)
	{
		return packRanks[static_cast<std::size_t>(card.card.rank - 1)] + " " + packSuits[static_cast<std::size_t>(card.card.suit - 1)];
	}
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

		int cardOf(const std::string& name) { return cardNamed(game, name); }

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
	// The order a stock is stacked in (first on top) so that a deal a
	// card a round gives each pile its cards, the piles in deal order.
	template <typename Table>
	std::vector<int> roundRobin(Table& table, const std::vector<std::vector<std::string>>& piles)
	{
		std::vector<int> order;
		for (std::size_t round = 0;; ++round)
		{
			bool any = false;
			for (const auto& cards : piles)
			{
				if (round < cards.size())
				{
					order.push_back(table.cardOf(cards[round]));
					any = true;
				}
			}
			if (!any) { break; }
		}
		return order;
	}

	// A trick game of four, you, west, north and east (Hearts, Spades).
	struct Tricks : Played
	{
		explicit Tricks(const std::string& file) : Played(file)
		{
			tap(KeyCode::Space);
			frames(2);
		}

		Object& pile(const std::string& name) { return get(name); }
		float table(const std::string& name) { return get("table").variable.at(name); }
		float of(const std::string& hand, const std::string& name) { return get(hand).variable.at(name); }
		void set(const std::string& object, const std::string& name, float value) { game.setVariable(get(object).name, name, value); }

		// Whose turn it is: the number of the player on it, 0 nobody's.
		float turn()
		{
			const char* names[] = { "you", "west", "north", "east" };
			for (int i = 0; i < 4; ++i)
			{
				if (of(names[i], "turn") == 1.0f) { return static_cast<float>(i + 1); }
			}
			return 0.0f;
		}

		int cardOf(const std::string& name) { return cardNamed(game, name); }

		// A new deal from a stacked stock: thirteen each to you, west,
		// north and east, in that order, the rest of each suit by rank
		// filling up; `leader` leads the first trick.
		void deal(const std::vector<std::vector<std::string>>& hands, int leader)
		{
			set("table", "phase", 3.0f);
			set("table", "first", static_cast<float>(leader == 1 ? 4 : leader - 1));
			// As a hand ends: the table done, nobody's turn.
			showLookNamed(get("table"), "done");
			game.passPlayersTo("players", 0);
			frames(2);
			// Dealt a card a round to the players in turn: you, west, north and east.
			game.stackedDeal = roundRobin(*this, hands);
			tap(KeyCode::Space);
			for (int i = 0; i < 100 && table("phase") != 0.0f && table("phase") != 5.0f; ++i) { frames(1); }
			frames(2);
		}

		// Your hand pointed at that card (as "Q spades"), and Space.
		bool play(const std::string& name)
		{
			const auto& mine = pile("mine").pile.cards;
			const auto at = std::find(mine.begin(), mine.end(), cardOf(name));
			if (at == mine.end()) { return false; }
			Object& hand = get("you");
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
				if (table("phase") == 0.0f && turn() == 1.0f && get("you").lookName() == "free")
				{
					// The computer's own press of the same key let go first.
					frames(1);
					return;
				}
				frames(1);
			}
		}
	};

	struct Hearts : Tricks
	{
		Hearts() : Tricks("descriptions/hearts.xml") {}
	};

	// Spades: the first deal comes after a moment; then you bid.
	struct Spades : Tricks
	{
		Spades() : Tricks("descriptions/spades.xml")
		{
			for (int i = 0; i < 100 && table("phase") != 5.0f; ++i) { frames(1); }
			frames(2);
		}

		void deal(const std::vector<std::vector<std::string>>& hands, int leader)
		{
			Tricks::deal(hands, leader);
			tap(KeyCode::Space);
			frames(2);
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
	for (const char* name : { "mine", "westcards", "northcards", "eastcards" }) { CHECK(play.pile(name).pile.cards.size() == 13); }
	CHECK(play.pile("stock").pile.cards.empty());
	const auto& mine = play.pile("mine").pile.cards;
	for (std::size_t i = 1; i < mine.size(); ++i)
	{
		const Object& before = *play.game.cardAt(mine[i - 1]);
		const Object& after = *play.game.cardAt(mine[i]);
		CHECK(std::make_pair(before.card.suit, before.card.worths.back()) < std::make_pair(after.card.suit, after.card.worths.back()));
		CHECK(after.faceUp);
	}
	CHECK(play.turn() == 1.0f);
	CHECK(play.get("you").lookName() == "free");
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
	REQUIRE(play.turn() == 1.0f);
	// Hearts are not broken: no heart leads.
	CHECK_FALSE(play.play("A hearts"));
	CHECK(play.pile("mine").pile.cards.size() == 13);
	CHECK(play.play("2 clubs"));
	CHECK(play.pile("trick").lead == 3.0f);

	// West follows low; north and east, with no clubs, shed their worst:
	// north its king of hearts, east the queen of spades.
	play.untilYou();
	CHECK(play.of("west", "got") == 14.0f);
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
	CHECK(play.get("you").lookName() == "free");
	CHECK(play.play("K clubs"));
	// Yours is the highest club: you take it, with north's heart and east's queen of spades.
	play.untilYou();
	CHECK(play.of("you", "got") == 14.0f);
	CHECK(play.pile("stock").pile.cards.size() == 4);
	CHECK(play.turn() == 1.0f);
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
	CHECK(play.of("you", "got") == 14.0f);
}

TEST_CASE("hearts.xml: the hand's points to the scores; all 26 to one shoots the moon", "[cards][hearts]")
{
	Hearts play;
	play.set("table", "tricks", 13.0f);
	play.set("you", "got", 3.0f);
	play.set("west", "got", 13.0f);
	play.set("north", "got", 10.0f);
	play.frames(2);
	CHECK(play.of("you", "score") == 3.0f);
	CHECK(play.of("west", "score") == 13.0f);
	CHECK(play.of("north", "score") == 10.0f);
	CHECK(play.table("phase") == 3.0f);
	CHECK(play.get("table").lookName() == "done");
	CHECK(play.turn() == 0.0f);

	play.set("you", "got", 0.0f);
	play.set("west", "got", 0.0f);
	play.set("north", "got", 26.0f);
	play.set("table", "phase", 0.0f);
	play.frames(2);
	CHECK(play.of("you", "score") == 29.0f);
	CHECK(play.of("west", "score") == 39.0f);
	CHECK(play.of("north", "score") == 10.0f);
	CHECK(play.of("east", "score") == 26.0f);
	CHECK(play.get("message").lookName() == "moon");
}

TEST_CASE("hearts.xml: at 100 after a hand the lowest score wins", "[cards][hearts]")
{
	Hearts play;
	play.set("you", "score", 40.0f);
	play.set("west", "score", 104.0f);
	play.set("north", "score", 35.0f);
	play.set("east", "score", 60.0f);
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
			if (play.play(nameOf(card)))
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
	const float total = play.of("you", "score") + play.of("west", "score") + play.of("north", "score") + play.of("east", "score");
	CHECK((total == 26.0f || total == 78.0f));
	CHECK(play.pile("stock").pile.cards.size() == 52);

	// Space deals again; the next to lead is west.
	play.frames(2);
	play.tap(KeyCode::Space);
	for (int i = 0; i < 100 && play.table("phase") != 0.0f; ++i) { play.frames(1); }
	play.frames(1);
	CHECK(play.pile("mine").pile.cards.size() == 13);
	CHECK(play.turn() == 2.0f);
}

TEST_CASE("spades.xml: everyone bids what their cards should take; you change yours and bid", "[cards][spades]")
{
	Spades play;
	REQUIRE(play.table("phase") == 5.0f);
	CHECK(play.get("table").lookName() == "bid");
	CHECK(play.turn() == 0.0f);
	for (const auto& [hand, pile] : { std::make_pair("west", "westcards"), std::make_pair("north", "northcards"), std::make_pair("east", "eastcards"), std::make_pair("you", "mine") })
	{
		CHECK(play.of(hand, "bid") == std::max(play.pile(pile).tricks, 1.0f));
	}
	const float first = play.of("you", "bid");
	play.tap(KeyCode::W);
	play.frames(1);
	CHECK(play.of("you", "bid") == std::min(first + 1.0f, 13.0f));
	for (int i = 0; i < 16; ++i) { play.tap(KeyCode::S); }
	play.frames(2);
	CHECK(play.of("you", "bid") == 1.0f);
	play.tap(KeyCode::Space);
	play.frames(2);
	CHECK(play.table("phase") == 0.0f);
	CHECK(play.table("usbid") == 1.0f + play.of("north", "bid"));
	CHECK(play.table("thembid") == play.of("west", "bid") + play.of("east", "bid"));
}

TEST_CASE("spades.xml: a hand's live number tricks reckons high cards, trumps and short suits", "[cards][spades]")
{
	Spades play;
	// Seven spades from the ace (three honours, four past three), three side aces, a guarded king.
	play.deal({
		joined(joined(suitOf("spades", { "A", "K", "Q", "J", "10", "9", "8" }), suitOf("hearts", { "A", "K" })), joined(suitOf("diamonds", { "A", "2" }), suitOf("clubs", { "A", "2" }))),
		joined(suitOf("spades", { "2", "3", "4", "5", "6", "7" }), suitOf("hearts", { "2", "3", "4", "5", "6", "7", "8" })),
		joined(suitOf("hearts", { "9", "10", "J", "Q" }), suitOf("diamonds", { "3", "4", "5", "6", "7", "8", "9", "10", "J" })),
		joined(suitOf("diamonds", { "Q", "K" }), suitOf("clubs", { "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K" })),
	}, 1);
	CHECK(play.pile("mine").tricks == 11.0f);
	CHECK(play.of("you", "bid") == 11.0f);
	// Six small trumps, no clubs or diamonds: three past three, and ruffs for the two missing suits.
	CHECK(play.pile("westcards").tricks == 5.0f);
}

TEST_CASE("spades.xml: spades lead only once broken; a spade takes a trick of another suit", "[cards][spades]")
{
	Spades play;
	play.deal({
		joined(suitOf("spades", { "2" }), suitOf("diamonds", { "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K" })),
		suitOf("clubs", { "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A" }),
		suitOf("hearts", { "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A" }),
		joined(suitOf("spades", { "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A" }), suitOf("diamonds", { "A" })),
	}, 2);
	// West leads its ace of clubs; north sheds a heart, east lets its partner's ace stand.
	play.untilYou();
	REQUIRE(play.pile("trick").pile.cards.size() == 3);
	CHECK(play.pile("trick").winner == 2.0f);
	CHECK(play.pile("eastcards").pile.cards.size() == 12);
	CHECK(std::find(play.pile("eastcards").pile.cards.begin(), play.pile("eastcards").pile.cards.end(), play.cardOf("A diamonds")) == play.pile("eastcards").pile.cards.end());
	// No club: your two of spades trumps the ace.
	CHECK(play.play("2 spades"));
	CHECK(play.pile("trick").winner == 1.0f);
	play.untilYou();
	CHECK(play.of("you", "won") == 1.0f);
	CHECK(play.of("north", "won") == 0.0f);
}

TEST_CASE("spades.xml: no spade leads before spades are broken, unless only spades are held", "[cards][spades]")
{
	Spades play;
	play.deal({
		joined(suitOf("spades", { "2", "3", "4", "5", "6", "7", "8" }), suitOf("diamonds", { "2", "3", "4", "5", "6", "7" })),
		joined(suitOf("spades", { "9", "10", "J", "Q", "K", "A" }), suitOf("diamonds", { "8", "9", "10", "J", "Q", "K", "A" })),
		suitOf("hearts", { "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A" }),
		suitOf("clubs", { "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A" }),
	}, 1);
	CHECK_FALSE(play.play("2 spades"));
	CHECK(play.play("2 diamonds"));
}

TEST_CASE("spades.xml: a partner's winning ace stands: north plays low under it", "[cards][spades]")
{
	Spades play;
	play.deal({
		joined(suitOf("hearts", { "A" }), suitOf("diamonds", { "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K" })),
		joined(suitOf("hearts", { "3", "4", "5", "6", "7", "8" }), suitOf("clubs", { "2", "3", "4", "5", "6", "7", "8" })),
		joined(joined(suitOf("hearts", { "K", "2" }), suitOf("clubs", { "9", "10", "J", "Q", "K", "A" })), suitOf("spades", { "2", "3", "4", "5", "6" })),
		joined(joined(suitOf("hearts", { "9", "10", "J", "Q" }), suitOf("spades", { "7", "8", "9", "10", "J", "Q", "K", "A" })), suitOf("diamonds", { "A" })),
	}, 1);
	CHECK(play.play("A hearts"));
	play.untilYou();
	CHECK(play.of("you", "won") == 1.0f);
	const auto& north = play.pile("northcards").pile.cards;
	CHECK(std::find(north.begin(), north.end(), play.cardOf("K hearts")) != north.end());
	CHECK(std::find(north.begin(), north.end(), play.cardOf("2 hearts")) == north.end());
}

TEST_CASE("spades.xml: a side that makes its bid scores ten a trick and a bag each over; short, it loses ten a trick", "[cards][spades]")
{
	Spades play;
	play.tap(KeyCode::Space);
	play.frames(2);
	play.set("table", "usbid", 4.0f);
	play.set("you", "won", 3.0f);
	play.set("north", "won", 2.0f);
	play.set("table", "thembid", 9.0f);
	play.set("west", "won", 4.0f);
	play.set("east", "won", 4.0f);
	play.set("table", "tricks", 13.0f);
	play.frames(2);
	CHECK(play.table("us") == 41.0f);
	CHECK(play.table("usbags") == 1.0f);
	CHECK(play.table("them") == -90.0f);
	CHECK(play.table("phase") == 3.0f);
	CHECK(play.get("message").lookName() == "made10");

	// Ten bags cost a hundred.
	play.set("table", "usbags", 11.0f);
	play.frames(2);
	CHECK(play.table("us") == -59.0f);
	CHECK(play.table("usbags") == 1.0f);
}

TEST_CASE("spades.xml: at 500 after a hand the higher score wins", "[cards][spades]")
{
	Spades play;
	play.set("table", "us", 320.0f);
	play.set("table", "them", 512.0f);
	play.set("table", "phase", 3.0f);
	play.frames(2);
	CHECK(play.state() == "over");
	CHECK(play.get("over.winner").lookName() == "them");
}

TEST_CASE("spades.xml: a whole hand, you playing the first card that goes: 13 tricks scored", "[cards][spades]")
{
	Spades play;
	play.tap(KeyCode::Space);
	play.frames(2);
	for (int trick = 0; trick < 13 && play.table("phase") != 3.0f; ++trick)
	{
		play.untilYou();
		if (play.table("phase") == 3.0f) { break; }
		bool played = false;
		const std::vector<int> mine = play.pile("mine").pile.cards;
		for (const int number : mine)
		{
			const Object& card = *play.game.cardAt(number);
			if (play.play(nameOf(card)))
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
	CHECK(play.of("you", "won") + play.of("north", "won") + play.of("west", "won") + play.of("east", "won") == 13.0f);
	CHECK(play.table("us") != 0.0f);
	CHECK(play.table("them") != 0.0f);
	CHECK(play.pile("stock").pile.cards.size() == 52);
}

namespace
{
	// Gin rummy: the first deal comes from a stock stacked as given, top
	// first: your ten, north's ten, the card turned up, then the stock.
	struct Gin : Played
	{
		explicit Gin(const std::vector<std::string>& top) : Played("descriptions/gin.xml")
		{
			tap(KeyCode::Space);
			frames(1);
			// Dealt a card a round to you and north, then one turned up.
			const auto part = [&](std::size_t from, std::size_t count)
			{
				std::vector<std::string> cards;
				for (std::size_t i = from; i < std::min(from + count, top.size()); ++i) { cards.push_back(top[i]); }
				return cards;
			};
			game.stackedDeal = roundRobin(*this, { part(0, 10), part(10, 10) });
			const std::vector<int> turned = roundRobin(*this, { part(20, 1) });
			game.stackedDeal.insert(game.stackedDeal.end(), turned.begin(), turned.end());
			for (int i = 0; i < 100 && table("phase") == 5.0f; ++i) { frames(1); }
			frames(2);
		}

		Object& pile(const std::string& name) { return get(name); }
		float of(const std::string& player, const std::string& name) { return get(player).variable.at(name); }
		float table(const std::string& name) { return get("table").variable.at(name); }
		void set(const std::string& object, const std::string& name, float value) { game.setVariable(get(object).name, name, value); }

		int cardOf(const std::string& name) { return cardNamed(game, name); }

		bool holds(const std::string& pileName, const std::string& card)
		{
			const auto& cards = pile(pileName).pile.cards;
			return std::find(cards.begin(), cards.end(), cardOf(card)) != cards.end();
		}

		// The hand at the top of the discard pile, and Space.
		void drawDiscard()
		{
			Object& hand = get("you");
			hand.hand.pile = "playing.discard";
			hand.hand.at = static_cast<int>(pile("discard").pile.cards.size()) - 1;
			tap(KeyCode::Space);
			frames(2);
		}

		// The hand at that card of yours, and the key.
		void discard(const std::string& name, KeyCode key)
		{
			const auto& mine = pile("mine").pile.cards;
			Object& hand = get("you");
			hand.hand.pile = "playing.mine";
			hand.hand.at = static_cast<int>(std::find(mine.begin(), mine.end(), cardOf(name)) - mine.begin());
			tap(key);
			frames(2);
		}
	};

	std::vector<std::string> cardsOf(std::initializer_list<std::vector<std::string>> parts)
	{
		std::vector<std::string> all;
		for (const auto& part : parts) { all.insert(all.end(), part.begin(), part.end()); }
		return all;
	}

	const std::vector<std::string> ginJunk{ "A spades", "3 spades", "5 spades", "9 diamonds", "J diamonds", "K clubs", "2 diamonds", "4 clubs", "6 clubs", "8 hearts" };
}

TEST_CASE("gin.xml: cards laid out in the melds that leave the least deadwood, the court cards ten", "[cards][gin]")
{
	// The seven of hearts in a run or in the set of sevens: the set leaves less.
	Gin play(cardsOf({ suitOf("hearts", { "5", "6", "7" }), { "7 spades", "7 diamonds" }, suitOf("spades", { "J", "Q", "K" }), { "A diamonds", "2 spades" }, ginJunk, { "10 hearts" } }));
	REQUIRE(play.table("phase") == 0.0f);
	CHECK(play.pile("mine").deadwood == 14.0f);
	CHECK(play.pile("northcards").deadwood == 58.0f);
	// Melds first, then the deadwood.
	const auto& mine = play.pile("mine").pile.cards;
	REQUIRE(mine.size() == 10);
	std::vector<std::string> last;
	for (std::size_t i = 6; i < 10; ++i) { last.push_back(nameOf(*play.game.cardAt(mine[i]))); }
	CHECK(std::find(last.begin(), last.end(), "5 hearts") != last.end());
	CHECK(std::find(last.begin(), last.end(), "A diamonds") != last.end());
	CHECK(std::find(last.begin(), last.end(), "7 hearts") == last.end());
}

TEST_CASE("gin.xml: a draw from the discard pile, then a discard; the deadwood counts the best ten", "[cards][gin]")
{
	Gin play(cardsOf({ suitOf("hearts", { "3", "4", "5" }), { "7 spades", "7 hearts", "7 diamonds" }, suitOf("clubs", { "9", "10", "J" }), { "K diamonds" }, ginJunk, { "2 spades" } }));
	CHECK(play.pile("mine").deadwood == 10.0f);
	play.drawDiscard();
	CHECK(play.pile("mine").pile.cards.size() == 11);
	CHECK(play.table("phase") == 1.0f);
	CHECK(play.pile("mine").deadwood == 2.0f);
	CHECK(play.get("you").lookName() == "discard");
	play.discard("K diamonds", KeyCode::Space);
	CHECK(play.holds("discard", "K diamonds"));
	CHECK(play.of("north", "turn") == 1.0f);
}

TEST_CASE("gin.xml: before the draw, Space on a card of your own does nothing", "[cards][gin]")
{
	Gin play(cardsOf({ suitOf("hearts", { "3", "4", "5" }), { "7 spades", "7 hearts", "7 diamonds" }, suitOf("clubs", { "9", "10", "J" }), { "K diamonds" }, ginJunk, { "2 spades" } }));
	play.discard("K diamonds", KeyCode::Space);
	CHECK(play.pile("mine").pile.cards.size() == 10);
	CHECK(play.holds("mine", "K diamonds"));
	CHECK(play.get("you").hand.held.empty());
	CHECK(play.table("phase") == 0.0f);
	CHECK(play.get("you").lookName() == "draw");
	play.drawDiscard();
	CHECK(play.pile("mine").pile.cards.size() == 11);
	CHECK(play.get("you").lookName() == "discard");
}

TEST_CASE("gin.xml: a knock with ten or less scores the difference, north's cards shown", "[cards][gin]")
{
	Gin play(cardsOf({ suitOf("hearts", { "3", "4", "5" }), { "7 spades", "7 hearts", "7 diamonds" }, suitOf("clubs", { "9", "10", "J" }), { "K diamonds" }, ginJunk, { "2 spades" } }));
	play.drawDiscard();
	play.discard("K diamonds", KeyCode::K);
	play.frames(2);
	CHECK(play.table("phase") == 4.0f);
	CHECK(play.of("you", "score") == 56.0f);
	CHECK(play.get("message").lookName() == "youknock");
	for (const int number : play.pile("northcards").pile.cards) { CHECK(play.game.cardAt(number)->faceUp); }
}

TEST_CASE("gin.xml: gin scores 25 more", "[cards][gin]")
{
	Gin play(cardsOf({ suitOf("hearts", { "3", "4", "5" }), { "7 spades", "7 hearts", "7 diamonds" }, suitOf("clubs", { "9", "10", "J", "Q" }), ginJunk, { "K diamonds" } }));
	play.drawDiscard();
	play.discard("K diamonds", KeyCode::K);
	play.frames(2);
	CHECK(play.of("you", "score") == 83.0f);
	CHECK(play.get("message").lookName() == "yougin");
}

TEST_CASE("gin.xml: a knock against no more deadwood is undercut: 25 and the difference to the other", "[cards][gin]")
{
	Gin play(cardsOf({ suitOf("hearts", { "3", "4", "5" }), { "7 spades", "7 hearts", "7 diamonds" }, suitOf("clubs", { "9", "10", "J" }), { "9 diamonds" },
		suitOf("spades", { "A", "2", "3" }), suitOf("diamonds", { "4", "5", "6" }), { "8 clubs", "8 hearts", "8 spades", "2 clubs" }, { "K diamonds" } }));
	CHECK(play.pile("northcards").deadwood == 2.0f);
	play.drawDiscard();
	play.discard("K diamonds", KeyCode::K);
	play.frames(2);
	CHECK(play.of("north", "score") == 32.0f);
	CHECK(play.of("you", "score") == 0.0f);
	CHECK(play.get("message").lookName() == "youcut");
}

TEST_CASE("gin.xml: a knock with more than ten left is only a discard", "[cards][gin]")
{
	Gin play(cardsOf({ suitOf("hearts", { "3", "4", "5" }), { "7 spades", "7 hearts", "7 diamonds" }, suitOf("clubs", { "9", "10", "J" }), { "K diamonds" }, ginJunk, { "Q spades" } }));
	play.drawDiscard();
	play.discard("9 clubs", KeyCode::K);
	play.frames(2);
	CHECK(play.table("knocking") == 0.0f);
	CHECK(play.of("north", "turn") == 1.0f);
	CHECK(play.table("phase") != 4.0f);
}

TEST_CASE("gin.xml: north draws a discard that melds, and lets go of what melds least", "[cards][gin]")
{
	Gin play(cardsOf({ suitOf("hearts", { "3", "4", "5" }), { "7 spades", "7 hearts", "7 diamonds" }, suitOf("clubs", { "9", "10", "J" }), { "6 diamonds" },
		{ "4 diamonds", "5 diamonds", "A spades", "3 spades", "9 spades", "J spades", "K clubs", "2 clubs", "4 clubs", "8 hearts" }, { "Q hearts" } }));
	play.drawDiscard();
	play.discard("6 diamonds", KeyCode::Space);
	for (int i = 0; i < 600 && play.of("you", "turn") != 1.0f; ++i) { play.frames(1); }
	REQUIRE(play.of("you", "turn") == 1.0f);
	CHECK(play.holds("northcards", "6 diamonds"));
	CHECK(play.pile("northcards").pile.cards.size() == 10);
	CHECK(play.pile("northcards").deadwood == 37.0f);
}

TEST_CASE("gin.xml: north knocks as soon as its discard leaves ten or less", "[cards][gin]")
{
	// Yours: hearts, clubs and diamonds that meld nowhere. North's: three melds and the king of spades.
	Gin play(cardsOf({ { "2 hearts", "4 hearts", "6 hearts", "8 hearts", "10 hearts", "Q hearts", "2 clubs", "4 clubs", "6 clubs", "8 clubs" },
		suitOf("spades", { "A", "2", "3" }), suitOf("diamonds", { "4", "5", "6" }), { "9 clubs", "9 hearts", "9 spades", "K spades" }, { "Q diamonds" } }));
	REQUIRE(play.pile("northcards").deadwood == 10.0f);
	// North's draw from the stock melds nowhere, so it cannot go gin.
	auto& stock = play.pile("stock").pile.cards;
	const int king = play.cardOf("K hearts");
	stock.erase(std::find(stock.begin(), stock.end(), king));
	stock.push_back(king);
	play.drawDiscard();
	play.discard("Q diamonds", KeyCode::Space);
	for (int i = 0; i < 600 && play.table("phase") != 4.0f; ++i) { play.frames(1); }
	REQUIRE(play.table("phase") == 4.0f);
	CHECK(play.get("message").lookName() == "northknock");
	CHECK(play.of("north", "score") == play.pile("mine").deadwood - play.pile("northcards").deadwood);
	CHECK(play.of("you", "turn") == 0.0f);
	CHECK(play.of("north", "turn") == 0.0f);
	for (const int number : play.pile("northcards").pile.cards) { CHECK(play.game.cardAt(number)->faceUp); }
}

TEST_CASE("gin.xml: at 100 after a hand the higher score wins", "[cards][gin]")
{
	Gin play(ginJunk);
	play.set("you", "score", 105.0f);
	play.set("north", "score", 40.0f);
	play.set("table", "phase", 4.0f);
	play.frames(2);
	CHECK(play.state() == "over");
	CHECK(play.get("over.winner").lookName() == "you");
}

namespace
{
	struct Pyramid : Played
	{
		Pyramid() : Played("descriptions/pyramid.xml")
		{
			tap(KeyCode::Space);
			frames(2);
		}

		Object& pile(const std::string& name) { return get(name); }

		int cardOf(const std::string& name) { return cardNamed(game, name); }

		// That card swapped with the one on that pile.
		void put(const std::string& name, const std::string& onto)
		{
			const int number = cardOf(name);
			auto& target = pile(onto).pile.cards;
			for (Object& object : game.getCurrentObjects())
			{
				if (!object.pile.is) { continue; }
				const auto at = std::find(object.pile.cards.begin(), object.pile.cards.end(), number);
				if (at == object.pile.cards.end()) { continue; }
				if (&object.pile.cards == &target)
				{
					std::iter_swap(at, target.end() - 1);
					break;
				}
				Object& moved = *game.cardAt(number);
				Object& other = *game.cardAt(target.back());
				const bool up = other.faceUp;
				game.turnCard(other, moved.faceUp);
				game.turnCard(moved, up);
				*at = target.back();
				target.back() = number;
			}
			game.layOutCards();
		}

		bool holds(const std::string& pileName, const std::string& name)
		{
			const auto& cards = pile(pileName).pile.cards;
			return std::find(cards.begin(), cards.end(), cardOf(name)) != cards.end();
		}

		void space(const std::string& at)
		{
			Object& hand = get("hand");
			hand.hand.pile = pile(at).name;
			hand.hand.at = static_cast<int>(pile(at).pile.cards.size()) - 1;
			hand.hand.mouse = false;
			tap(KeyCode::Space);
			frames(1);
		}
	};
}

TEST_CASE("pyramid.xml: 28 cards in seven rows, 24 in the stock", "[cards][pyramid]")
{
	Pyramid play;
	int dealt = 0;
	for (int row = 1; row <= 7; ++row)
	{
		for (int column = 1; column <= row; ++column)
		{
			const auto& cards = play.pile("row" + std::to_string(row) + "." + std::to_string(column) + ".1").pile.cards;
			CHECK(cards.size() == 1);
			dealt += static_cast<int>(cards.size());
		}
	}
	CHECK(dealt == 28);
	CHECK(play.pile("stock").pile.cards.size() == 24);
}

TEST_CASE("pyramid.xml: a covered card cannot be taken; a king goes alone", "[cards][pyramid]")
{
	Pyramid play;
	play.put("K spades", "row1.1.1");
	play.space("row1.1.1");
	CHECK(play.holds("row1.1.1", "K spades"));
	CHECK(play.get("hand").lookName() == "free");
	play.put("K hearts", "row7.4.1");
	play.space("row7.4.1");
	CHECK(play.holds("pairs", "K hearts"));
	CHECK(play.pile("row7.4.1").pile.cards.empty());
	CHECK(play.get("hand").lookName() == "free");
}

TEST_CASE("pyramid.xml: two free cards making 13 go together; two that do not go back", "[cards][pyramid]")
{
	Pyramid play;
	play.put("Q hearts", "row7.1.1");
	play.put("A spades", "row7.2.1");
	play.put("5 hearts", "row7.3.1");
	play.put("6 clubs", "row7.4.1");
	play.space("row7.1.1");
	CHECK(play.get("hand").lookName() == "held");
	play.space("row7.2.1");
	CHECK(play.holds("pairs", "Q hearts"));
	CHECK(play.holds("pairs", "A spades"));
	CHECK(play.get("hand").lookName() == "free");

	play.space("row7.3.1");
	play.space("row7.4.1");
	CHECK(play.holds("row7.3.1", "5 hearts"));
	CHECK(play.holds("row7.4.1", "6 clubs"));
	CHECK(play.get("hand").lookName() == "free");

	// Row seven's first two gone: the first of row six is free.
	play.put("J diamonds", "row6.1.1");
	play.put("2 clubs", "row7.5.1");
	play.space("row6.1.1");
	play.space("row7.5.1");
	CHECK(play.holds("pairs", "J diamonds"));
	CHECK(play.holds("pairs", "2 clubs"));
}

TEST_CASE("pyramid.xml: the stock turns onto the waste, whose top pairs with the pyramid", "[cards][pyramid]")
{
	Pyramid play;
	play.put("10 spades", "stock");
	play.put("3 hearts", "row7.7.1");
	play.space("stock");
	CHECK(play.holds("waste", "10 spades"));
	play.space("waste");
	play.space("row7.7.1");
	CHECK(play.holds("pairs", "10 spades"));
	CHECK(play.holds("pairs", "3 hearts"));
}

TEST_CASE("pyramid.xml: the pyramid cleared wins", "[cards][pyramid]")
{
	Pyramid play;
	for (int row = 1; row <= 7; ++row)
	{
		for (int column = 1; column <= row; ++column)
		{
			Object& each = play.pile("row" + std::to_string(row) + "." + std::to_string(column) + ".1");
			for (const int number : each.pile.cards) { play.pile("pairs").pile.cards.push_back(number); }
			each.pile.cards.clear();
			play.game.refreshPile(each);
		}
	}
	play.frames(2);
	CHECK(play.state() == "won");
}
