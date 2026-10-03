// test_pictures.cpp
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026
//
// Catch2 tests for the one path every picture the engine draws itself goes
// down - a sprite of <line>s, a <bitmap> of rows, an <svg> drawing: it is
// read, drawn into a Bitmap, flipped if the sprite says so, and, for an
// object with a <heading>, kept and turned to the heading. Small games are
// written to a scratch file and loaded by a real xge::Game; nothing needs a
// window.

#include "bitmap.h"
#include "game.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

using namespace xge;
using Catch::Matchers::ContainsSubstring;

namespace
{
	struct ScratchFile
	{
		std::filesystem::path path;

		explicit ScratchFile(std::filesystem::path where) : path(std::move(where)) {}
		ScratchFile(const ScratchFile&) = delete;
		ScratchFile& operator=(const ScratchFile&) = delete;
		~ScratchFile()
		{
			std::error_code ignored;
			std::filesystem::remove(path, ignored);
		}
	};

	// A game of one object per sprite given, each called o1, o2, ...; `extra`
	// goes after each one's <velocity> (a <heading>).
	struct Pictures
	{
		ScratchFile file{ std::filesystem::temp_directory_path() / "xge_test_pictures.xml" };
		std::unique_ptr<Game> game;

		Pictures(const std::vector<std::string>& sprites, const std::string& extra = {})
		{
			std::string objects;
			std::string shows;
			for (std::size_t i = 0; i < sprites.size(); ++i)
			{
				const std::string name = "o" + std::to_string(i + 1);
				objects += "<object name=\"" + name + "\"><sprite>" + sprites[i] + "</sprite>"
					"<position><x>0</x><y>0</y></position><velocity><x>0</x><y>0</y></velocity>" + extra
					+ "<collisions><enabled>false</enabled></collisions></object>";
				shows += "<show object=\"" + name + "\" />";
			}

			{
				std::ofstream out(file.path);
				out << "<game><window name=\"pictures\"><width>800</width><height>600</height><background>color.black</background>"
					"<fullscreen>false</fullscreen><framerate>60</framerate></window><variables><variable name=\"unused\">0</variable></variables>"
					"<objects>" + objects + "</objects><states><state name=\"s\"><shows>" + shows + "</shows>"
					"<inputs><input button=\"space\"><pop /></input></inputs></state></states></game>";
			}
			game = std::make_unique<Game>(file.path.string());
		}

		const Object& object(int number) { return game->getObject("o" + std::to_string(number)); }
	};

	// An L, so every flip and turn of it is different.
	const std::string kRows = "<row>*..</row><row>*..</row><row>***</row>";
	const std::string kSvg = "<path>assets/Space Invaders Color Sprites.svg</path><x>11</x><y>37</y><width>10</width><height>23</height><hide>backdrop</hide>";

	bool samePixels(const Bitmap& a, const Bitmap& b)
	{
		return a.width == b.width && a.height == b.height && a.rgba == b.rgba;
	}
}

TEST_CASE("a flip mirrors the pixels and keeps the size", "[pictures][flip]")
{
	const Bitmap l = rasterizeRows({ "*..", "*..", "***" }, 1, Color{ 255, 255, 255, 255 });

	const Bitmap h = flipBitmap(l, true, false);
	CHECK(h.width == 3);
	CHECK(h.height == 3);
	CHECK(h.solidAt(2, 0));
	CHECK_FALSE(h.solidAt(0, 0));
	CHECK(h.solidAt(0, 2));

	const Bitmap v = flipBitmap(l, false, true);
	CHECK(v.solidAt(0, 0));
	CHECK(v.solidAt(2, 0));
	CHECK_FALSE(v.solidAt(2, 2));

	// Flipped twice is the picture again, and no flip changes nothing.
	CHECK(samePixels(flipBitmap(h, true, false), l));
	CHECK(samePixels(flipBitmap(flipBitmap(l, true, true), true, true), l));
	CHECK(samePixels(flipBitmap(l, false, false), l));
}

TEST_CASE("a <bitmap> and an <svg> are drawn, then flipped", "[pictures][flip][xml]")
{
	Pictures pictures({
		"<bitmap>" + kRows + "</bitmap>",
		"<bitmap>" + kRows + "<flip>horizontal</flip></bitmap>",
		"<bitmap>" + kRows + "<scale>2</scale><color>color.red</color><flip>vertical</flip></bitmap>",
		"<svg>" + kSvg + "</svg>",
		"<svg>" + kSvg + "<flip>vertical</flip></svg>",
	});

	const Bitmap& plain = *pictures.object(1).bitmap;
	CHECK(samePixels(*pictures.object(2).bitmap, flipBitmap(plain, true, false)));

	const Bitmap red = rasterizeRows({ "*..", "*..", "***" }, 2, colorFromName("color.red"));
	CHECK(samePixels(*pictures.object(3).bitmap, flipBitmap(red, false, true)));

	const Bitmap& sheet = *pictures.object(4).bitmap;
	REQUIRE(sheet.width > 0);
	CHECK(samePixels(*pictures.object(5).bitmap, flipBitmap(sheet, false, true)));
	CHECK(pictures.object(5).size.x == pictures.object(4).size.x);
}

TEST_CASE("with a heading, every kind of picture is drawn once, flipped, and turned from there", "[pictures][flip][heading][xml]")
{
	const std::string lines = "<line><from><x>0</x><y>0</y></from><to><x>0</x><y>9</y></to><thickness>2</thickness></line>"
		"<line><from><x>0</x><y>9</y></from><to><x>5</x><y>9</y></to><thickness>2</thickness></line>";

	Pictures pictures({
		"<bitmap>" + kRows + "<scale>3</scale><flip>horizontal</flip></bitmap>",
		"<svg>" + kSvg + "<flip>vertical</flip></svg>",
		lines,
	}, "<heading>90</heading>");

	for (int number : { 1, 2 })
	{
		const Object& object = pictures.object(number);
		REQUIRE(object.turnables.size() == 1);
		const Turnable& kept = *object.turnables.front();
		REQUIRE(kept.picture);
		CHECK(kept.lines.empty());

		// What is shown is the kept (already flipped) picture turned to the heading.
		CHECK(samePixels(*object.bitmap, turnBitmap(*kept.picture, 90.0f)));
	}

	// The kept bitmap is the flipped one.
	const Bitmap rows = rasterizeRows({ "*..", "*..", "***" }, 3, colorFromName("color.white"));
	CHECK(samePixels(*pictures.object(1).turnables.front()->picture, flipBitmap(rows, true, false)));

	// Lines are kept as lines, and drawn again at the heading.
	const Object& drawing = pictures.object(3);
	REQUIRE(drawing.turnables.size() == 1);
	CHECK(drawing.turnables.front()->lines.size() == 2);
	CHECK(samePixels(*drawing.bitmap, rasterizeTurned(drawing.turnables.front()->lines, 90.0f)));
}

TEST_CASE("a flip that is not horizontal or vertical is turned away, saying where", "[pictures][flip][errors]")
{
	CHECK_THROWS_WITH(Pictures({ "<bitmap>" + kRows + "<flip>sideways</flip></bitmap>" }),
		ContainsSubstring("object 'o1'") && ContainsSubstring("<flip> is \"sideways\""));
	CHECK_THROWS_WITH(Pictures({ "<svg>" + kSvg + "<flip>diagonal</flip></svg>" }),
		ContainsSubstring("<flip> is \"diagonal\""));
}

TEST_CASE("Space Invaders 2's alien bolts are the sheet's bolt turned upside down, to fly down", "[pictures][flip][spaceinvaders2]")
{
	Game game{ "games/spaceinvaders2.xml" };
	Pictures sheet({ "<svg>" + kSvg + "<scale>1.5</scale></svg>" });

	const Object& bolt = game.getObject("bombs.1");
	CHECK(samePixels(*bolt.bitmap, flipBitmap(*sheet.object(1).bitmap, false, true)));
}
