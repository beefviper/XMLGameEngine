// test_lines_and_pixels.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026
//
// Catch2 tests for the <line> shape and for collisions of type pixel: lines
// drawn into a bitmap when the game loads, a sprite made of them, and a
// collision that sweeps the bounding boxes first and then asks whether pixels
// really touch. Also the other things Lunar Lander needed that are small
// enough to test here: <acceleration>, <accelerate>, <stop /> and the
// slower/faster filters on a collision rule.
//
// Objects for the geometry tests are built by hand; small games are written to
// a scratch file and loaded by a real xge::Game. None of it needs a window.

#include "bitmap.h"
#include "collision_detector.h"
#include "command_executor.h"
#include "game.h"
#include "xml_document.h"
#include "xsd_lite.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace xge;
using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::WithinAbs;

namespace
{
	LineSegment segment(float x1, float y1, float x2, float y2, int thickness = 1)
	{
		LineSegment line;
		line.x1 = x1;
		line.y1 = y1;
		line.x2 = x2;
		line.y2 = y2;
		line.color = colorFromName("color.white");
		line.thickness = thickness;
		return line;
	}

	// An object whose sprite is these lines, placed at (x, y), as game_expr would make it.
	Object lineObject(const std::string& name, const std::vector<LineSegment>& lines, float x, float y, CollisionType type = CollisionType::Pixel)
	{
		Object object;
		object.name = name;
		object.baseName = name;
		object.shapeKind = ShapeKind::Line;
		object.bitmap = std::make_shared<const Bitmap>(rasterizeLines(lines));
		object.size = { static_cast<float>(object.bitmap->width), static_cast<float>(object.bitmap->height) };
		object.position = { x, y };
		object.collisionData.enabled = true;
		object.collisionData.type = type;
		return object;
	}

	Object rectangleObject(const std::string& name, float width, float height, float x, float y)
	{
		Object object;
		object.name = name;
		object.baseName = name;
		object.shapeKind = ShapeKind::Rectangle;
		object.size = { width, height };
		object.position = { x, y };
		object.collisionData.enabled = true;
		return object;
	}

	Object circleObject(const std::string& name, float radius, float x, float y)
	{
		Object object;
		object.name = name;
		object.baseName = name;
		object.shapeKind = ShapeKind::Circle;
		object.size = { radius * 2, radius * 2 };
		object.position = { x, y };
		object.collisionData.enabled = true;
		return object;
	}

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

	std::string gameXml(const std::string& objects, const std::string& shows)
	{
		return "<game>"
			"<window name=\"test\"><width>800</width><height>600</height><background>color.black</background>"
			"<fullscreen>false</fullscreen><framerate>60</framerate></window>"
			"<variables><variable name=\"g\">0.5</variable></variables>"
			"<objects>" + objects + "</objects>"
			"<states><state name=\"playing\"><shows>" + shows + "</shows>"
			"<inputs><input button=\"space\"><pop /></input></inputs></state></states>"
			"</game>";
	}

	struct Loaded
	{
		ScratchFile file;
		Game game;

		explicit Loaded(const std::string& xml) :
			file(writeScratch("xge_test_lines.xml", xml)),
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

	const std::string kHut =
		"<object name=\"hut\">"
		"<sprite>"
		"<line><from><x>0</x><y>10</y></from><to><x>10</x><y>0</y></to><color>color.red</color></line>"
		"<line><from><x>10</x><y>0</y></from><to><x>20</x><y>10</y></to><thickness>2</thickness></line>"
		"</sprite>"
		"<position><x>30</x><y>40</y></position>"
		"<velocity><x>0</x><y>0</y></velocity>"
		"<collisions><enabled>true</enabled><type>pixel</type></collisions>"
		"</object>";

	// One object with a mistake to make: what goes between <sprite> and <position>.
	std::string objectWith(const std::string& sprite, const std::string& collisions = "<enabled>true</enabled>")
	{
		return "<object name=\"thing\"><sprite>" + sprite + "</sprite>"
			"<position><x>0</x><y>0</y></position><velocity><x>0</x><y>0</y></velocity>"
			"<collisions>" + collisions + "</collisions></object>";
	}
}

// ---------------------------------------------------------------- drawing

TEST_CASE("a horizontal line is drawn as a row of pixels with nothing around it", "[lines]")
{
	const Bitmap bitmap = rasterizeLines({ segment(0, 0, 4, 0) });

	CHECK(bitmap.width == 5);
	CHECK(bitmap.height == 1);
	for (int x = 0; x < 5; ++x) { CHECK(bitmap.solidAt(x, 0)); }

	// Outside the bitmap is empty, not an error.
	CHECK_FALSE(bitmap.solidAt(5, 0));
	CHECK_FALSE(bitmap.solidAt(-1, 0));
	CHECK_FALSE(bitmap.solidAt(0, 1));
}

TEST_CASE("a slanted line has no gaps, and either end can come first", "[lines]")
{
	for (const auto& line : { segment(0, 0, 9, 4), segment(9, 4, 0, 0) })
	{
		const Bitmap bitmap = rasterizeLines({ line });

		CHECK(bitmap.width == 10);
		CHECK(bitmap.height == 5);

		// One pixel in every column from end to end, each within a row of the last.
		int previousRow = -1;
		for (int x = 0; x < 10; ++x)
		{
			int row = -1;
			for (int y = 0; y < 5; ++y)
			{
				if (bitmap.solidAt(x, y)) { row = y; break; }
			}
			REQUIRE(row >= 0);
			if (previousRow >= 0) { CHECK(std::abs(row - previousRow) <= 1); }
			previousRow = row;
		}

		CHECK(bitmap.solidAt(0, 0));
		CHECK(bitmap.solidAt(9, 4));
	}
}

TEST_CASE("thickness widens a line to a square brush and adds to the picture's size", "[lines]")
{
	const Bitmap bitmap = rasterizeLines({ segment(2, 2, 6, 2, 3) });

	CHECK(bitmap.width == 9);   // up to x 6, and the brush reaches 2 pixels past it
	CHECK(bitmap.height == 5);  // y 2 and the brush's 3 rows
	CHECK_FALSE(bitmap.solidAt(1, 2));
	for (int y = 2; y < 5; ++y)
	{
		for (int x = 2; x < 9; ++x) { CHECK(bitmap.solidAt(x, y)); }
	}
	CHECK_FALSE(bitmap.solidAt(2, 1));
}

TEST_CASE("a later line covers an earlier one where they meet, and a gap stays transparent", "[lines]")
{
	LineSegment first = segment(0, 0, 5, 0);
	first.color = colorFromName("color.red");
	LineSegment second = segment(3, 0, 8, 0);
	second.color = colorFromName("color.blue");

	const Bitmap bitmap = rasterizeLines({ first, second });
	const auto pixel = [&](int x, int channel) { return bitmap.rgba[static_cast<std::size_t>(x) * 4 + static_cast<std::size_t>(channel)]; };

	CHECK(pixel(1, 0) == 255); // red
	CHECK(pixel(4, 2) == 255); // blue covers red
	CHECK(pixel(4, 0) == 0);
	CHECK(pixel(8, 3) == 255);

	const Bitmap gap = rasterizeLines({ segment(0, 0, 1, 0), segment(6, 0, 7, 0) });
	CHECK(gap.solidAt(1, 0));
	CHECK_FALSE(gap.solidAt(3, 0));
	CHECK(gap.solidAt(6, 0));
}

TEST_CASE("endpoints are rounded to whole pixels, no lines make an empty bitmap, a negative coordinate is refused", "[lines]")
{
	const Bitmap rounded = rasterizeLines({ segment(0.4f, 0.0f, 2.6f, 0.0f) });
	CHECK(rounded.width == 4); // 0 to 3
	CHECK(rounded.solidAt(3, 0));

	const Bitmap none = rasterizeLines({});
	CHECK(none.width == 0);
	CHECK(none.height == 0);
	CHECK(none.rgba.empty());
	CHECK_FALSE(none.solidAt(0, 0));

	CHECK_THROWS_AS(rasterizeLines({ segment(-1, 0, 4, 0) }), std::invalid_argument);
}

TEST_CASE("two one-pixel lines can cross without touching a pixel, two-pixel lines cannot", "[lines]")
{
	// Why thickness matters to a pixel collision: crossing slants can slip
	// between each other's pixels when they are one pixel wide.
	const Object thin1 = lineObject("a", { segment(0, 0, 5, 5) }, 0, 0);
	const Object thin2 = lineObject("b", { segment(0, 5, 5, 0) }, 0, 0);
	CHECK_FALSE(CollisionDetector::pixelsOverlap(thin1, thin2));

	const Object thick1 = lineObject("a", { segment(0, 0, 5, 5, 2) }, 0, 0);
	const Object thick2 = lineObject("b", { segment(0, 5, 5, 0, 2) }, 0, 0);
	CHECK(CollisionDetector::pixelsOverlap(thick1, thick2));
}

// ------------------------------------------------------ pixel collisions

TEST_CASE("boxes that overlap are not a hit when the pixels do not, for a pixel object", "[pixel]")
{
	// An L: a vertical stroke and a floor, the top right of its box empty.
	const std::vector<LineSegment> ell = { segment(0, 0, 0, 9), segment(0, 9, 9, 9) };
	const Object shape = lineObject("ell", ell, 100, 100);
	const Object inCorner = rectangleObject("dot", 3, 3, 106, 101); // in the box, in the empty part
	const Object onStroke = rectangleObject("dot", 3, 3, 98, 104);  // across the vertical stroke

	REQUIRE(CollisionDetector::usesPixels(shape, inCorner));
	CHECK_FALSE(CollisionDetector::overlap(shape, inCorner));
	CHECK_FALSE(CollisionDetector::overlap(inCorner, shape));
	CHECK(CollisionDetector::overlap(shape, onStroke));
	CHECK(CollisionDetector::overlap(onStroke, shape));

	// The same shape as a plain box is hit by both: the box test is the default.
	const Object asBox = lineObject("ell", ell, 100, 100, CollisionType::Box);
	REQUIRE_FALSE(CollisionDetector::usesPixels(asBox, inCorner));
	CHECK(CollisionDetector::overlap(asBox, inCorner));
}

TEST_CASE("a circle is solid as a circle against a pixel object", "[pixel]")
{
	const Object ground = lineObject("ground", { segment(0, 10, 29, 10, 2) }, 100, 100);
	// Radius 5: the line's two rows are at y 110 and 111.
	const Object touching = circleObject("ball", 5, 110, 106);
	const Object above = circleObject("ball", 5, 110, 100);
	const Object pastTheEnd = circleObject("ball", 5, 130, 100); // its box reaches the end of the line, the circle itself does not

	CHECK(CollisionDetector::overlap(ground, touching));
	CHECK_FALSE(CollisionDetector::overlap(ground, above));
	CHECK_FALSE(CollisionDetector::overlap(ground, pastTheEnd));
}

TEST_CASE("a fast mover is stopped where pixels meet, not where the boxes do", "[pixel]")
{
	// A one-pixel post ten high, dropped 100 pixels in one step onto a thin
	// line 50 down, inside a box of its own that starts at y 50.
	const Object post = lineObject("post", { segment(0, 0, 0, 9) }, 10, 0);
	const Object ground = lineObject("ground", { segment(0, 40, 99, 40, 2) }, 0, 50); // the line is at y 90 and 91

	const auto hit = CollisionDetector::sweep(post, { 0, 100 }, ground, { 0, 0 });

	REQUIRE(hit);
	// The post's last pixel row meets the line's first just past y 80.
	CHECK_THAT(hit->time * 100.0f, WithinAbs(80.5f, 0.6f));
	CHECK(hit->edgeOfSecond == Edge::Top);

	// As plain boxes they would have met where the ground's box begins, at y 40.
	const Object postBox = lineObject("post", { segment(0, 0, 0, 9) }, 10, 0, CollisionType::Box);
	const Object groundBox = lineObject("ground", { segment(0, 40, 99, 40, 2) }, 0, 50, CollisionType::Box);
	const auto boxHit = CollisionDetector::sweep(postBox, { 0, 100 }, groundBox, { 0, 0 });
	REQUIRE(boxHit);
	CHECK_THAT(boxHit->time * 100.0f, WithinAbs(40.0f, 0.001f));
}

TEST_CASE("a mover already inside the box finds the pixel touch later in the step", "[pixel]")
{
	const Object post = lineObject("post", { segment(0, 0, 0, 9) }, 10, 60);
	const Object ground = lineObject("ground", { segment(0, 40, 99, 40, 2) }, 0, 50);

	REQUIRE_FALSE(CollisionDetector::overlap(post, ground)); // boxes overlap, pixels do not
	const auto hit = CollisionDetector::sweep(post, { 0, 30 }, ground, { 0, 0 });

	REQUIRE(hit);
	CHECK_THAT(60.0f + hit->time * 30.0f, WithinAbs(80.5f, 0.6f));
}

TEST_CASE("a mover that passes the pixels by is not a hit, and one already touching is a hit at once", "[pixel]")
{
	const Object ground = lineObject("ground", { segment(0, 40, 99, 40, 2) }, 0, 50);

	// Across the whole step the post never reaches the line: it stops short.
	const Object shortPost = lineObject("post", { segment(0, 0, 0, 9) }, 10, 0);
	CHECK_FALSE(CollisionDetector::sweep(shortPost, { 0, 75 }, ground, { 0, 0 }));

	// Nowhere near the box at all.
	const Object farAway = lineObject("post", { segment(0, 0, 0, 9) }, 300, 0);
	CHECK_FALSE(CollisionDetector::sweep(farAway, { 0, 100 }, ground, { 0, 0 }));

	// Already on the line.
	const Object resting = lineObject("post", { segment(0, 0, 0, 9) }, 10, 85);
	const auto hit = CollisionDetector::sweep(resting, { 0, 1 }, ground, { 0, 0 });
	REQUIRE(hit);
	CHECK(hit->time == 0.0f);
}

TEST_CASE("a plain box mover is solid all over against a pixel object's pixels", "[pixel]")
{
	const Object ground = lineObject("ground", { segment(0, 40, 99, 40, 2) }, 0, 50);
	const Object crate = rectangleObject("crate", 10, 10, 20, 0);

	const auto hit = CollisionDetector::sweep(crate, { 0, 100 }, ground, { 0, 0 });

	REQUIRE(hit);
	CHECK_THAT(hit->time * 100.0f, WithinAbs(80.5f, 0.6f)); // its bottom edge meets the line
	CHECK(hit->edgeOfSecond == Edge::Top);

	// Moving sideways into the side of the ground's box: the line's first pixel is at x 0.
	const Object slider = rectangleObject("crate", 10, 10, -50, 82);
	const auto side = CollisionDetector::sweep(slider, { 100, 0 }, ground, { 0, 0 });
	REQUIRE(side);
	CHECK(side->edgeOfSecond == Edge::Left);
}

// ------------------------------------------------------------- loading

TEST_CASE("a sprite of lines is drawn when the game loads and has the size of its picture", "[lines][xml]")
{
	Loaded loaded{ gameXml(kHut, "<show object=\"hut\" />") };
	const Object& hut = loaded.game.getObject("hut");

	REQUIRE(hut.shapeKind == ShapeKind::Line);
	REQUIRE(hut.bitmap);

	// From x 0 to 20 and y 0 to 10, the second pair two pixels thick: 22 by 12.
	CHECK(hut.bitmap->width == 22);
	CHECK(hut.bitmap->height == 12);
	CHECK(hut.spriteParams == std::vector<std::string>{ "line", "22", "12" });
	CHECK(hut.collisionData.type == CollisionType::Pixel);

	// Known without a window, like a circle's or a rectangle's.
	const Vector2f size = measureShapeSize(hut.spriteParams, hut.shapeKind);
	CHECK(size.x == 22.0f);
	CHECK(size.y == 12.0f);

	// The first line is red, the second the default white.
	CHECK(hut.bitmap->rgba[(10 * 22 + 0) * 4 + 0] == 255);
	CHECK(hut.bitmap->rgba[(10 * 22 + 0) * 4 + 1] == 0);
	CHECK(hut.bitmap->rgba[(0 * 22 + 10) * 4 + 1] == 255);
}

TEST_CASE("the size of a sprite of lines can be used in an expression like any shape's", "[lines][xml]")
{
	const std::string second =
		"<object name=\"flag\"><sprite><rectangle><width>5</width><height>5</height></rectangle></sprite>"
		"<position><x>window.right - hut.width</x><y>hut.height</y></position>"
		"<velocity><x>0</x><y>0</y></velocity>"
		"<collisions><enabled>false</enabled></collisions></object>";
	Loaded loaded{ gameXml(kHut + second, "<show object=\"hut\" /><show object=\"flag\" />") };

	const Object& flag = loaded.game.getObject("flag");
	CHECK(flag.position.x == 800.0f - 22.0f);
	CHECK(flag.position.y == 12.0f);
}

TEST_CASE("a random coordinate is drawn once, so the picture and its size agree", "[lines][xml]")
{
	const std::string sprite =
		"<line><from><x>0</x><y>0</y></from><to><x><random min=\"10\" max=\"60\" /></x><y>4</y></to></line>";

	for (int i = 0; i < 5; ++i)
	{
		Loaded loaded{ gameXml(objectWith(sprite), "<show object=\"thing\" />") };
		const Object& thing = loaded.game.getObject("thing");
		const Vector2f size = measureShapeSize(thing.spriteParams, thing.shapeKind);

		CHECK(size.x == static_cast<float>(thing.bitmap->width));
		CHECK(size.x >= 11.0f);
		CHECK(size.x <= 61.0f);
		CHECK(thing.bitmap->solidAt(thing.bitmap->width - 1, 4));
	}
}

TEST_CASE("mistakes in a sprite of lines are reported where they are", "[lines][xml]")
{
	const std::string line = "<line><from><x>0</x><y>0</y></from><to><x>9</x><y>9</y></to></line>";

	SECTION("a negative coordinate")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith("<line><from><x>-3</x><y>0</y></from><to><x>9</x><y>9</y></to></line>"), "<show object=\"thing\" />")),
			ContainsSubstring("object 'thing'") && ContainsSubstring("negative coordinate"));
	}

	SECTION("a thickness under 1")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith("<line><from><x>0</x><y>0</y></from><to><x>9</x><y>9</y></to><thickness>0</thickness></line>"), "<show object=\"thing\" />")),
			ContainsSubstring("object 'thing'") && ContainsSubstring("thickness"));
	}

	SECTION("a line missing its end")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith("<line><from><x>0</x><y>0</y></from></line>"), "<show object=\"thing\" />")),
			ContainsSubstring("<line>") && ContainsSubstring("missing <to>"));
	}

	SECTION("lines mixed with another shape")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(line + "<circle><radius>4</radius></circle>"), "<show object=\"thing\" />")),
			ContainsSubstring("a sprite of lines holds only <line>s"));
	}

	SECTION("a grid of lines")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith("<grid><columns>2</columns><rows>2</rows>" + line + "</grid>"), "<show object=\"thing\" />")),
			ContainsSubstring("cannot repeat a <line>"));
	}

	SECTION("a collision type that is not one")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(line, "<enabled>true</enabled><type>round</type>"), "<show object=\"thing\" />")),
			ContainsSubstring("expected box or pixel"));
	}

	SECTION("pixel collisions on text, which only a window can draw")
	{
		const std::string text = "<text><content>hi</content><size>20</size></text>";
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(text, "<enabled>true</enabled><type>pixel</type>"), "<show object=\"thing\" />")),
			ContainsSubstring("object 'thing'") && ContainsSubstring("type>pixel"));
	}

	SECTION("a speed filter on a screen edge")
	{
		CHECK_THROWS_WITH(Loaded(gameXml(objectWith(line, "<enabled>true</enabled><collision edge=\"left\"><slower>2</slower><stop /></collision>"), "<show object=\"thing\" />")),
			ContainsSubstring("screen edge"));
	}
}

TEST_CASE("type is box when it is left out, and a circle or rectangle can be pixel too", "[pixel][xml]")
{
	const std::string circle = "<circle><radius>4</radius></circle>";

	Loaded plain{ gameXml(objectWith(circle), "<show object=\"thing\" />") };
	CHECK(plain.game.getObject("thing").collisionData.type == CollisionType::Box);
	CHECK_FALSE(plain.game.getObject("thing").bitmap);

	Loaded pixel{ gameXml(objectWith(circle, "<enabled>true</enabled><type>pixel</type>"), "<show object=\"thing\" />") };
	CHECK(pixel.game.getObject("thing").collisionData.type == CollisionType::Pixel);
}

// ------------------------------------------- verbs and filters for landing

namespace
{
	// A ship with its own pull down, a thruster that burns fuel, and a floor it
	// stops on when it is slow and loses when it is fast.
	const std::string kShip =
		"<object name=\"ship\">"
		"<sprite><rectangle><width>10</width><height>10</height></rectangle></sprite>"
		"<position><x>100</x><y>0</y></position>"
		"<velocity><x>0</x><y>0</y></velocity>"
		"<acceleration><x>0</x><y>g</y></acceleration>"
		"<collisions><enabled>true</enabled>"
		"<collision object=\"floor\"><slower>3</slower><inc variable=\"ship.landed\" /><stop /></collision>"
		"<collision object=\"floor\"><faster>3</faster><inc variable=\"ship.crashed\" /><stop /></collision>"
		"</collisions>"
		"<actions>"
		"<action name=\"burn\"><accelerate direction=\"up\" burn=\"fuel\">1.5</accelerate></action>"
		"<action name=\"free\"><accelerate direction=\"right\">0.25</accelerate></action>"
		"</actions>"
		"<variables><variable name=\"fuel\">3</variable><variable name=\"landed\">0</variable><variable name=\"crashed\">0</variable></variables>"
		"</object>"
		"<object name=\"floor\">"
		"<sprite><rectangle><width>400</width><height>20</height></rectangle></sprite>"
		"<position><x>0</x><y>500</y></position>"
		"<velocity><x>0</x><y>0</y></velocity>"
		"<collisions><enabled>true</enabled></collisions>"
		"</object>";

	struct Flight
	{
		Loaded loaded{ gameXml(kShip, "<show object=\"ship\" /><show object=\"floor\" />") };
		Game& game{ loaded.game };
		CommandExecutor executor{ game };

		Flight()
		{
			measure(game);
			game.setCurrentState("playing");
		}

		Object& ship() { return game.getObject("ship"); }
		void key(const char* action, bool down) { executor.executeInput(Command{ CmdTriggerAction{ "ship", action } }, down); }
		void frames(int count) { for (int i = 0; i < count; ++i) { game.updateObjects(); } }
	};
}

TEST_CASE("an object's acceleration changes its velocity every frame, before it moves", "[acceleration]")
{
	Flight flight;
	REQUIRE(flight.ship().acceleration.y == 0.5f);

	flight.frames(1);
	CHECK_THAT(flight.ship().velocity.y, WithinAbs(0.5f, 1e-5f));
	CHECK_THAT(flight.ship().position.y, WithinAbs(0.5f, 1e-5f)); // the new velocity is what moved it

	flight.frames(3);
	CHECK_THAT(flight.ship().velocity.y, WithinAbs(2.0f, 1e-5f));
	CHECK_THAT(flight.ship().position.y, WithinAbs(0.5f + 1.0f + 1.5f + 2.0f, 1e-4f));
}

TEST_CASE("only what is shown is pulled", "[acceleration]")
{
	Flight flight;
	flight.game.setCurrentState("playing");
	flight.ship().isVisible = false;

	flight.frames(5);

	CHECK(flight.ship().velocity.y == 0.0f);
}

TEST_CASE("accelerate adds to the velocity while the key is held and stops when it is let go", "[acceleration]")
{
	Flight flight;
	flight.ship().acceleration = {};

	flight.key("free", true);
	flight.frames(4);
	CHECK_THAT(flight.ship().velocity.x, WithinAbs(1.0f, 1e-5f)); // not set to 0.25: it built up

	flight.key("free", false);
	flight.frames(4);
	CHECK_THAT(flight.ship().velocity.x, WithinAbs(1.0f, 1e-5f)); // and is kept, with nothing to slow it
}

TEST_CASE("thrust against the pull, and opposite thrusts cancel", "[acceleration]")
{
	Flight flight;

	flight.key("burn", true);
	flight.frames(1);
	CHECK_THAT(flight.ship().velocity.y, WithinAbs(0.5f - 1.5f, 1e-5f)); // pull down 0.5, push up 1.5

	flight.key("burn", false);
	flight.frames(1);
	CHECK_THAT(flight.ship().velocity.y, WithinAbs(-1.0f + 0.5f, 1e-5f));
}

TEST_CASE("thrust burns one unit of its variable a frame, and does nothing once it is gone", "[acceleration]")
{
	Flight flight;
	REQUIRE(flight.ship().variable["fuel"] == 3.0f);

	flight.key("burn", true);
	flight.frames(2);
	CHECK(flight.ship().variable["fuel"] == 1.0f);
	CHECK_THAT(flight.ship().velocity.y, WithinAbs(2 * (0.5f - 1.5f), 1e-5f));

	flight.frames(1);
	CHECK(flight.ship().variable["fuel"] == 0.0f);
	const float withLastFuel = flight.ship().velocity.y;
	CHECK_THAT(withLastFuel, WithinAbs(3 * (0.5f - 1.5f), 1e-5f));

	// Still holding it, with nothing left: only the pull is left.
	flight.frames(2);
	CHECK(flight.ship().variable["fuel"] == 0.0f);
	CHECK_THAT(flight.ship().velocity.y, WithinAbs(withLastFuel + 2 * 0.5f, 1e-5f));
}

TEST_CASE("a held thrust is forgotten by a reset and the pull is given back by it", "[acceleration]")
{
	Flight flight;
	flight.key("burn", true);
	flight.frames(1);

	flight.game.resetObject("ship");

	CHECK(flight.ship().velocity.y == 0.0f);
	CHECK(flight.ship().acceleration.y == 0.5f);
	CHECK(flight.ship().activeThrust[static_cast<std::size_t>(Direction::Up)] == 0.0f);
	CHECK(flight.ship().variable["fuel"] == 3.0f);
}

TEST_CASE("slower and faster pick which rule about the floor runs", "[rules]")
{
	SECTION("slow, it lands")
	{
		Flight flight;
		flight.ship().position = { 100, 480 };
		flight.ship().velocity = { 0, 1 };
		flight.ship().acceleration = {};

		flight.frames(30);

		CHECK(flight.ship().variable["landed"] == 1.0f);
		CHECK(flight.ship().variable["crashed"] == 0.0f);
	}

	SECTION("fast, it crashes")
	{
		Flight flight;
		flight.ship().position = { 100, 480 };
		flight.ship().velocity = { 0, 8 };
		flight.ship().acceleration = {};

		flight.frames(30);

		CHECK(flight.ship().variable["landed"] == 0.0f);
		CHECK(flight.ship().variable["crashed"] == 1.0f);
	}

	SECTION("sideways speed counts too")
	{
		Flight flight;
		flight.ship().position = { 100, 480 };
		flight.ship().velocity = { 2.6f, 2.6f }; // each under 3, the two together over it
		flight.ship().acceleration = {};

		flight.frames(30);

		CHECK(flight.ship().variable["crashed"] == 1.0f);
	}

	SECTION("exactly the limit is faster, not slower")
	{
		Flight flight;
		flight.ship().position = { 100, 480 };
		flight.ship().velocity = { 0, 3 };
		flight.ship().acceleration = {};

		flight.frames(30);

		CHECK(flight.ship().variable["crashed"] == 1.0f);
		CHECK(flight.ship().variable["landed"] == 0.0f);
	}
}

TEST_CASE("stop brings an object to rest for good, and a reset gives it its motion back", "[rules]")
{
	Flight flight;
	flight.ship().position = { 100, 489.5f };
	flight.ship().velocity = { 0, 1 };
	flight.ship().acceleration = {};
	flight.key("free", true); // still held when it lands

	flight.frames(30);

	REQUIRE(flight.ship().variable["landed"] == 1.0f);
	CHECK(flight.ship().velocity.x == 0.0f);
	CHECK(flight.ship().velocity.y == 0.0f);
	CHECK(flight.ship().acceleration.y == 0.0f);

	// Resting on the floor, held key and all, nothing moves it and it lands only once.
	const Vector2f where = flight.ship().position;
	flight.frames(60);
	CHECK(flight.ship().position == where);
	CHECK(flight.ship().variable["landed"] == 1.0f);

	flight.game.resetObject("ship");
	CHECK(flight.ship().acceleration.y == 0.5f);
	CHECK(flight.ship().variable["landed"] == 0.0f);
}

TEST_CASE("the new tags read back the way they were written", "[rules][xml]")
{
	Flight flight;
	const Object& ship = flight.ship();

	REQUIRE(ship.collisionData.basic.size() == 2);
	REQUIRE(ship.collisionData.basic[0].slower);
	CHECK(*ship.collisionData.basic[0].slower == 3.0f);
	CHECK_FALSE(ship.collisionData.basic[0].faster);
	REQUIRE(ship.collisionData.basic[1].faster);
	CHECK(*ship.collisionData.basic[1].faster == 3.0f);

	REQUIRE(ship.action.at("burn").size() == 1);
	const auto* burn = std::get_if<CmdAccelerate>(&ship.action.at("burn")[0]);
	REQUIRE(burn);
	CHECK(burn->direction == Direction::Up);
	CHECK(burn->amount == 1.5f);
	CHECK(burn->burn == "fuel");

	const auto* free = std::get_if<CmdAccelerate>(&ship.action.at("free")[0]);
	REQUIRE(free);
	CHECK(free->burn.empty());

	REQUIRE(std::holds_alternative<CmdStop>(ship.collisionData.basic[0].commands.back()));
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
		const ScratchFile scratch{ "games/lines_scratch.xml" };
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

TEST_CASE("both schema checkers accept Lunar Lander", "[lines][schema]")
{
	const Verdict verdict = judge(readFile("games/lunarlander.xml"));

	CHECK(verdict.weakAccepts);
	CHECK(verdict.strongAccepts);
}

TEST_CASE("both schema checkers turn away the same mistakes in lines, types and rules", "[lines][schema]")
{
	struct Mistake
	{
		const char* what;
		const char* from;
		const char* to;
		const char* weakSays; // part of the weak validator's message
	};

	const Mistake mistakes[] = {
		{ "a line with no end", "<to><x>76</x><y>0</y></to>", "", "to" },
		{ "a line with a corner it cannot hold", "<thickness>4</thickness>", "<thickness>4</thickness><corner />", "corner" },
		{ "a collision type that is not one", "<enabled>true</enabled>\n        <type>pixel</type>", "<enabled>true</enabled>\n        <type>round</type>", "round" },
		{ "an acceleration missing its y", "<y>gravity</y>", "", "y" },
		{ "a thruster with no direction", "<accelerate direction=\"left\" burn=\"fuel\">", "<accelerate burn=\"fuel\">", "direction" },
	};

	for (const Mistake& mistake : mistakes)
	{
		DYNAMIC_SECTION(mistake.what)
		{
			const Verdict verdict = judge(gameWith("games/lunarlander.xml", mistake.from, mistake.to));

			CHECK_FALSE(verdict.weakAccepts);
			CHECK_THAT(verdict.weakMessage, ContainsSubstring(mistake.weakSays));
			CHECK_FALSE(verdict.strongAccepts);
		}
	}
}

TEST_CASE("an attribute the schema does not declare is turned away by Xerces, which the weak validator cannot see", "[lines][schema]")
{
	// The weak validator cannot list a node's attributes (see xsd_lite.h), so
	// this is the one kind of mistake only the real validation catches.
	const Verdict verdict = judge(gameWith("games/lunarlander.xml", "<accelerate direction=\"up\" burn=\"fuel\">", "<accelerate direction=\"up\" fuel=\"fuel\">"));

	CHECK(verdict.weakAccepts);
	CHECK_FALSE(verdict.strongAccepts);
}
