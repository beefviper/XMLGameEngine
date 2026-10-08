// test_new_verbs.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026
//
// Catch2 tests for the verbs Frogger added, on their own: how dec, hop, wrap
// and ride parse and print, what a collision rule about another object can do
// now (reset, inc, dec, move, ride), and the new named colors. The verbs at
// work in a whole game are in test_frogger.cpp. Also here: an amount on inc
// and dec.

#include "color.h"
#include "command.h"
#include "command_executor.h"
#include "game.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <sstream>
#include <variant>

using namespace xge;

namespace
{
	float evaluatePlainNumber(const RawValue& value)
	{
		return std::stof(value.text);
	}

	RawCommand tag(const std::string& verb)
	{
		RawCommand command;
		command.verb = verb;
		return command;
	}

	RawCommand step(const std::string& verb, const std::string& direction, const std::string& amount)
	{
		RawCommand command = tag(verb);
		command.direction = direction;
		command.amount = RawValue::expression(amount);
		return command;
	}
}

TEST_CASE("<dec variable=\"owner.variable\" /> makes a CmdDecrement", "[new_verbs][command_parsing]")
{
	RawCommand dec = tag("dec");
	dec.variable = "frog.lives";

	const auto commands = makeCommands({ dec, tag("reset") }, evaluatePlainNumber);

	REQUIRE(commands.size() == 2);
	REQUIRE(std::holds_alternative<CmdDecrement>(commands[0]));
	CHECK(std::get<CmdDecrement>(commands[0]).target == "frog.lives");
	CHECK(std::holds_alternative<CmdReset>(commands[1]));
}

TEST_CASE("<hop direction=\"...\">distance</hop> makes a CmdHop in that direction", "[new_verbs][command_parsing]")
{
	const auto commands = makeCommands({
		step("hop", "up", "48"), step("hop", "down", "48"), step("hop", "left", "24"), step("hop", "right", "12") },
		evaluatePlainNumber);

	REQUIRE(commands.size() == 4);
	const Direction expected[] = { Direction::Up, Direction::Down, Direction::Left, Direction::Right };
	const float distance[] = { 48.0f, 48.0f, 24.0f, 12.0f };
	for (std::size_t i = 0; i < 4; ++i)
	{
		REQUIRE(std::holds_alternative<CmdHop>(commands[i]));
		CHECK(std::get<CmdHop>(commands[i]).direction == expected[i]);
		CHECK(std::get<CmdHop>(commands[i]).distance == distance[i]);
	}
}

TEST_CASE("<move> and <hop> need a direction they know", "[new_verbs][command_parsing]")
{
	REQUIRE_THROWS_WITH(makeCommands({ step("move", "sideways", "2") }, evaluatePlainNumber),
		"<move> has direction=\"sideways\"; expected up, down, left or right");
}

TEST_CASE("<wrap /> and <ride /> are collision verbs", "[new_verbs][command_parsing]")
{
	const auto commands = makeCommands({ tag("wrap"), tag("ride") }, evaluatePlainNumber);

	REQUIRE(commands.size() == 2);
	CHECK(std::holds_alternative<CmdWrap>(commands[0]));
	CHECK(std::holds_alternative<CmdRide>(commands[1]));
}

TEST_CASE("the new commands print the way they are written", "[new_verbs][command_parsing]")
{
	std::ostringstream out;
	out << Command{ CmdDecrement{ "frog.lives" } } << ' '
		<< Command{ CmdHop{ Direction::Left, 48.0f } } << ' '
		<< Command{ CmdWrap{} } << ' '
		<< Command{ CmdRide{} };

	CHECK(out.str() == "dec(frog.lives) hop.left(48) wrap ride");
}

TEST_CASE("the new colors are named, opaque, and different from each other", "[new_verbs][colors]")
{
	const char* names[] = {
		"color.grey", "color.darkgrey", "color.lightgrey", "color.brown", "color.orange",
		"color.purple", "color.darkblue", "color.darkgreen", "color.forestgreen",
	};

	for (const char* name : names)
	{
		INFO(name);
		CHECK(colorFromName(name).a == 255);
	}

	const Color darkgreen = colorFromName("color.darkgreen");
	const Color forestgreen = colorFromName("color.forestgreen");
	const Color green = colorFromName("color.green");
	CHECK((darkgreen.r != forestgreen.r || darkgreen.g != forestgreen.g || darkgreen.b != forestgreen.b));
	CHECK(forestgreen.g < green.g); // the frog is the brightest green there is

	// A name nobody knows is still see-through, as before.
	CHECK(colorFromName("color.chartreuse").a == 0);
}

namespace
{
	// frogger.xml's frog and one of its hedges, sized as a backend would.
	struct Pair
	{
		Game game{ "games/frogger.xml" };
		CommandExecutor executor{ game };
		Object& frog;
		Object& hedge;

		Pair() : frog(game.getObject("frog")), hedge(game.getObject("hedges.2"))
		{
			frog.size = { 36.0f, 36.0f };
			hedge.size = { 48.0f, 48.0f };
			game.setCurrentState("playing");
		}
	};
}

TEST_CASE("a rule about another object can reset, move, count and ride", "[new_verbs][collision_rules]")
{
	Pair pair;

	pair.frog.position = { 100.0f, 100.0f };
	pair.executor.executeObjectCollision(Command{ CmdMove{ Direction::Up, 10.0f } }, pair.frog, pair.hedge, Edge::Top);
	CHECK(pair.frog.position.x == 100.0f);
	CHECK(pair.frog.position.y == 90.0f);

	pair.executor.executeObjectCollision(Command{ CmdReset{} }, pair.frog, pair.hedge, Edge::Top);
	CHECK(pair.frog.position.x == pair.frog.positionOriginal.x);
	CHECK(pair.frog.position.y == pair.frog.positionOriginal.y);

	pair.executor.executeObjectCollision(Command{ CmdIncrement{ "frog.score" } }, pair.frog, pair.hedge, Edge::Top);
	pair.executor.executeObjectCollision(Command{ CmdDecrement{ "frog.lives" } }, pair.frog, pair.hedge, Edge::Top);
	CHECK(pair.frog.variable["score"] == 1.0f);
	CHECK(pair.frog.variable["lives"] == 2.0f);

	pair.hedge.velocity = { 2.5f, 0.0f };
	pair.executor.executeObjectCollision(Command{ CmdRide{} }, pair.frog, pair.hedge, Edge::Top);
	CHECK(pair.frog.riding.x == 2.5f);
	CHECK(pair.frog.velocity.x == 0.0f); // its own velocity is not touched
}

TEST_CASE("a hop and a ride are only for the frame they were asked for", "[new_verbs][ride][hop]")
{
	Pair pair;

	pair.frog.position = { 300.0f, 630.0f };
	pair.executor.executeInput(Command{ CmdTriggerAction{ "frog", "up" } }, true);
	CHECK(pair.frog.hopPending.y == -48.0f);

	// Letting go of the key asks for nothing, and takes nothing back.
	pair.executor.executeInput(Command{ CmdTriggerAction{ "frog", "up" } }, false);
	CHECK(pair.frog.hopPending.y == -48.0f);

	pair.game.updateObjects();
	CHECK(pair.frog.position.y == 630.0f - 48.0f);
	CHECK(pair.frog.hopPending.y == 0.0f);
	CHECK_FALSE(pair.frog.hopped);
}

// ------------------------------------------------------------ inc and dec with an amount

TEST_CASE("<inc> and <dec> change by 1 unless they are given an amount", "[new_verbs][command_parsing]")
{
	RawCommand bare = tag("inc");
	bare.variable = "frog.score";

	RawCommand five = tag("inc");
	five.variable = "frog.score";
	five.amount = RawValue::expression("5");

	RawCommand three = tag("dec");
	three.variable = "frog.lives";
	three.amount = RawValue::expression("3");

	const auto commands = makeCommands({ bare, five, three }, evaluatePlainNumber);

	REQUIRE(commands.size() == 3);
	CHECK(std::get<CmdIncrement>(commands[0]).amount == 1.0f);
	CHECK(std::get<CmdIncrement>(commands[1]).amount == 5.0f);
	CHECK(std::get<CmdDecrement>(commands[2]).amount == 3.0f);
}

TEST_CASE("an <inc> or <dec> amount is added to or taken off the variable", "[new_verbs][collision_rules]")
{
	Pair pair;

	pair.executor.executeObjectCollision(Command{ CmdIncrement{ "frog.score", 5.0f } }, pair.frog, pair.hedge, Edge::Top);
	pair.executor.executeObjectCollision(Command{ CmdIncrement{ "frog.score", 2.5f } }, pair.frog, pair.hedge, Edge::Top);
	CHECK(pair.frog.variable["score"] == 7.5f);

	pair.executor.executeObjectCollision(Command{ CmdDecrement{ "frog.lives", 2.0f } }, pair.frog, pair.hedge, Edge::Top);
	CHECK(pair.frog.variable["lives"] == 1.0f);
}

TEST_CASE("an amount shows in how <inc> and <dec> print, and a plain one does not", "[new_verbs][command_parsing]")
{
	std::ostringstream out;
	out << Command{ CmdIncrement{ "frog.score" } } << ' ' << Command{ CmdIncrement{ "frog.score", 10.0f } } << ' '
		<< Command{ CmdDecrement{ "frog.lives", 2.0f } };

	CHECK(out.str() == "inc(frog.score) inc(frog.score, 10) dec(frog.lives, 2)");
}
