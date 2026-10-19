// test_collision_rules.cpp
// XML Game Engine
// author: beefviper
// date: Sept 29, 2026
//
// Catch2 tests for object-against-object <collision> rules, loaded from the
// shipped games by a real xge::Game. A rule can be written with just a
// class (or object) to apply to - <collision class="aliens"><die /></collision> -
// and a <collision> with no selector at all is the rule that matches anything.
//
// Rules can also <trigger> an action (of the object itself when object= is
// left out), hold <check>s that must all hold, and name the side= touched.
//
// Regression covered: Space Invaders' bullet used an unfiltered collision
// rule, so it died the moment it overlapped the ship that fired it.

#include "game/game.h"
#include "load/xml.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

using namespace xge;

TEST_CASE("a collision with no selector still matches anything", "[collision_rules]")
{
	Game game{ "descriptions/pong_min.xml" };
	const Object& ball = game.getObject("ball");

	// pong_min.xml's ball: <collision><deflect>45</deflect></collision>
	REQUIRE(ball.collisionData.others.size() == 1);
	CHECK(ball.collisionData.others[0].filterClass.empty());
	CHECK(ball.collisionData.others[0].filterObject.empty());
}

TEST_CASE("a collision naming only a class is an object rule for that class", "[collision_rules]")
{
	Game game{ "descriptions/spaceinvaders.xml" };
	const Object& bullet = game.getObject("bullet");

	// spaceinvaders.xml's bullet: <collision class="aliens"><die /></collision>,
	// then the same for the shields, the bombs and the mystery ship
	REQUIRE(bullet.collisionData.others.size() == 4);
	CHECK(bullet.collisionData.others[0].filterClass == "aliens");
	CHECK(bullet.collisionData.others[0].filterObject.empty());
	CHECK(bullet.collisionData.others[1].filterClass == "shield");

	// ...and it did not also turn into a screen-edge rule.
	CHECK(bullet.collisionData.left.size() == 1); // only the edge="all" <die />
}

TEST_CASE("a bullet fired from the ship survives touching it", "[collision_rules]")
{
	Game game{ "descriptions/spaceinvaders.xml" };
	game.setCurrentState("playing");

	Object& player = game.getObject("player");
	Object& bullet = game.getObject("bullet");
	player.size = { 50.0f, 50.0f };
	bullet.size = { 8.0f, 8.0f };

	// The engine spawns a bullet with its top-left corner at the ship's top
	// edge, so it starts out overlapping the ship and only clears it a few
	// frames later (6 px a frame, 8 px tall).
	bullet.position = { player.position.x + 25.0f, player.position.y };
	bullet.velocity.y = -6.0f;
	bullet.isVisible = true;
	bullet.collisionData.enabled = true;

	for (int frame = 0; frame < 4; ++frame)
	{
		game.updateObjects();
		CHECK(bullet.collisionData.enabled);
		CHECK(bullet.isVisible);
	}
	CHECK(bullet.position.y < player.position.y - bullet.size.y);
}

TEST_CASE("a bullet dies on touching an alien", "[collision_rules]")
{
	Game game{ "descriptions/spaceinvaders.xml" };
	game.setCurrentState("playing");

	Object& bullet = game.getObject("bullet");
	Object& alien = game.getObject("aliens");
	alien.size = { 50.0f, 50.0f };
	bullet.size = { 8.0f, 8.0f };

	// Just under the alien, its centre a couple of pixels below the bottom
	// edge, so it overlaps by less than its radius.
	bullet.position = { alien.position.x + 20.0f, alien.position.y + alien.size.y - 2.0f };
	bullet.velocity.y = -6.0f;
	bullet.isVisible = true;
	bullet.collisionData.enabled = true;

	game.updateObjects();

	CHECK_FALSE(bullet.collisionData.enabled);
}

namespace
{
	// A ball falling onto a block, its rule about the block given; the game
	// written beside the shipped ones for as long as it lives.
	struct Falling
	{
		std::filesystem::path path = "descriptions/rules_scratch.xml";
		std::optional<Game> game;

		Falling(const std::string& rule, const std::string& armed, XmlBackend backend = XmlBackend::Xerces)
		{
			{
				std::ofstream out(path);
				out << "<game xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"../../xge/xgedef.xsd\">"
					"<window name=\"rules\"><width>800</width><height>600</height><background>color.black</background>"
					"<fullscreen>false</fullscreen><framerate>60</framerate></window>"
					"<variables><variable name=\"unused\">0</variable></variables><objects>"
					"<object name=\"ball\"><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite>"
					"<position><x>100</x><y>0</y></position><velocity><y>5</y></velocity>"
					"<collisions><enabled>true</enabled>" << rule << "</collisions>"
					"<actions><action name=\"hit\"><reset /></action></actions>"
					"<variables><variable name=\"armed\">" << armed << "</variable></variables></object>"
					"<object name=\"block\" class=\"block\"><sprite><rectangle><width>100</width><height>20</height></rectangle></sprite>"
					"<position><x>50</x><y>100</y></position><collisions><enabled>true</enabled></collisions></object>"
					"</objects><states><state name=\"playing\"><shows><show object=\"ball\" /><show object=\"block\" /></shows>"
					"<inputs><input button=\"space\"><pop /></input></inputs></state></states></game>";
			}
			game.emplace(readGame(path.string(), backend));
			game->setCurrentState(0);
			for (auto& object : game->getCurrentObjects()) { object.size = measureShapeSize(object.spriteParams, object.shapeKind); }
		}
		Falling(const Falling&) = delete;
		Falling& operator=(const Falling&) = delete;
		~Falling()
		{
			std::error_code ignored;
			std::filesystem::remove(path, ignored);
		}

		// Where the ball is after `frames` frames.
		float after(int frames)
		{
			for (int i = 0; i < frames; ++i) { game->updateObjects(); }
			return game->getObject("ball").position.y;
		}
	};
}

TEST_CASE("a collision can <trigger> an action of the object itself, left to mean it", "[collision_rules]")
{
	auto backend = GENERATE(XmlBackend::Xerces, XmlBackend::TinyXml2);
	Falling falling("<collision class=\"block\"><trigger action=\"hit\" /></collision>", "1", backend);
	// It reaches the block at y 90 in frame 18 and is put back at the top.
	CHECK(falling.after(18) < 10.0f);
}

TEST_CASE("a collision rule with a <check> runs only while the check holds", "[collision_rules]")
{
	const std::string rule = "<collision class=\"block\"><check variable=\"armed\"><atleast>1</atleast></check><trigger action=\"hit\" /></collision>";
	SECTION("its own variable holds: the rule runs")
	{
		Falling falling(rule, "1");
		CHECK(falling.after(18) < 10.0f);
	}
	SECTION("it does not: the ball falls on through")
	{
		Falling falling(rule, "0");
		CHECK(falling.after(18) > 80.0f);
	}
}

TEST_CASE("a collision rule with side= runs only when touched on that side of its own", "[collision_rules]")
{
	SECTION("the ball's bottom meets the block")
	{
		Falling falling("<collision class=\"block\" side=\"bottom\"><trigger action=\"hit\" /></collision>", "1");
		CHECK(falling.after(18) < 10.0f);
	}
	SECTION("a rule for its top passes it over")
	{
		Falling falling("<collision class=\"block\" side=\"top\"><trigger action=\"hit\" /></collision>", "1");
		CHECK(falling.after(18) > 80.0f);
	}
	SECTION("vertical is top or bottom")
	{
		Falling falling("<collision class=\"block\" side=\"vertical\"><trigger action=\"hit\" /></collision>", "1");
		CHECK(falling.after(18) < 10.0f);
	}
}

TEST_CASE("a <trigger> with no object= outside an object's own lists stops the load", "[collision_rules]")
{
	const std::filesystem::path path = "descriptions/rules_scratch.xml";
	{
		std::ofstream out(path);
		out << "<game xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"../../xge/xgedef.xsd\">"
			"<window name=\"rules\"><width>800</width><height>600</height><background>color.black</background>"
			"<fullscreen>false</fullscreen><framerate>60</framerate></window>"
			"<variables><variable name=\"unused\">0</variable></variables><objects>"
			"<object name=\"ball\"><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite>"
			"<position><x>100</x><y>0</y></position><actions><action name=\"hit\"><reset /></action></actions></object>"
			"</objects><states><state name=\"playing\"><inputs><input button=\"space\"><trigger action=\"hit\" /></input></inputs></state></states></game>";
	}
	CHECK_THROWS_WITH(Game(readGame(path.string(), XmlBackend::Xerces)), Catch::Matchers::ContainsSubstring("needs object="));
	std::error_code ignored;
	std::filesystem::remove(path, ignored);
}
