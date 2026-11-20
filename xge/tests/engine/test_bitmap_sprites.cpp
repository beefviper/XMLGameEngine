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
// and frames are played with Game::updateObjects(xge::kSixtieth); none of it needs a window.

#include "scratch.h"
#include "file_text.h"
#include "picture/bitmap.h"
#include "play/executor.h"
#include "engine/folder.h"
#include "copies.h"
#include "game/game.h"
#include "xml/document.h"
#include "load/xsd.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <cmath>
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
		ScratchFile scratch{ std::filesystem::temp_directory_path() / scratchName(name) };
		std::ofstream out(scratch.path);
		out << xml;
		return scratch;
	}

	// An object nothing but a plain rectangle, for the second state to show.
	const std::string kOther =
		"<object name=\"another\"><sprite><rectangle><width>5</width><height>5</height></rectangle></sprite>"
		"<position><x>0</x><y>0</y></position><velocity><x>0</x><y>0</y></velocity>"
		"<collisions><off /></collisions></object>";

	// A game with these objects (and `other`), whose `playing` state shows the
	// named ones and whose `away` state shows only `other`.
	std::string gameXml(const std::string& objects, const std::string& shows, int framerate = 60)
	{
		return "<game>"
			"<window name=\"test\"><width>800</width><height>600</height><background>color.black</background>"
			"<framerate>" + std::to_string(framerate) + "</framerate></window>"
			"<variables><variable name=\"pixel\">3</variable></variables>"
			"<states><objects>" + objects + kOther + "</objects>"
			""
			"<state name=\"playing\"><shows>" + shows + "</shows><inputs><input button=\"space\"><pop /></input></inputs></state>"
			"<state name=\"away\"><shows><show object=\"another\" /></shows><inputs><input button=\"space\"><pop /></input></inputs></state>"
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
			object.size = measureShapeSize(object.drawn.picture);
		}
	}

	void frames(Game& game, int count)
	{
		for (int i = 0; i < count; ++i) { game.updateObjects(xge::kSixtieth); }
	}

	// What is wrapped around a sprite, or sprites, and an animation.
	std::string objectWith(const std::string& sprites, const std::string& animation = "", const std::string& extra = "")
	{
		return "<object name=\"thing\">" + sprites + animation +
			"<position><x>10</x><y>20</y></position><velocity><x>0</x><y>0</y></velocity>" + extra +
			"<collisions><off /></collisions></object>";
	}

	// A group "thing" of 3 columns and 2 rows of the sprites (and animation)
	// given, starting where objectWith's thing is.
	std::string cellsWith(const std::string& sprites, const std::string& padding = "")
	{
		return "<group name=\"thing\"><columns>3</columns><rows>2</rows>" + padding + sprites +
			"<position><x>10</x><y>20</y></position><velocity><x>0</x><y>0</y></velocity>"
			"<collisions><off /></collisions></group>";
	}

	// Two 2 by 2 pictures, 4 by 4 pixels at a scale of 2: a has its solid
	// pixels at the top left and bottom right, b at the top right and bottom left.
	const std::string kA =
		"<sprite name=\"a\"><bitmap><row>*.</row><row>.*</row><scale>2</scale><color>color.red</color></bitmap></sprite>";
	const std::string kB =
		"<sprite name=\"b\"><bitmap><row>.*</row><row>*.</row><scale>2</scale><color>color.red</color></bitmap></sprite>";
	const std::string kTwoFrames =
		"<animation><seconds>0.5</seconds><frame sprite=\"a\" /><frame sprite=\"b\" /></animation>";

	// Which of kA and kB an object is showing.
	char showing(const Object& object)
	{
		REQUIRE(object.drawn.bitmap);
		if (object.drawn.bitmap->solidAt(0, 0) && !object.drawn.bitmap->solidAt(2, 0)) { return 'a'; }
		if (object.drawn.bitmap->solidAt(2, 0) && !object.drawn.bitmap->solidAt(0, 0)) { return 'b'; }
		return '?';
	}

	std::string readFile(const std::string& path)
	{
		std::ifstream in(path);
		REQUIRE(in.good());
		std::string text = fileText(in);
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
	const Object& thing = loaded.game.object("thing");

	REQUIRE(thing.drawn.picture->kind == ShapeKind::Line);
	REQUIRE(thing.drawn.bitmap);

	// 3 characters by 2 rows, each 3 pixels (the variable `pixel`).
	CHECK(thing.drawn.bitmap->width == 9);
	CHECK(thing.drawn.bitmap->height == 6);
	CHECK(thing.drawn.picture->size == Vector2f{ 9.0f, 6.0f });

	const Vector2f size = measureShapeSize(thing.drawn.picture);
	CHECK(size.x == 9.0f);
	CHECK(size.y == 6.0f);

	CHECK(thing.drawn.bitmap->solidAt(4, 0));
	CHECK_FALSE(thing.drawn.bitmap->solidAt(0, 0));
	CHECK(thing.drawn.bitmap->solidAt(0, 5));
	CHECK(thing.drawn.bitmap->rgba[(0 * 9 + 4) * 4 + 0] == 255);
	CHECK(thing.drawn.bitmap->rgba[(0 * 9 + 4) * 4 + 1] == 0);
}

TEST_CASE("the scale and the color can be left out: one pixel to a character, white", "[bitmap][xml]")
{
	Loaded loaded{ gameXml(objectWith("<sprite><bitmap><row>**</row></bitmap></sprite>"), "<show object=\"thing\" />") };
	const Object& thing = loaded.game.object("thing");

	CHECK(thing.drawn.bitmap->width == 2);
	CHECK(thing.drawn.bitmap->height == 1);
	CHECK(thing.drawn.bitmap->rgba[0] == 255);
	CHECK(thing.drawn.bitmap->rgba[1] == 255);
	CHECK(thing.drawn.bitmap->rgba[2] == 255);
}

TEST_CASE("a sprite takes a picture of the <pictures> by <bitmap from>, and what it gives changes the picture's", "[bitmap][pictures][xml]")
{
	const std::string pictures = "<pictures><bitmap name=\"ship\"><row>.*.</row><row>***</row><scale>pixel</scale><color>color.red</color></bitmap></pictures>";
	const auto with = [&](const std::string& sprite)
	{
		std::string xml = gameXml(objectWith(sprite), "<show object=\"thing\" />");
		return xml.insert(xml.rfind("</game>"), pictures);
	};

	// As it is: its rows, scale and red.
	Loaded taken{ with("<sprite><bitmap from=\"ship\" /></sprite>") };
	const Object& ship = taken.game.object("thing");
	REQUIRE(ship.drawn.bitmap);
	CHECK(ship.drawn.bitmap->width == 9);
	CHECK(ship.drawn.bitmap->height == 6);
	CHECK(ship.drawn.bitmap->solidAt(4, 0));
	CHECK(ship.drawn.bitmap->rgba[(0 * 9 + 4) * 4 + 0] == 255);
	CHECK(ship.drawn.bitmap->rgba[(0 * 9 + 4) * 4 + 1] == 0);

	// Its own scale and colour over the picture's; the rows stay.
	Loaded changed{ with("<sprite><bitmap from=\"ship\"><scale>1</scale><color>color.green</color></bitmap></sprite>") };
	const Object& small = changed.game.object("thing");
	REQUIRE(small.drawn.bitmap);
	CHECK(small.drawn.bitmap->width == 3);
	CHECK(small.drawn.bitmap->height == 2);
	CHECK(small.drawn.bitmap->rgba[(0 * 3 + 1) * 4 + 0] == 0);
	CHECK(small.drawn.bitmap->rgba[(0 * 3 + 1) * 4 + 1] == 255);

	// Its own rows replace all the picture's.
	Loaded drawn{ with("<sprite><bitmap from=\"ship\"><row>**</row></bitmap></sprite>") };
	CHECK(drawn.game.object("thing").drawn.bitmap->width == 6);
	CHECK(drawn.game.object("thing").drawn.bitmap->height == 3);

	// A name the pictures have not got stops the load.
	CHECK_THROWS_WITH(Loaded{ with("<sprite><bitmap from=\"saucer\" /></sprite>") }, ContainsSubstring("names no <bitmap> of the <pictures>"));
}

TEST_CASE("a sprite takes a line drawing of the <pictures> by <drawing from>, as it is", "[lines][pictures][xml]")
{
	const std::string line = "<line><from><x>0</x><y>0</y></from><to><x>20</x><y>10</y></to><thickness>2</thickness></line>";
	const auto with = [&](const std::string& sprite, const std::string& pictures)
	{
		std::string xml = gameXml(objectWith(sprite), "<show object=\"thing\" />");
		return xml.insert(xml.rfind("</game>"), pictures);
	};

	// The same as the lines written in the sprite.
	Loaded written{ with("<sprite>" + line + "</sprite>", "") };
	Loaded taken{ with("<sprite><drawing from=\"rock\" /></sprite>", "<pictures><drawing name=\"rock\">" + line + "</drawing></pictures>") };
	const Object& mine = written.game.object("thing");
	const Object& rock = taken.game.object("thing");
	REQUIRE(rock.drawn.bitmap);
	CHECK(rock.drawn.bitmap->width == mine.drawn.bitmap->width);
	CHECK(rock.drawn.bitmap->height == mine.drawn.bitmap->height);
	CHECK(rock.drawn.bitmap->rgba == mine.drawn.bitmap->rgba);

	// A name the pictures have not got, or two drawings of one name, stop the load.
	CHECK_THROWS_WITH(Loaded{ with("<sprite><drawing from=\"saucer\" /></sprite>", "<pictures><drawing name=\"rock\">" + line + "</drawing></pictures>") },
		ContainsSubstring("names no <drawing> of the <pictures>"));
	CHECK_THROWS_WITH(Loaded{ with("<sprite><drawing from=\"rock\" /></sprite>",
		"<pictures><drawing name=\"rock\">" + line + "</drawing><drawing name=\"rock\">" + line + "</drawing></pictures>") },
		ContainsSubstring("two <drawing>s are called 'rock'"));
}

TEST_CASE("a bitmap's size can be used in an expression like any shape's", "[bitmap][xml]")
{
	const std::string flag =
		"<object name=\"flag\"><sprite><rectangle><width>5</width><height>5</height></rectangle></sprite>"
		"<position><x><subtract><value>window.right</value><value>thing.width</value></subtract></x><y>thing.height</y></position>"
		"<velocity><x>0</x><y>0</y></velocity><collisions><off /></collisions></object>";
	Loaded loaded{ gameXml(objectWith("<sprite><bitmap><row>*.*.*</row><row>*.*.*</row><scale>2</scale></bitmap></sprite>") + flag,
		"<show object=\"thing\" /><show object=\"flag\" />") };

	CHECK(loaded.game.object("flag").position.x == 800.0f - 10.0f);
	CHECK(loaded.game.object("flag").position.y == 4.0f);
}

TEST_CASE("the cells of a group of a bitmap are one object each, all sharing one picture", "[bitmap][xml]")
{
	Loaded loaded{ gameXml(cellsWith(
		"<sprite><bitmap><row>**</row><row>*.</row><scale>5</scale></bitmap></sprite>", "<padding><x>4</x><y>6</y></padding>"),
		"<show object=\"thing\" />") };

	std::set<const Bitmap*> pictures;
	int cells = 0;
	for (const Object& object : loaded.game.getCurrentObjects())
	{
		if (object.groupName != "thing") { continue; }

		++cells;
		REQUIRE(object.drawn.bitmap);
		pictures.insert(object.drawn.bitmap.get());
	}
	CHECK(cells == 6);
	CHECK(pictures.size() == 1);

	// 10 by 10 each, 4 across and 6 down between: the cell in column 3, row 2.
	const Object& last = loaded.game.object("thing.3.2");
	CHECK(last.position.x == 10.0f + 2 * (10.0f + 4.0f));
	CHECK(last.position.y == 20.0f + (10.0f + 6.0f));
}

TEST_CASE("a bitmap object can be tested by its pixels", "[bitmap][xml]")
{
	Loaded loaded{ gameXml(objectWith("<sprite><bitmap><row>*.</row><row>**</row><scale>4</scale></bitmap></sprite>"),
		"<show object=\"thing\" />") };
	const Object& thing = loaded.game.object("thing");

	// The clear corner is clear, and what is drawn is what is tested.
	CHECK_FALSE(thing.drawn.bitmap->solidAt(5, 1));
	CHECK(thing.drawn.bitmap->solidAt(1, 1));
	CHECK(thing.drawn.bitmap->solidAt(5, 5));
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
	Object& thing = loaded.game.object("thing");

	REQUIRE(thing.drawn.turnables.size() == 1);
	CHECK(thing.drawn.turnables[0]->picture);
	CHECK(thing.drawn.turnables[0]->picture->width == 3);

	thing.heading = 30.0f;
	thing.showHeading();
	const auto drawn = thing.drawn.bitmap;
	CHECK(thing.drawn.turnedDegrees == 30);

	// Less than half a degree away: the same picture, nothing marked.
	thing.drawn.visualDirty = false;
	thing.heading = 30.4f;
	thing.showHeading();
	CHECK(thing.drawn.bitmap == drawn);
	CHECK_FALSE(thing.drawn.visualDirty);

	// A degree on: a new one.
	thing.heading = 31.0f;
	thing.showHeading();
	CHECK(thing.drawn.bitmap != drawn);
	CHECK(thing.drawn.turnedDegrees == 31);
	CHECK(thing.drawn.visualDirty);

	// Round the top of the circle and back to 0.
	thing.heading = 359.6f;
	thing.showHeading();
	CHECK(thing.drawn.turnedDegrees == 0);
}

TEST_CASE("an object with a <heading> can be a bitmap: drawn once, then turned to every heading", "[bitmap][heading][xml]")
{
	Loaded loaded{ gameXml(objectWith(kCorner, "", "<heading>0</heading>"), "<show object=\"thing\" />") };
	Game& game = loaded.game;
	game.setCurrentState("playing");
	Object& thing = game.object("thing");

	REQUIRE(thing.drawn.turnables.size() == 1);
	CHECK(thing.drawn.bitmap->width == 5);
	CHECK(thing.drawn.bitmap->solidAt(1, 1));

	// Its size is the square every heading shares.
	measure(game);
	CHECK(thing.size.x == 5.0f);
	CHECK(thing.size.y == 5.0f);

	thing.drawn.visualDirty = false;
	thing.heading = 90.0f;
	thing.showHeading();
	CHECK(thing.drawn.turnedDegrees == 90);
	CHECK(thing.drawn.bitmap->solidAt(3, 1));
	CHECK(thing.drawn.visualDirty);
}

TEST_CASE("a bitmap that turns can be tested by its pixels at the heading it faces", "[bitmap][heading][xml]")
{
	Loaded loaded{ gameXml(objectWith(kCorner, "", "<heading>180</heading>"), "<show object=\"thing\" />") };
	Object& thing = loaded.game.object("thing");

	CHECK(thing.drawn.bitmap->solidAt(3, 3));
	CHECK_FALSE(thing.drawn.bitmap->solidAt(1, 1));
}

// -------------------------------------------------------------- animation

TEST_CASE("an animation shows its sprites in turn, each for the interval, round and round", "[animation]")
{
	// Half a second is 30 steps of a sixtieth.
	Loaded loaded{ gameXml(objectWith(kA + kB, kTwoFrames), "<show object=\"thing\" />") };
	Game& game = loaded.game;
	game.setCurrentState("playing");
	Object& thing = game.object("thing");

	CHECK(thing.animation.frames.shows().seconds == 0.5f);
	REQUIRE(thing.animation.frames.shows().bitmaps.size() == 2);
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

TEST_CASE("a frame with its own seconds shows that long; the others the animation's", "[animation]")
{
	const char* const animation = "<animation><seconds>0.1</seconds><frame sprite=\"a\" /><frame sprite=\"b\"><seconds>0.5</seconds></frame></animation>";

	SECTION("an object's one animation")
	{
		Loaded loaded{ gameXml(objectWith(kA + kB, animation), "<show object=\"thing\" />") };
		Game& game = loaded.game;
		game.setCurrentState("playing");
		Object& thing = game.object("thing");
		CHECK(thing.animation.frames.shows().times == std::vector<float>{ 0.1f, 0.5f });

		frames(game, 5);
		CHECK(showing(thing) == 'a');
		frames(game, 1);
		CHECK(showing(thing) == 'b');
		frames(game, 29);
		CHECK(showing(thing) == 'b');
		frames(game, 1);
		CHECK(showing(thing) == 'a');
		frames(game, 6);
		CHECK(showing(thing) == 'b');
	}

	SECTION("a look that moves")
	{
		const std::string named = "<animation name=\"blink\"><seconds>0.1</seconds><frame sprite=\"a\" /><frame sprite=\"b\"><seconds>0.5</seconds></frame></animation>";
		Loaded loaded{ gameXml(objectWith(kA + kB, named), "<show object=\"thing\" />") };
		Game& game = loaded.game;
		game.setCurrentState("playing");
		Object& thing = game.object("thing");
		CHECK(thing.animation.frames.shows().times == std::vector<float>{ 0.1f, 0.5f });
		frames(game, 6);
		CHECK(showing(thing) == 'b');
		frames(game, 29);
		CHECK(showing(thing) == 'b');
		frames(game, 1);
		CHECK(showing(thing) == 'a');
	}

	SECTION("none of its own: every frame the animation's")
	{
		Loaded loaded{ gameXml(objectWith(kA + kB, kTwoFrames), "<show object=\"thing\" />") };
		CHECK(loaded.game.object("thing").animation.frames.shows().times.empty());
	}

	SECTION("a frame's own time must be above 0")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + kB, "<animation><seconds>1</seconds><frame sprite=\"a\"><seconds>0</seconds></frame><frame sprite=\"b\" /></animation>"),
			"<show object=\"thing\" />")), ContainsSubstring("<frame> 1") && ContainsSubstring("above 0"));
	}
}

TEST_CASE("an animation with once plays through one time", "[animation]")
{
	SECTION("a look goes back to the look shown before it")
	{
		const std::string animations =
			"<animation name=\"idle\"><seconds>1</seconds><frame sprite=\"a\" /><frame sprite=\"a\" /></animation>"
			"<animation name=\"flash\"><seconds>0.1</seconds><once /><frame sprite=\"b\" /><frame sprite=\"a\" /><frame sprite=\"b\" /></animation>";
		Loaded loaded{ gameXml(objectWith(kA + kB, animations), "<show object=\"thing\" />") };
		Game& game = loaded.game;
		game.setCurrentState("playing");
		Object& thing = game.object("thing");
		CHECK(thing.lookName() == "idle");

		game.showLook(thing, "flash");
		CHECK(showing(thing) == 'b');
		frames(game, 6);
		CHECK(showing(thing) == 'a');
		frames(game, 6);
		CHECK(showing(thing) == 'b');
		frames(game, 5);
		CHECK(thing.lookName() == "flash");
		frames(game, 1);
		CHECK(thing.lookName() == "idle");
		CHECK(showing(thing) == 'a');

		// A reset half way puts it on its first look, playing as at the start.
		game.showLook(thing, "flash");
		frames(game, 7);
		game.resetAll();
		CHECK(thing.lookName() == "idle");
		CHECK(showing(thing) == 'a');
		frames(game, 120);
		CHECK(thing.lookName() == "idle");
	}

	SECTION("an object's one animation stays on its last picture until a reset")
	{
		const char* const animation = "<animation><seconds>0.1</seconds><once /><frame sprite=\"a\" /><frame sprite=\"b\" /></animation>";
		Loaded loaded{ gameXml(objectWith(kA + kB, animation), "<show object=\"thing\" />") };
		Game& game = loaded.game;
		game.setCurrentState("playing");
		Object& thing = game.object("thing");

		frames(game, 6);
		CHECK(showing(thing) == 'b');
		frames(game, 60);
		CHECK(showing(thing) == 'b');
		CHECK(thing.animation.done);

		game.resetAll();
		CHECK(showing(thing) == 'a');
		frames(game, 6);
		CHECK(showing(thing) == 'b');
	}
}

TEST_CASE("an animation's then names the look it goes to and the commands it runs", "[animation]")
{
	const std::string idle = "<animation name=\"idle\"><seconds>1</seconds><frame sprite=\"a\" /><frame sprite=\"a\" /></animation>";
	// The thing, counting in its own number `ended`.
	const auto counting = [](const std::string& animations)
	{
		std::string object = objectWith(kA + kB, animations);
		object.insert(object.find("</object>"), "<variables><variable name=\"ended\">0</variable></variables>");
		return object;
	};

	SECTION("then sprite= goes to that look in place of the one before")
	{
		const std::string animations = idle +
			"<animation name=\"flash\"><seconds>0.1</seconds><once /><frame sprite=\"b\" /><frame sprite=\"a\" /><then sprite=\"b\" /></animation>";
		Loaded loaded{ gameXml(objectWith(kA + kB, animations), "<show object=\"thing\" />") };
		Game& game = loaded.game;
		game.setCurrentState("playing");
		Object& thing = game.object("thing");
		game.showLook(thing, "flash");
		frames(game, 12);
		CHECK(thing.lookName() == "b");
		frames(game, 60);
		CHECK(thing.lookName() == "b");
	}

	SECTION("then's commands run once, as the animation ends")
	{
		const std::string animations = idle +
			"<animation name=\"flash\"><seconds>0.1</seconds><once /><frame sprite=\"b\" /><frame sprite=\"a\" /><then><inc variable=\"thing.ended\" /></then></animation>";
		Loaded loaded{ gameXml(counting(animations), "<show object=\"thing\" />") };
		Game& game = loaded.game;
		game.setCurrentState("playing");
		Object& thing = game.object("thing");
		game.showLook(thing, "flash");
		frames(game, 11);
		CHECK(thing.variable.at("ended") == 0.0f);
		frames(game, 1);
		CHECK(thing.variable.at("ended") == 1.0f);
		CHECK(thing.lookName() == "idle");
		frames(game, 60);
		CHECK(thing.variable.at("ended") == 1.0f);
	}

	SECTION("an object's one animation runs them too, and stays on its last picture")
	{
		const char* const animation = "<animation><seconds>0.1</seconds><once /><frame sprite=\"a\" /><frame sprite=\"b\" /><then><inc variable=\"thing.ended\" /></then></animation>";
		Loaded loaded{ gameXml(counting(animation), "<show object=\"thing\" />") };
		Game& game = loaded.game;
		game.setCurrentState("playing");
		const Object& thing = game.object("thing");
		frames(game, 12);
		CHECK(thing.variable.at("ended") == 1.0f);
		CHECK(showing(thing) == 'b');
		frames(game, 60);
		CHECK(thing.variable.at("ended") == 1.0f);
	}

	SECTION("what cannot be")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + kB, idle +
			"<animation name=\"flash\"><seconds>0.1</seconds><once /><frame sprite=\"b\" /><frame sprite=\"a\" /><then sprite=\"glow\" /></animation>"),
			"<show object=\"thing\" />")), ContainsSubstring("<then sprite=\"glow\">") && ContainsSubstring("no look of that name"));
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + kB, idle +
			"<animation name=\"flash\"><seconds>0.1</seconds><frame sprite=\"b\" /><frame sprite=\"a\" /><then sprite=\"a\" /></animation>"),
			"<show object=\"thing\" />")), ContainsSubstring("needs <once />"));
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + kB,
			"<animation><seconds>0.1</seconds><once /><frame sprite=\"a\" /><frame sprite=\"b\" /><then sprite=\"a\" /></animation>"),
			"<show object=\"thing\" />")), ContainsSubstring("no looks to go to"));
	}
}

TEST_CASE("an animation that plays once may be one frame: a look held for a time", "[animation]")
{
	const std::string animations =
		"<animation name=\"idle\"><seconds>1</seconds><frame sprite=\"a\" /><frame sprite=\"a\" /></animation>"
		"<animation name=\"lit\"><seconds>0.1</seconds><once /><frame sprite=\"b\" /></animation>";
	Loaded loaded{ gameXml(objectWith(kA + kB, animations), "<show object=\"thing\" />") };
	Game& game = loaded.game;
	game.setCurrentState("playing");
	Object& thing = game.object("thing");
	game.showLook(thing, "lit");
	CHECK(showing(thing) == 'b');
	frames(game, 5);
	CHECK(thing.lookName() == "lit");
	frames(game, 1);
	CHECK(thing.lookName() == "idle");

	// With no animation that goes round, it starts as its first sprite,
	// never as a look that plays once.
	Loaded still{ gameXml(objectWith(kA + kB, "<animation name=\"lit\"><seconds>0.1</seconds><once /><frame sprite=\"b\" /></animation>"),
		"<show object=\"thing\" />") };
	CHECK(still.game.object("thing").lookName() == "a");

	// Without <once />, one frame is still too few.
	CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + kB, "<animation name=\"lit\"><seconds>0.1</seconds><frame sprite=\"b\" /></animation>"),
		"<show object=\"thing\" />")), ContainsSubstring("at least two"));
}

TEST_CASE("becoming a look that plays once starts it from its first frame, even when it shows already", "[animation]")
{
	const std::string animations =
		"<animation name=\"idle\"><seconds>1</seconds><frame sprite=\"a\" /><frame sprite=\"a\" /></animation>"
		"<animation name=\"flash\"><seconds>0.1</seconds><once /><frame sprite=\"b\" /><frame sprite=\"a\" /></animation>";
	// Every 3 frames it becomes the flash, whose first frame lasts 6.
	std::string object = objectWith(kA + kB, animations);
	object.insert(object.find("</object>"), "<timers><timer><every>0.05</every><become sprite=\"flash\" /></timer></timers>");
	Loaded loaded{ gameXml(object, "<show object=\"thing\" />") };
	Game& game = loaded.game;
	game.setCurrentState("playing");
	Object& thing = game.object("thing");

	frames(game, 3);
	CHECK(thing.lookName() == "flash");
	CHECK(showing(thing) == 'b');
	frames(game, 30);
	CHECK(thing.lookName() == "flash");
	CHECK(showing(thing) == 'b');
	CHECK(thing.animation.index == 0);
}

TEST_CASE("the interval is in seconds, whatever the window's framerate", "[animation]")
{
	const std::string objects = objectWith(kA + kB, "<animation><seconds>1</seconds><frame sprite=\"a\" /><frame sprite=\"b\" /></animation>");

	Loaded fast{ gameXml(objects, "<show object=\"thing\" />", 120) };
	CHECK(fast.game.object("thing").animation.frames.shows().seconds == 1.0f);

	Loaded slow{ gameXml(objects, "<show object=\"thing\" />", 25) };
	CHECK(slow.game.object("thing").animation.frames.shows().seconds == 1.0f);

	// However short the interval, a step passes through every picture.
	Loaded quick{ gameXml(objectWith(kA + kB, "<animation><seconds>0.001</seconds><frame sprite=\"a\" /><frame sprite=\"b\" /></animation>"),
		"<show object=\"thing\" />") };
	CHECK(quick.game.object("thing").animation.frames.shows().seconds == 0.001f);
}

TEST_CASE("the picture is marked to be drawn again when it changes, and not before", "[animation]")
{
	Loaded loaded{ gameXml(objectWith(kA + kB, kTwoFrames), "<show object=\"thing\" />") };
	Game& game = loaded.game;
	game.setCurrentState("playing");
	Object& thing = game.object("thing");

	thing.drawn.visualDirty = false;
	frames(game, 29);
	CHECK_FALSE(thing.drawn.visualDirty);

	frames(game, 1);
	CHECK(thing.drawn.visualDirty);
}

TEST_CASE("a frame can come up more than once, so a sequence need not be only two pictures", "[animation]")
{
	// a, b, a, then round again: three frames of 0.5 s each.
	Loaded loaded{ gameXml(objectWith(kA + kB,
		"<animation><seconds>0.5</seconds><frame sprite=\"a\" /><frame sprite=\"b\" /><frame sprite=\"a\" /></animation>"),
		"<show object=\"thing\" />") };
	Game& game = loaded.game;
	game.setCurrentState("playing");
	Object& thing = game.object("thing");

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
	Object& thing = game.object("thing");

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
		CHECK(thing.animation.elapsed == 0.0);

		frames(game, 29);
		CHECK(showing(thing) == 'a');
		frames(game, 1);
		CHECK(showing(thing) == 'b');
	}
}

TEST_CASE("every cell of a group animates together", "[animation]")
{
	const std::string a = "<sprite name=\"a\"><bitmap><row>*.</row><row>.*</row><scale>2</scale></bitmap></sprite>";
	const std::string b = "<sprite name=\"b\"><bitmap><row>.*</row><row>*.</row><scale>2</scale></bitmap></sprite>";

	Loaded loaded{ gameXml(cellsWith(a + b + kTwoFrames, "<padding><x>2</x><y>2</y></padding>"), "<show object=\"thing\" />") };
	Game& game = loaded.game;
	game.setCurrentState("playing");

	frames(game, 30);

	int cells = 0;
	for (const Object& cell : game.getCurrentObjects())
	{
		if (cell.groupName != "thing") { continue; }
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
		"<animation><seconds>1</seconds><frame sprite=\"a\" /><frame sprite=\"b\" /></animation>"
		"<position><y>50</y></position><velocity><x>0</x><y>0</y></velocity>"
		"<collisions><off /></collisions>"
		// shares everything the group gives
		"<member name=\"same\"><position><x>10</x></position></member>"
		// its own interval, the group's sprites
		"<member name=\"quick\">"
		"<animation><seconds>0.5</seconds><frame sprite=\"a\" /><frame sprite=\"b\" /></animation><position><x>40</x></position></member>"
		// sprites of its own, with the group's animation picking them out by name
		"<member name=\"mine\">"
		"<sprite name=\"a\"><bitmap><row>**</row><row>**</row><scale>2</scale></bitmap></sprite>"
		"<sprite name=\"b\"><bitmap><row>..</row><row>..</row><scale>2</scale></bitmap></sprite><position><x>70</x></position></member>"
		"</group>";

	Loaded loaded{ gameXml(group, "<show object=\"pair\" />") };
	Game& game = loaded.game;
	game.setCurrentState("playing");

	CHECK(game.object("same").animation.frames.shows().seconds == 1.0f);
	CHECK(game.object("quick").animation.frames.shows().seconds == 0.5f);
	CHECK(game.object("mine").animation.frames.shows().seconds == 1.0f);

	// The third member shows its own sprites, which are not kA and kB: its
	// first is solid all over, its second clear all over.
	const Object& mine = game.object("mine");
	CHECK(mine.drawn.bitmap->solidAt(0, 0));
	CHECK(mine.drawn.bitmap->solidAt(3, 3));

	frames(game, 30);
	CHECK(showing(game.object("same")) == 'a');
	CHECK(showing(game.object("quick")) == 'b');

	frames(game, 30);
	CHECK(showing(game.object("same")) == 'b');
	CHECK(showing(game.object("quick")) == 'a');

	CHECK_FALSE(mine.drawn.bitmap->solidAt(0, 0));
	CHECK_FALSE(mine.drawn.bitmap->solidAt(3, 3));
}

TEST_CASE("an object with one sprite is not animated, named or not", "[animation]")
{
	Loaded loaded{ gameXml(objectWith(kA), "<show object=\"thing\" />") };
	const Object& thing = loaded.game.object("thing");

	CHECK(thing.animation.frames.shows().bitmaps.empty());
	CHECK(thing.animation.frames.shows().seconds == 0.0f);
	CHECK(showing(thing) == 'a');
}

TEST_CASE("mistakes in sprites and animations are reported where they are", "[animation][xml]")
{
	const std::string shows = "<show object=\"thing\" />";
	const std::string aLine =
		"<sprite name=\"c\"><line><from><x>0</x><y>0</y></from><to><x>3</x><y>3</y></to></line></sprite>";

	SECTION("several sprites and no animation are looks for <become>, so each needs a name")
	{
		const std::string unnamed = "<sprite><line><from><x>0</x><y>0</y></from><to><x>3</x><y>3</y></to></line></sprite>";
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + unnamed), shows)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("2 <sprite>s") && ContainsSubstring("every one needs a name"));

		// Named, they load: the object shows the first and can become the other.
		Loaded loaded(gameXml(objectWith(kA + kB), shows));
		const Object& thing = loaded.game.object("thing");
		REQUIRE(thing.looks.all().size() == 2);
		CHECK(thing.lookName() == "a");
		CHECK(thing.looks.all()[1].name == "b");
	}

	SECTION("a frame that names a sprite that is not there")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + kB, "<animation><seconds>1</seconds><frame sprite=\"a\" /><frame sprite=\"z\" /></animation>"), shows)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("frame sprite=\"z\"") && ContainsSubstring("no <sprite name=\"z\">"));
	}

	SECTION("a sprite the animation never shows")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + kB + "<sprite name=\"spare\"><bitmap><row>*</row></bitmap></sprite>", kTwoFrames), shows)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("\"spare\"") && ContainsSubstring("never shown"));
	}

	SECTION("a sprite with no name when there are several")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + "<sprite><bitmap><row>*</row></bitmap></sprite>", "<animation><seconds>1</seconds><frame sprite=\"a\" /><frame sprite=\"a\" /></animation>"), shows)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("no name"));
	}

	SECTION("two sprites with one name")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + kA, kTwoFrames), shows)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("two <sprite>s are called \"a\""));
	}

	SECTION("an animation of one frame")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA, "<animation><seconds>1</seconds><frame sprite=\"a\" /></animation>"), shows)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("at least two <frame>s"));
	}

	SECTION("an animation with no interval")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + kB, "<animation><frame sprite=\"a\" /><frame sprite=\"b\" /></animation>"), shows)),
			ContainsSubstring("animation[1]: unexpected element <frame>, expected <seconds>"));
	}

	SECTION("a frame with no sprite")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + kB, "<animation><seconds>1</seconds><frame /><frame sprite=\"b\" /></animation>"), shows)),
			ContainsSubstring("animation[1]/frame: missing required attribute 'sprite'"));
	}

	SECTION("an interval that is not above 0")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + kB, "<animation><seconds>0</seconds><frame sprite=\"a\" /><frame sprite=\"b\" /></animation>"), shows)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("<seconds>") && ContainsSubstring("above 0"));
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + kB, "<animation><seconds>-2</seconds><frame sprite=\"a\" /><frame sprite=\"b\" /></animation>"), shows)),
			ContainsSubstring("above 0"));
	}

	SECTION("a window with a framerate of 0, which could count no seconds")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + kB, kTwoFrames), shows, 0)),
			ContainsSubstring("framerate: \"0\" is not a valid xs:positiveInteger"));
	}

	SECTION("frames that are not all the same size")
	{
		const std::string big = "<sprite name=\"b\"><bitmap><row>.**</row><row>*..</row><scale>2</scale></bitmap></sprite>";
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + big, kTwoFrames), shows)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("frame 2 (\"b\")") && ContainsSubstring("6 by 4") && ContainsSubstring("4 by 4"));
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
			"<animation><seconds>1</seconds><frame sprite=\"a\" /><frame sprite=\"c\" /></animation>"), shows) };
		CHECK(loaded.game.object("thing").animation.frames.shows().bitmaps.size() == 2);
	}

	SECTION("frames of different shapes that would be turned into different squares")
	{
		const std::string wide = "<sprite name=\"w\"><bitmap><row>****</row><row>****</row><scale>2</scale></bitmap></sprite>";
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(kA + wide,
			"<animation><seconds>1</seconds><frame sprite=\"a\" /><frame sprite=\"w\" /></animation>", "<heading>0</heading>"), shows)),
			ContainsSubstring("frame 2") && ContainsSubstring("same size"));
	}

	SECTION("a member that adds a sprite the group's animation never shows")
	{
		const std::string group =
			"<group name=\"pair\">" + kA + kB + kTwoFrames +
			"<position><y>50</y></position><velocity><x>0</x><y>0</y></velocity>"
			"<collisions><off /></collisions>"
			"<member name=\"odd\"><sprite name=\"only\"><bitmap><row>*</row></bitmap></sprite><position><x>10</x></position></member>"
			"</group>";
		CHECK_THROWS_WITH(Loaded(gameXml(group, "<show object=\"pair\" />")),
			ContainsSubstring("object 'odd'") && ContainsSubstring("<sprite name=\"only\"> is never shown"));
	}
}

TEST_CASE("an animated object that turns keeps its picture and its heading together", "[animation][heading][xml]")
{
	Loaded loaded{ gameXml(objectWith(kA + kB, kTwoFrames, "<heading>0</heading>"), "<show object=\"thing\" />") };
	Game& game = loaded.game;
	game.setCurrentState("playing");
	Object& thing = game.object("thing");

	// What each picture turns from, one for each, all turning in one size.
	REQUIRE(thing.drawn.turnables.size() == 2);
	CHECK(thing.drawn.turnables[0]->at(0.0f).width == thing.drawn.turnables[1]->at(0.0f).width);
	CHECK(thing.drawn.turnables[0]->at(0.0f).rgba != thing.drawn.turnables[1]->at(0.0f).rgba);
	CHECK(thing.drawn.bitmap->rgba == thing.drawn.turnables[0]->at(0.0f).rgba);

	// Turning keeps the picture that is showing.
	thing.heading = 90.0f;
	thing.showHeading();
	CHECK(thing.drawn.bitmap->rgba == thing.drawn.turnables[0]->at(90.0f).rgba);

	// The next picture comes up at the heading it is facing.
	frames(game, 30);
	CHECK(thing.drawn.bitmap->rgba == thing.drawn.turnables[1]->at(90.0f).rgba);

	thing.heading = 181.0f;
	thing.showHeading();
	CHECK(thing.drawn.bitmap->rgba == thing.drawn.turnables[1]->at(181.0f).rgba);

	frames(game, 30);
	CHECK(thing.drawn.bitmap->rgba == thing.drawn.turnables[0]->at(181.0f).rgba);

	// A reset faces it as it started, and starts the animation again.
	game.resetAll();
	CHECK(thing.heading == 0.0f);
	CHECK(thing.drawn.bitmap->rgba == thing.drawn.turnables[0]->at(0.0f).rgba);
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
		Game game{ "descriptions/spaceinvaders.xml" };
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
	for (const char* name : { "aliens.1.1", "aliens.11.1", "aliens.1.2", "aliens.11.3", "aliens.1.4", "aliens.11.5" })
	{
		INFO(name);
		CHECK(game.find(name) != nullptr);
	}
	CHECK(game.find("aliens.12.1") == nullptr);
	CHECK(game.find("aliens.1.6") == nullptr);

	// Every alien is a picture of 11 by 8 characters at 5 pixels each, two
	// pictures that take turns, four steps of half a second (a, b, a, b).
	for (const Object* alien : aliensOf(game))
	{
		REQUIRE(alien->drawn.bitmap);
		CHECK(alien->drawn.bitmap->width == 55);
		CHECK(alien->drawn.bitmap->height == 40);
		CHECK(alien->animation.frames.shows().bitmaps.size() == 4);
		CHECK(alien->animation.frames.shows().seconds == 0.5f);
	}

	// The three kinds are laid out one under the other, a row of gap between.
	CHECK(game.object("aliens.1.1").position.y == 120.0f);
	CHECK(game.object("aliens.1.2").position.y == 175.0f);
	CHECK(game.object("aliens.1.3").position.y == 230.0f);
	CHECK(game.object("aliens.1.4").position.y == 285.0f);
	CHECK(game.object("aliens.1.5").position.y == 340.0f);
	CHECK(game.object("aliens.1.1").position.x == 80.0f);
	CHECK(game.object("aliens.2.1").position.x == 150.0f);

	// All of them are one block: one lockstep number.
	std::set<int> lockstep;
	for (const Object* alien : aliensOf(game)) { lockstep.insert(alien->kind->collisionData.lockstep); }
	CHECK(lockstep.size() == 1);
	CHECK(*lockstep.begin() > 0);

	// The ship is a picture, in the middle of the screen.
	const Object& player = game.object("player");
	REQUIRE(player.drawn.bitmap);
	CHECK(player.drawn.bitmap->width == 55);
	CHECK(player.drawn.bitmap->height == 35);
	CHECK(player.animation.frames.shows().bitmaps.empty());
	CHECK(player.position.x == (1024.0f - 55.0f) / 2.0f);

	// The bullet is a thin tall rectangle.
	const Object& bullet = game.object("bullet");
	CHECK(bullet.drawn.picture->kind == ShapeKind::Rectangle);
	const Vector2f size = measureShapeSize(bullet.drawn.picture);
	CHECK(size.x == 4.0f);
	CHECK(size.y == 24.0f);
}

TEST_CASE("the aliens change picture every half second, all together, and march on meanwhile", "[spaceinvaders][animation]")
{
	Invasion invasion;
	Game& game = invasion.game;

	const auto onPicture = [&](std::size_t index)
	{
		for (const Object* alien : aliensOf(game))
		{
			if (alien->drawn.bitmap != alien->animation.frames.shows().bitmaps[index]) { return false; }
		}
		return true;
	};

	REQUIRE(onPicture(0));

	frames(game, 29);
	CHECK(onPicture(0));

	frames(game, 1);
	CHECK(onPicture(1));
	CHECK_FALSE(onPicture(0));

	// Held still for 29 frames, then one step of half a second's worth at
	// 30 pixels a second (<stepped />).
	CHECK(game.object("aliens.1.1").position.x == Catch::Approx(80.0f + 30.0f * 0.5f));

	// Four steps, a note each: a, b, a, b.
	frames(game, 30);
	CHECK(onPicture(2));
	CHECK(game.object("aliens.1.1").drawn.bitmap == game.object("aliens.1.1").animation.frames.shows().bitmaps[0]);
}

TEST_CASE("the march keeps step with the aliens: a note a step, quicker as they fall", "[spaceinvaders][animation][sound]")
{
	Invasion invasion;
	Game& game = invasion.game;
	game.object("player").collisionsOn = false; // no bomb holds the march still
	(void)game.sounds().takeRequests();

	// The frames between march notes over the next `count` frames.
	const auto gaps = [&](int count)
	{
		std::vector<int> between;
		int since = 0;
		bool heard = false;
		for (int i = 0; i < count; ++i)
		{
			frames(game, 1);
			++since;
			const std::vector<std::string> asked = game.sounds().takeRequests();
			if (std::any_of(asked.begin(), asked.end(), [](const std::string& sound) { return sound.rfind("march", 0) == 0; }))
			{
				if (heard) { between.push_back(since); }
				heard = true;
				since = 0;
			}
		}
		return between;
	};

	const std::vector<int> start = gaps(150);
	REQUIRE(start.size() >= 3);
	CHECK(std::all_of(start.begin(), start.end(), [](int gap) { return gap == 30; }));

	// Down to 28 aliens: the fleet goes 1.3 times as fast, and so does the march.
	int gone = 0;
	for (const Object* alien : aliensOf(game))
	{
		if (gone == 27) { break; }
		const_cast<Object*>(alien)->isVisible = false;
		++gone;
	}
	frames(game, 40);
	(void)game.sounds().takeRequests(); // the notes of those 40
	const std::vector<int> quicker = gaps(150);
	REQUIRE(quicker.size() >= 3);
	CHECK(std::all_of(quicker.begin(), quicker.end(), [](int gap) { return gap == 23; }));
}

TEST_CASE("the aliens' two pictures differ, so there is something to see", "[spaceinvaders][animation]")
{
	Invasion invasion;
	const Object& alien = invasion.game.object("aliens.4.2");

	REQUIRE(alien.animation.frames.shows().bitmaps.size() == 4);
	CHECK(alien.animation.frames.shows().bitmaps[0]->rgba != alien.animation.frames.shows().bitmaps[1]->rgba);
	CHECK(alien.animation.frames.shows().bitmaps[0]->width == alien.animation.frames.shows().bitmaps[1]->width);
	CHECK(alien.animation.frames.shows().bitmaps[0]->height == alien.animation.frames.shows().bitmaps[1]->height);
}

TEST_CASE("a shot from the ship in the middle kills exactly one alien and is taken away", "[spaceinvaders]")
{
	Invasion invasion;
	Game& game = invasion.game;

	REQUIRE(invasion.alive() == 55);

	invasion.executor.executeInput(Command{ CmdTriggerAction{ "player", "gun" } }, true);
	game.settleSpawns();
	REQUIRE(copies::count(game, "bullet") == 1);

	// A thin bullet leaves the middle of the cannon.
	const Object& cannon = game.object("player");
	Object& shot = game.object("bullet.1");
	shot.size = measureShapeSize(shot.drawn.picture);
	CHECK(shot.position.x + 2.0f == Catch::Approx(cannon.position.x + 27.5f));

	frames(game, 80);

	CHECK(invasion.alive() == 54);
	CHECK(copies::count(game, "bullet") == 0);
}

TEST_CASE("the aliens still win the game for the player when they are all gone, and a reset brings them back", "[spaceinvaders]")
{
	Invasion invasion;
	Game& game = invasion.game;

	for (const Object* alien : aliensOf(game)) { const_cast<Object*>(alien)->isVisible = false; }
	REQUIRE(invasion.alive() == 0);

	frames(game, 1);
	CHECK(game.getCurrentState().name == "over");
	CHECK(game.object("over.message").lookName() == "won");

	game.resetAll();
	CHECK(invasion.alive() == 55);
}

TEST_CASE("each state of Space Invaders has its own objects, and paused and the endings draw the invasion still", "[spaceinvaders][scenes][shows]")
{
	Invasion invasion;
	Game& game = invasion.game;
	CHECK(game.object("aliens.1.1").name == "playing.aliens.1.1");
	CHECK(game.object("lives").name == "playing.lives");
	CHECK(game.object("player").name == "player"); // the game's own: the title and playing show it
	CHECK(game.isShown(game.object("player")));
	CHECK(game.isShown(game.object("aliens.1.1")));
	CHECK_FALSE(game.isShown(game.object("title")));

	game.setCurrentState("title");
	CHECK(game.isShown(game.object("title")));
	CHECK(game.isShown(game.object("player")));
	CHECK_FALSE(game.isShown(game.object("aliens.1.1")));
	CHECK_FALSE(game.isStill(game.object("aliens.1.1")));

	// Paused: the block stands where it was, drawn under the word, ship and all.
	game.setCurrentState("playing");
	frames(game, 5);
	game.pushState("paused");
	const Vector2f at = game.object("aliens.1.1").position;
	frames(game, 30);
	CHECK(game.object("aliens.1.1").position.x == at.x);
	CHECK(game.isStill(game.object("aliens.1.1")));
	CHECK(game.isStill(game.object("player")));
	CHECK(game.isStill(game.object("lives")));
	CHECK(game.isShown(game.object("paused")));
	game.popState();
	frames(game, 30);
	CHECK(game.object("aliens.1.1").position.x != at.x);

	game.pushState("over");
	CHECK(game.isStill(game.object("aliens.1.1")));
	CHECK(game.isShown(game.object("over.message")));
	CHECK(game.isShown(game.object("continue")));
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
		const ScratchFile scratch{ scratchName("descriptions/bitmaps_scratch.xml") };
		{
			std::ofstream out(scratch.path);
			out << xml;
		}

		Verdict verdict{};

		auto weak = XmlDocumentFactory::create(XmlBackend::TinyXml2);
		REQUIRE(weak->load(scratch.path.string()));
		XsdLiteValidator validator;
		REQUIRE(validator.loadSchema(findSchema("descriptions/pong.xml", "../xgedef.xsd").string(), XmlBackend::TinyXml2));
		verdict.weakAccepts = validator.validate(*weak->getRootElement());
		verdict.weakMessage = validator.getErrorMessage();

		auto strong = XmlDocumentFactory::create(XmlBackend::Xerces);
		verdict.strongAccepts = strong->load(scratch.path.string());

		return verdict;
	}
}

TEST_CASE("both schema checkers accept Space Invaders", "[spaceinvaders][schema]")
{
	const Verdict verdict = judge(readFile("descriptions/spaceinvaders.xml"));

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
		{ "an animation with no interval", "<seconds update=\"live\">pace.beat</seconds>", "", "seconds" },
		{ "an animation whose frames are not frames", "<frame sprite=\"a\" />", "<picture sprite=\"a\" />", "picture" },
		{ "a bitmap with its scale before its rows", "<bitmap name=\"player\">\n      <row>.....*.....</row>", "<bitmap name=\"player\">\n      <scale>pixel</scale>\n      <row>.....*.....</row>", "scale" },
		{ "a bitmap row with something inside it", "<row>.....*.....</row>", "<row><x>1</x></row>", "x" },
		{ "a frame with no sprite", "<frame sprite=\"a\" />", "<frame />", "sprite" },
	};

	for (const Mistake& mistake : mistakes)
	{
		DYNAMIC_SECTION(mistake.what)
		{
			const Verdict verdict = judge(gameWith("descriptions/spaceinvaders.xml", mistake.from, mistake.to));

			CHECK_FALSE(verdict.weakAccepts);
			CHECK_THAT(verdict.weakMessage, ContainsSubstring(mistake.weakSays));
			CHECK_FALSE(verdict.strongAccepts);
		}
	}
}

TEST_CASE("an attribute the schema does not declare is turned away by both validators", "[animation][schema]")
{
	const Verdict verdict = judge(gameWith("descriptions/spaceinvaders.xml", "<sprite name=\"a\">", "<sprite title=\"a\">"));

	CHECK_FALSE(verdict.weakAccepts);
	CHECK_THAT(verdict.weakMessage, ContainsSubstring("is not declared for"));
	CHECK_FALSE(verdict.strongAccepts);
}

TEST_CASE("the aliens drop bombs from a pool of three, and three hits on the cannon end the game", "[spaceinvaders][facing]")
{
	Game game{ "descriptions/spaceinvaders.xml" };
	for (auto& object : game.getCurrentObjects())
	{
		object.size = measureShapeSize(object.drawn.picture);
	}
	game.setCurrentState(0);
	game.pushState("playing");

	const auto falling = [&]
	{
		int n = 0;
		for (const auto& object : game.getCurrentObjects()) { if (object.objClass == "bomb" && object.isVisible) { ++n; } }
		return n;
	};

	// The cannon out of the way of the bombs while they are counted.
	game.object("player").collisionsOn = false;
	int most = 0;
	for (int frame = 0; frame < 600; ++frame)
	{
		game.updateObjects(xge::kSixtieth);
		most = std::max(most, falling());
	}
	CHECK(most >= 1);
	CHECK(most <= 3);

	// Every bomb falls straight down from below an alien, and none kills one.
	for (const auto& object : game.getCurrentObjects())
	{
		if (object.objClass == "bomb" && object.isVisible) { CHECK(object.velocity.y > 0); }
	}
	int aliens = 0;
	for (const auto& object : game.getCurrentObjects()) { if (object.objClass == "aliens" && object.isVisible) { ++aliens; } }
	CHECK(aliens == 55);

	// Bombs put on the cannon (a stray one may find it first) until the game is
	// lost. Each hit holds the invasion still for a second ("blasted").
	Object& player = game.object("player");
	player.collisionsOn = true;
	bool blasted = false;
	for (int frame = 0; frame < 20000 && game.getCurrentState().name != "over"; ++frame)
	{
		game.updateObjects(xge::kSixtieth);
		if (game.getCurrentState().name == "blasted") { blasted = true; continue; }
		for (auto& object : game.getCurrentObjects())
		{
			if (object.objClass == "bomb" && object.isVisible && object.position.y < player.position.y - 40)
			{
				object.position = { player.position.x + player.size.x / 2, player.position.y - object.size.y - 2 };
				break;
			}
		}
	}
	CHECK(blasted);
	CHECK(player.variable.at("lives") <= 0);
	CHECK(game.getCurrentState().name == "over");
	CHECK(game.object("over.message").lookName() == "lost");
}

TEST_CASE("each kind of alien scores its own, and the block marches faster as it thins", "[spaceinvaders]")
{
	Invasion invasion;
	Game& game = invasion.game;

	CHECK(game.object("aliens.1.1").variable.at("worth") == 30.0f);
	CHECK(game.object("aliens.1.3").variable.at("worth") == 20.0f);
	CHECK(game.object("aliens.1.5").variable.at("worth") == 10.0f);

	// A shot under a crab of the third row, the rows below gone.
	for (const Object* alien : aliensOf(game))
	{
		if (alien->name.str().find("aliens.6.") == std::string::npos) { continue; }
		if (alien->name.str().back() == '4' || alien->name.str().back() == '5') { const_cast<Object*>(alien)->isVisible = false; }
	}
	const Object& crab = game.object("aliens.6.3");
	Object& bullet = copies::spawned(game, "bullet");
	bullet.size = measureShapeSize(bullet.drawn.picture);
	bullet.position = { crab.position.x + crab.size.x / 2.0f, crab.position.y + crab.size.y + 2.0f };
	bullet.velocity = { 0.0f, -720.0f };
	frames(game, 1);
	CHECK_FALSE(crab.isVisible);
	CHECK(game.object("player").variable.at("score") == 20.0f);

	// 28 left: half as fast again, and the next speed-up at 14.
	int gone = 55 - invasion.alive();
	for (const Object* alien : aliensOf(game))
	{
		if (gone >= 27) { break; }
		if (alien->isVisible) { const_cast<Object*>(alien)->isVisible = false; ++gone; }
	}
	REQUIRE(invasion.alive() == 28);
	const float before = std::abs(game.object("aliens.11.1").velocity.x);
	frames(game, 1);
	CHECK(std::abs(game.object("aliens.11.1").velocity.x) == Catch::Approx(before * 1.3f));
	CHECK(game.object("pace").variable.at("next") == 14.0f);
	frames(game, 1);
	CHECK(std::abs(game.object("aliens.11.1").velocity.x) == Catch::Approx(before * 1.3f));
}

TEST_CASE("the mystery ship crosses the top now and then and scores 50 to 200", "[spaceinvaders]")
{
	Invasion invasion;
	Game& game = invasion.game;
	const Object& ufo = game.object("ufo");
	REQUIRE_FALSE(ufo.isVisible);
	game.object("player").collisionsOn = false; // no bombs on it meanwhile

	for (int frame = 0; frame < 60 * 31 && !ufo.isVisible; ++frame) { frames(game, 1); }
	REQUIRE(ufo.isVisible);
	CHECK(ufo.velocity.x > 0.0f);
	frames(game, 60);
	REQUIRE(ufo.isVisible);

	Object& player = game.object("player");
	Object& bullet = copies::spawned(game, "bullet");
	bullet.size = measureShapeSize(bullet.drawn.picture);
	bullet.position = { ufo.position.x + ufo.size.x / 2.0f, ufo.position.y + ufo.size.y + 2.0f };
	bullet.velocity = { 0.0f, -720.0f };
	const float score = player.variable.at("score");
	frames(game, 1);
	CHECK_FALSE(ufo.isVisible);
	const float scored = player.variable.at("score") - score;
	CHECK(scored >= 50.0f);
	CHECK(scored <= 200.0f);
	CHECK(std::fmod(scored, 50.0f) == 0.0f);
}

TEST_CASE("the invasion landing on the shields ends the game", "[spaceinvaders]")
{
	Invasion invasion;
	Game& game = invasion.game;
	for (const Object* alien : aliensOf(game)) { const_cast<Object*>(alien)->position.y += 300.0f; }
	frames(game, 1);
	CHECK(game.getCurrentState().name == "over");
	CHECK(game.object("over.message").lookName() == "lost");
}
