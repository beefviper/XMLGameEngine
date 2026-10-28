// test_text.cpp
// XML Game Engine
// author: beefviper
// date: Oct 16, 2026
//
// Catch2 tests for the game's <styles>, a <text style="...">, and
// <centered> in a <position>. Each game is written beside the shipped ones
// so that its schema path resolves, and loaded with Xerces (full
// validation) and TinyXml2 (the built-in validator). Nothing needs a
// window.

#include "game/game.h"
#include "load/xml.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <filesystem>
#include <fstream>
#include <string>

using namespace xge;
using Catch::Approx;
using Catch::Matchers::ContainsSubstring;

namespace
{
	// The game file `objects` and `styles` make, written for as long as it lives.
	struct Written
	{
		std::filesystem::path path = "descriptions/text_scratch.xml";

		Written(const std::string& objects, const std::string& styles)
		{
			std::ofstream out(path);
			out << "<game xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"../xgedef.xsd\">"
				"<window name=\"text\"><width>800</width><height>600</height><background>color.black</background>"
				"<fullscreen>false</fullscreen><framerate>60</framerate></window>"
				"<variables><variable name=\"big\">90</variable></variables>"
				<< styles << "<objects>" << objects << "</objects>"
				"<states><state name=\"playing\">"
				"<inputs><input button=\"space\"><pop /></input></inputs></state></states></game>";
		}
		Written(const Written&) = delete;
		Written& operator=(const Written&) = delete;
		~Written()
		{
			std::error_code ignored;
			std::filesystem::remove(path, ignored);
		}
	};

	Game load(const std::string& objects, const std::string& styles, XmlBackend backend = XmlBackend::Xerces)
	{
		Written file(objects, styles);
		return Game(readGame(file.path.string(), backend));
	}

	const std::string kStyles = "<styles><style name=\"title\"><size>big</size><color>color.red</color></style>"
		"<style name=\"plain\"><size>20</size></style></styles>";
}

TEST_CASE("a text takes the size and color it leaves out from its style", "[text]")
{
	auto backend = GENERATE(XmlBackend::Xerces, XmlBackend::TinyXml2);
	Game game = load(
		"<object name=\"heading\"><sprite><text style=\"title\"><content>HELLO</content></text></sprite><position><x>0</x><y>0</y></position></object>"
		"<object name=\"smaller\"><sprite><text style=\"title\"><content>HELLO</content><size>30</size></text></sprite><position><x>0</x><y>0</y></position></object>"
		"<object name=\"note\"><sprite><text style=\"plain\"><content>hi</content></text></sprite><position><x>0</x><y>0</y></position></object>",
		kStyles, backend);

	const std::vector<std::string>& heading = game.getObject("heading").spriteParams;
	REQUIRE(heading.size() == 4);
	CHECK(std::stof(heading[2]) == Approx(90));   // the style's size, a game variable
	CHECK(heading[3] == "color.red");
	CHECK(std::stof(game.getObject("smaller").spriteParams[2]) == Approx(30)); // its own size wins
	CHECK(game.getObject("smaller").spriteParams[3] == "color.red");
	CHECK(game.getObject("note").spriteParams[3] == "color.white");           // a style with no color: white
}

TEST_CASE("a style that is not there, or a text with no size at all, stops the load", "[text]")
{
	SECTION("a style no <style> has")
	{
		REQUIRE_THROWS_WITH(load("<object name=\"heading\"><sprite><text style=\"titel\"><content>HI</content></text></sprite>"
			"<position><x>0</x><y>0</y></position></object>", kStyles), ContainsSubstring("style=\"titel\" names no <style>"));
	}
	SECTION("a text with neither a size nor a style")
	{
		REQUIRE_THROWS_WITH(load("<object name=\"heading\"><sprite><text><content>HI</content></text></sprite>"
			"<position><x>0</x><y>0</y></position></object>", kStyles), ContainsSubstring("needs a <size>"));
	}
	SECTION("a style with no size, for a text with none")
	{
		REQUIRE_THROWS_WITH(load("<object name=\"heading\"><sprite><text style=\"bare\"><content>HI</content></text></sprite>"
			"<position><x>0</x><y>0</y></position></object>", "<styles><style name=\"bare\"><color>color.red</color></style></styles>"),
			ContainsSubstring("needs a <size>"));
	}
}

TEST_CASE("<centered> puts an object's middle at the window's middle, or at the place it holds", "[text]")
{
	auto backend = GENERATE(XmlBackend::Xerces, XmlBackend::TinyXml2);
	Game game = load(
		"<object name=\"box\"><sprite><rectangle><width>100</width><height>40</height></rectangle></sprite>"
		"<position><x><centered /></x><y><centered /></y></position></object>"
		"<object name=\"left\"><sprite><rectangle><width>20</width><height>40</height></rectangle></sprite>"
		"<position><x><centered>window.width.center / 2</centered></x><y>10</y></position></object>"
		"<group name=\"row\"><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite>"
		"<position><y><centered>window.height.center</centered></y></position>"
		"<member><position><x>0</x></position></member>"
		"<member><sprite><rectangle><height>30</height></rectangle></sprite><position><x>50</x></position></member></group>",
		"", backend);

	CHECK(game.getObject("box").position.x == Approx(350));
	CHECK(game.getObject("box").position.y == Approx(280));
	CHECK(game.getObject("left").position.x == Approx(190));
	CHECK(game.getObject("left").position.y == Approx(10));
	// Each member is centered on its own height.
	CHECK(game.getObject("row.1").position.y == Approx(300 - 5));
	CHECK(game.getObject("row.2").position.y == Approx(300 - 15));
}
