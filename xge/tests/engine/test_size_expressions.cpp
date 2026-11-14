// test_size_expressions.cpp
// XML Game Engine
// author: beefviper
// date: Sept 29, 2026
//
// Catch2 tests for an object's size in <position> expressions - name.width and
// name.height, its own or another object's - e.g. centring a text with
// window.width.center - title.width / 2 - against real xge::Games built from
// games/descriptions/pong.xml, and a small game of its own.
//
// Every size is known when the game loads, with no window: a shape's from its
// sprite, a text's as the engine draws it (drawText), an image file's by its
// pixels. A position using a text's size is worked out again when the text's
// words change its size (Game::resolveSizeDependentPositions).

#include "scratch.h"
#include "game/game.h"
#include "picture/font.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

using namespace xge;

namespace
{
	// Removes a scratch game file when it goes out of scope, pass or fail.
	struct ScratchFile
	{
		std::filesystem::path path;

		// Moved, never copied: a copy left behind by returning one from a function
		// (which Debug builds do not elide) would delete the file it was written to.
		explicit ScratchFile(std::filesystem::path where) : path(std::move(where)) {}
		ScratchFile(const ScratchFile&) = delete;
		ScratchFile& operator=(const ScratchFile&) = delete;
		ScratchFile(ScratchFile&& other) noexcept : path(std::move(other.path)) { other.path.clear(); }
		~ScratchFile()
		{
			if (path.empty()) { return; }
			std::error_code ignored;
			std::filesystem::remove(path, ignored);
		}
	};
}

TEST_CASE("a text's size and the position made from it are known when the game loads", "[size_expressions]")
{
	Game game{ "descriptions/pong.xml" };
	const Object& title = game.getObject("title"); // x = the middle - title.width / 2

	CHECK(title.positionUsesSize);
	REQUIRE(title.size.x > 0.0f);
	const Bitmap drawn = drawText("PONG", title.picture.textSize, Color{}, textFontFile(title.picture.font));
	CHECK(title.size.x == static_cast<float>(drawn.width));
	CHECK(title.size.y == static_cast<float>(drawn.height));
	CHECK(title.position.x == 640.0f - title.size.x / 2.0f); // 1280 wide window
	CHECK(title.position.y == 120.0f);                       // y is "margin * 4"
	CHECK(title.start.position == title.position);
}

TEST_CASE("a position is worked out again when the text's words change its size", "[size_expressions]")
{
	Game game{ "descriptions/pong.xml" };
	game.setCurrentState(0);
	Object& score1 = game.getObject("score1"); // x = the middle - 80 - score1.width
	const float one = score1.size.x;            // "0"
	CHECK(score1.position.x == 640.0f - 80.0f - one);

	// "10": the game measures it again and moves it, its right edge kept.
	REQUIRE(game.setVariable("paddle1", "score", 10.0f));
	CHECK(score1.size.x > one);
	game.updateObjects();
	CHECK(score1.position.x == 640.0f - 80.0f - score1.size.x);

	// Unchanged size: an object moved since is left where it is.
	score1.position.y = 5.0f;
	game.resolveSizeDependentPositions();
	CHECK(score1.position.y == 5.0f);
}

namespace
{
	// A small game of a logo (a circle), a title (a text) and a paddle (an
	// image), with some text replaced (the first
	// occurrence of each), written beside the real games so its relative
	// schema path still resolves.
	const char* const kSizes = R"(<game xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="../xgedef.xsd">
  <window name="sizes"><width>1280</width><height>720</height><background>color.black</background><framerate>60</framerate></window>
  <variables><variable name="logo.radius">100</variable></variables>
  <states>
    <objects>
      <object name="logo">
        <sprite><circle><radius>logo.radius</radius></circle></sprite>
        <position><x><subtract><value>window.width.center</value><value>logo.radius</value></subtract></x><y><subtract><value>window.height.center</value><value>logo.radius</value></subtract></y></position>
      </object>
    </objects>
    <state name="title">
      <objects>
        <object name="title"><sprite><text><content>PONG</content><size>128</size></text></sprite><position><x>0</x><y>0</y></position></object>
      </objects>
      <shows><show object="logo" /></shows>
      <inputs><input button="space"><push state="playing" /></input></inputs>
    </state>
    <state name="playing">
      <objects>
        <object name="paddle1">
          <sprite><image><path>assets/paddle.jpg</path></image></sprite>
          <position><x>0</x><y>0</y></position>
          <variables><variable name="score">0</variable></variables>
        </object>
      </objects>
      <inputs><input button="space"><pop /></input></inputs>
    </state>
  </states>
</game>
)";

	ScratchFile writeGame(const std::vector<std::pair<std::string, std::string>>& replacements)
	{
		std::string xml = kSizes;
		for (const auto& [from, to] : replacements)
		{
			const auto at = xml.find(from);
			REQUIRE(at != std::string::npos);
			xml.replace(at, from.size(), to);
		}

		ScratchFile scratch{ scratchName("descriptions/size_expressions_scratch.xml") };
		std::ofstream out(scratch.path);
		out << xml;
		return scratch;
	}

	// The logo is a 100-radius circle (200 x 200); the title is a text. Its
	// position is the only place these two expressions are written in kSizes.
	const std::string kLogoX = "<subtract><value>window.width.center</value><value>logo.radius</value></subtract>";
	const std::string kLogoY = "<subtract><value>window.height.center</value><value>logo.radius</value></subtract>";

	std::vector<std::pair<std::string, std::string>> logoAt(const std::string& x, const std::string& y)
	{
		return { { kLogoX, x }, { kLogoY, y } };
	}
}

TEST_CASE("a shape's position that uses its own size is exact as soon as the game loads", "[size_expressions]")
{
	const ScratchFile scratch = writeGame(logoAt("logo.width", "<multiply><value>logo.height</value><value>2</value></multiply>"));

	Game game{ scratch.path.string() };
	const Object& logo = game.getObject("logo");

	CHECK_FALSE(logo.positionUsesSize);
	CHECK(logo.position.x == 200.0f);
	CHECK(logo.position.y == 400.0f);
}

TEST_CASE("an object can be placed by another object's size", "[size_expressions]")
{
	const ScratchFile scratch = writeGame(logoAt("<add><value>title.width</value><value>10</value></add>", "title.height"));

	Game game{ scratch.path.string() };
	Object& logo = game.getObject("logo");
	Object& title = game.getObject("title");

	// The title is a text: placed by its size at once, and again if it changes.
	CHECK(logo.positionUsesSize);
	CHECK(logo.sizeDependencies == std::vector<std::string>{ "title.title" }); // the title is the main menu's
	REQUIRE(title.size.x > 0.0f);
	CHECK(logo.position.x == title.size.x + 10.0f);
	CHECK(logo.position.y == title.size.y);

	title.size = { 700.0f, 150.0f };
	game.resolveSizeDependentPositions();
	CHECK(logo.position.x == 710.0f);
	CHECK(logo.position.y == 150.0f);
}

TEST_CASE("an object's own <variable> named width is not replaced by its size", "[size_expressions]")
{
	// paddle1, an image 30 by 150, is given a width variable of its own.
	// paddle1.width must read that variable, while paddle1.height, which it
	// does not declare, is the image's height, known when the game loads.
	auto replacements = logoAt("paddle1.width", "paddle1.height");
	replacements.push_back({ "<variable name=\"score\">0</variable>", "<variable name=\"score\">0</variable><variable name=\"width\">7</variable>" });
	const ScratchFile scratch = writeGame(replacements);

	Game game{ scratch.path.string() };
	const Object& logo = game.getObject("logo");
	const Object& paddle1 = game.getObject("paddle1");

	CHECK(paddle1.size.x == 30.0f);
	CHECK(paddle1.size.y == 150.0f);
	CHECK_FALSE(logo.positionUsesSize); // an image's size never changes
	CHECK(logo.position.x == 7.0f);
	CHECK(logo.position.y == 150.0f);
}
