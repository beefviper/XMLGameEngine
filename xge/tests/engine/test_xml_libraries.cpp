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
#include "load/merge.h"
#include "load/xml.h"
#include "xml/document.h"
#include "scratch.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <memory>

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

TEST_CASE("every XML library loads these games the same: berserk, asteroids, megamania, pinball", "[xml_format][backends]")
{
	for (const char* game : { "berserk", "asteroids", "megamania", "pinball" }) { loadsTheSame(game); }
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

TEST_CASE("an element knows the line its tag starts on", "[xml_format][backends][line_numbers]")
{
	const std::string file = scratchName("descriptions/lines.xml");
	std::ofstream(file) << "<a>\n  <b />\n  <c>\n    <d>text</d>\n  </c>\n</a>\n";

	for (const XmlBackend backend : XmlDocumentFactory::availableBackends())
	{
		INFO(XmlDocumentFactory::name(backend));
		std::unique_ptr<XmlDocument> document = XmlDocumentFactory::create(backend);
		REQUIRE(document->load(file));
		const std::unique_ptr<XmlNode> a = document->getRootElement();
		const std::unique_ptr<XmlNode> b = a->getFirstChild();
		const std::unique_ptr<XmlNode> c = b->getNextSibling();
		const std::unique_ptr<XmlNode> d = c->getFirstChild();
		CHECK(a->line() == 1);
		CHECK(b->line() == 2);
		CHECK(c->line() == 3);
		CHECK(d->line() == 4);
	}
	std::filesystem::remove(file);
}

TEST_CASE("an element taken from a piece keeps the lines it was written on", "[xml_format][line_numbers]")
{
	const std::string file = scratchName("descriptions/lines_piece.xml");
	std::ofstream(file) << "<objects>\n"
		"  <object name=\"ball\">\n"
		"    <position><x>1</x><y>2</y></position>\n"
		"  </object>\n"
		"  <object name=\"second\" from=\"ball\">\n"
		"    <velocity><x>3</x><y>4</y></velocity>\n"
		"  </object>\n"
		"</objects>\n";

	for (const XmlBackend backend : XmlDocumentFactory::availableBackends())
	{
		INFO(XmlDocumentFactory::name(backend));
		std::unique_ptr<XmlDocument> document = XmlDocumentFactory::create(backend);
		REQUIRE(document->load(file));
		const std::unique_ptr<XmlNode> objects = document->getRootElement();
		const std::unique_ptr<XmlNode> ball = objects->getFirstChild();
		const std::unique_ptr<XmlNode> second = ball->getNextSibling();

		Kit kit;
		kit.add(copyElement(*ball), "<objects>");
		const ElementPtr taken = kit.take("ball", copyElement(*second), "<objects>");
		const std::unique_ptr<XmlNode> view = viewOf(taken);
		CHECK(view->getAttribute("name") == "second");
		CHECK(view->line() == 2); // the piece's
		const std::unique_ptr<XmlNode> position = view->getFirstChild();
		REQUIRE(position);
		CHECK(position->getName() == "position");
		CHECK(position->line() == 3);
		const std::unique_ptr<XmlNode> velocity = position->getNextSibling();
		REQUIRE(velocity);
		CHECK(velocity->getName() == "velocity");
		CHECK(velocity->line() == 6); // its own
	}
	std::filesystem::remove(file);
}

TEST_CASE("a reader's message ends with the line of the tag it is about", "[xml_format][backends][line_numbers]")
{
	// No schema is named at first, so the reader is the first to see the mistakes.
	const std::string file = scratchName("descriptions/lines_error.xml");
	const auto write = [&](const std::string& sprite, const std::string& onSpace, bool schema = false)
	{
		std::ofstream(file) << (schema ? "<game xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"../xgedef.xsd\">\n" : "<game>\n") <<
			"  <window name=\"lines\"><width>800</width><height>600</height><background>color.black</background><framerate>60</framerate></window><variables><variable name=\"unused\">0</variable></variables>\n"
			"  <states>\n"
			"    <state name=\"playing\">\n"
			"      <objects>\n"
			"        <object name=\"ball\">\n"
			"          <sprite>\n"
			"            " << sprite << "\n"
			"          </sprite>\n"
			"          <position><x>1</x><y>2</y></position>\n"
			"        </object>\n"
			"      </objects>\n"
			"      <inputs><input button=\"space\">\n"
			"        " << onSpace << "\n"
			"      </input></inputs>\n"
			"    </state>\n"
			"  </states>\n"
			"</game>\n";
	};
	const auto errorOf = [&](XmlBackend backend)
	{
		try
		{
			readGame(file, backend);
		}
		catch (const std::exception& error)
		{
			return std::string(error.what());
		}
		return std::string();
	};
	const auto endsWith = [](const std::string& text, const std::string& end)
	{
		return text.size() >= end.size() && text.compare(text.size() - end.size(), end.size(), end) == 0;
	};

	for (const XmlBackend backend : XmlDocumentFactory::availableBackends())
	{
		INFO(XmlDocumentFactory::name(backend));
		// A polygon with a tag it does not take, on line 8.
		write("<polygon><point><x>0</x><y>0</y></point><corner /></polygon>", "<pop />");
		const std::string sprite = errorOf(backend);
		INFO(sprite);
		CHECK(sprite.find("unknown <corner>") != std::string::npos);
		CHECK(endsWith(sprite, " (line 8)"));
		// A verb the engine does not have, on line 14.
		write("<circle><radius>5</radius></circle>", "<leap />");
		const std::string verb = errorOf(backend);
		INFO(verb);
		CHECK(endsWith(verb, " (line 14)"));
		// With the schema named, it is the first to see a tag it does not
		// declare, and says the line too (Xerces in its own words).
		write("<circle><radius>5</radius></circle><corner />", "<pop />", true);
		const std::string schema = errorOf(backend);
		INFO(schema);
		CHECK(schema.find("line 8") != std::string::npos);
	}
	std::filesystem::remove(file);
}
