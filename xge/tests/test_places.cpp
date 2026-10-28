// test_places.cpp
// XML Game Engine
// author: beefviper
// date: Oct 27, 2026
//
// Catch2 tests for where each command goes: the groups the schema sorts the
// commands into and the places that take them (a rule, an action, a timer, a
// state's keys), the commands an action, a rule and a timer now all run, an
// action that runs itself again turned away, and both schema checkers on a
// command out of its place.

#include "played.h"
#include "engine/folder.h"
#include "load/xml.h"
#include "load/xsd.h"
#include "xml/document.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <fstream>
#include <iterator>
#include <string>

using namespace xge;
using played::box;
using played::Played;
using played::Scratch;
using Catch::Approx;
using Catch::Matchers::ContainsSubstring;

namespace
{
	// What the loader alone says, with no schema to turn it away first: the
	// game written without naming one.
	std::string looseError(const std::string& objects, const std::string& onSpace)
	{
		const Scratch file(objects, onSpace);
		std::string text;
		{
			std::ifstream in(file.path);
			text.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
		}
		const std::string named = " xsi:noNamespaceSchemaLocation=\"../../xge/xgedef.xsd\"";
		text.erase(text.find(named), named.size());
		{
			std::ofstream out(file.path);
			out << text;
		}
		try
		{
			Game game(file.path);
		}
		catch (const std::exception& thrown)
		{
			return thrown.what();
		}
		return {};
	}

	// Whether this project's own schema checker takes the scratch game.
	bool weakAccepts(const std::string& objects, const std::string& onSpace)
	{
		const Scratch file(objects, onSpace);
		auto document = XmlDocumentFactory::create(XmlBackend::TinyXml2);
		REQUIRE(document->load(file.path));
		XsdLiteValidator validator;
		REQUIRE(validator.loadSchema(findSchema("descriptions/pong.xml", "../../xge/xgedef.xsd").string(), XmlBackend::TinyXml2));
		return validator.validate(*document->getRootElement());
	}
}

TEST_CASE("an action stops, turns back, lets out and runs another action", "[places][action]")
{
	const std::string ship = box("ship", 400, 300, "<velocity><x>2</x><y>0</y></velocity>"
		"<actions><action name=\"back\"><reverse /></action>"
		"<action name=\"turn\"><trigger object=\"ship\" action=\"back\" /><inc variable=\"ship.turns\" /></action>"
		"<action name=\"halt\"><stop /></action></actions>"
		"<variables><variable name=\"turns\">0</variable></variables>");
	const Scratch file(ship, "<trigger object=\"ship\" action=\"turn\" />");
	Played play(file.path);

	play.tap(KeyCode::Space);
	CHECK(play.get("ship").velocity.x == Approx(-2));
	CHECK(play.get("ship").variable.at("turns") == Approx(1));

	play.engine.handleKeyPressed(KeyCode::Space);
	play.engine.handleKeyReleased(KeyCode::Space);
	CHECK(play.get("ship").velocity.x == Approx(2));
}

TEST_CASE("a collision rule can fire", "[places][fire]")
{
	const std::string shot = "<object name=\"shot\"><sprite><rectangle><width>2</width><height>2</height></rectangle></sprite>"
		"<position><x>0</x><y>0</y></position><velocity><x>0</x><y>-4</y></velocity><active>false</active></object>";
	const std::string bumper = box("bumper", 400, 100, "<velocity><x>0</x><y>-5</y></velocity>"
		"<collisions><enabled>true</enabled><collision edge=\"top\"><fire object=\"shot\" /><stop /></collision></collisions>");
	const Scratch file(bumper + shot, "<pop />");
	Played play(file.path);

	CHECK_FALSE(play.get("shot").isVisible);
	play.frames(30);
	CHECK(play.get("shot").isVisible);
}

TEST_CASE("an action that runs itself again, round a circle of <trigger>s, is turned away", "[places][errors]")
{
	const Scratch file(box("ship", 0, 0, "<actions><action name=\"a\"><trigger object=\"ship\" action=\"b\" /></action>"
		"<action name=\"b\"><trigger object=\"ship\" action=\"a\" /></action></actions>"), "<trigger object=\"ship\" action=\"a\" />");
	REQUIRE_THROWS_WITH(Game(file.path), ContainsSubstring("run actions in a circle"));
}

TEST_CASE("a command out of its place: the schema turns it away, and so does the loader without one", "[places][schema]")
{
	struct Misplaced
	{
		const char* what;
		std::string objects;
		std::string onSpace;
		const char* loaderSays;
		bool schemaSays; // a rule's kind is in its attributes, which the schema cannot tell apart
	};
	const Misplaced cases[] = {
		{ "a <wrap /> in a rule about another object", box("a", 0, 0, "<collisions><enabled>true</enabled><collision object=\"b\"><wrap /></collision></collisions>") + box("b", 50, 0), "<pop />",
			"<wrap /> does nothing in a <collision> with another object", false },
		{ "a <move> on a key", box("a", 0, 0), "<move direction=\"left\">2</move>", "<move> does nothing in an <input>", true },
		{ "a <push> in an object's timer", box("a", 0, 0, "<timers><timer><every>1</every><push state=\"playing\" /></timer></timers>"), "<pop />",
			"<push> does nothing in an object's <timer>", true },
		{ "a <hold> in an action", box("a", 0, 0, "<actions><action name=\"x\"><hold button=\"left\" /></action></actions>"), "<pop />", "<hold> does nothing in an <action>", true },
	};

	for (const Misplaced& misplaced : cases)
	{
		DYNAMIC_SECTION(misplaced.what)
		{
			CHECK(weakAccepts(misplaced.objects, misplaced.onSpace) != misplaced.schemaSays);
			const Scratch file(misplaced.objects, misplaced.onSpace);
			CHECK_THROWS(Game(file.path));
			CHECK_THAT(looseError(misplaced.objects, misplaced.onSpace), ContainsSubstring(misplaced.loaderSays));
		}
	}
}
