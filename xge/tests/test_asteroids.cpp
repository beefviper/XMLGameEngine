// test_asteroids.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026
//
// Catch2 tests for games/descriptions/asteroids.xml and for what it added to the language,
// played frame by frame by a real xge::Game with no window: an object with a
// <heading> that turns (<turn>), is pushed along the way it faces (<thrust>)
// and loses speed to <drag>; a drawing of lines made at every heading
// (rasterizeTurned, whole degrees) and the picture for the current heading
// being the one that is shown and tested; <fire> along the heading, each shot
// a copy of a shot that does not <exist>; <release>, which is how a rock that
// breaks spawns two smaller ones; and wrapping round the screen.
//
// Window::init() normally measures each object's size once a backend exists;
// a Game built on its own has {0, 0}, so measure() gives every shape the size
// its sprite implies, which is what the backends would measure too.

#include "scratch.h"
#include "file_text.h"
#include "picture/bitmap.h"
#include "play/executor.h"
#include "engine/folder.h"
#include "game/game.h"
#include "load/xml.h"
#include "xml/document.h"
#include "load/xsd.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

using namespace xge;
using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::WithinAbs;

namespace
{
	void measure(Game& game)
	{
		for (auto& object : game.getCurrentObjects())
		{
			object.size = measureShapeSize(object.picture);
		}
	}

	// A game in the "playing" state, ready to be stepped a frame at a time.
	struct Field
	{
		Game game{ "descriptions/asteroids.xml" };
		CommandExecutor executor{ game };

		Field()
		{
			measure(game);
			game.setCurrentState("playing");
		}

		Object& ship() { return game.getObject("ship"); }
		Object& named(const std::string& name) { return game.getObject(name); }
		float score() { return ship().variable["score"]; }
		float lives() { return ship().variable["lives"]; }
		std::string state() { return game.getCurrentState().name; }

		// The members of one of the playing state's groups that exist (its
		// full name is playing.<name>), copies made at run time among them.
		std::vector<Object*> group(const std::string& name)
		{
			std::vector<Object*> members;
			for (auto& object : game.getCurrentObjects())
			{
				if (object.exists && object.inGroup("playing." + name)) { members.push_back(&object); }
			}
			return members;
		}

		// A copy of a rock or shot, made as a <release> or <fire> would make it,
		// in the game at once.
		Object& spawn(const std::string& name)
		{
			const std::string made = game.spawn(name)->name;
			game.settleSpawns();
			return game.getObject(made);
		}

		bool has(const std::string& name)
		{
			const auto& objects = game.getCurrentObjects();
			return std::any_of(objects.begin(), objects.end(), [&](const Object& object) { return object.name == name; });
		}

		int inPlay(const std::string& name)
		{
			int count = 0;
			for (Object* object : group(name)) { if (object->isVisible) { ++count; } }
			return count;
		}

		void key(const char* action, bool down)
		{
			executor.executeInput(Command{ CmdTriggerAction{ "ship", action } }, down);
			game.settleSpawns(); // what a key spawns joins the game at once, not at the next frame
		}

		void frames(int count)
		{
			for (int i = 0; i < count; ++i)
			{
				game.updateObjects();
				if (saucersKeptOut) { hideSaucers(); }
			}
		}

		// Whether a saucer that comes in is sent straight out again, for a
		// test longer than the 12 to 24 seconds between saucers, whose rocks
		// a saucer would break.
		bool saucersKeptOut{ false };

		// Stops every rock in play, so that only what a test does moves anything.
		void freezeRocks()
		{
			for (const char* name : { "bigrocks", "mediumrocks", "smallrocks" })
			{
				for (Object* rock : group(name)) { rock->velocity = {}; }
			}
		}

		// Moves every rock in play out of the way, to the far corner, and the
		// saucers and their shots out of play.
		void clearTheMiddle()
		{
			for (const char* name : { "bigrocks", "mediumrocks", "smallrocks" })
			{
				for (Object* rock : group(name)) { rock->position = { 740, 540 }; rock->velocity = {}; }
			}
			hideSaucers();
		}

		void hideSaucers()
		{
			for (const char* name : { "saucer", "smallsaucer", "saucershot", "wildshot" })
			{
				for (auto& object : game.getCurrentObjects())
				{
					if (object.name == "playing." + std::string(name) || object.inGroup("playing." + std::string(name)))
					{
						object.isVisible = false;
						object.collisionsOn = false;
					}
				}
			}
		}

		Vector2f centre(const Object& object) { return object.position + object.size * 0.5f; }
	};

	// The solid pixels of a bitmap, as the top, bottom, left and right-most.
	struct Extent
	{
		int top = 1 << 30;
		int bottom = -1;
		int left = 1 << 30;
		int right = -1;
	};

	Extent extentOf(const Bitmap& bitmap)
	{
		Extent extent;
		for (int y = 0; y < bitmap.height; ++y)
		{
			for (int x = 0; x < bitmap.width; ++x)
			{
				if (!bitmap.solidAt(x, y)) { continue; }
				extent.top = std::min(extent.top, y);
				extent.bottom = std::max(extent.bottom, y);
				extent.left = std::min(extent.left, x);
				extent.right = std::max(extent.right, x);
			}
		}
		return extent;
	}

	LineSegment line(float x1, float y1, float x2, float y2, int thickness = 2)
	{
		LineSegment segment;
		segment.x1 = x1;
		segment.y1 = y1;
		segment.x2 = x2;
		segment.y2 = y2;
		segment.thickness = thickness;
		return segment;
	}
}

// ------------------------------------------------------------ the drawing, turned

TEST_CASE("a drawing of lines can be made at every heading, all the same size", "[asteroids][bitmap]")
{
	// An arrow pointing up: a shaft and a head.
	const std::vector<LineSegment> arrow = { line(10, 0, 10, 24), line(10, 0, 4, 8), line(10, 0, 16, 8) };

	const Bitmap first = rasterizeTurned(arrow, 0);

	for (int degrees = 0; degrees < 360; ++degrees)
	{
		const Bitmap bitmap = rasterizeTurned(arrow, static_cast<float>(degrees));
		CHECK(bitmap.width == first.width);
		CHECK(bitmap.height == first.height);
		CHECK(bitmap.width == bitmap.height);
	}

	// Heading 0 is the drawing as it was written: tall, and the same size as
	// the plain drawing is not required, but it is upright.
	const Extent upright = extentOf(first);
	CHECK((upright.bottom - upright.top) > (upright.right - upright.left));

	// A quarter of the way round it lies on its side, pointing right: wide, and
	// the tip is the right-most pixel in the middle row of the picture.
	const Bitmap quarter = rasterizeTurned(arrow, 90);
	const Extent sideways = extentOf(quarter);
	CHECK((sideways.right - sideways.left) > (sideways.bottom - sideways.top));
	CHECK(quarter.solidAt(sideways.right, quarter.height / 2));

	// Half way round it is upside down: its tip is now at the bottom.
	const Bitmap half = rasterizeTurned(arrow, 180);
	CHECK(half.solidAt(half.width / 2, extentOf(half).bottom));

	// Three quarters: pointing left.
	const Bitmap three = rasterizeTurned(arrow, 270);
	CHECK(three.solidAt(extentOf(three).left, three.height / 2));
}

TEST_CASE("the middle of the drawing stays at the middle of the picture at every heading", "[asteroids][bitmap]")
{
	// A dot-like cross about (10, 10): its middle must stay put as it turns.
	const std::vector<LineSegment> cross = { line(0, 10, 20, 10), line(10, 0, 10, 20) };

	for (int degrees = 0; degrees < 360; ++degrees)
	{
		const Bitmap bitmap = rasterizeTurned(cross, static_cast<float>(degrees));
		CHECK(bitmap.solidAt(bitmap.width / 2, bitmap.height / 2));
	}
}

TEST_CASE("a heading is taken to the nearest whole degree and wrapped round", "[asteroids][bitmap]")
{
	const std::vector<LineSegment> arrow = { line(10, 0, 10, 24), line(10, 0, 4, 8), line(10, 0, 16, 8) };
	const auto same = [&](float a, float b) { return rasterizeTurned(arrow, a).rgba == rasterizeTurned(arrow, b).rgba; };

	CHECK(same(360.0f, 0.0f));
	CHECK(same(361.0f, 1.0f));
	CHECK(same(-90.0f, 270.0f));
	CHECK(same(0.4f, 0.0f));
	CHECK(same(44.6f, 45.0f));
	CHECK_FALSE(same(0.0f, 90.0f));

	// A degree is a step of its own: at the far end of a long drawing it moves
	// the line by a pixel or more.
	const std::vector<LineSegment> pole = { line(0, 0, 0, 300) };
	CHECK(rasterizeTurned(pole, 10.0f).rgba != rasterizeTurned(pole, 11.0f).rgba);
}

TEST_CASE("no lines, or a negative coordinate, are turned away", "[asteroids][bitmap]")
{
	CHECK(rasterizeTurned({}, 0.0f).width == 0);
	CHECK_THROWS_AS(rasterizeTurned({ line(-1, 0, 5, 5) }, 0.0f), std::invalid_argument);
}

// ------------------------------------------------------------ the game loads

TEST_CASE("asteroids.xml loads with a ship that has a heading, shots, rocks and pools", "[asteroids]")
{
	Field field;
	const Object& ship = field.ship();

	CHECK(ship.picture.kind == ShapeKind::Line);
	CHECK(ship.collisionData.type == CollisionType::Pixel);
	CHECK(ship.hasHeading);
	CHECK(ship.heading == 0.0f);
	REQUIRE(ship.turnables.size() == 1);
	CHECK(ship.bitmap->rgba == ship.turnables[0]->at(0.0f).rgba);
	CHECK_THAT(ship.drag, WithinAbs(0.02f, 1e-6f));
	CHECK(ship.size.x == static_cast<float>(ship.bitmap->width));
	CHECK(ship.size.x == ship.size.y);

	// It starts in the middle of the screen.
	CHECK_THAT(field.centre(ship).x, WithinAbs(400.0f, 1.0f));
	CHECK_THAT(field.centre(ship).y, WithinAbs(300.0f, 1.0f));

	CHECK(field.group("bigrocks").size() == 4);

	// Only the big rocks exist to begin with; the shots and the smaller rocks
	// are descriptions, copied when they are fired or released.
	CHECK_FALSE(field.named("shot").exists);
	CHECK_FALSE(field.named("mediumA").exists);
	CHECK_FALSE(field.named("smallC").exists);
	CHECK(field.inPlay("bigrocks") == 4);
	CHECK(field.inPlay("mediumrocks") == 0);
	CHECK(field.inPlay("smallrocks") == 0);
	CHECK(field.inPlay("shot") == 0);

	CHECK(field.lives() == 3.0f);
	CHECK(field.score() == 0.0f);
	CHECK(field.state() == "playing");
}

TEST_CASE("asteroids.xml loads under every XML backend, checked against the schema", "[asteroids]")
{
	for (const XmlBackend backend : { XmlBackend::Xerces, XmlBackend::TinyXml2, XmlBackend::PugiXml, XmlBackend::RapidXml })
	{
		Game game{ readGame("descriptions/asteroids.xml", backend) };

		const Object& ship = game.getObject("ship");
		CHECK(ship.hasHeading);
		CHECK_THAT(ship.drag, WithinAbs(0.02f, 1e-6f));
		CHECK(ship.turnables.size() == 1);

		int descriptions = 0;
		int turns = 0;
		int thrusts = 0;
		int releases = 0;
		for (const Object& object : game.getCurrentObjects())
		{
			if (!object.exists) { ++descriptions; }
			for (const auto& [name, commands] : object.action)
			{
				for (const Command& command : commands)
				{
					if (std::holds_alternative<CmdTurn>(command)) { ++turns; }
					if (std::holds_alternative<CmdThrust>(command)) { ++thrusts; }
				}
			}
			for (const auto& rule : object.collisionData.others)
			{
				for (const Command& command : rule.commands)
				{
					if (std::holds_alternative<CmdRelease>(command)) { ++releases; }
				}
			}
		}

		CHECK(descriptions == 3 + 3 + 3); // three shapes of each smaller rock, and the three shots
		CHECK(turns == 2);
		CHECK(thrusts == 1);
		CHECK(releases == (4 + 3) * 2); // every big rock and medium shape hands out smaller ones, hit by a shot or a saucer
	}
}

// ------------------------------------------------------------ turning

TEST_CASE("holding a turn key turns the ship by the turn rate every frame, and letting go stops it", "[asteroids][heading]")
{
	Field field;

	field.key("right", true);
	field.frames(10);
	CHECK_THAT(field.ship().heading, WithinAbs(40.0f, 1e-3f));

	field.key("right", false);
	field.frames(10);
	CHECK_THAT(field.ship().heading, WithinAbs(40.0f, 1e-3f));

	field.key("left", true);
	field.frames(15);
	CHECK_THAT(field.ship().heading, WithinAbs(340.0f, 1e-3f)); // 40 - 60, kept from 0 up to 360

	// Opposite keys together cancel.
	field.key("right", true);
	field.frames(5);
	CHECK_THAT(field.ship().heading, WithinAbs(340.0f, 1e-3f));
}

TEST_CASE("the picture shown and tested is the one for the heading", "[asteroids][heading]")
{
	Field field;
	Object& ship = field.ship();
	const auto upright = ship.bitmap;

	// 22 frames at 4 degrees is 88: very nearly a quarter turn, and a different
	// picture; the visual is marked to be rebuilt.
	ship.visualDirty = false;
	field.key("right", true);
	field.frames(22);
	field.key("right", false);

	CHECK(ship.bitmap != upright);
	CHECK(ship.visualDirty);
	CHECK(ship.bitmap->width == upright->width);
	CHECK(ship.bitmap->height == upright->height);

	// It is drawn at the heading to the nearest whole degree, 88, which is all
	// but a quarter turn: the nose is the right-most pixel, near the middle.
	CHECK(ship.turnedDegrees == 88);
	CHECK(ship.bitmap->rgba == ship.turnables[0]->at(88.0f).rgba);
	const Extent nose = extentOf(*ship.bitmap);
	const int middle = ship.bitmap->height / 2;
	CHECK((ship.bitmap->solidAt(nose.right, middle - 1) || ship.bitmap->solidAt(nose.right, middle) || ship.bitmap->solidAt(nose.right, middle + 1)));
	CHECK(nose.right - nose.left > nose.bottom - nose.top);

	// A full turn comes back to the first picture.
	field.key("right", true);
	field.frames(68); // 88 + 272 = 360
	field.key("right", false);
	CHECK_THAT(ship.heading, WithinAbs(0.0f, 1e-2f));
	CHECK(ship.bitmap->rgba == upright->rgba);
}

TEST_CASE("turning does not move the ship or change its size", "[asteroids][heading]")
{
	Field field;
	const Vector2f where = field.ship().position;
	const Vector2f size = field.ship().size;

	field.key("left", true);
	field.frames(40);

	CHECK(field.ship().position == where);
	CHECK(field.ship().size == size);
	CHECK(field.ship().velocity == Vector2f{});
}

TEST_CASE("held thrust shows the flickering flame, turned with the ship; letting go puts it out", "[asteroids][heading]")
{
	Field field;
	field.clearTheMiddle();
	Object& ship = field.ship();
	const Vector2f size = ship.size;
	CHECK(ship.lookName() == "coasting");
	const auto coasting = ship.bitmap;

	// Turned a quarter, then thrust: the flame shows at that heading, and
	// flickers with the plain ship every three frames.
	field.key("right", true);
	field.frames(22);
	field.key("right", false);
	const auto turned = ship.bitmap;
	field.key("thrust", true);
	field.frames(1);
	CHECK(ship.lookName() == "burning");
	CHECK(ship.bitmap != turned);
	CHECK(ship.bitmap->rgba == ship.turnables[0]->at(88.0f).rgba);
	CHECK(ship.size == size);
	bool flickered = false;
	for (int i = 0; i < 6; ++i)
	{
		field.frames(1);
		if (ship.bitmap->rgba == turned->rgba) { flickered = true; }
	}
	CHECK(flickered);

	field.key("thrust", false);
	field.frames(1);
	CHECK(ship.lookName() == "coasting");
	CHECK(ship.bitmap->rgba == turned->rgba);
	CHECK(ship.bitmap->rgba != coasting->rgba);
}

// ------------------------------------------------------------ thrust and drag

TEST_CASE("thrust pushes the ship along the way it faces", "[asteroids][heading]")
{
	Field field;
	field.clearTheMiddle();

	// Facing up: the velocity goes up the screen, which is negative y, and
	// sideways not at all.
	field.key("thrust", true);
	field.frames(1);
	CHECK_THAT(field.ship().velocity.y, WithinAbs(-0.09f * 0.98f, 1e-4f));
	CHECK_THAT(field.ship().velocity.x, WithinAbs(0.0f, 1e-5f));
	field.key("thrust", false);

	// Turned a quarter of the way clockwise it faces right.
	field.ship().velocity = {};
	field.ship().heading = 90.0f;
	field.key("thrust", true);
	field.frames(1);
	CHECK_THAT(field.ship().velocity.x, WithinAbs(0.09f * 0.98f, 1e-4f));
	CHECK_THAT(field.ship().velocity.y, WithinAbs(0.0f, 1e-4f));
	field.key("thrust", false);

	// Facing down, and facing left.
	field.ship().velocity = {};
	field.ship().heading = 180.0f;
	field.key("thrust", true);
	field.frames(1);
	CHECK(field.ship().velocity.y > 0.0f);
	field.key("thrust", false);

	field.ship().velocity = {};
	field.ship().heading = 270.0f;
	field.key("thrust", true);
	field.frames(1);
	CHECK(field.ship().velocity.x < 0.0f);
	field.key("thrust", false);

	// Half way between, it pushes both ways at once.
	field.ship().velocity = {};
	field.ship().heading = 45.0f;
	field.key("thrust", true);
	field.frames(1);
	CHECK(field.ship().velocity.x > 0.0f);
	CHECK(field.ship().velocity.y < 0.0f);
	CHECK_THAT(field.ship().velocity.x, WithinAbs(-field.ship().velocity.y, 1e-4f));
}

TEST_CASE("the heading it turned to is the way it is pushed", "[asteroids][heading]")
{
	Field field;
	field.clearTheMiddle();

	field.key("right", true);
	field.frames(23); // 92 degrees
	field.key("right", false);

	field.key("thrust", true);
	field.frames(5);
	field.key("thrust", false);

	CHECK(field.ship().velocity.x > 0.3f);
	CHECK(std::abs(field.ship().velocity.y) < 0.05f);
}

TEST_CASE("with no thrust the ship drifts, losing a fiftieth of its speed a frame", "[asteroids][heading]")
{
	Field field;
	field.clearTheMiddle();

	field.ship().position = { 300, 250 };
	field.ship().velocity = { 3.0f, 0.0f };
	field.frames(10);

	CHECK_THAT(field.ship().velocity.x, WithinAbs(3.0f * std::pow(0.98f, 10.0f), 1e-3f));
	CHECK(field.ship().position.x > 300.0f + 10.0f);
}

TEST_CASE("held thrust has a top speed instead of speeding up for ever", "[asteroids][heading]")
{
	Field field;
	field.clearTheMiddle();
	field.ship().heading = 90.0f;

	field.key("thrust", true);
	for (int i = 0; i < 400; ++i)
	{
		field.frames(1);
		field.ship().position = { 380, 280 }; // keep it in the middle; only its speed matters
	}

	const float top = 0.09f * 0.98f / 0.02f;
	CHECK_THAT(field.ship().velocity.x, WithinAbs(top, 0.05f));
}

TEST_CASE("a thrust held across a pause is still held afterwards", "[asteroids][heading]")
{
	Field field;
	field.clearTheMiddle();

	// The same resume as for accelerate: the key was down when the state changed.
	field.key("thrust", true);
	field.frames(2);

	const Command held{ CmdTriggerAction{ "ship", "thrust" } };
	CHECK(field.executor.executeHeldInput(held));
	field.ship().velocity = {};
	field.frames(1);
	CHECK(field.ship().velocity.y < 0.0f);
}

// ------------------------------------------------------------ wrapping

TEST_CASE("the ship leaves one side of the screen and comes back in the other", "[asteroids][wrap]")
{
	Field field;
	field.clearTheMiddle();

	field.ship().position = { 760, 250 };
	field.ship().velocity = { 3.0f, 0.0f };

	float lowest = 800.0f;
	bool wrapped = false;
	for (int i = 0; i < 60 && !wrapped; ++i)
	{
		const float before = field.ship().position.x;
		field.frames(1);
		if (field.ship().position.x < before) { wrapped = true; }
		lowest = std::min(lowest, field.ship().position.x);
	}

	CHECK(wrapped);
	CHECK(field.ship().position.x < 0.0f); // coming in from the left, not yet whole
	CHECK(field.ship().position.x + field.ship().size.x > 0.0f);
}

TEST_CASE("it wraps top and bottom too", "[asteroids][wrap]")
{
	Field field;
	field.clearTheMiddle();

	field.ship().position = { 300, 4 };
	field.ship().velocity = { 0.0f, -3.0f };

	bool wrapped = false;
	for (int i = 0; i < 60 && !wrapped; ++i)
	{
		const float before = field.ship().position.y;
		field.frames(1);
		if (field.ship().position.y > before) { wrapped = true; }
	}

	CHECK(wrapped);
	CHECK(field.ship().position.y > 500.0f); // back in at the bottom
}

TEST_CASE("a rock wraps as well", "[asteroids][wrap]")
{
	Field field;
	Object& rock = *field.group("bigrocks")[0];

	rock.position = { 780, 300 };
	rock.velocity = { 2.0f, 0.0f };

	bool wrapped = false;
	for (int i = 0; i < 100 && !wrapped; ++i)
	{
		const float before = rock.position.x;
		field.frames(1);
		if (rock.position.x < before) { wrapped = true; }
	}

	CHECK(wrapped);
	CHECK(rock.isVisible);
}

// ------------------------------------------------------------ shooting

TEST_CASE("a shot leaves the nose along the heading, at the shot's own speed", "[asteroids][fire]")
{
	Field field;
	field.clearTheMiddle();

	// Facing up (the start).
	field.key("fire", true);
	field.key("fire", false);

	REQUIRE(field.inPlay("shot") == 1);
	Object& shot = *field.group("shot")[0];
	CHECK(shot.isVisible);
	CHECK(shot.collisionsOn);
	CHECK_THAT(shot.velocity.x, WithinAbs(0.0f, 1e-4f));
	CHECK_THAT(shot.velocity.y, WithinAbs(-8.0f, 1e-4f));
	CHECK(shot.position.y + shot.size.y / 2 < field.centre(field.ship()).y - 10.0f); // out in front
	CHECK_THAT(shot.position.x + shot.size.x / 2, WithinAbs(field.centre(field.ship()).x, 0.5f));

	// Facing right, a later shot goes right.
	field.ship().heading = 90.0f;
	field.key("fire", true);
	field.key("fire", false);

	REQUIRE(field.inPlay("shot") == 2);
	Object& second = *field.group("shot")[1];
	CHECK_THAT(second.velocity.x, WithinAbs(8.0f, 1e-3f));
	CHECK_THAT(second.velocity.y, WithinAbs(0.0f, 1e-3f));
	CHECK(second.position.x + second.size.x / 2 > field.centre(field.ship()).x + 10.0f);
}

TEST_CASE("only four shots can be in the air; a fifth does nothing until one is gone", "[asteroids][fire]")
{
	Field field;
	field.clearTheMiddle();

	for (int i = 0; i < 4; ++i)
	{
		field.key("fire", true);
		field.key("fire", false);
	}
	CHECK(field.inPlay("shot") == 4);

	// The fifth press: still four, and none of them was moved or relaunched.
	const Vector2f first = field.group("shot")[0]->position;
	field.key("fire", true);
	field.key("fire", false);
	CHECK(field.inPlay("shot") == 4);
	CHECK(field.group("shot")[0]->position == first);

	// They fly up to the top of the screen and are despawned; then there is room.
	field.frames(80);
	CHECK(field.inPlay("shot") == 0);

	field.key("fire", true);
	field.key("fire", false);
	CHECK(field.inPlay("shot") == 1);
}

TEST_CASE("a shot wraps round the screen, and a second on it is gone from the game", "[asteroids][fire]")
{
	Field field;
	field.clearTheMiddle();
	const std::size_t before = field.game.getCurrentObjects().size();

	field.key("fire", true);
	field.key("fire", false);
	CHECK(field.game.getCurrentObjects().size() == before + 1);

	// Up from the middle at 8 a frame, it is off the top in about 36 frames
	// and back in at the bottom.
	field.frames(45);
	REQUIRE(field.group("shot").size() == 1);
	CHECK(field.group("shot")[0]->position.y > 300.0f);

	field.frames(16);
	CHECK(field.group("shot").empty());
	CHECK(field.game.getCurrentObjects().size() == before);
}

TEST_CASE("a hit ship waits for the middle to be clear before it comes back", "[asteroids][ship]")
{
	Field field;
	field.clearTheMiddle();
	field.ship().position = { 100, 100 };
	field.ship().velocity = { 1.0f, 0.5f };
	Object& rock = *field.group("bigrocks")[0];
	rock.position = { 105, 105 };
	rock.velocity = {};
	field.frames(2);
	REQUIRE_FALSE(field.ship().isVisible);

	// A rock parked by the middle keeps it away, however long.
	field.clearTheMiddle();
	Object& other = *field.group("bigrocks")[1];
	other.isVisible = true;
	other.collisionsOn = true;
	other.position = { 400.0f - other.size.x / 2.0f + 60.0f, 300.0f - other.size.y / 2.0f };
	field.frames(300);
	CHECK_FALSE(field.ship().isVisible);

	// Moved off, the ship is back the next frame.
	other.position = { 740, 540 };
	field.frames(2);
	CHECK(field.ship().isVisible);
	CHECK(field.ship().position == field.ship().start.position);
}

// ------------------------------------------------------------ breaking rocks

namespace
{
	// A shot meets a rock: stands the ship below the rock, facing it, with
	// every other rock out of the way, and fires.
	void shoot(Field& field, Object& rock)
	{
		field.clearTheMiddle();
		field.ship().position = { 300, 480 };
		field.ship().velocity = {};
		field.ship().heading = 0.0f;

		rock.position = { field.centre(field.ship()).x - rock.size.x / 2, 200 };
		rock.velocity = {};

		field.key("fire", true);
		field.key("fire", false);
		field.frames(60);
	}
}

TEST_CASE("a shot breaks a big rock into two medium ones where it was", "[asteroids][release]")
{
	Field field;
	Object& rock = *field.group("bigrocks")[0];

	shoot(field, rock);
	const Vector2f from = field.centre(rock); // it stood still, so this is where it was hit

	CHECK_FALSE(rock.isVisible);
	CHECK(field.inPlay("shot") == 0);
	CHECK(field.inPlay("mediumrocks") == 2);
	CHECK(field.score() == 20.0f);

	// Both of them where the big rock was, and moving, each at its own velocity.
	for (Object* medium : field.group("mediumrocks"))
	{
		if (!medium->isVisible) { continue; }
		CHECK(medium->collisionsOn);
		CHECK_THAT(field.centre(*medium).x, WithinAbs(from.x, 3.0f * 60.0f)); // moved on since, at its own speed
		CHECK(medium->velocity == medium->start.velocity);
		CHECK((medium->velocity.x != 0.0f || medium->velocity.y != 0.0f));
	}
}

TEST_CASE("the two medium rocks are released the moment of the hit, centred on the big one", "[asteroids][release]")
{
	Field field;
	Object& rock = *field.group("bigrocks")[1];
	field.clearTheMiddle();

	// A shot already touching the rock: the first frame is the hit.
	rock.position = { 500, 200 };
	rock.velocity = {};
	const Vector2f from = field.centre(rock);

	Object& shot = field.spawn("shot");
	shot.position = { from.x - 1.0f, from.y + rock.size.y / 2 + 6.0f }; // just below the rock's edge
	shot.velocity = { 0.0f, -4.0f };

	field.frames(4);

	REQUIRE(field.inPlay("mediumrocks") == 2);
	int found = 0;
	for (Object* medium : field.group("mediumrocks"))
	{
		if (!medium->isVisible) { continue; }
		++found;
		const float speed = std::hypot(medium->velocity.x, medium->velocity.y);
		CHECK_THAT(field.centre(*medium).x, WithinAbs(from.x, 12.0f));
		CHECK_THAT(field.centre(*medium).y, WithinAbs(from.y, 12.0f));
		CHECK(speed > 0.5f);
	}
	CHECK(found == 2);
}

TEST_CASE("a medium rock breaks into two small ones, and a small one into none", "[asteroids][release]")
{
	Field field;
	// Bring one medium rock into play (as a hit on a big rock would) and shoot it.
	Object& medium = field.spawn("mediumrocks");
	const std::string mediumName = medium.name;
	shoot(field, medium);

	CHECK_FALSE(field.has(mediumName)); // despawned, and gone
	CHECK(field.inPlay("smallrocks") == 2);
	CHECK(field.score() == 50.0f);

	// Now a small one: a hundred points and nothing comes out of it.
	Object& small = field.spawn("smallrocks");
	const std::string smallName = small.name;
	shoot(field, small);

	CHECK_FALSE(field.has(smallName));
	CHECK(field.score() == 150.0f);
	CHECK(field.inPlay("smallrocks") == 2); // the two from before, which clearTheMiddle moved aside
}

TEST_CASE("every rock of a wave can be broken down to nothing, and the next wave comes", "[asteroids][release]")
{
	Field field;
	field.saucersKeptOut = true;

	// Break everything in play, one rock at a time, until nothing is left.
	int broken = 0;
	for (int guard = 0; guard < 100; ++guard)
	{
		Object* next = nullptr;
		for (const char* name : { "smallrocks", "mediumrocks", "bigrocks" })
		{
			for (Object* rock : field.group(name))
			{
				if (rock->isVisible) { next = rock; break; }
			}
			if (next) { break; }
		}
		if (!next || (broken == 4 + 8 + 16)) { break; }

		shoot(field, *next);
		++broken;
	}

	CHECK(broken == 4 + 8 + 16);
	CHECK(field.score() == 4 * 20 + 8 * 50 + 16 * 100);
	CHECK(field.state() == "playing");
	CHECK(field.inPlay("bigrocks") == 4); // a new wave
	CHECK(field.inPlay("mediumrocks") + field.inPlay("smallrocks") == 0);
}

TEST_CASE("there is no limit to the smaller rocks: a break always releases two more", "[asteroids][release]")
{
	Field field;

	// Eight medium rocks already out: a break adds two more, the shapes in turn.
	for (int i = 0; i < 8; ++i) { field.spawn("mediumrocks"); }
	CHECK(field.has("playing.mediumA.3"));
	CHECK(field.has("playing.mediumB.3"));
	CHECK(field.has("playing.mediumC.2"));

	Object& big = *field.group("bigrocks")[0];
	shoot(field, big);

	CHECK(field.inPlay("mediumrocks") == 10);
}

// ------------------------------------------------------------ the ship is hit

TEST_CASE("a rock hitting the ship costs a ship and breaks, and the ship comes back at the middle, at rest, facing up", "[asteroids][ship]")
{
	Field field;
	field.clearTheMiddle();

	field.ship().position = { 100, 100 };
	field.ship().velocity = { 1.0f, 0.5f };
	field.ship().heading = 135.0f;
	field.ship().showHeading();

	Object& rock = *field.group("bigrocks")[2];
	rock.position = { 105, 105 };
	rock.velocity = { 0.0f, 0.0f };

	field.frames(2);

	CHECK(field.lives() == 2.0f);
	CHECK(field.ship().position == field.ship().start.position);
	CHECK(field.ship().velocity == Vector2f{});
	CHECK(field.ship().heading == 0.0f);
	CHECK(field.ship().turnedDegrees == 0);
	CHECK_FALSE(rock.isVisible); // the ship breaks it, as a shot would
	CHECK(field.inPlay("mediumrocks") == 2);
	CHECK(field.score() == 20.0f);

	// The ship is out of play for a moment, then back.
	CHECK_FALSE(field.ship().isVisible);
	field.clearTheMiddle();
	field.frames(121);
	CHECK(field.ship().isVisible);
	CHECK(field.ship().position == field.ship().start.position);
}

TEST_CASE("the pixels decide: the boxes overlap long before a rock touches the ship", "[asteroids][ship]")
{
	Field field;
	field.clearTheMiddle();

	// The ship's picture is a triangle in a square: a rock sitting in the empty
	// corner of the square is inside the box and nowhere near a pixel.
	field.ship().position = { 100, 100 };
	field.ship().velocity = {};

	Object& rock = *field.group("bigrocks")[3];
	rock.velocity = { 0.0f, 0.0f };
	rock.position = { 100.0f + field.ship().size.x - 4.0f, 100.0f + 2.0f }; // by the top right corner, away from the nose
	field.frames(3);

	CHECK(field.lives() == 3.0f);
}

TEST_CASE("three ships and then it is game over; space plays again from the start", "[asteroids][ship]")
{
	Field field;

	for (int hit = 0; hit < 3; ++hit)
	{
		field.clearTheMiddle();
		field.frames(121); // back after the last hit, the middle clear
		Object& rock = *field.group("bigrocks")[static_cast<std::size_t>(hit)];
		field.ship().position = { 100, 100 }; // it starts again in the middle after each hit
		field.ship().velocity = { 1.0f, 0.5f }; // contact is found as things move into each other
		rock.position = { 105, 105 };
		rock.velocity = { 0.0f, 0.0f };
		field.frames(3);
	}

	CHECK(field.lives() == 0.0f);
	CHECK(field.state() == "gameover");

	// The ship is gone and the rocks that drift on cannot cost it more ships.
	field.frames(240);
	CHECK(field.lives() == 0.0f);
	CHECK_FALSE(field.game.isShown(field.ship()));

	// Space starts it all over: every rock, shot, score and ship back.
	field.executor.executeInput(Command{ CmdReset{} }, true);
	CHECK(field.state() == "mainmenu");
	CHECK(field.lives() == 3.0f);
	CHECK(field.score() == 0.0f);
	CHECK(field.inPlay("bigrocks") == 4);
}

// ------------------------------------------------------------ winning

TEST_CASE("when every rock is gone the next wave comes in", "[asteroids][states]")
{
	Field field;

	for (const char* name : { "bigrocks", "mediumrocks", "smallrocks" })
	{
		for (Object* rock : field.group(name)) { rock->isVisible = false; }
	}
	field.frames(1);

	CHECK(field.state() == "playing");
	CHECK(field.inPlay("bigrocks") == 4);
}

TEST_CASE("with rocks still left, even small ones, the wave goes on", "[asteroids][states]")
{
	Field field;

	for (Object* rock : field.group("bigrocks")) { rock->isVisible = false; }
	Object& small = field.spawn("smallrocks");
	small.velocity = {};
	small.position = { 700, 500 };
	field.frames(1);

	CHECK(field.state() == "playing");
	CHECK(field.inPlay("bigrocks") == 0);
}

TEST_CASE("paused, the field is drawn still: the rocks and the ship stop where they are", "[asteroids][states]")
{
	Field field;
	field.frames(5);
	Object& rock = *field.group("bigrocks")[0];
	const Vector2f rockAt = rock.position;
	const Vector2f shipAt = field.ship().position;

	field.game.pushState("paused");
	field.frames(60);
	CHECK(rock.position == rockAt);
	CHECK(field.ship().position == shipAt);
	CHECK(field.game.isStill(rock));
	CHECK(field.game.isStill(field.ship()));
	CHECK(field.game.isStill(field.named("score")));
	CHECK_FALSE(field.game.isShown(rock));
	CHECK(field.game.isShown(field.named("paused")));

	// A rock that does not exist is not drawn, even still.
	CHECK_FALSE(field.game.isStill(field.named("mediumA")));

	field.game.popState();
	field.frames(1);
	CHECK(rock.position != rockAt);
}

TEST_CASE("each state owns what it draws, and game over draws the field still under its message", "[asteroids][states]")
{
	Field field;
	CHECK(field.ship().ownerState == "playing");
	CHECK(field.group("bigrocks")[0]->ownerState == "playing");
	CHECK(field.named("title").ownerState == "mainmenu");
	CHECK(field.named("again").ownerState == "gameover");
	CHECK(field.named("best").ownerState.empty());
	CHECK_FALSE(field.game.isShown(field.named("title")));
	CHECK(field.game.isShown(field.named("best")));

	field.ship().variable["lives"] = 0.0f;
	field.frames(1);
	REQUIRE(field.state() == "gameover");
	CHECK(field.game.isStill(field.ship()));
	CHECK(field.game.isShown(field.named("gameover.message")));
	CHECK(field.game.isShown(field.named("again")));
}

TEST_CASE("playing again puts everything back: rocks, shots, score and ships", "[asteroids][states]")
{
	Field field;

	field.group("bigrocks")[0]->isVisible = false;
	field.spawn("mediumrocks");
	field.spawn("shot");
	field.ship().variable["score"] = 55.0f;
	field.ship().variable["lives"] = 0.0f;
	field.ship().heading = 90.0f;
	field.game.resetAll();

	CHECK(field.inPlay("bigrocks") == 4);
	CHECK(field.inPlay("mediumrocks") == 0);
	CHECK(field.inPlay("smallrocks") == 0);
	CHECK(field.group("shot").empty()); // the copies are gone
	CHECK(field.score() == 0.0f);
	CHECK(field.lives() == 3.0f);
	CHECK(field.ship().heading == 0.0f);
	CHECK(field.ship().turnedDegrees == 0);
	CHECK(field.state() == "mainmenu");
}

// ------------------------------------------------------------ the saucer, hyperspace, extra ships

TEST_CASE("the big saucer comes in from the left now and then, and shoots", "[asteroids][saucer]")
{
	Field field;
	field.clearTheMiddle();
	Object& saucer = field.named("saucer");
	CHECK_FALSE(saucer.isVisible);

	// Within half a minute it is in, coming from the left.
	int waited = 0;
	while (!saucer.isVisible && waited < 60 * 30) { field.frames(1); ++waited; }
	REQUIRE(saucer.isVisible);
	CHECK(saucer.position.x < 0.0f);
	CHECK(saucer.velocity.x > 0.0f);

	// A second on, it has fired a shot, off any way.
	field.frames(61);
	int flying = 0;
	for (Object* shot : field.group("wildshot"))
	{
		if (shot->isVisible) { ++flying; }
	}
	CHECK(flying >= 1);
}

TEST_CASE("a shot that hits the big saucer is 200 points", "[asteroids][saucer]")
{
	Field field;
	field.clearTheMiddle();
	field.ship().position = { 300, 480 };
	Object& saucer = field.named("saucer");
	saucer.isVisible = true;
	saucer.collisionsOn = true;
	saucer.position = { field.centre(field.ship()).x - saucer.size.x / 2, 200 };
	saucer.velocity = {};
	field.key("fire", true);
	field.key("fire", false);
	field.frames(40);

	CHECK_FALSE(saucer.isVisible);
	CHECK(field.score() == 200.0f);
}

TEST_CASE("a rock that meets a saucer breaks it, and breaks itself for no points", "[asteroids][saucer]")
{
	for (const std::string which : { "saucer", "smallsaucer" })
	{
		Field field;
		field.clearTheMiddle();
		field.ship().position = { 60, 60 };
		Object& saucer = field.named(which);
		saucer.isVisible = true;
		saucer.collisionsOn = true;
		saucer.position = { 380, 300 };
		saucer.velocity = {};
		Object& rock = *field.group("bigrocks").front();
		rock.position = { saucer.position.x - 20, saucer.position.y - 20 };
		rock.velocity = { 1, 1 };
		const std::size_t medium = field.group("mediumrocks").size();
		field.frames(30);

		CHECK_FALSE(saucer.isVisible);
		CHECK_FALSE(rock.isVisible);
		CHECK(field.group("mediumrocks").size() == medium + 2);
		CHECK(field.score() == 0.0f);
	}
}

TEST_CASE("the small saucer aims at the ship and is a thousand points; the higher the score, the likelier it comes", "[asteroids][saucer]")
{
	Field field;
	field.clearTheMiddle();
	Object& saucer = field.named("saucer");
	Object& small = field.named("smallsaucer");

	// At no score it is always the big one.
	field.game.setVariable("playing.saucers", "luck", 0.1f);
	field.game.setVariable("playing.saucers", "due", 1.0f);
	field.frames(1);
	CHECK(saucer.isVisible);
	CHECK_FALSE(small.isVisible);
	CHECK(field.named("saucers").variable["due"] == 0.0f);

	// At 8000 a luck below a fifth sends the small one, once the big is gone.
	saucer.isVisible = false;
	field.game.setVariable("playing.ship", "score", 8000.0f);
	field.game.setVariable("playing.saucers", "luck", 0.1f);
	field.game.setVariable("playing.saucers", "due", 1.0f);
	field.frames(1);
	CHECK(small.isVisible);
	CHECK_FALSE(saucer.isVisible);
	CHECK(small.size.x < saucer.size.x);

	// It aims: a second on, its shot is going toward the ship.
	field.frames(61);
	int flying = 0;
	for (Object* shot : field.group("saucershot"))
	{
		if (!shot->isVisible) { continue; }
		++flying;
		const Vector2f toShip = field.centre(field.ship()) - field.centre(*shot);
		CHECK(shot->velocity.x * toShip.x + shot->velocity.y * toShip.y > 0.0f);
	}
	CHECK(flying >= 1);

	small.position = { field.centre(field.ship()).x - small.size.x / 2, 200 };
	small.velocity = {};
	field.ship().position = { field.ship().position.x, 480 };
	field.ship().velocity = {};
	field.ship().heading = 0.0f;
	for (Object* shot : field.group("saucershot")) { shot->position = { 10, 10 }; shot->velocity = {}; }
	field.key("fire", true);
	field.key("fire", false);
	field.frames(40);
	CHECK_FALSE(small.isVisible);
	CHECK(field.score() == 9000.0f);
}

TEST_CASE("hyperspace puts the ship somewhere else at random, inside the window", "[asteroids][ship]")
{
	Field field;
	field.clearTheMiddle();
	const Vector2f before = field.ship().position;
	field.key("hyperspace", true);
	field.key("hyperspace", false);

	const Vector2f after = field.ship().position;
	CHECK(after != before);
	CHECK(after.x >= 20.0f);
	CHECK(after.x <= 740.0f);
	CHECK(after.y >= 20.0f);
	CHECK(after.y <= 540.0f);
}

TEST_CASE("every ten thousand points is another ship, and the best score follows the score", "[asteroids][ship]")
{
	Field field;
	field.clearTheMiddle();
	field.ship().variable["score"] = 10020.0f;
	field.frames(1);
	CHECK(field.lives() == 4.0f);
	CHECK(field.ship().variable["next"] == 20000.0f);
	field.frames(1);
	CHECK(field.lives() == 4.0f);
	CHECK(field.named("best").variable["score"] == 10020.0f);
}

// ------------------------------------------------------------ mistakes

namespace
{
	std::string asteroidsWith(const std::string& from, const std::string& to)
	{
		std::ifstream in("descriptions/asteroids.xml");
		REQUIRE(in.good());
		std::string xml = fileText(in);
		xml.erase(std::remove(xml.begin(), xml.end(), '\r'), xml.end());
		const auto at = xml.find(from);
		REQUIRE(at != std::string::npos);
		xml.replace(at, from.size(), to);
		return xml;
	}

	// Writes `xml` beside the games (so the schema path resolves), loads it as a
	// Game, and removes it again.
	struct Scratch
	{
		std::filesystem::path path{ scratchName("descriptions/asteroids_scratch.xml") };

		explicit Scratch(const std::string& xml)
		{
			std::ofstream out(path);
			out << xml;
		}
		Scratch(const Scratch&) = delete;
		Scratch& operator=(const Scratch&) = delete;
		~Scratch()
		{
			std::error_code ignored;
			std::filesystem::remove(path, ignored);
		}
	};
}

TEST_CASE("a turn or a thrust on an object with no heading is reported when the game loads", "[asteroids][errors]")
{
	const Scratch scratch{ asteroidsWith("<heading>0</heading>", "") };
	CHECK_THROWS_WITH(Game{ scratch.path.string() }, ContainsSubstring("heading"));
}

TEST_CASE("a release of something that is not in the game is reported when it loads", "[asteroids][errors]")
{
	const Scratch scratch{ asteroidsWith("<release object=\"mediumrocks\">", "<release object=\"nosuchrocks\">") };
	CHECK_THROWS_WITH(Game{ scratch.path.string() }, ContainsSubstring("nosuchrocks"));
}

TEST_CASE("a drag of 1 or more is reported when it loads", "[asteroids][errors]")
{
	const Scratch scratch{ asteroidsWith("<drag>drag</drag>", "<drag>1</drag>") };
	CHECK_THROWS_WITH(Game{ scratch.path.string() }, ContainsSubstring("drag"));
}

TEST_CASE("both schema checkers turn away a turn that is not left or right", "[asteroids][schema]")
{
	const Scratch scratch{ asteroidsWith("<turn direction=\"left\">", "<turn direction=\"up\">") };

	auto weak = XmlDocumentFactory::create(XmlBackend::TinyXml2);
	REQUIRE(weak->load(scratch.path.string()));
	XsdLiteValidator validator;
	REQUIRE(validator.loadSchema(findSchema("descriptions/pong.xml", "../xgedef.xsd").string(), XmlBackend::TinyXml2));
	CHECK_FALSE(validator.validate(*weak->getRootElement()));

	auto strong = XmlDocumentFactory::create(XmlBackend::Xerces);
	CHECK_FALSE(strong->load(scratch.path.string()));
}

TEST_CASE("a heading, a drag and an active are read from the file", "[asteroids][parsing]")
{
	Game game{ "descriptions/asteroids.xml" };

	const Object& ship = game.getObject("ship");
	CHECK(ship.hasHeading);
	CHECK(ship.start.heading == 0.0f);
	CHECK(ship.drag == 0.02f);
	CHECK(ship.turnables.size() == 1);

	for (const Object& object : game.getCurrentObjects())
	{
		if (object.groupName == "playing.mediumrocks" || object.groupName == "playing.smallrocks" || object.groupName == "playing.shot")
		{
			CHECK_FALSE(object.isVisible);
		}
	}
}
