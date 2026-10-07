// test_svg_sprites.cpp
// XML Game Engine
// author: beefviper
// date: Oct 2, 2026
//
// Catch2 tests for pictures drawn from an SVG file (<svg>): the rasterizer on
// its own (rasterizeSvg: the part of the drawing taken, the scale, the
// elements left out, antialiased transparency, the mistakes), an <svg> sprite
// in a game (its size, a group's cells of it, animations of it, mistakes reported where
// they are), and then Space Invaders 2, which is drawn from a sheet of SVG
// sprites, with both schema checkers on it.
//
// Small games and SVG files are written to scratch files and loaded by a real
// xge::Game; none of it needs a window.

#include "bitmap.h"
#include "command_executor.h"
#include "game.h"
#include "svg.h"
#include "xml_document.h"
#include "xsd_lite.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace xge;
using Catch::Matchers::ContainsSubstring;

namespace
{
	// Removes a scratch file when it goes out of scope, pass or fail.
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

	ScratchFile writeScratch(const std::string& name, const std::string& text)
	{
		ScratchFile scratch{ std::filesystem::temp_directory_path() / name };
		std::ofstream out(scratch.path, std::ios::binary);
		out << text;
		return scratch;
	}

	// An 8 by 4 drawing: a blue backdrop (id backdrop), a red square on the
	// left half, a green square on the right (drawn through a <use>, as the
	// ship of the real sprite sheet is), and one half transparent red pixel.
	//
	//   x:  0 1 2 3 | 4 5 6 7
	//   y0: R R R R | b b b b      R red, G green, h half red, b backdrop only
	//   y1: R R R R | b G G b
	//   y2: R R R R | b G G b
	//   y3: R R R R | h b b b
	const std::string kDrawing =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"8\" height=\"4\" viewBox=\"0 0 8 4\">"
		"<defs><g id=\"dot\"><rect x=\"5\" y=\"1\" width=\"2\" height=\"2\" fill=\"#00ff00\"/></g></defs>"
		"<rect id=\"backdrop\" width=\"8\" height=\"4\" fill=\"#0000ff\"/>"
		"<rect x=\"0\" y=\"0\" width=\"4\" height=\"4\" fill=\"#ff0000\"/>"
		"<use href=\"#dot\"/>"
		"<rect x=\"4\" y=\"3\" width=\"1\" height=\"1\" fill=\"#ff0000\" fill-opacity=\"0.5\"/>"
		"</svg>";

	struct Pixel
	{
		int r, g, b, a;
	};

	Pixel pixelAt(const Bitmap& bitmap, int x, int y)
	{
		REQUIRE(x >= 0);
		REQUIRE(y >= 0);
		REQUIRE(x < bitmap.width);
		REQUIRE(y < bitmap.height);
		const std::size_t at = (static_cast<std::size_t>(y) * static_cast<std::size_t>(bitmap.width) + static_cast<std::size_t>(x)) * 4;
		return { bitmap.rgba[at], bitmap.rgba[at + 1], bitmap.rgba[at + 2], bitmap.rgba[at + 3] };
	}

	bool isColor(const Pixel& pixel, int r, int g, int b)
	{
		return pixel.a == 255 && pixel.r == r && pixel.g == g && pixel.b == b;
	}

	// The scratch file's path as XML text and as the string the engine is given.
	std::string pathOf(const ScratchFile& file)
	{
		return file.path.generic_string();
	}

	// An object nothing but a plain rectangle, for the second state to show.
	const std::string kOther =
		"<object name=\"other\"><sprite><rectangle><width>5</width><height>5</height></rectangle></sprite>"
		"<position><x>0</x><y>0</y></position><velocity><x>0</x><y>0</y></velocity>"
		"<collisions><enabled>false</enabled></collisions></object>";

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
			file(writeScratch("xge_test_svg_game.xml", xml)),
			game(file.path.string())
		{
		}
	};

	std::string objectWith(const std::string& sprites, const std::string& animation = "")
	{
		return "<object name=\"thing\">" + sprites + animation +
			"<position><x>10</x><y>20</y></position><velocity><x>0</x><y>0</y></velocity>"
			"<collisions><enabled>false</enabled></collisions></object>";
	}

	// An <svg> sprite's tags, with whatever is wanted after the path.
	std::string svgTag(const ScratchFile& drawing, const std::string& rest)
	{
		return "<svg><path>" + pathOf(drawing) + "</path>" + rest + "</svg>";
	}

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

TEST_CASE("a whole drawing is drawn at its own size, with its own colors", "[svg]")
{
	const ScratchFile drawing = writeScratch("xge_test_drawing_whole.svg", kDrawing);
	const Bitmap bitmap = rasterizeSvg(pathOf(drawing), SvgRegion{}, 1.0f);

	REQUIRE(bitmap.width == 8);
	REQUIRE(bitmap.height == 4);
	REQUIRE(bitmap.rgba.size() == 8u * 4u * 4u);

	CHECK(isColor(pixelAt(bitmap, 1, 1), 255, 0, 0));
	CHECK(isColor(pixelAt(bitmap, 5, 1), 0, 255, 0));   // through the <use>
	CHECK(isColor(pixelAt(bitmap, 7, 0), 0, 0, 255));   // the backdrop
}

TEST_CASE("an element can be left out by its id, and what it covered is transparent", "[svg]")
{
	const ScratchFile drawing = writeScratch("xge_test_drawing_hide.svg", kDrawing);
	const Bitmap bitmap = rasterizeSvg(pathOf(drawing), SvgRegion{}, 1.0f, { "backdrop" });

	CHECK_FALSE(bitmap.solidAt(7, 0));
	CHECK_FALSE(bitmap.solidAt(4, 0));
	CHECK(isColor(pixelAt(bitmap, 1, 1), 255, 0, 0));
	CHECK(isColor(pixelAt(bitmap, 5, 2), 0, 255, 0));

	SECTION("an id nothing has changes nothing")
	{
		const Bitmap same = rasterizeSvg(pathOf(drawing), SvgRegion{}, 1.0f, { "nothing.here" });
		CHECK(same.rgba == rasterizeSvg(pathOf(drawing), SvgRegion{}, 1.0f).rgba);
	}
}

TEST_CASE("transparency keeps its color and is not premultiplied", "[svg]")
{
	const ScratchFile drawing = writeScratch("xge_test_drawing_alpha.svg", kDrawing);
	const Bitmap bitmap = rasterizeSvg(pathOf(drawing), SvgRegion{}, 1.0f, { "backdrop" });

	const Pixel half = pixelAt(bitmap, 4, 3);
	CHECK(half.a >= 120);
	CHECK(half.a <= 135);
	CHECK(half.r >= 250);
	CHECK(half.g <= 5);
	CHECK(half.b <= 5);
}

TEST_CASE("the scale is how many pixels a unit of the drawing is, and need not be whole", "[svg]")
{
	const ScratchFile drawing = writeScratch("xge_test_drawing_scale.svg", kDrawing);

	const Bitmap doubled = rasterizeSvg(pathOf(drawing), SvgRegion{}, 2.0f, { "backdrop" });
	CHECK(doubled.width == 16);
	CHECK(doubled.height == 8);
	CHECK(isColor(pixelAt(doubled, 3, 3), 255, 0, 0));
	CHECK(isColor(pixelAt(doubled, 11, 5), 0, 255, 0));
	CHECK_FALSE(doubled.solidAt(15, 0));

	// 8 by 4 units at 1.5 is 12 by 6 pixels; a part that does not come out
	// whole is rounded up to whole pixels.
	const Bitmap fraction = rasterizeSvg(pathOf(drawing), SvgRegion{}, 1.5f);
	CHECK(fraction.width == 12);
	CHECK(fraction.height == 6);

	const Bitmap rounded = rasterizeSvg(pathOf(drawing), SvgRegion{ 0.0f, 0.0f, 3.0f, 3.0f }, 1.5f);
	CHECK(rounded.width == 5);
	CHECK(rounded.height == 5);
}

TEST_CASE("a part of the drawing is moved to the top left", "[svg]")
{
	const ScratchFile drawing = writeScratch("xge_test_drawing_region.svg", kDrawing);

	// The right half, three pixels to a unit: the green square is one unit in
	// and one unit down.
	const Bitmap right = rasterizeSvg(pathOf(drawing), SvgRegion{ 4.0f, 0.0f, 4.0f, 4.0f }, 3.0f, { "backdrop" });
	REQUIRE(right.width == 12);
	REQUIRE(right.height == 12);
	CHECK(isColor(pixelAt(right, 6, 6), 0, 255, 0));
	CHECK(isColor(pixelAt(right, 3, 3), 0, 255, 0));
	CHECK_FALSE(right.solidAt(0, 0));
	CHECK_FALSE(right.solidAt(2, 4));
	CHECK_FALSE(right.solidAt(10, 6));

	// The left half is all red and nothing else.
	const Bitmap left = rasterizeSvg(pathOf(drawing), SvgRegion{ 0.0f, 0.0f, 4.0f, 4.0f }, 1.0f, { "backdrop" });
	REQUIRE(left.width == 4);
	for (int y = 0; y < 4; ++y)
	{
		for (int x = 0; x < 4; ++x) { CHECK(isColor(pixelAt(left, x, y), 255, 0, 0)); }
	}

	SECTION("a part that hangs over the edge is drawn as far as the drawing goes")
	{
		const Bitmap over = rasterizeSvg(pathOf(drawing), SvgRegion{ 6.0f, 0.0f, 4.0f, 4.0f }, 1.0f, { "backdrop" });
		CHECK(over.width == 4);
		CHECK(isColor(pixelAt(over, 0, 1), 0, 255, 0));
		CHECK_FALSE(over.solidAt(3, 1));
	}
}

TEST_CASE("mistakes in an svg are reported, naming the file", "[svg]")
{
	const ScratchFile drawing = writeScratch("xge_test_drawing_mistakes.svg", kDrawing);
	const std::string path = pathOf(drawing);

	SECTION("a file that is not there")
	{
		CHECK_THROWS_WITH(rasterizeSvg("no_such_folder/nothing.svg", SvgRegion{}, 1.0f),
			ContainsSubstring("nothing.svg"));
		CHECK_THROWS_AS(rasterizeSvg("no_such_folder/nothing.svg", SvgRegion{}, 1.0f), std::runtime_error);
	}

	SECTION("a file that is not an svg")
	{
		const ScratchFile notSvg = writeScratch("xge_test_not_an_svg.svg", "this is not a drawing at all");
		CHECK_THROWS_WITH(rasterizeSvg(pathOf(notSvg), SvgRegion{}, 1.0f), ContainsSubstring("xge_test_not_an_svg.svg"));
	}

	SECTION("a scale that is not above 0")
	{
		CHECK_THROWS_WITH(rasterizeSvg(path, SvgRegion{}, 0.0f), ContainsSubstring("scale"));
		CHECK_THROWS_AS(rasterizeSvg(path, SvgRegion{}, -1.0f), std::invalid_argument);
	}

	SECTION("a part with a start but no size")
	{
		CHECK_THROWS_WITH(rasterizeSvg(path, SvgRegion{ 2.0f, 0.0f, 0.0f, 0.0f }, 1.0f), ContainsSubstring("width and a height"));
	}

	SECTION("a part outside the drawing")
	{
		CHECK_THROWS_WITH(rasterizeSvg(path, SvgRegion{ 20.0f, 0.0f, 4.0f, 4.0f }, 1.0f), ContainsSubstring("outside the drawing"));
		CHECK_THROWS_WITH(rasterizeSvg(path, SvgRegion{ -9.0f, 0.0f, 4.0f, 4.0f }, 1.0f), ContainsSubstring("outside the drawing"));
	}

	SECTION("something that is not an id to hide")
	{
		CHECK_THROWS_WITH(rasterizeSvg(path, SvgRegion{}, 1.0f, { "a}b" }), ContainsSubstring("not an element id"));
		CHECK_THROWS_WITH(rasterizeSvg(path, SvgRegion{}, 1.0f, { "" }), ContainsSubstring("not an element id"));
	}
}

// ---------------------------------------------------------------- in a game

TEST_CASE("an svg sprite is drawn when the game loads and has the size of its picture", "[svg][xml]")
{
	const ScratchFile drawing = writeScratch("xge_test_game_drawing.svg", kDrawing);
	Loaded loaded{ gameXml(objectWith(
		"<sprite>" + svgTag(drawing, "<x>4</x><y>0</y><width>4</width><height>4</height><scale>pixel</scale><hide>backdrop</hide>") + "</sprite>"),
		"<show object=\"thing\" />") };
	const Object& thing = loaded.game.getObject("thing");

	REQUIRE(thing.shapeKind == ShapeKind::Line);
	REQUIRE(thing.bitmap);

	// 4 units at 3 pixels each (the variable `pixel`).
	CHECK(thing.bitmap->width == 12);
	CHECK(thing.bitmap->height == 12);
	CHECK(thing.spriteParams == std::vector<std::string>{ "line", "12", "12" });

	const Vector2f size = measureShapeSize(thing.spriteParams, thing.shapeKind);
	CHECK(size.x == 12.0f);
	CHECK(size.y == 12.0f);

	CHECK(isColor(pixelAt(*thing.bitmap, 6, 6), 0, 255, 0));
	CHECK_FALSE(thing.bitmap->solidAt(0, 0));
}

TEST_CASE("an svg sprite with no part and no scale is the whole drawing at its own size", "[svg][xml]")
{
	const ScratchFile drawing = writeScratch("xge_test_game_whole.svg", kDrawing);
	Loaded loaded{ gameXml(objectWith("<sprite>" + svgTag(drawing, "") + "</sprite>"), "<show object=\"thing\" />") };
	const Object& thing = loaded.game.getObject("thing");

	REQUIRE(thing.bitmap);
	CHECK(thing.bitmap->width == 8);
	CHECK(thing.bitmap->height == 4);
	CHECK(isColor(pixelAt(*thing.bitmap, 7, 0), 0, 0, 255));
}

TEST_CASE("an svg object's picture has solid pixels only where something was drawn, for a pixel collision to test", "[svg][xml]")
{
	const ScratchFile drawing = writeScratch("xge_test_game_collision.svg", kDrawing);
	Loaded loaded{ gameXml(objectWith(
		"<sprite>" + svgTag(drawing, "<hide>backdrop</hide>") + "</sprite>"),
		"<show object=\"thing\" />") };
	const Object& thing = loaded.game.getObject("thing");

	REQUIRE(thing.bitmap);
	CHECK(thing.bitmap->solidAt(1, 1));
	CHECK_FALSE(thing.bitmap->solidAt(7, 0));
	CHECK_FALSE(thing.bitmap->solidAt(-1, 0));
}

TEST_CASE("the cells of a group of an svg are one object each, all sharing one picture", "[svg][xml]")
{
	const ScratchFile drawing = writeScratch("xge_test_game_cells.svg", kDrawing);
	Loaded loaded{ gameXml("<group name=\"thing\"><columns>3</columns><rows>2</rows><padding><x>2</x><y>2</y></padding><sprite>"
		+ svgTag(drawing, "<x>0</x><y>0</y><width>4</width><height>4</height><scale>2</scale>") + "</sprite>"
		"<position><x>10</x><y>20</y></position><velocity><x>0</x><y>0</y></velocity>"
		"<collisions><enabled>false</enabled></collisions></group>",
		"<show object=\"thing\" />") };
	Game& game = loaded.game;
	measure(game);

	const Object* first = game.tryGetObject("thing.1.1");
	const Object* last = game.tryGetObject("thing.3.2");
	REQUIRE(first != nullptr);
	REQUIRE(last != nullptr);
	CHECK(game.tryGetObject("thing.4.1") == nullptr);

	REQUIRE(first->bitmap);
	CHECK(first->bitmap->width == 8);
	CHECK(first->bitmap == last->bitmap);

	// 8 pixel cells, 2 apart.
	CHECK(game.getObject("thing.2.1").position.x == 10.0f + 10.0f);
	CHECK(game.getObject("thing.1.2").position.y == 20.0f + 10.0f);
}

TEST_CASE("svg sprites animate like any other pictures, if they are the same size", "[svg][animation][xml]")
{
	const ScratchFile drawing = writeScratch("xge_test_game_animation.svg", kDrawing);
	const std::string left = "<sprite name=\"left\">" + svgTag(drawing, "<x>0</x><y>0</y><width>4</width><height>4</height><hide>backdrop</hide>") + "</sprite>";
	const std::string right = "<sprite name=\"right\">" + svgTag(drawing, "<x>4</x><y>0</y><width>4</width><height>4</height><hide>backdrop</hide>") + "</sprite>";
	const std::string animation = "<animation><interval>0.5</interval><frame sprite=\"left\" /><frame sprite=\"right\" /></animation>";

	Loaded loaded{ gameXml(objectWith(left + right, animation), "<show object=\"thing\" />") };
	Game& game = loaded.game;
	game.setCurrentState("playing");
	const Object& thing = game.getObject("thing");

	REQUIRE(thing.animationBitmaps.size() == 2);
	CHECK(thing.animationFrames == 30);
	CHECK(thing.bitmap == thing.animationBitmaps[0]);
	CHECK(isColor(pixelAt(*thing.bitmap, 0, 0), 255, 0, 0));

	frames(game, 30);
	CHECK(thing.bitmap == thing.animationBitmaps[1]);
	CHECK_FALSE(thing.bitmap->solidAt(0, 0));
	CHECK(isColor(pixelAt(*thing.bitmap, 1, 1), 0, 255, 0));

	frames(game, 30);
	CHECK(thing.bitmap == thing.animationBitmaps[0]);
}

TEST_CASE("svg frames of different sizes are refused", "[svg][animation][xml]")
{
	const ScratchFile drawing = writeScratch("xge_test_game_sizes.svg", kDrawing);
	const std::string small = "<sprite name=\"small\">" + svgTag(drawing, "<x>0</x><y>0</y><width>4</width><height>4</height>") + "</sprite>";
	const std::string large = "<sprite name=\"large\">" + svgTag(drawing, "<x>0</x><y>0</y><width>4</width><height>4</height><scale>2</scale>") + "</sprite>";
	const std::string animation = "<animation><interval>0.5</interval><frame sprite=\"small\" /><frame sprite=\"large\" /></animation>";

	CHECK_THROWS_WITH(Loaded(gameXml(objectWith(small + large, animation), "<show object=\"thing\" />")),
		ContainsSubstring("the frames of an animation must all be the same size"));
}

TEST_CASE("mistakes in an svg sprite are reported where they are", "[svg][xml]")
{
	const ScratchFile drawing = writeScratch("xge_test_game_mistakes.svg", kDrawing);
	const std::string shows = "<show object=\"thing\" />";

	SECTION("a file that is not there")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith("<sprite><svg><path>no_such_folder/nothing.svg</path></svg></sprite>"), shows)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("nothing.svg"));
	}

	SECTION("no path")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith("<sprite><svg><scale>2</scale></svg></sprite>"), shows)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("path"));
	}

	SECTION("only some of x, y, width and height")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith("<sprite>" + svgTag(drawing, "<x>1</x><y>1</y>") + "</sprite>"), shows)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("all four"));
	}

	SECTION("a part with no size")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(
			"<sprite>" + svgTag(drawing, "<x>0</x><y>0</y><width>0</width><height>4</height>") + "</sprite>"), shows)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("above 0"));
	}

	SECTION("a scale that is not above 0")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith("<sprite>" + svgTag(drawing, "<scale>0</scale>") + "</sprite>"), shows)),
			ContainsSubstring("object 'thing'") && ContainsSubstring("scale"));
	}
}

// ------------------------------------------------------- Space Invaders 2

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
		Game game{ "games/spaceinvaders2.xml" };
		CommandExecutor executor{ game };

		Invasion()
		{
			measure(game);
			game.setCurrentState("playing");
		}
	};
}

TEST_CASE("Space Invaders 2 draws the ship, the bolt and three kinds of alien from the sprite sheet", "[spaceinvaders2][svg]")
{
	Invasion invasion;
	Game& game = invasion.game;

	CHECK(aliensOf(game).size() == 55);

	// Each kind is cut to the same width, so the columns line up, and has
	// three frames of one size.
	struct Kind
	{
		const char* name;
		int width;
		int height;
	};
	for (const Kind& kind : { Kind{ "aliens.1.1", 56, 50 }, Kind{ "aliens.1.2", 56, 46 }, Kind{ "aliens.1.4", 56, 49 } })
	{
		INFO(kind.name);
		const Object& alien = game.getObject(kind.name);
		REQUIRE(alien.bitmap);
		REQUIRE(alien.animationBitmaps.size() == 3);
		CHECK(alien.animationFrames == 30);

		for (const auto& picture : alien.animationBitmaps)
		{
			CHECK(picture->width == kind.width);
			CHECK(picture->height == kind.height);
		}

		// The pictures differ, so there is something to see; and each is
		// drawn, not blank.
		CHECK(alien.animationBitmaps[0]->rgba != alien.animationBitmaps[1]->rgba);
		CHECK(alien.animationBitmaps[1]->rgba != alien.animationBitmaps[2]->rgba);
		for (const auto& picture : alien.animationBitmaps)
		{
			int drawn = 0;
			for (std::size_t at = 3; at < picture->rgba.size(); at += 4) { drawn += picture->rgba[at] != 0 ? 1 : 0; }
			CHECK(drawn > picture->width * picture->height / 4);
		}
	}

	// A kind keeps its own colour: pink squids, orange crabs, green octopuses.
	const auto dominant = [&](const char* name)
	{
		const Bitmap& picture = *game.getObject(name).bitmap;
		long red = 0;
		long green = 0;
		long blue = 0;
		for (std::size_t at = 0; at + 3 < picture.rgba.size(); at += 4)
		{
			if (picture.rgba[at + 3] != 255) { continue; }
			red += picture.rgba[at];
			green += picture.rgba[at + 1];
			blue += picture.rgba[at + 2];
		}
		return std::make_tuple(red, green, blue);
	};
	const auto [squidRed, squidGreen, squidBlue] = dominant("aliens.1.1");
	const auto [crabRed, crabGreen, crabBlue] = dominant("aliens.1.2");
	const auto [octopusRed, octopusGreen, octopusBlue] = dominant("aliens.1.4");
	CHECK(squidRed > squidGreen);
	CHECK(crabRed > crabBlue);
	CHECK(octopusGreen > octopusRed);
	CHECK(octopusGreen > octopusBlue);
	(void)squidBlue;
	(void)crabGreen;

	// Laid out one kind under the other, a gap between, and one column pitch
	// (56 and a gap of 15) the same for all of them.
	CHECK(game.getObject("aliens.1.1").position.y == 40.0f);
	CHECK(game.getObject("aliens.1.2").position.y == 105.0f);
	CHECK(game.getObject("aliens.1.3").position.y == 166.0f);
	CHECK(game.getObject("aliens.1.4").position.y == 227.0f);
	CHECK(game.getObject("aliens.1.5").position.y == 291.0f);
	for (const char* name : { "aliens.2.1", "aliens.2.2", "aliens.2.4" })
	{
		INFO(name);
		CHECK(game.getObject(name).position.x == 80.0f + 71.0f);
	}

	// One block: one lockstep number.
	std::set<int> lockstep;
	for (const Object* alien : aliensOf(game)) { lockstep.insert(alien->collisionData.lockstep); }
	CHECK(lockstep.size() == 1);
	CHECK(*lockstep.begin() > 0);

	// The ship is one drawn picture, in the middle of the screen; the bolt is
	// thin and tall.
	const Object& player = game.getObject("player");
	REQUIRE(player.bitmap);
	CHECK(player.bitmap->width == 48);
	CHECK(player.bitmap->height == 52);
	CHECK(player.animationBitmaps.empty());
	CHECK(player.position.x == (1024.0f - 48.0f) / 2.0f);

	const Object& bullet = game.getObject("bullet");
	REQUIRE(bullet.bitmap);
	CHECK(bullet.bitmap->width < bullet.bitmap->height / 3);
}

TEST_CASE("the aliens of Space Invaders 2 step through three pictures, all together, and march on meanwhile", "[spaceinvaders2][animation]")
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

	frames(game, 29);
	CHECK(onPicture(0));
	frames(game, 1);
	CHECK(onPicture(1));

	// Two pixels a frame to the right in the meantime, as before.
	CHECK(game.getObject("aliens.1.1").position.x == 80.0f + 2.0f * 30.0f);

	frames(game, 30);
	CHECK(onPicture(2));
	frames(game, 30);
	CHECK(onPicture(0));
}

TEST_CASE("a shot from the ship of Space Invaders 2 can still kill an alien", "[spaceinvaders2]")
{
	Invasion invasion;
	Game& game = invasion.game;
	const auto alive = [&]()
	{
		const auto aliens = aliensOf(game);
		return static_cast<int>(std::count_if(aliens.begin(), aliens.end(), [](const Object* alien) { return alien->isVisible; }));
	};
	REQUIRE(alive() == 55);

	invasion.executor.executeInput(Command{ CmdTriggerAction{ "player", "gun" } }, true);
	REQUIRE(game.getObject("bullet").isVisible);
	frames(game, 80);

	CHECK(alive() == 54);
	CHECK_FALSE(game.getObject("bullet").isVisible);
}

// ----------------------------------------------------------------- schema

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
		const ScratchFile scratch{ "games/svg_scratch.xml" };
		{
			std::ofstream out(scratch.path);
			out << xml;
		}

		Verdict verdict{};

		auto weak = XmlDocumentFactory::create(XmlBackend::TinyXml2);
		REQUIRE(weak->load(scratch.path.string()));
		XsdLiteValidator validator;
		REQUIRE(validator.loadSchema("xgedef.xsd", XmlBackend::TinyXml2));
		verdict.weakAccepts = validator.validate(*weak->getRootElement());
		verdict.weakMessage = validator.getErrorMessage();

		auto strong = XmlDocumentFactory::create(XmlBackend::Xerces);
		verdict.strongAccepts = strong->load(scratch.path.string());

		return verdict;
	}
}

TEST_CASE("both schema checkers accept Space Invaders 2", "[spaceinvaders2][schema]")
{
	const Verdict verdict = judge(readFile("games/spaceinvaders2.xml"));

	CHECK(verdict.weakAccepts);
	CHECK(verdict.strongAccepts);
}

TEST_CASE("both schema checkers turn away the same mistakes in an svg", "[svg][schema]")
{
	struct Mistake
	{
		const char* what;
		const char* from;
		const char* to;
		const char* weakSays; // part of the weak validator's message
	};

	const Mistake mistakes[] = {
		{ "an svg with no path", "<path>assets/Space Invaders Color Sprites.svg</path>", "", "path" },
		{ "an svg whose scale comes before its part", "<x>4</x>\n          <y>3.5</y>", "<scale>zoom</scale>\n          <x>4</x>\n          <y>3.5</y>", "<x>" },
		{ "an svg with a tag it does not have", "<hide>backdrop</hide>", "<opacity>1</opacity>", "opacity" },
		{ "an svg hiding nothing by name", "<hide>backdrop</hide>", "<hide><x>1</x></hide>", "x" },
	};

	for (const Mistake& mistake : mistakes)
	{
		DYNAMIC_SECTION(mistake.what)
		{
			const Verdict verdict = judge(gameWith("games/spaceinvaders2.xml", mistake.from, mistake.to));

			CHECK_FALSE(verdict.weakAccepts);
			CHECK_THAT(verdict.weakMessage, ContainsSubstring(mistake.weakSays));
			CHECK_FALSE(verdict.strongAccepts);
		}
	}
}
