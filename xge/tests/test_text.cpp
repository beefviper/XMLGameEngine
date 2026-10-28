// test_text.cpp
// XML Game Engine
// author: beefviper
// date: Oct 16, 2026
//
// Catch2 tests for the game's <styles>, a <text style="..."> and a
// <bitmap style="...">, a <become style="..."> putting another style's
// colors on (a palette swap), and <centered> in a <position>. Each game is written beside the shipped ones
// so that its schema path resolves, and loaded with Xerces (full
// validation) and TinyXml2 (the built-in validator). Nothing needs a
// window.

#include "played.h"
#include "scratch.h"
#include "game/game.h"
#include "load/xml.h"
#include "picture/font.h"

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
		std::filesystem::path path = scratchName("descriptions/text_scratch.xml");

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

TEST_CASE("a bitmap takes the scale and colors it leaves out from its style", "[text]")
{
	auto backend = GENERATE(XmlBackend::Xerces, XmlBackend::TinyXml2);
	const std::string styles = "<styles><style name=\"wood\"><scale>2</scale><color>color.red</color>"
		"<color char=\"b\">color.blue</color><color char=\"g\">color.green</color></style></styles>";
	Game game = load(
		"<object name=\"plain\"><sprite><bitmap style=\"wood\"><row>*bg</row></bitmap></sprite><position><x>0</x><y>0</y></position></object>"
		"<object name=\"own\"><sprite><bitmap style=\"wood\"><row>*bg</row><scale>1</scale>"
		"<color>color.white</color><color char=\"b\">color.black</color></bitmap></sprite><position><x>0</x><y>0</y></position></object>",
		styles, backend);

	// The pixel at (x, 0) of an object's picture: red, green, blue.
	const auto rgb = [&](const std::string& name, int x)
	{
		const auto& rgba = game.getObject(name).bitmap->rgba;
		const std::size_t at = static_cast<std::size_t>(x) * 4;
		return std::vector<int>{ rgba[at], rgba[at + 1], rgba[at + 2] };
	};

	CHECK(game.getObject("plain").bitmap->width == 6);   // the style's scale
	CHECK(rgb("plain", 0) == std::vector<int>{ 255, 0, 0 });
	CHECK(rgb("plain", 2) == std::vector<int>{ 0, 0, 255 });
	CHECK(rgb("plain", 4) == std::vector<int>{ 0, 255, 0 });

	CHECK(game.getObject("own").bitmap->width == 3);     // its own scale wins
	CHECK(rgb("own", 0) == std::vector<int>{ 255, 255, 255 });
	CHECK(rgb("own", 1) == std::vector<int>{ 0, 0, 0 }); // its own b
	CHECK(rgb("own", 2) == std::vector<int>{ 0, 255, 0 }); // the style's g

	SECTION("a style that is not there stops the load")
	{
		REQUIRE_THROWS_WITH(load("<object name=\"thing\"><sprite><bitmap style=\"wod\"><row>*</row></bitmap></sprite>"
			"<position><x>0</x><y>0</y></position></object>", styles), ContainsSubstring("style=\"wod\" names no <style>"));
	}
}

TEST_CASE("a text's font file is its own <font>, its style's, or the standard one", "[text]")
{
	auto backend = GENERATE(XmlBackend::Xerces, XmlBackend::TinyXml2);
	const std::string styles = "<styles><style name=\"arcade\"><size>8</size><font>assets/pressstart2p.ttf</font></style>"
		"<style name=\"plain\"><size>20</size></style></styles>";
	Game game = load(
		"<object name=\"styled\"><sprite><text style=\"arcade\"><content>HI</content></text></sprite><position><x>0</x><y>0</y></position></object>"
		"<object name=\"own\"><sprite><text style=\"arcade\"><content>HI</content><font>assets/other.ttf</font></text></sprite><position><x>0</x><y>0</y></position></object>"
		"<object name=\"standard\"><sprite><text style=\"plain\"><content>HI</content></text></sprite><position><x>0</x><y>0</y></position></object>",
		styles, backend);

	CHECK(textFontFile(game.getObject("styled").spriteParams) == "assets/pressstart2p.ttf");
	CHECK(textFontFile(game.getObject("own").spriteParams) == "assets/other.ttf");
	CHECK(game.getObject("standard").spriteParams.size() == 4);
	CHECK(textFontFile(game.getObject("standard").spriteParams) == "assets/tuffy.ttf");
}

TEST_CASE("<become style> puts another style's colors on an object's bitmaps, and a reset the first", "[text]")
{
	const std::string styles = "<styles><style name=\"green\"><color>color.green</color>"
		"<color char=\"b\">color.blue</color><color char=\"w\">color.white</color></style>"
		"<style name=\"angry\"><color>color.red</color><color char=\"b\">color.yellow</color><color char=\"w\">color.black</color></style></styles>";
	Written file(
		"<object name=\"thing\">"
		"<sprite name=\"a\"><bitmap style=\"green\"><row>*bw</row></bitmap></sprite>"
		"<sprite name=\"b\"><bitmap style=\"green\"><row>*bw</row><color char=\"w\">color.grey</color></bitmap></sprite>"
		"<position><x>0</x><y>0</y></position>"
		"<timers><timer><after>0.5</after><become sprite=\"b\" style=\"angry\" /><become object=\"lamp\" style=\"angry\" /></timer></timers></object>"
		"<object name=\"lamp\"><sprite><bitmap style=\"green\"><row>*</row></bitmap></sprite><position><x>0</x><y>0</y></position></object>",
		styles);
	played::Played play(file.path.string());
	Object& thing = play.get("thing");
	Object& lamp = play.get("lamp");

	// The pixel at (x, 0) of what an object shows: red, green, blue.
	const auto rgb = [](const Object& object, int x)
	{
		const auto& rgba = object.bitmap->rgba;
		const std::size_t at = static_cast<std::size_t>(x) * 4;
		return std::vector<int>{ rgba[at], rgba[at + 1], rgba[at + 2] };
	};
	REQUIRE(thing.palettes.size() == 2);
	CHECK(rgb(thing, 0) == std::vector<int>{ 0, 255, 0 });
	CHECK(rgb(lamp, 0) == std::vector<int>{ 0, 255, 0 });

	// What the timer's two <become>s do (the objects are in no state's
	// <shows>, so their timers do not run).
	play.game.become("thing", "b", {}, "angry");
	play.game.become("lamp", {}, {}, "angry");
	CHECK(thing.lookName() == "b");
	CHECK(rgb(thing, 0) == std::vector<int>{ 255, 0, 0 });       // the new style's
	CHECK(rgb(thing, 1) == std::vector<int>{ 255, 255, 0 });
	CHECK(rgb(thing, 2) == std::vector<int>{ 128, 128, 128 });   // its own stays
	CHECK(rgb(lamp, 0) == std::vector<int>{ 255, 0, 0 });

	// Another look keeps the palette on.
	play.game.become("thing", "a");
	CHECK(rgb(thing, 2) == std::vector<int>{ 0, 0, 0 });

	play.game.resetObject("thing");
	CHECK(thing.lookName() == "a");
	CHECK(rgb(thing, 0) == std::vector<int>{ 0, 255, 0 });
	CHECK(rgb(thing, 2) == std::vector<int>{ 255, 255, 255 });

	SECTION("a style that is not there, or nothing to recolor, stops the load")
	{
		const std::string timer = "<timers><timer><after>1</after><become style=\"angri\" /></timer></timers>";
		REQUIRE_THROWS_WITH(load("<object name=\"x\"><sprite><bitmap style=\"green\"><row>*</row></bitmap></sprite>"
			"<position><x>0</x><y>0</y></position>" + timer + "</object>", styles), ContainsSubstring("<become style=\"angri\" /> names no <style>"));
		REQUIRE_THROWS_WITH(load("<object name=\"x\"><sprite><bitmap><row>*</row></bitmap></sprite>"
			"<position><x>0</x><y>0</y></position><timers><timer><after>1</after><become style=\"angry\" /></timer></timers></object>", styles),
			ContainsSubstring("no <bitmap style="));
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
