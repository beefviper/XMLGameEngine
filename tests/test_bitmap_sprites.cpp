// test_bitmap_sprites.cpp
// XML Game Engine
// author: beefviper
// date: Oct 2, 2026
//
// Catch2 tests for pictures written as rows of text (<bitmap>: a period is a
// clear pixel, an asterisk a solid one, each character drawn scale by scale
// real pixels) and for animations: an object with several named <sprite>s and
// an <animation> that shows them one after the other, each for some seconds.
// Then Space Invaders, which is written with both, and both schema checkers
// (Xerces's real validation and this project's own XsdLiteValidator) on it.
//
// Small games are written to a scratch file and loaded by a real xge::Game,
// and frames are played with Game::updateObjects(); none of it needs a window.

#include "bitmap.h"
#include "command_executor.h"
#include "game.h"
#include "xml_document.h"
#include "xsd_lite.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace xge;
using Catch::Matchers::ContainsSubstring;

namespace
{
	// Removes a scratch game file when it goes out of scope, pass or fail.
	struct ScratchFile
	{
		std::filesystem::path path;

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

	ScratchFile writeScratch(const std::string& name, const std::string& xml)
	{
		ScratchFile scratch{ std::filesystem::temp_directory_path() / name };
		std::ofstream out(scratch.path);
		out << xml;
		return scratch;
	}

	// An object nothing but a plain rectangle, for the second state to show.
	const std::string kOther =
		"<object name=\"other\"><sprite><rectangle><width>5</width><height>5</height></rectangle></sprite>"
		"<position><x>0</x><y>0</y></position><velocity><x>0</x><y>0</y></velocity>"
		"<collisions><enabled>false</enabled></collisions></object>";

	// A game with these objects (and `other`), whose `playing` state shows the
	// named ones and whose `away` state shows only `other`.
	std::string gameXml(const std::string& objects, const std::string& shows, int framerate = 60)
	{
		return "<game>"
			"<window name=\"test\"><width>800</width><height>600</height><background>color.black</background>"
			"<fullscreen>false</fullscreen><framerate>" + std::to_string(framerate) + "</framerate></window>"
			"<variables><variable name=\"pixel\">3</variable></variables>"
			"<objects>" + objects + kOther + "</objects>"
			"<states>"
			"<state name=\"playing\"><shows>" + shows + "</shows><inputs><input button=\"space\"><pop /></input></inputs></state>"
			"<state name=\"away\"><shows><show object=\"other\" /></shows><inputs><input button=\"space\"><pop /></input></inputs></state>"
			"</states>"
			"</game>";
	}

	struct Loaded
	{
		ScratchFile file;
		Game game;

		explicit Loaded(const std::string& xml) :
			file(writeScratch("xge_test_bitmaps.xml", xml)),
			game(file.path.string())
		{
		}
	};

	void measure(Game& game)
	{
		for (auto& object : game.getCurrentObjects())
		{
			object.size = measureShapeSize(object.spriteParams, object.shapeKind);
		}
	}

	void frames(Game& game, int count)
	{
		for (int i = 0; i < count; ++i) { game.updateObjects(); }
	}

	// What is wrapped around a sprite, or sprites, and an animation.
	std::string objectWith(const std::string& sprites, const std::string& animation = "", const std::string& extra = "")
	{
		return "<object name=\"thing\">" + sprites + animation + extra +
			"<position><x>10</x><y>20</y></position><velocity><x>0</x><y>0</y></velocity>"
			"<collisions><enabled>false</enabled></collisions></object>";
	}

	// Two 2 by 2 pictures, 4 by 4 pixels at a scale of 2: a has its solid
	// pixels at the top left and bottom right, b at the top right and bottom left.
	const std::string kA =
		"<sprite name=\"a\"><bitmap><row>*.</row><row>.*</row><scale>2</scale><color>color.red</color></bitmap></sprite>";
	const std::string kB =
		"<sprite name=\"b\"><bitmap><row>.*</row><row>*.</row><scale>2</scale><color>color.red</color></bitmap></sprite>";
	const std::string kTwoFrames =
		"<animation><interval>0.5</interval><frame sprite=\"a\" /><frame sprite=\"b\" /></animation>";

	// Which of kA and kB an object is showing.
	char showing(const Object& object)
	{
		REQUIRE(object.bitmap);
		if (object.bitmap->solidAt(0, 0) && !object.bitmap->solidAt(2, 0)) { return 'a'; }
		if (object.bitmap->solidAt(2, 0) && !object.bitmap->solidAt(0, 0)) { return 'b'; }
		return '?';
	}

	std::string readFile(const std::string& path)
	{
		std::ifstream in(path);
		REQUIRE(in.good());
		std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
		text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
		return text;
	}

	// A shipped game with the first occurrence of `from` replaced by `to`.
	std::string gameWith(const char* file, const std::string& from, const std::string& to)
	{
		std::string xml = readFile(file);
		const auto at = xml.find(from);
		REQUIRE(at != std::string::npos);
		xml.replace(at, from.size(), to);
		return xml;
	}
}

// ----------------------------------------------------------------- drawing

TEST_CASE("a bitmap is one pixel to a character, each blown up by the scale", "[bitmap]")
{
	const Bitmap bitmap = rasterizeRows({ "*.", ".*" }, 3, colorFromName("color.green"));

	CHECK(bitmap.width == 6);
	CHECK(bitmap.height == 6);

	// The top left block and the bottom right block are solid, the others clear.
	for (int y = 0; y < 6; ++y)
	{
		for (int x = 0; x < 6; ++x)
		{
			const bool solid = (x < 3 && y < 3) || (x >= 3 && y >= 3);
			CHECK(bitmap.solidAt(x, y) == solid);
		}
	}

	// Drawn in the color, and a clear pixel has no alpha.
	CHECK(bitmap.rgba[(0 * 6 + 0) * 4 + 0] == colorFromName("color.green").r);
	CHECK(bitmap.rgba[(0 * 6 + 0) * 4 + 1] == colorFromName("color.green").g);
	CHECK(bitmap.rgba[(0 * 6 + 0) * 4 + 3] == 255);
	CHECK(bitmap.rgba[(0 * 6 + 4) * 4 + 3] == 0);
}

TEST_CASE("a scale of 1 is the rows as they are, and rows can be all clear", "[bitmap]")
{
	const Bitmap bitmap = rasterizeRows({ "*.*", "..." }, 1, colorFromName("color.white"));

	CHECK(bitmap.width == 3);
	CHECK(bitmap.height == 2);
	CHECK(bitmap.solidAt(0, 0));
	CHECK_FALSE(bitmap.solidAt(1, 0));
	CHECK(bitmap.solidAt(2, 0));
	for (int x = 0; x < 3; ++x) { CHECK_FALSE(bitmap.solidAt(x, 1)); }
}

TEST_CASE("a bitmap that is not rows of equal length of periods and asterisks is refused, saying which row", "[bitmap]")
{
	const Color color = colorFromName("color.white");

	SECTION("no rows") { CHECK_THROWS_WITH(rasterizeRows({}, 1, color), ContainsSubstring("no <row>s")); }
	SECTION("an empty row") { CHECK_THROWS_WITH(rasterizeRows({ "*.", "" }, 1, color), ContainsSubstring("row 2") && ContainsSubstring("empty")); }
	SECTION("rows of different widths")
	{
		CHECK_THROWS_WITH(rasterizeRows({ "*.*", "*." }, 1, color),
			ContainsSubstring("row 2") && ContainsSubstring("2 characters") && ContainsSubstring("row 1 is 3"));
	}
	SECTION("another character")
	{
		CHECK_THROWS_WITH(rasterizeRows({ "*.", ".#" }, 1, color),
			ContainsSubstring("row 2") && ContainsSubstring("'#'") && ContainsSubstring("character 2"));
	}
	SECTION("a space, which would hide a mistake") { CHECK_THROWS_AS(rasterizeRows({ "* ." }, 1, color), std::invalid_argument); }
	SECTION("a scale under 1") { CHECK_THROWS_WITH(rasterizeRows({ "*" }, 0, color), ContainsSubstring("scale")); }
}

// ------------------------------------------------------------- in a game

TEST_CASE("a bitmap sprite is drawn when the game loads and has the size of its picture", "[bitmap][xml]")
{
	Loaded loaded{ gameXml(objectWith(
		"<sprite><bitmap><row>.*.</row><row>***</row><scale>pixel</scale><color>color.red</color></bitmap></sprite>"),
		"<show object=\"thing\" />") };
	const Object& thing = loaded.game.getObject("thing");

	REQUIRE(thing.shapeKind == ShapeKind::Line);
	REQUIRE(thing.bitmap);

	// 3 characters by 2 rows, each 3 pixels (the variable `pixel`).
	CHECK(thing.bitmap->width == 9);
	CHECK(thing.bitmap->height == 6);
	CHECK(thing.spriteParams == std::vector<std::string>{ "line", "9", "6" });

	const Vector2f size = measureShapeSize(thing.spriteParams, thing.shapeKind);
	CHECK(size.x == 9.0f);
	CHECK(size.y == 6.0f);

	CHECK(thing.bitmap->solidAt(4, 0));
	CHECK_FALSE(thing.bitmap->solidAt(0, 0));
	CHECK(thing.bitmap->solidAt(0, 5));
	CHECK(thing.bitmap->rgba[(0 * 9 + 4) * 4 + 0] == 255);
	CHECK(thing.bitmap->rgba[(0 * 9 + 4) * 4 + 1] == 0);
}

TEST_CASE("the scale and the color can be left out: one pixel to a character, white", "[bitmap][xml]")
{
	Loaded loaded{ gameXml(objectWith("<sprite><bitmap><row>**</row></bitmap></sprite>"), "<show object=\"thing\" />") };
	const Object& thing = loaded.game.getObject("thing");

	CHECK(thing.bitmap->width == 2);
	CHECK(thing.bitmap->height == 1);
	CHECK(thing.bitmap->rgba[0] == 255);
	CHECK(thing.bitmap->rgba[1] == 255);
	CHECK(thing.bitmap->rgba[2] == 255);
}

TEST_CASE("a bitmap's size can be used in an expression like any shape's", "[bitmap][xml]")
{
	const std::string flag =
		"<object name=\"flag\"><sprite><rectangle><width>5</width><height>5</height></rectangle></sprite>"
		"<position><x>window.right - thing.width</x><y>thing.height</y></position>"
		"<velocity><x>0</x><y>0</y></velocity><collisions><enabled>false</enabled></collisions></object>";
	Loaded loaded{ gameXml(objectWith("<sprite><bitmap><row>*.*.*</row><row>*.*.*</row><scale>2</scale></bitmap></sprite>") + flag,
		"<show object=\"thing\" /><show object=\"flag\" />") };

	CHECK(loaded.game.getObject("flag").position.x == 800.0f - 10.0f);
	CHECK(loaded.game.getObject("flag").position.y == 4.0f);
}

TEST_CASE("a grid of a bitmap makes one object per cell, all sharing one picture", "[bitmap][xml]")
{
	Loaded loaded{ gameXml(objectWith(
		"<sprite><grid><columns>3</columns><rows>2</rows><padding><x>4</x><y>6</y></padding>"
		"<bitmap><row>**</row><row>*.</row><scale>5</scale></bitmap></grid></sprite>"),
		"<show object=\"thing\" />") };

	std::set<const Bitmap*> pictures;
	int cells = 0;
	for (const Object& object : loaded.game.getCurrentObjects())
	{
		if (object.baseName != "thing") { continue; }

		++cells;
		REQUIRE(object.bitmap);
		pictures.insert(object.bitmap.get());
	}
	CHECK(cells == 6);
	CHECK(pictures.size() == 1);

	// 10 by 10 each, 4 across and 6 down between: the cell in column 3, row 2.
	const Object& last = loaded.game.getObject("thing.3.2");
	CHECK(last.position.x == 10.0f + 2 * (10.0f + 4.0f));
	CHECK(last.position.y == 20.0f + (10.0f + 6.0f));
}

TEST_CASE("a bitmap object can be tested by its pixels", "[bitmap][xml]")
{
	Loaded loaded{ gameXml(objectWith("<sprite><bitmap><row>*.</row><row>**</row><scale>4</scale></bitmap></sprite>"),
		"<show object=\"thing\" />") };
	const Object& thing = loaded.game.getObject("thing");

	// The clear corner is clear, and what is drawn is what is tested.
	CHECK_FALSE(thing.bitmap->solidAt(5, 1));
	CHECK(thing.bitmap->solidAt(1, 1));
	CHECK(thing.bitmap->solidAt(5, 5));
}

TEST_CASE("mistakes in a bitmap are reported where they are", "[bitmap][xml]")
{
	const std::string shows = "<show object=\"thing\" />";

	SECTION("no rows")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith("<sprite><bitmap><scale>2</scale></bitmap></sprite>"), shows)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("needs at least one <row>"));
	}

	SECTION("rows of different widths")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith("<sprite><bitmap><row>***</row><row>**</row></bitmap></sprite>"), shows)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("row 2") && ContainsSubstring("same width"));
	}

	SECTION("a character that is not a period or an asterisk")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith("<sprite><bitmap><row>*o*</row></bitmap></sprite>"), shows)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("'o'"));
	}

	SECTION("a scale under 1")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith("<sprite><bitmap><row>*</row><scale>0</scale></bitmap></sprite>"), shows)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("scale"));
	}
}

// ---------------------------------------------------------------- turning

namespace
{
	// A 3 by 3 picture with one solid pixel at the top left: it can only be
	// turned one way to put it somewhere else.
	Bitmap corner()
	{
		return rasterizeRows({ "*..", "...", "..." }, 1, Color{ 255, 0, 0, 255 });
	}

	const std::string kCorner = "<sprite><bitmap><row>*..</row><row>...</row><row>...</row></bitmap></sprite>";
}

TEST_CASE("a finished picture turned: one square size, the first as it was, then clockwise", "[bitmap][heading]")
{
	const std::vector<Bitmap> turned = { turnBitmap(corner(), 0), turnBitmap(corner(), 90), turnBitmap(corner(), 180), turnBitmap(corner(), 270) };

	// 3 by 3 needs a square of 5 to hold it at any angle (its diagonal is 4.2).
	for (int degrees = 0; degrees < 360; ++degrees)
	{
		const Bitmap picture = turnBitmap(corner(), static_cast<float>(degrees));
		CHECK(picture.width == 5);
		CHECK(picture.height == 5);
	}

	// The picture sits in the middle (one pixel in) with nothing else drawn.
	const auto solidPixels = [](const Bitmap& picture)
	{
		std::vector<std::pair<int, int>> where;
		for (int y = 0; y < picture.height; ++y)
		{
			for (int x = 0; x < picture.width; ++x)
			{
				if (picture.solidAt(x, y)) { where.emplace_back(x, y); }
			}
		}
		return where;
	};

	using Spots = std::vector<std::pair<int, int>>;
	CHECK(solidPixels(turned[0]) == Spots{ { 1, 1 } });
	CHECK(solidPixels(turned[1]) == Spots{ { 3, 1 } });
	CHECK(solidPixels(turned[2]) == Spots{ { 3, 3 } });
	CHECK(solidPixels(turned[3]) == Spots{ { 1, 3 } });
}

TEST_CASE("a turned picture keeps its colours and never blends them", "[bitmap][heading]")
{
	const Bitmap original = rasterizeRows({ "**", "*." }, 4, Color{ 10, 20, 30, 255 });
	for (int degrees = 0; degrees < 360; ++degrees)
	{
		const Bitmap picture = turnBitmap(original, static_cast<float>(degrees));
		for (std::size_t at = 0; at < picture.rgba.size(); at += 4)
		{
			const bool solid = picture.rgba[at + 3] != 0;
			CHECK(picture.rgba[at + 3] == (solid ? 255 : 0));
			if (solid)
			{
				CHECK(picture.rgba[at + 0] == 10);
				CHECK(picture.rgba[at + 1] == 20);
				CHECK(picture.rgba[at + 2] == 30);
			}
		}
	}
}

TEST_CASE("nothing to turn gives nothing", "[bitmap][heading]")
{
	CHECK(turnBitmap(Bitmap{}, 45.0f).width == 0);
}

TEST_CASE("a picture is turned a whole degree at a time, and a full circle is no turn", "[bitmap][heading]")
{
	const Bitmap art = rasterizeRows({ "*****.", "**....", "*.....", "*....." }, 5, Color{ 255, 255, 255, 255 });
	const auto same = [&](float a, float b) { return turnBitmap(art, a).rgba == turnBitmap(art, b).rgba; };

	CHECK(same(360.0f, 0.0f));
	CHECK(same(405.0f, 45.0f));
	CHECK(same(-90.0f, 270.0f));
	CHECK(same(10.4f, 10.0f));
	CHECK(same(10.6f, 11.0f));

	// A single degree is a step of its own: of 360 headings, a lopsided
	// picture comes out differently at every one of them.
	int different = 0;
	for (int degrees = 1; degrees < 360; ++degrees)
	{
		if (!same(static_cast<float>(degrees), static_cast<float>(degrees - 1))) { ++different; }
	}
	CHECK(different > 300);
}

TEST_CASE("an object keeps the one original, and draws a turned copy only when the whole degree changes", "[bitmap][heading][xml]")
{
	Loaded loaded{ gameXml(objectWith(kCorner, "", "<heading>0</heading>"), "<show object=\"thing\" />") };
	Object& thing = loaded.game.getObject("thing");

	REQUIRE(thing.turnables.size() == 1);
	CHECK(thing.turnables[0]->picture);
	CHECK(thing.turnables[0]->picture->width == 3);

	thing.heading = 30.0f;
	thing.showHeading();
	const auto drawn = thing.bitmap;
	CHECK(thing.turnedDegrees == 30);

	// Less than half a degree away: the same picture, nothing marked.
	thing.visualDirty = false;
	thing.heading = 30.4f;
	thing.showHeading();
	CHECK(thing.bitmap == drawn);
	CHECK_FALSE(thing.visualDirty);

	// A degree on: a new one.
	thing.heading = 31.0f;
	thing.showHeading();
	CHECK(thing.bitmap != drawn);
	CHECK(thing.turnedDegrees == 31);
	CHECK(thing.visualDirty);

	// Round the top of the circle and back to 0.
	thing.heading = 359.6f;
	thing.showHeading();
	CHECK(thing.turnedDegrees == 0);
}

TEST_CASE("an object with a <heading> can be a bitmap: drawn once, then turned to every heading", "[bitmap][heading][xml]")
{
	Loaded loaded{ gameXml(objectWith(kCorner, "", "<heading>0</heading>"), "<show object=\"thing\" />") };
	Game& game = loaded.game;
	game.setCurrentState("playing");
	Object& thing = game.getObject("thing");

	REQUIRE(thing.turnables.size() == 1);
	CHECK(thing.bitmap->width == 5);
	CHECK(thing.bitmap->solidAt(1, 1));

	// Its size is the square every heading shares.
	measure(game);
	CHECK(thing.size.x == 5.0f);
	CHECK(thing.size.y == 5.0f);

	thing.visualDirty = false;
	thing.heading = 90.0f;
	thing.showHeading();
	CHECK(thing.turnedDegrees == 90);
	CHECK(thing.bitmap->solidAt(3, 1));
	CHECK(thing.visualDirty);
}

TEST_CASE("a bitmap that turns can be tested by its pixels at the heading it faces", "[bitmap][heading][xml]")
{
	Loaded loaded{ gameXml(objectWith(kCorner, "", "<heading>180</heading>"), "<show object=\"thing\" />") };
	Object& thing = loaded.game.getObject("thing");

	CHECK(thing.bitmap->solidAt(3, 3));
	CHECK_FALSE(thing.bitmap->solidAt(1, 1));
}

// -------------------------------------------------------------- animation

TEST_CASE("an animation shows its sprites in turn, each for the interval, round and round", "[animation]")
{
	// Half a second at 60 frames a second is 30 frames of the game.
	Loaded loaded{ gameXml(objectWith(kA + kB, kTwoFrames), "<show object=\"thing\" />") };
	Game& game = loaded.game;
	game.setCurrentState("playing");
	Object& thing = game.getObject("thing");

	CHECK(thing.animationFrames == 30);
	REQUIRE(thing.animationBitmaps.size() == 2);
	CHECK(showing(thing) == 'a');

	frames(game, 29);
	CHECK(showing(thing) == 'a');

	frames(game, 1);
	CHECK(showing(thing) == 'b');

	frames(game, 29);
	CHECK(showing(thing) == 'b');

	frames(game, 1);
	CHECK(showing(thing) == 'a');

	frames(game, 60);
	CHECK(showing(thing) == 'a');
}

TEST_CASE("the interval is in seconds, so it follows the window's framerate", "[animation]")
{
	const std::string objects = objectWith(kA + kB, "<animation><interval>1</interval><frame sprite=\"a\" /><frame sprite=\"b\" /></animation>");

	Loaded fast{ gameXml(objects, "<show object=\"thing\" />", 120) };
	CHECK(fast.game.getObject("thing").animationFrames == 120);

	Loaded slow{ gameXml(objects, "<show object=\"thing\" />", 25) };
	CHECK(slow.game.getObject("thing").animationFrames == 25);

	// An interval so short it is under one frame still takes one.
	Loaded quick{ gameXml(objectWith(kA + kB, "<animation><interval>0.001</interval><frame sprite=\"a\" /><frame sprite=\"b\" /></animation>"),
		"<show object=\"thing\" />") };
	CHECK(quick.game.getObject("thing").animationFrames == 1);
}

TEST_CASE("the picture is marked to be drawn again when it changes, and not before", "[animation]")
{
	Loaded loaded{ gameXml(objectWith(kA + kB, kTwoFrames), "<show object=\"thing\" />") };
	Game& game = loaded.game;
	game.setCurrentState("playing");
	Object& thing = game.getObject("thing");

	thing.visualDirty = false;
	frames(game, 29);
	CHECK_FALSE(thing.visualDirty);

	frames(game, 1);
	CHECK(thing.visualDirty);
}

TEST_CASE("a frame can come up more than once, so a sequence need not be only two pictures", "[animation]")
{
	// a, b, a, then round again: three frames of 0.5 s each.
	Loaded loaded{ gameXml(objectWith(kA + kB,
		"<animation><interval>0.5</interval><frame sprite=\"a\" /><frame sprite=\"b\" /><frame sprite=\"a\" /></animation>"),
		"<show object=\"thing\" />") };
	Game& game = loaded.game;
	game.setCurrentState("playing");
	Object& thing = game.getObject("thing");

	const char expected[] = { 'a', 'b', 'a', 'a', 'b', 'a' };
	for (const char picture : expected)
	{
		CHECK(showing(thing) == picture);
		frames(game, 30);
	}
}

TEST_CASE("only what is shown and in play animates, and a reset starts it over", "[animation]")
{
	Loaded loaded{ gameXml(objectWith(kA + kB, kTwoFrames), "<show object=\"thing\" />") };
	Game& game = loaded.game;
	game.setCurrentState("playing");
	Object& thing = game.getObject("thing");

	SECTION("a state that does not show it holds it where it was")
	{
		frames(game, 20);
		game.setCurrentState("away");
		frames(game, 100);
		game.setCurrentState("playing");

		// Twenty frames in, and still twenty frames in: ten more are needed.
		frames(game, 9);
		CHECK(showing(thing) == 'a');
		frames(game, 1);
		CHECK(showing(thing) == 'b');
	}

	SECTION("an object that is out of play holds it where it was")
	{
		thing.isVisible = false;
		frames(game, 100);
		thing.isVisible = true;
		CHECK(showing(thing) == 'a');
	}

	SECTION("a reset puts it back on the first picture, from the start of its time")
	{
		frames(game, 40);
		REQUIRE(showing(thing) == 'b');

		game.resetAll();
		game.setCurrentState("playing");
		CHECK(showing(thing) == 'a');
		CHECK(thing.animationTick == 0);

		frames(game, 29);
		CHECK(showing(thing) == 'a');
		frames(game, 1);
		CHECK(showing(thing) == 'b');
	}
}

TEST_CASE("every cell of a grid animates together", "[animation]")
{
	const std::string grid = "<grid><columns>3</columns><rows>2</rows><padding><x>2</x><y>2</y></padding>";
	const std::string a = "<sprite name=\"a\">" + grid + "<bitmap><row>*.</row><row>.*</row><scale>2</scale></bitmap></grid></sprite>";
	const std::string b = "<sprite name=\"b\">" + grid + "<bitmap><row>.*</row><row>*.</row><scale>2</scale></bitmap></grid></sprite>";

	Loaded loaded{ gameXml(objectWith(a + b, kTwoFrames), "<show object=\"thing\" />") };
	Game& game = loaded.game;
	game.setCurrentState("playing");

	frames(game, 30);

	int cells = 0;
	for (const Object& cell : game.getCurrentObjects())
	{
		if (cell.baseName != "thing") { continue; }
		++cells;
		CHECK(showing(cell) == 'b');
	}
	CHECK(cells == 6);
}

TEST_CASE("the members of a group can each have sprites and an animation of their own, or share the group's", "[animation][group]")
{
	const std::string group =
		"<group name=\"pair\" class=\"pair\">"
		"<sprite name=\"a\"><bitmap><row>*.</row><row>.*</row><scale>2</scale></bitmap></sprite>"
		"<sprite name=\"b\"><bitmap><row>.*</row><row>*.</row><scale>2</scale></bitmap></sprite>"
		"<animation><interval>1</interval><frame sprite=\"a\" /><frame sprite=\"b\" /></animation>"
		"<position><y>50</y></position><velocity><x>0</x><y>0</y></velocity>"
		"<collisions><enabled>false</enabled></collisions>"
		// shares everything the group gives
		"<member name=\"same\"><position><x>10</x></position></member>"
		// its own interval, the group's sprites
		"<member name=\"quick\"><position><x>40</x></position>"
		"<animation><interval>0.5</interval><frame sprite=\"a\" /><frame sprite=\"b\" /></animation></member>"
		// sprites of its own, with the group's animation picking them out by name
		"<member name=\"mine\"><position><x>70</x></position>"
		"<sprite name=\"a\"><bitmap><row>**</row><row>**</row><scale>2</scale></bitmap></sprite>"
		"<sprite name=\"b\"><bitmap><row>..</row><row>..</row><scale>2</scale></bitmap></sprite></member>"
		"</group>";

	Loaded loaded{ gameXml(group, "<show object=\"pair\" />") };
	Game& game = loaded.game;
	game.setCurrentState("playing");

	CHECK(game.getObject("same").animationFrames == 60);
	CHECK(game.getObject("quick").animationFrames == 30);
	CHECK(game.getObject("mine").animationFrames == 60);

	// The third member shows its own sprites, which are not kA and kB: its
	// first is solid all over, its second clear all over.
	const Object& mine = game.getObject("mine");
	CHECK(mine.bitmap->solidAt(0, 0));
	CHECK(mine.bitmap->solidAt(3, 3));

	frames(game, 30);
	CHECK(showing(game.getObject("same")) == 'a');
	CHECK(showing(game.getObject("quick")) == 'b');

	frames(game, 30);
	CHECK(showing(game.getObject("same")) == 'b');
	CHECK(showing(game.getObject("quick")) == 'a');

	CHECK_FALSE(mine.bitmap->solidAt(0, 0));
	CHECK_FALSE(mine.bitmap->solidAt(3, 3));
}

TEST_CASE("an object with one sprite is not animated, named or not", "[animation]")
{
	Loaded loaded{ gameXml(objectWith(kA), "<show object=\"thing\" />") };
	const Object& thing = loaded.game.getObject("thing");

	CHECK(thing.animationBitmaps.empty());
	CHECK(thing.animationFrames == 0);
	CHECK(showing(thing) == 'a');
}

TEST_CASE("mistakes in sprites and animations are reported where they are", "[animation][xml]")
{
	const std::string shows = "<show object=\"thing\" />";
	const std::string aLine =
		"<sprite name=\"c\"><line><from><x>0</x><y>0</y></from><to><x>3</x><y>3</y></to></line></sprite>";

	SECTION("several sprites and nothing to say when each is shown")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + kB), shows)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("2 <sprite>s") && ContainsSubstring("<animation>"));
	}

	SECTION("a frame that names a sprite that is not there")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + kB, "<animation><interval>1</interval><frame sprite=\"a\" /><frame sprite=\"z\" /></animation>"), shows)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("frame sprite=\"z\"") && ContainsSubstring("no <sprite name=\"z\">"));
	}

	SECTION("a sprite the animation never shows")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + kB + "<sprite name=\"spare\"><bitmap><row>*</row></bitmap></sprite>", kTwoFrames), shows)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("\"spare\"") && ContainsSubstring("never shown"));
	}

	SECTION("a sprite with no name when there are several")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + "<sprite><bitmap><row>*</row></bitmap></sprite>", "<animation><interval>1</interval><frame sprite=\"a\" /><frame sprite=\"a\" /></animation>"), shows)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("no name"));
	}

	SECTION("two sprites with one name")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + kA, kTwoFrames), shows)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("two <sprite>s are called \"a\""));
	}

	SECTION("an animation of one frame")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA, "<animation><interval>1</interval><frame sprite=\"a\" /></animation>"), shows)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("at least two <frame>s"));
	}

	SECTION("an animation with no interval")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + kB, "<animation><frame sprite=\"a\" /><frame sprite=\"b\" /></animation>"), shows)),
			ContainsSubstring("<animation>") && ContainsSubstring("missing <interval>"));
	}

	SECTION("a frame with no sprite")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + kB, "<animation><interval>1</interval><frame /><frame sprite=\"b\" /></animation>"), shows)),
			ContainsSubstring("<frame>") && ContainsSubstring("sprite=\"...\""));
	}

	SECTION("an interval that is not above 0")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + kB, "<animation><interval>0</interval><frame sprite=\"a\" /><frame sprite=\"b\" /></animation>"), shows)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("<interval>") && ContainsSubstring("above 0"));
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + kB, "<animation><interval>-2</interval><frame sprite=\"a\" /><frame sprite=\"b\" /></animation>"), shows)),
			ContainsSubstring("above 0"));
	}

	SECTION("seconds in a window with no framerate to count them in")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + kB, kTwoFrames), shows, 0)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("framerate"));
	}

	SECTION("frames that are not all the same size")
	{
		const std::string big = "<sprite name=\"b\"><bitmap><row>.**</row><row>*..</row><scale>2</scale></bitmap></sprite>";
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + big, kTwoFrames), shows)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("frame 2 (\"b\")") && ContainsSubstring("6 by 4") && ContainsSubstring("4 by 4"));
	}

	SECTION("frames laid out as different grids")
	{
		const std::string gridA = "<sprite name=\"a\"><grid><columns>2</columns><rows>1</rows><bitmap><row>*</row></bitmap></grid></sprite>";
		const std::string gridB = "<sprite name=\"b\"><grid><columns>3</columns><rows>1</rows><bitmap><row>*</row></bitmap></grid></sprite>";
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(gridA + gridB, kTwoFrames), shows)),
			ContainsSubstring("frame 2") && ContainsSubstring("<grid>"));
	}

	SECTION("a frame that is not a picture")
	{
		const std::string box = "<sprite name=\"b\"><rectangle><width>4</width><height>4</height></rectangle></sprite>";
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + box, kTwoFrames), shows)),
			ContainsSubstring("frame 2") && ContainsSubstring("<bitmap>") && ContainsSubstring("<rectangle>"));
	}

	SECTION("a frame that is a drawing of lines is fine, if it is the same size")
	{
		// 4 by 4: a line from 0, 0 to 3, 3.
		Loaded loaded{ gameXml(objectWith(kA + aLine,
			"<animation><interval>1</interval><frame sprite=\"a\" /><frame sprite=\"c\" /></animation>"), shows) };
		CHECK(loaded.game.getObject("thing").animationBitmaps.size() == 2);
	}

	SECTION("frames of different shapes that would be turned into different squares")
	{
		const std::string wide = "<sprite name=\"w\"><bitmap><row>****</row><row>****</row><scale>2</scale></bitmap></sprite>";
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + wide,
			"<animation><interval>1</interval><frame sprite=\"a\" /><frame sprite=\"w\" /></animation>", "<heading>0</heading>"), shows)),
			ContainsSubstring("frame 2") && ContainsSubstring("same size"));
	}

	SECTION("a member whose own sprites leave the group's animation with nothing to show")
	{
		const std::string group =
			"<group name=\"pair\">" + kA + kB + kTwoFrames +
			"<position><y>50</y></position><velocity><x>0</x><y>0</y></velocity>"
			"<collisions><enabled>false</enabled></collisions>"
			"<member name=\"odd\"><position><x>10</x></position><sprite name=\"only\"><bitmap><row>*</row></bitmap></sprite></member>"
			"</group>";
		CHECK_THROWS_WITH(Loaded(gameXml(group, "<show object=\"pair\" />")),
			ContainsSubstring("object 'odd'") && ContainsSubstring("no <sprite name=\"a\">"));
	}
}

TEST_CASE("an animated object that turns keeps its picture and its heading together", "[animation][heading][xml]")
{
	Loaded loaded{ gameXml(objectWith(kA + kB, kTwoFrames, "<heading>0</heading>"), "<show object=\"thing\" />") };
	Game& game = loaded.game;
	game.setCurrentState("playing");
	Object& thing = game.getObject("thing");

	// What each picture turns from, one for each, all turning in one size.
	REQUIRE(thing.turnables.size() == 2);
	CHECK(thing.turnables[0]->at(0.0f).width == thing.turnables[1]->at(0.0f).width);
	CHECK(thing.turnables[0]->at(0.0f).rgba != thing.turnables[1]->at(0.0f).rgba);
	CHECK(thing.bitmap->rgba == thing.turnables[0]->at(0.0f).rgba);

	// Turning keeps the picture that is showing.
	thing.heading = 90.0f;
	thing.showHeading();
	CHECK(thing.bitmap->rgba == thing.turnables[0]->at(90.0f).rgba);

	// The next picture comes up at the heading it is facing.
	frames(game, 30);
	CHECK(thing.bitmap->rgba == thing.turnables[1]->at(90.0f).rgba);

	thing.heading = 181.0f;
	thing.showHeading();
	CHECK(thing.bitmap->rgba == thing.turnables[1]->at(181.0f).rgba);

	frames(game, 30);
	CHECK(thing.bitmap->rgba == thing.turnables[0]->at(181.0f).rgba);

	// A reset faces it as it started, and starts the animation again.
	game.resetAll();
	CHECK(thing.heading == 0.0f);
	CHECK(thing.bitmap->rgba == thing.turnables[0]->at(0.0f).rgba);
}

// ---------------------------------------------------------- Space Invaders

namespace
{
	std::vector<const Object*> aliensOf(Game& game)
	{
		std::vector<const Object*> aliens;
		for (const Object& object : game.getCurrentObjects())
		{
			if (object.objClass == "aliens") { aliens.push_back(&object); }
		}
		return aliens;
	}

	struct Invasion
	{
		Game game{ "games/spaceinvaders.xml" };
		CommandExecutor executor{ game };

		Invasion()
		{
			measure(game);
			game.setCurrentState("playing");
		}

		int alive()
		{
			const auto aliens = aliensOf(game);
			return static_cast<int>(std::count_if(aliens.begin(), aliens.end(), [](const Object* alien) { return alien->isVisible; }));
		}
	};
}

TEST_CASE("Space Invaders has three kinds of alien, a ship and a bullet drawn as the sprites they are", "[spaceinvaders]")
{
	Invasion invasion;
	Game& game = invasion.game;

	// Eleven to a row: one row of squids, two of crabs, two of octopuses.
	CHECK(aliensOf(game).size() == 55);
	for (const char* name : { "squids.1.1", "squids.11.1", "crabs.1.1", "crabs.11.2", "octopuses.1.1", "octopuses.11.2" })
	{
		INFO(name);
		CHECK(game.tryGetObject(name) != nullptr);
	}
	CHECK(game.tryGetObject("squids.1.2") == nullptr);
	CHECK(game.tryGetObject("crabs.1.3") == nullptr);

	// Every alien is a picture of 11 by 8 characters at 5 pixels each, two
	// pictures that take turns.
	for (const Object* alien : aliensOf(game))
	{
		REQUIRE(alien->bitmap);
		CHECK(alien->bitmap->width == 55);
		CHECK(alien->bitmap->height == 40);
		CHECK(alien->animationBitmaps.size() == 2);
		CHECK(alien->animationFrames == 60);
	}

	// The three kinds are laid out one under the other, a row of gap between.
	CHECK(game.getObject("squids.1.1").position.y == 40.0f);
	CHECK(game.getObject("crabs.1.1").position.y == 95.0f);
	CHECK(game.getObject("crabs.1.2").position.y == 150.0f);
	CHECK(game.getObject("octopuses.1.1").position.y == 205.0f);
	CHECK(game.getObject("octopuses.1.2").position.y == 260.0f);
	CHECK(game.getObject("squids.1.1").position.x == 80.0f);
	CHECK(game.getObject("squids.2.1").position.x == 150.0f);

	// All of them are one block: one lockstep number between the three grids.
	std::set<int> lockstep;
	for (const Object* alien : aliensOf(game)) { lockstep.insert(alien->collisionData.lockstep); }
	CHECK(lockstep.size() == 1);
	CHECK(*lockstep.begin() > 0);

	// The ship is a picture, in the middle of the screen.
	const Object& player = game.getObject("player");
	REQUIRE(player.bitmap);
	CHECK(player.bitmap->width == 55);
	CHECK(player.bitmap->height == 35);
	CHECK(player.animationBitmaps.empty());
	CHECK(player.position.x == (1024.0f - 55.0f) / 2.0f);

	// The bullet is a thin tall rectangle.
	const Object& bullet = game.getObject("bullet");
	CHECK(bullet.shapeKind == ShapeKind::Rectangle);
	const Vector2f size = measureShapeSize(bullet.spriteParams, bullet.shapeKind);
	CHECK(size.x == 4.0f);
	CHECK(size.y == 24.0f);
}

TEST_CASE("the aliens change picture every second, all together, and march on meanwhile", "[spaceinvaders][animation]")
{
	Invasion invasion;
	Game& game = invasion.game;

	const auto onPicture = [&](std::size_t index)
	{
		for (const Object* alien : aliensOf(game))
		{
			if (alien->bitmap != alien->animationBitmaps[index]) { return false; }
		}
		return true;
	};

	REQUIRE(onPicture(0));

	frames(game, 59);
	CHECK(onPicture(0));

	frames(game, 1);
	CHECK(onPicture(1));
	CHECK_FALSE(onPicture(0));

	// Two pixels a frame to the right in the meantime, as before.
	CHECK(game.getObject("squids.1.1").position.x == 80.0f + 2.0f * 60.0f);

	frames(game, 60);
	CHECK(onPicture(0));
}

TEST_CASE("the aliens' two pictures differ, so there is something to see", "[spaceinvaders][animation]")
{
	Invasion invasion;
	const Object& alien = invasion.game.getObject("crabs.4.1");

	REQUIRE(alien.animationBitmaps.size() == 2);
	CHECK(alien.animationBitmaps[0]->rgba != alien.animationBitmaps[1]->rgba);
	CHECK(alien.animationBitmaps[0]->width == alien.animationBitmaps[1]->width);
	CHECK(alien.animationBitmaps[0]->height == alien.animationBitmaps[1]->height);
}

TEST_CASE("a shot from the ship in the middle kills exactly one alien and is put away", "[spaceinvaders]")
{
	Invasion invasion;
	Game& game = invasion.game;

	REQUIRE(invasion.alive() == 55);

	invasion.executor.executeInput(Command{ CmdTriggerAction{ "player", "gun" } }, true);
	REQUIRE(game.getObject("bullet").isVisible);

	// A thin bullet leaves the middle of the cannon.
	const Object& cannon = game.getObject("player");
	const Object& shot = game.getObject("bullet");
	CHECK(shot.position.x + 2.0 == Catch::Approx(cannon.position.x + 27.5));

	frames(game, 80);

	CHECK(invasion.alive() == 54);
	CHECK_FALSE(game.getObject("bullet").isVisible);
	CHECK_FALSE(game.getObject("bullet").collisionData.enabled);
}

TEST_CASE("the aliens still win the game for the player when they are all gone, and a reset brings them back", "[spaceinvaders]")
{
	Invasion invasion;
	Game& game = invasion.game;

	for (const Object* alien : aliensOf(game)) { const_cast<Object*>(alien)->isVisible = false; }
	REQUIRE(invasion.alive() == 0);

	frames(game, 1);
	CHECK(game.getCurrentState().name == "gameover");

	game.resetAll();
	CHECK(invasion.alive() == 55);
}

// ------------------------------------------------------------------ schema

namespace
{
	struct Verdict
	{
		bool weakAccepts;
		std::string weakMessage;
		bool strongAccepts;
	};

	// What the weak validator (through TinyXML2) and Xerces's real validation each
	// make of a document. The scratch file sits beside the games so that its
	// relative schema path resolves.
	Verdict judge(const std::string& xml)
	{
		const ScratchFile scratch{ "games/bitmaps_scratch.xml" };
		{
			std::ofstream out(scratch.path);
			out << xml;
		}

		Verdict verdict{};

		auto weak = XmlDocumentFactory::create(XmlBackend::TinyXml2);
		REQUIRE(weak->load(scratch.path.string()));
		XsdLiteValidator validator;
		REQUIRE(validator.loadSchema("assets/xmlgameengine.xsd", XmlBackend::TinyXml2));
		verdict.weakAccepts = validator.validate(*weak->getRootElement());
		verdict.weakMessage = validator.getErrorMessage();

		auto strong = XmlDocumentFactory::create(XmlBackend::Xerces);
		verdict.strongAccepts = strong->load(scratch.path.string());

		return verdict;
	}
}

TEST_CASE("both schema checkers accept Space Invaders", "[spaceinvaders][schema]")
{
	const Verdict verdict = judge(readFile("games/spaceinvaders.xml"));

	CHECK(verdict.weakAccepts);
	CHECK(verdict.strongAccepts);
}

TEST_CASE("both schema checkers turn away the same mistakes in bitmaps and animations", "[bitmap][animation][schema]")
{
	struct Mistake
	{
		const char* what;
		const char* from;
		const char* to;
		const char* weakSays; // part of the weak validator's message
	};

	const Mistake mistakes[] = {
		{ "an animation with no interval", "<interval>animation.seconds</interval>", "", "interval" },
		{ "an animation whose frames are not frames", "<frame sprite=\"a\" />", "<picture sprite=\"a\" />", "picture" },
		{ "a bitmap with its scale before its rows", "<bitmap>\n          <row>.....*.....</row>", "<bitmap>\n          <scale>pixel</scale>\n          <row>.....*.....</row>", "scale" },
		{ "a bitmap row with something inside it", "<row>.....*.....</row>", "<row><x>1</x></row>", "x" },
		{ "a frame with no sprite", "<frame sprite=\"a\" />", "<frame />", "sprite" },
	};

	for (const Mistake& mistake : mistakes)
	{
		DYNAMIC_SECTION(mistake.what)
		{
			const Verdict verdict = judge(gameWith("games/spaceinvaders.xml", mistake.from, mistake.to));

			CHECK_FALSE(verdict.weakAccepts);
			CHECK_THAT(verdict.weakMessage, ContainsSubstring(mistake.weakSays));
			CHECK_FALSE(verdict.strongAccepts);
		}
	}
}

TEST_CASE("an attribute the schema does not declare is turned away by Xerces, which the weak validator cannot see", "[animation][schema]")
{
	// The weak validator cannot list a node's attributes (see xsd_lite.h), so
	// this is the one kind of mistake only the real validation catches.
	const Verdict verdict = judge(gameWith("games/spaceinvaders.xml", "<sprite name=\"a\">", "<sprite title=\"a\">"));

	CHECK(verdict.weakAccepts);
	CHECK_FALSE(verdict.strongAccepts);
}
