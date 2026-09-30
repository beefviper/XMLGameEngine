// test_size_expressions.cpp
// XML Game Engine
// author: beefviper
// date: Sept 29, 2026
//
// Catch2 tests for an object's size in <position> expressions - name.width and
// name.height, its own or another object's - e.g. centring a text with
// window.width.center - title.width / 2 - against real xge::Games built from
// games/pong.xml and games/spaceinvaders.xml.
//
// A shape's size follows from its sprite, so a position using only shapes is
// exact when the game loads. A text's size only exists once a Window backend
// has measured it: until then a position that uses it is unknown (printGame
// says so), and Game::resolveSizeDependentPositions() works it out once the
// size is real and again whenever it changes. The tests stand in for a
// backend by setting Object::size and sizeKnown themselves, or by using a
// fake Window whose init() measures every text.

#include "engine.h"
#include "game.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace xge;

namespace
{
	std::string printed(const Object& object)
	{
		std::ostringstream text;
		text << object;
		return text.str();
	}

	void measure(Object& object, float width, float height)
	{
		object.size = { width, height };
		object.sizeKnown = true;
	}

	// A Window that "measures" every text as 100 x 40 and draws nothing.
	class MeasuringWindow : public Window
	{
	public:
		bool isOpen() const override { return true; }
		void close() override {}
		void init(std::vector<Object>& objects) override
		{
			for (auto& object : objects)
			{
				if (object.shapeKind == ShapeKind::Text) { measure(object, 100.0f, 40.0f); }
			}
		}
		std::vector<std::pair<KeyCode, bool>> pollEvents() override { return {}; }
		void clear(const std::string&) override {}
		void draw(Object&) override {}
		void display() override {}
	};

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

TEST_CASE("a text's position that uses its size is unknown until the text is measured", "[size_expressions]")
{
	Game game{ "games/pong.xml" };
	Object& title = game.getObject("title"); // x = window.width.center - title.width / 2

	CHECK(title.positionUsesSize);
	CHECK_FALSE(title.positionResolved);
	CHECK(printed(title).find("pos= ( Unknown") != std::string::npos);

	game.resolveSizeDependentPositions(); // no size yet: nothing to work from
	CHECK_FALSE(title.positionResolved);

	measure(title, 600.0f, 200.0f);
	game.resolveSizeDependentPositions();

	CHECK(title.positionResolved);
	CHECK(title.position.x == 640.0f - 300.0f); // 1280 wide window
	CHECK(title.position.y == 30.0f);           // y is just "margin"
	CHECK(title.positionOriginal == title.position);
	CHECK(printed(title).find("pos.x=340") != std::string::npos);
}

TEST_CASE("a position is worked out again when the text's size changes", "[size_expressions]")
{
	Game game{ "games/pong.xml" };
	Object& score1 = game.getObject("score1"); // x = window.width.center - 50 - score1.width

	measure(score1, 60.0f, 90.0f); // "0"
	game.resolveSizeDependentPositions();
	CHECK(score1.position.x == 640.0f - 50.0f - 60.0f);

	measure(score1, 120.0f, 90.0f); // "10": right edge stays put, left edge moves
	game.resolveSizeDependentPositions();
	CHECK(score1.position.x == 640.0f - 50.0f - 120.0f);

	// Unchanged size: an object moved since is left where it is.
	score1.position.y = 5.0f;
	game.resolveSizeDependentPositions();
	CHECK(score1.position.y == 5.0f);
}

TEST_CASE("Engine finishes size-dependent positions once its window has measured", "[size_expressions]")
{
	Game game{ "games/spaceinvaders.xml" };
	Object& title = game.getObject("title");
	REQUIRE_FALSE(title.positionResolved);

	Engine engine(game, std::make_unique<MeasuringWindow>());

	CHECK(title.positionResolved);
	CHECK(title.position.x == 512.0f - 50.0f);          // 1024 wide, text 100 wide
	CHECK(title.position.y == 384.0f - 20.0f - 100.0f); // 768 high, text 40 high
}

namespace
{
	// pong.xml with some text replaced (the first occurrence of each), written
	// beside the real games so its relative schema path still resolves.
	ScratchFile writeGame(const std::vector<std::pair<std::string, std::string>>& replacements)
	{
		std::ifstream in("games/pong.xml");
		REQUIRE(in.good());
		std::string xml((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

		// Whatever line endings the file has, the replacements below are one line each.
		xml.erase(std::remove(xml.begin(), xml.end(), '\r'), xml.end());

		for (const auto& [from, to] : replacements)
		{
			const auto at = xml.find(from);
			REQUIRE(at != std::string::npos);
			xml.replace(at, from.size(), to);
		}

		ScratchFile scratch{ "games/size_expressions_scratch.xml" };
		std::ofstream out(scratch.path);
		out << xml;
		return scratch;
	}

	// The logo is a 100-radius circle (200 x 200); the title is a text. Its
	// position is the only place these two expressions are written in pong.xml.
	const std::string kLogoX = "window.width.center - logo.radius";
	const std::string kLogoY = "window.height.center - logo.radius";

	std::vector<std::pair<std::string, std::string>> logoAt(const std::string& x, const std::string& y)
	{
		return { { kLogoX, x }, { kLogoY, y } };
	}
}

TEST_CASE("a shape's position that uses its own size is exact as soon as the game loads", "[size_expressions]")
{
	const ScratchFile scratch = writeGame(logoAt("logo.width", "logo.height * 2"));

	Game game{ scratch.path.string() };
	const Object& logo = game.getObject("logo");

	CHECK_FALSE(logo.positionUsesSize);
	CHECK(logo.positionResolved);
	CHECK(logo.position.x == 200.0f);
	CHECK(logo.position.y == 400.0f);
}

TEST_CASE("an object can be placed by another object's size", "[size_expressions]")
{
	const ScratchFile scratch = writeGame(logoAt("title.width + 10", "title.height"));

	Game game{ scratch.path.string() };
	Object& logo = game.getObject("logo");
	Object& title = game.getObject("title");

	// The title is a text, so the logo has to wait for it to be measured.
	CHECK(logo.positionUsesSize);
	CHECK_FALSE(logo.positionResolved);
	CHECK(logo.sizeDependencies == std::vector<std::string>{ "title" });

	measure(title, 600.0f, 200.0f);
	game.resolveSizeDependentPositions();
	CHECK(logo.positionResolved);
	CHECK(logo.position.x == 610.0f);
	CHECK(logo.position.y == 200.0f);

	// ...and follows it when it changes.
	measure(title, 700.0f, 150.0f);
	game.resolveSizeDependentPositions();
	CHECK(logo.position.x == 710.0f);
	CHECK(logo.position.y == 150.0f);
}

TEST_CASE("an object's own <variable> named width is not replaced by its size", "[size_expressions]")
{
	// paddle1 (an image, so its real size needs a backend) is given a width
	// variable of its own. paddle1.width must read that variable, while
	// paddle1.height, which it does not declare, is still the measured height.
	auto replacements = logoAt("paddle1.width", "paddle1.height");
	replacements.push_back({ "<variable name=\"score\">0</variable>", "<variable name=\"score\">0</variable><variable name=\"width\">7</variable>" });
	const ScratchFile scratch = writeGame(replacements);

	Game game{ scratch.path.string() };
	Object& logo = game.getObject("logo");
	Object& paddle1 = game.getObject("paddle1");

	CHECK(logo.sizeDependencies == std::vector<std::string>{ "paddle1" }); // for the height only
	CHECK_FALSE(logo.positionResolved); // still waiting on the height

	measure(paddle1, 30.0f, 150.0f);
	game.resolveSizeDependentPositions();

	CHECK(logo.positionResolved);
	CHECK(logo.position.x == 7.0f);
	CHECK(logo.position.y == 150.0f);
}
