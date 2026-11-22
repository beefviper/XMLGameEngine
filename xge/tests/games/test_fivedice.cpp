// test_fivedice.cpp
// XML Game Engine
// author: beefviper
// date: Nov 22, 2026
//
// Catch2 tests for fivedice.xml: dice as held items whose worth is rolled,
// kept dice, the lower boxes scored by patterns on the cup, the upper
// bonus, and a whole card written.

#include "played.h"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace xge;
using played::Played;

namespace
{
	struct Dice : Played
	{
		Dice() : Played("descriptions/fivedice.xml")
		{
			tap(KeyCode::Space);
			frames(2);
		}

		float of(const std::string& name, const std::string& variable) { return game.object(name).variable.valueOr(variable, -1.0f); }
		float card(const std::string& variable) { return of("card", variable); }
		float die(int number) { return of("dice." + std::to_string(number), "worth"); }
		float box(int row, const std::string& variable) { return of("boxes." + std::to_string(row), variable); }

		void roll()
		{
			tap(KeyCode::Space);
			frames(2);
		}

		// The cup holding these five, as if rolled.
		void showing(const std::vector<float>& faces)
		{
			for (int number = 1; number <= 5; ++number)
			{
				game.setVariable("dice." + std::to_string(number), "worth", faces[static_cast<std::size_t>(number - 1)]);
			}
			frames(1);
		}

		void write()
		{
			tap(KeyCode::E);
			frames(2);
		}
	};
}

TEST_CASE("fivedice: a roll gives each die a face and the cup their sum", "[fivedice]")
{
	Dice play;
	REQUIRE(play.state() == "playing");
	REQUIRE(play.card("rolls") == 0);
	play.roll();
	REQUIRE(play.card("rolls") == 1);
	float sum = 0;
	for (int number = 1; number <= 5; ++number)
	{
		REQUIRE(play.die(number) >= 1);
		REQUIRE(play.die(number) <= 6);
		sum += play.die(number);
	}
	REQUIRE(play.of("cup", "worth") == sum);
}

TEST_CASE("fivedice: a kept die turns gold and is not rolled, and a turn has three rolls", "[fivedice]")
{
	Dice play;
	play.tap(KeyCode::Num2);
	play.frames(1);
	REQUIRE(play.of("dice.2", "kept") == 0); // nothing to keep before a roll

	play.roll();
	play.tap(KeyCode::Num2);
	play.frames(1);
	REQUIRE(play.of("dice.2", "kept") == 1);
	REQUIRE(play.get("dice.2").looks.worn != 0);
	REQUIRE(play.get("dice.1").looks.worn != play.get("dice.2").looks.worn);

	const float kept = play.die(2);
	play.roll();
	play.roll();
	REQUIRE(play.card("rolls") == 3);
	REQUIRE(play.die(2) == kept);
	play.roll();
	REQUIRE(play.card("rolls") == 3);

	play.tap(KeyCode::Num2);
	play.frames(1);
	REQUIRE(play.of("dice.2", "kept") == 0);
}

TEST_CASE("fivedice: the lower boxes offer what the patterns make", "[fivedice]")
{
	Dice play;
	play.roll();

	play.showing({ 3, 5, 3, 5, 3 });
	REQUIRE(play.holderOf("three").pattern == 1);
	REQUIRE(play.holderOf("house").pattern == 1);
	REQUIRE(play.holderOf("four").pattern == 0);
	REQUIRE(play.box(3, "offer") == 9);
	REQUIRE(play.box(5, "offer") == 10);
	REQUIRE(play.box(7, "offer") == 19);
	REQUIRE(play.box(8, "offer") == 0);
	REQUIRE(play.box(9, "offer") == 25);
	REQUIRE(play.box(13, "offer") == 19);

	play.showing({ 6, 1, 3, 2, 4 });
	REQUIRE(play.box(10, "offer") == 30);
	REQUIRE(play.box(11, "offer") == 0);
	REQUIRE(play.box(9, "offer") == 0);

	play.showing({ 2, 6, 4, 5, 3 });
	REQUIRE(play.box(10, "offer") == 30);
	REQUIRE(play.box(11, "offer") == 40);

	play.showing({ 4, 4, 4, 4, 4 });
	REQUIRE(play.box(8, "offer") == 20);
	REQUIRE(play.box(12, "offer") == 50);
	REQUIRE(play.box(4, "offer") == 20);
}

TEST_CASE("fivedice: a written box ends the turn, and a full card ends the game with its bonus", "[fivedice]")
{
	Dice play;
	play.write();
	REQUIRE(play.card("turn") == 1); // nothing rolled yet

	for (int row = 1; row <= 13; ++row)
	{
		play.roll();
		play.tap(KeyCode::Num1);
		play.tap(KeyCode::Num4);
		play.frames(1);
		if (row <= 6) { play.showing({ float(row), float(row), float(row), float(row), 1 }); }
		else { play.showing({ 2, 3, 4, 5, 6 }); }
		play.write();
		REQUIRE(play.box(row, "filled") == 1);
		REQUIRE(play.card("rolls") == 0);
		for (int number = 1; number <= 5; ++number)
		{
			REQUIRE(play.of("dice." + std::to_string(number), "kept") == 0);
			REQUIRE(play.get("dice." + std::to_string(number)).looks.worn == play.get("dice.2").looks.worn);
		}
		if (row < 13)
		{
			REQUIRE(play.card("turn") == float(row + 1));
			play.tap(KeyCode::S);
			play.frames(1);
		}
	}
	// Upper: five ones, then four of each other face: 5 + 4*(2+..+6) = 85.
	REQUIRE(play.card("upper") == 85);
	REQUIRE(play.card("bonus") == 35);
	// Lower on 2 3 4 5 6: both runs 70 and chance 20, nothing else.
	REQUIRE(play.card("total") == 85 + 35 + 70 + 20);
	REQUIRE(play.state() == "over");
}
