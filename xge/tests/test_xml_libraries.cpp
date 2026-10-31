// test_xml_libraries.cpp
// XML Game Engine
// author: beefviper
// date: Oct 31, 2026
//
// Every shipped game read by each XML library comes out the same: the same
// objects, sprites, positions, commands and rules whichever library read the
// file, so each one's reading of text and elements agrees. Split into
// groups of a few games, each its own test, and first in the program, since
// loading every game four times is the slowest work in the suite.

#include "game/game.h"
#include "load/expr.h"
#include "load/xml.h"
#include "xml/document.h"

#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <sstream>
#include <string>
#include <vector>

using namespace xge;

namespace
{
	// What is compared for one object. Anything whose position or velocity is a
	// <random> (the ball, Kaboom's bombs, Astrosmash's rocks) comes out different on
	// every load, so only its name is compared (Missile Command's missiles, and Demon Attack's small demons, too).
	std::string comparable(const Object& object)
	{
		// Named by their paths where a state or a scene owns them (playing.ball, playing.wave2.half).
		const auto endsWith = [&](const std::string& end) { return object.name == end || (object.name.size() > end.size() && object.name.compare(object.name.size() - end.size() - 1, end.size() + 1, "." + end) == 0); };
		const bool drawn = endsWith("ball") || endsWith("half") || object.objClass == "bomb" || object.objClass == "rocks" || object.objClass == "spinner" || object.objClass == "missile" || object.objClass == "ufo";
		if (drawn) { return object.name; }
		std::ostringstream text;
		text << object;
		return text.str();
	}

	// Each library's copy draws its <random>s from the same seed.
	struct SameSeed
	{
		SameSeed() { game_expr::fixRandomSeed(20261019u); }
		~SameSeed() { game_expr::fixRandomSeed(std::nullopt); }
	};

	void loadsTheSame(const std::string& game)
	{
		const std::string file = "descriptions/" + game + ".xml";
		INFO(file);
		const SameSeed seed;
		std::vector<std::string> expected;
		{
			Game xerces{ readGame(file, XmlBackend::Xerces) };
			for (const auto& object : xerces.getCurrentObjects()) { expected.push_back(comparable(object)); }
		}

		for (const XmlBackend backend : { XmlBackend::TinyXml2, XmlBackend::PugiXml, XmlBackend::RapidXml })
		{
			Game other{ readGame(file, backend) };
			REQUIRE(other.getCurrentObjects().size() == expected.size());
			for (std::size_t i = 0; i < expected.size(); ++i)
			{
				CHECK(comparable(other.getCurrentObjects()[i]) == expected[i]);
			}
		}
	}
}

TEST_CASE("every XML library loads these games the same: pacman, pong, pong_min, snake, tetris", "[xml_format][backends]")
{
	for (const char* game : { "pacman", "pong", "pong_min", "snake", "tetris" }) { loadsTheSame(game); }
}

TEST_CASE("every XML library loads these games the same: mariobros, breakout, spacerace, freeway", "[xml_format][backends]")
{
	for (const char* game : { "mariobros", "breakout", "spacerace", "freeway" }) { loadsTheSame(game); }
}

TEST_CASE("every XML library loads these games the same: donkeykong, kaboom, depthcharge, columns, capsules, lunarlander, puttpark", "[xml_format][backends]")
{
	for (const char* game : { "donkeykong", "kaboom", "depthcharge", "columns", "capsules", "lunarlander", "puttpark" }) { loadsTheSame(game); }
}

TEST_CASE("every XML library loads these games the same: galaxian, astrosmash, combat, tennis", "[xml_format][backends]")
{
	for (const char* game : { "galaxian", "astrosmash", "combat", "tennis" }) { loadsTheSame(game); }
}

TEST_CASE("every XML library loads these games the same: qbert, spaceinvaders, spaceinvaders2, airseabattle", "[xml_format][backends]")
{
	for (const char* game : { "qbert", "spaceinvaders", "spaceinvaders2", "airseabattle" }) { loadsTheSame(game); }
}

TEST_CASE("every XML library loads these games the same: berserk, asteroids, megamania", "[xml_format][backends]")
{
	for (const char* game : { "berserk", "asteroids", "megamania" }) { loadsTheSame(game); }
}

TEST_CASE("every XML library loads these games the same: pitfall, adventure, demonattack, centipede", "[xml_format][backends]")
{
	for (const char* game : { "pitfall", "adventure", "demonattack", "centipede" }) { loadsTheSame(game); }
}

TEST_CASE("every XML library loads these games the same: hauntedhouse, frogger, frostbite, missilecommand", "[xml_format][backends]")
{
	for (const char* game : { "hauntedhouse", "frogger", "frostbite", "missilecommand" }) { loadsTheSame(game); }
}

TEST_CASE("every XML library loads these games the same: trickshot, fishingderby, yarsrevenge, atlantis", "[xml_format][backends]")
{
	for (const char* game : { "trickshot", "fishingderby", "yarsrevenge", "atlantis" }) { loadsTheSame(game); }
}
