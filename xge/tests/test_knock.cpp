// test_knock.cpp
// XML Game Engine
// author: beefviper
// date: Oct 16, 2026
//
// Catch2 tests for a touch worked out once (contactOf: where, the way
// apart, where along each side, how deep), two circles meeting as circles,
// and <knock> with <mass>: the arithmetic on its own, then played in
// games/descriptions/trickshot.xml (balls knocking balls, a cue tip with no
// mass) and games/descriptions/mariobros.xml (a knock through a floor turns
// over what walks on it).

#include "engine/engine.h"
#include "game/game.h"
#include "play/collision.h"
#include "play/geometry.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace xge;
using Catch::Approx;
using Catch::Matchers::ContainsSubstring;

namespace
{
	class FakeWindow : public Window
	{
	public:
		bool isOpen() const override { return true; }
		void close() override {}
		void init(std::vector<Object>&) override {}
		std::vector<std::pair<KeyCode, bool>> pollEvents() override { return {}; }
		void clear(const std::string&) override {}
		void draw(Object&) override {}
		void display() override {}
	};

	// A game played with no window, each shape the size its sprite implies.
	struct Played
	{
		Game game;
		Engine engine;

		explicit Played(const char* file) : game(file), engine(game, std::make_unique<FakeWindow>())
		{
			for (auto& object : game.getCurrentObjects())
			{
				object.size = measureShapeSize(object.spriteParams, object.shapeKind);
			}
		}

		void tap(KeyCode key)
		{
			engine.handleKeyPressed(key);
			engine.handleKeyReleased(key);
		}

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { engine.step(); }
		}

		std::string state() { return game.getCurrentState().name; }
		Object& get(const std::string& name) { return game.getObject(name); }
	};

	float length(const Vector2f& v) { return std::hypot(v.x, v.y); }
}

TEST_CASE("two circles touch as circles, not as their boxes", "[knock]")
{
	// Their boxes overlap at the corner; the circles are well apart.
	const Box a{ { 0, 0 }, { 20, 20 } };
	const Box b{ { 17, 17 }, { 20, 20 } };
	CHECK(boxesOverlap(a, b).has_value());
	CHECK_FALSE(shapesOverlap(a, true, b, true).has_value());

	// Middles 15 apart, radii 10 each: touching, the first left of the second.
	CHECK(shapesOverlap(Box{ { 0, 0 }, { 20, 20 } }, true, Box{ { 15, 0 }, { 20, 20 } }, true) == Edge::Left);
	CHECK(shapesOverlap(Box{ { 15, 0 }, { 20, 20 } }, true, Box{ { 0, 0 }, { 20, 20 } }, true) == Edge::Right);
}

TEST_CASE("two circles are swept to the moment their middles are two radii apart", "[knock]")
{
	// Head on: middles 50 apart, 20 to close, a move of 40.
	const auto hit = sweepShapes(Box{ { 0, 0 }, { 20, 20 } }, true, { 40, 0 }, Box{ { 50, 0 }, { 20, 20 } }, true, { 0, 0 });
	REQUIRE(hit.has_value());
	CHECK(hit->time == Approx(30.0f / 40.0f));
	CHECK(hit->edgeOfSecond == Edge::Left);

	// Going by on a slant, the middles never nearer than 21: no touch,
	// though the boxes would clip corners.
	CHECK(sweepShapes(Box{ { 0, 0 }, { 20, 20 } }, false, { 100, 100 }, Box{ { 35, 65 }, { 20, 20 } }, false, { 0, 0 }).has_value());
	CHECK_FALSE(sweepShapes(Box{ { 0, 0 }, { 20, 20 } }, true, { 100, 100 }, Box{ { 35, 65 }, { 20, 20 } }, true, { 0, 0 }).has_value());
	// Moving apart: none either.
	CHECK_FALSE(sweepShapes(Box{ { 0, 0 }, { 20, 20 } }, true, { -10, 0 }, Box{ { 21, 0 }, { 20, 20 } }, true, { 0, 0 }).has_value());
}

TEST_CASE("a contact says where two things meet, the way apart, where along each side and how deep", "[knock]")
{
	SECTION("two boxes: the middle of the stretch they share")
	{
		// a's bottom on b's top, a over the left part of b.
		const Contact contact = contactOf(Box{ { 0, 0 }, { 20, 10 } }, false, Box{ { 10, 10 }, { 40, 10 } }, false, Edge::Top, 0.5f);
		CHECK(contact.point.x == Approx(15.0f));
		CHECK(contact.point.y == Approx(10.0f));
		CHECK(contact.normal.x == Approx(0.0f));
		CHECK(contact.normal.y == Approx(-1.0f));
		CHECK(contact.alongSecond == Approx(5.0f / 40.0f));
		CHECK(contact.alongFirst == Approx(15.0f / 20.0f));
		CHECK(contact.depth == Approx(0.0f));
		CHECK(contact.time == Approx(0.5f));

		const Touch first = touchOfFirst(contact);
		const Touch second = touchOfSecond(contact);
		CHECK(first.edge == Edge::Bottom);
		CHECK(second.edge == Edge::Top);
		CHECK(first.normal.y == Approx(-1.0f));
		CHECK(second.normal.y == Approx(1.0f));
	}
	SECTION("two circles: along the line through their middles")
	{
		// Middles (10, 10) and (26, 22): 20 apart, radii 10 and 10, no depth.
		const Contact contact = contactOf(Box{ { 16, 12 }, { 20, 20 } }, true, Box{ { 0, 0 }, { 20, 20 } }, true, Edge::Right, 0.0f);
		CHECK(contact.normal.x == Approx(0.8f));
		CHECK(contact.normal.y == Approx(0.6f));
		CHECK(contact.point.x == Approx(18.0f));
		CHECK(contact.point.y == Approx(16.0f));
		CHECK(contact.depth == Approx(0.0f).margin(0.0001f));
	}
	SECTION("a circle at a box's corner: out of the corner")
	{
		const Contact contact = contactOf(Box{ { 23, -17 }, { 20, 20 } }, true, Box{ { 0, 0 }, { 30, 30 } }, false, Edge::Top, 0.0f);
		// Middle (33, -7), the corner (30, 0): 3 across, 7 up.
		CHECK(contact.point.x == Approx(30.0f));
		CHECK(contact.point.y == Approx(0.0f));
		CHECK(contact.normal.x == Approx(3.0f / std::sqrt(58.0f)));
		CHECK(contact.normal.y == Approx(-7.0f / std::sqrt(58.0f)));
		CHECK(contact.depth == Approx(10.0f - std::sqrt(58.0f)));
	}
}

TEST_CASE("a knock shares out what two things carry along the line they meet on", "[knock]")
{
	const Vector2f right{ 1, 0 };

	SECTION("equal masses, straight on: the first stops, the second goes on")
	{
		const auto after = knockedApart({ 6, 0 }, 1, { 0, 0 }, 1, right, 1);
		REQUIRE(after.has_value());
		CHECK(after->first.x == Approx(0.0f));
		CHECK(after->second.x == Approx(6.0f));
	}
	SECTION("across the line nothing changes")
	{
		const auto after = knockedApart({ 6, 3 }, 1, { 0, -2 }, 1, right, 1);
		REQUIRE(after.has_value());
		CHECK(after->first.y == Approx(3.0f));
		CHECK(after->second.y == Approx(-2.0f));
	}
	SECTION("heavier, and less bouncy")
	{
		const auto after = knockedApart({ 6, 0 }, 3, { 0, 0 }, 1, right, 0);
		REQUIRE(after.has_value());
		// No bounce: they go on together.
		CHECK(after->first.x == Approx(4.5f));
		CHECK(after->second.x == Approx(4.5f));
	}
	SECTION("one with no mass is as heavy as can be")
	{
		const auto pushed = knockedApart({ 4, 0 }, 0, { 0, 0 }, 1, right, 1);
		REQUIRE(pushed.has_value());
		CHECK(pushed->first.x == Approx(4.0f));
		CHECK(pushed->second.x == Approx(8.0f));

		const auto off = knockedApart({ 4, 0 }, 1, { 0, 0 }, 0, right, 0.5f);
		REQUIRE(off.has_value());
		CHECK(off->first.x == Approx(-2.0f));
		CHECK(off->second.x == Approx(0.0f));
	}
	SECTION("coming apart, or level: nothing")
	{
		CHECK_FALSE(knockedApart({ -1, 0 }, 1, { 0, 0 }, 1, right, 1).has_value());
		CHECK_FALSE(knockedApart({ 2, 0 }, 1, { 2, 0 }, 1, right, 1).has_value());
	}
}

TEST_CASE("a mass is more than 0, and a knock's bounce 0 up to 1", "[knock]")
{
	const auto load = [](const std::string& object)
	{
		const std::filesystem::path path = "descriptions/knock_scratch.xml";
		{
			std::ofstream out(path);
			out << "<game xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"../../xge/xgedef.xsd\">"
				"<window name=\"knock\"><width>800</width><height>600</height><background>color.black</background>"
				"<fullscreen>false</fullscreen><framerate>60</framerate></window>"
				"<variables><variable name=\"unused\">0</variable></variables><objects>" << object << "<object name=\"wall\"><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite>"
				"<position><x>0</x><y>0</y></position></object></objects>"
				"<states><state name=\"playing\"><inputs><input button=\"space\"><pop /></input></inputs></state></states></game>";
		}
		try
		{
			Game game(path.string());
			std::filesystem::remove(path);
		}
		catch (...)
		{
			std::filesystem::remove(path);
			throw;
		}
	};

	CHECK_THROWS_WITH(load("<object name=\"ball\"><sprite><circle><radius>5</radius></circle></sprite><position><x>0</x><y>0</y></position>"
		"<mass>0</mass></object>"), ContainsSubstring("<mass> is 0; expected more than 0"));
	CHECK_THROWS_WITH(load("<object name=\"ball\"><sprite><circle><radius>5</radius></circle></sprite><position><x>0</x><y>0</y></position>"
		"<collisions><enabled>true</enabled><collision object=\"wall\"><knock>2</knock></collision></collisions></object>"),
		ContainsSubstring("<knock> has a bounce of 2"));
	CHECK_NOTHROW(load("<object name=\"ball\"><sprite><circle><radius>5</radius></circle></sprite><position><x>0</x><y>0</y></position>"
		"<mass>2</mass><collisions><enabled>true</enabled><collision object=\"wall\"><knock>0.5</knock></collision></collisions></object>"));
}

TEST_CASE("trickshot.xml: the white ball straight into the rack stops, and the rack takes it on", "[knock][trickshot]")
{
	Played play("descriptions/trickshot.xml");
	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "playing");

	Object& white = play.get("cueball");
	white.velocity = { 8, 0 };

	// From 200 to the apex at 500 less a diameter, slowed by the cloth: about
	// fifty frames. What it carries when it gets there is all the rack gets.
	float carried = 0.0f;
	for (int i = 0; i < 70 && play.get("balls.1").velocity.x == 0.0f; ++i)
	{
		carried = white.velocity.x;
		play.frames(1);
	}
	play.frames(5);
	CHECK(length(white.velocity) < 1.0f);

	// Every ball of the rack was knocked on in that moment, each by the one
	// touching it; nothing was lost but to the cloth.
	const float starts[] = { 500, 521, 521, 542, 542, 542 };
	for (int i = 0; i < 6; ++i)
	{
		CHECK(play.get("balls." + std::to_string(i + 1)).position.x != Approx(starts[i]));
	}
	float total = 0.0f;
	for (const Object& object : play.game.getCurrentObjects())
	{
		if (object.objClass == "ball") { total += object.velocity.x; }
	}
	CHECK(total < carried);
	CHECK(total > carried * 0.8f);
}

TEST_CASE("trickshot.xml: the cue tip has no mass, so it sends the white ball off at twice its speed", "[knock][trickshot]")
{
	Played play("descriptions/trickshot.xml");
	play.tap(KeyCode::Space);

	play.engine.handleKeyPressed(KeyCode::D);
	float fastest = 0.0f;
	for (int i = 0; i < 20; ++i)
	{
		play.frames(1);
		fastest = std::max(fastest, play.get("cueball").velocity.x);
	}
	play.engine.handleKeyReleased(KeyCode::D);
	CHECK(fastest == Approx(8.0f).margin(0.2f));
	CHECK(play.get("cue").velocity.x == Approx(0.0f));
}

TEST_CASE("trickshot.xml: a ball in a pocket scores, and the white one there comes back", "[knock][trickshot]")
{
	Played play("descriptions/trickshot.xml");
	play.tap(KeyCode::Space);

	Object& ball = play.get("balls.4");
	ball.position = { 100, 100 };
	ball.velocity = { -5, -5 };
	play.frames(30);
	CHECK_FALSE(ball.isVisible);
	CHECK(play.get("player").variable.at("score") == Approx(1.0f));

	Object& white = play.get("cueball");
	white.position = { 700, 500 };
	white.velocity = { 5, 5 };
	play.frames(30);
	CHECK(white.isVisible);
	CHECK(white.position.x == Approx(200.0f));
	CHECK(play.get("player").variable.at("score") == Approx(0.0f));
}

namespace
{
	struct Mario : Played
	{
		Mario() : Played("descriptions/mariobros.xml")
		{
			tap(KeyCode::Space);
			REQUIRE(state() == "playing");
			// Only the one on the low left floor walks; the rest are put away.
			for (const char* name : { "creepers.1", "creepers.2", "creepers.3", "creepers.4", "creepers.6" })
			{
				get(name).isVisible = false;
			}
			creeper().velocity.x = 0.0f;
			frames(5);
		}

		Object& mario() { return get("mario"); }
		Object& creeper() { return get("creepers.5"); }
	};
}

TEST_CASE("mariobros.xml: a bump under the floor turns over what stands on it, and kicking it scores", "[knock][mariobros]")
{
	Mario play;
	REQUIRE(play.creeper().lookName() == "walking");
	const float standing = play.creeper().position.y;

	// Under the creeper, on the ground, and up.
	play.mario().position.x = play.creeper().position.x;
	play.frames(2);
	play.tap(KeyCode::Space);

	float highest = standing;
	for (int i = 0; i < 40; ++i)
	{
		play.frames(1);
		highest = std::min(highest, play.creeper().position.y);
	}
	CHECK(play.creeper().lookName() == "flipped");
	CHECK(highest < standing - 10.0f);     // knocked up off the floor
	CHECK(play.mario().variable.at("lives") == Approx(3.0f));

	// It comes down where it was and lies there.
	play.frames(60);
	CHECK(play.creeper().position.y == Approx(standing));
	CHECK(play.creeper().velocity.x == Approx(0.0f));
	CHECK(play.creeper().velocity.y == Approx(0.0f));

	// Up on its floor and into it: kicked off.
	play.mario().position = { play.creeper().position.x - 60.0f, 452.0f - play.mario().size.y };
	play.mario().velocity = {};
	play.frames(2);
	play.engine.handleKeyPressed(KeyCode::D);
	play.frames(40);
	play.engine.handleKeyReleased(KeyCode::D);
	CHECK_FALSE(play.creeper().isVisible);
	CHECK(play.mario().variable.at("score") == Approx(800.0f));
	CHECK(play.mario().variable.at("lives") == Approx(3.0f));
}

TEST_CASE("mariobros.xml: a shellcreeper on its feet costs a life, and a bump reaches only its own tile", "[knock][mariobros]")
{
	Mario play;

	// A bump two tiles along does not reach it.
	play.mario().position.x = play.creeper().position.x + 90.0f;
	play.frames(2);
	play.tap(KeyCode::Space);
	play.frames(80);
	CHECK(play.creeper().lookName() == "walking");

	// Walked into on its feet.
	play.mario().position = { play.creeper().position.x - 60.0f, 452.0f - play.mario().size.y };
	play.mario().velocity = {};
	play.frames(2);
	play.engine.handleKeyPressed(KeyCode::D);
	play.frames(40);
	play.engine.handleKeyReleased(KeyCode::D);
	CHECK(play.creeper().isVisible);
	CHECK(play.mario().variable.at("lives") == Approx(2.0f));
}
