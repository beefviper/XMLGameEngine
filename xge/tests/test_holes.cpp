// test_holes.cpp
// XML Game Engine
// author: beefviper
// date: Oct 17, 2026
//
// Catch2 tests for <hole>: a rule that cuts a round hole in an object's own
// picture where something hit it, which a pixel collision then lets the next
// thing through; then the shields of games/descriptions/spaceinvaders.xml.

#include "played.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <string>

using namespace xge;
using namespace played;
using Catch::Approx;
using Catch::Matchers::ContainsSubstring;

namespace
{
	// A wall, 40 by 40 solid pixels at 100, 100, that holes are cut in.
	std::string wall(const std::string& rule, const std::string& extra = "")
	{
		std::string rows;
		for (int i = 0; i < 10; ++i) { rows += "<row>**********</row>"; }
		return "<object name=\"wall\"><sprite><bitmap>" + rows + "<scale>4</scale><color>color.green</color></bitmap></sprite>"
			"<position><x>100</x><y>100</y></position>" + extra + "<collisions><enabled>true</enabled><type>pixel</type>" + rule + "</collisions></object>";
	}

	int solidPixels(const Bitmap& bitmap)
	{
		int count = 0;
		for (int y = 0; y < bitmap.height; ++y)
		{
			for (int x = 0; x < bitmap.width; ++x) { count += bitmap.solidAt(x, y) ? 1 : 0; }
		}
		return count;
	}

	int solidPixels(const Object& object) { return solidPixels(*object.bitmap); }
}

TEST_CASE("a <hole> is cut round the middle of what hit, and the next shot goes through it", "[hole]")
{
	const std::string shot = box("shot", 115, 300, "<velocity><x>0</x><y>-8</y></velocity><collisions><enabled>true</enabled>"
		"<collision object=\"wall\"><reset /></collision></collisions>");
	const Scratch file(wall("<collision object=\"shot\"><hole><radius>8</radius></hole></collision>") + shot
		+ box("other", 600, 100), "<reset object=\"wall\" />");
	Played play(file.path);
	const auto whole = play.get("wall").bitmap;
	REQUIRE(solidPixels(play.get("wall")) == 1600);

	// It hits the bottom of the wall, a hole is cut there, and it is put back.
	play.frames(30);
	const Object& hit = play.get("wall");
	CHECK(hit.bitmap != whole);
	CHECK(hit.unholed == whole);
	CHECK_FALSE(hit.bitmap->solidAt(20, 39));
	CHECK(hit.bitmap->solidAt(20, 20));
	CHECK(hit.bitmap->solidAt(2, 39));
	const int afterOne = solidPixels(hit);
	CHECK(afterOne < 1600);
	CHECK(afterOne > 1600 - 260);

	// Every shot up the same line goes a little deeper, until one goes
	// right through and out of the top.
	bool through = false;
	for (int i = 0; i < 300 && !through; ++i)
	{
		play.frames(1);
		through = play.get("shot").position.y < 90.0f;
	}
	CHECK(through);
	CHECK(solidPixels(play.get("wall")) < afterOne);

	// The picture the wall started with was never touched, and a reset puts it back.
	CHECK(solidPixels(*whole) == 1600);
	play.tap(KeyCode::Space);
	CHECK(play.get("wall").bitmap == whole);
	CHECK_FALSE(play.get("wall").unholed);
	CHECK(solidPixels(play.get("wall")) == 1600);
}

TEST_CASE("a rough hole keeps some pixels at its edge, never more than it would have had", "[hole]")
{
	const auto cut = [](const std::string& rough)
	{
		const Scratch file(wall("<collision object=\"shot\"><hole><radius>12</radius>" + rough + "</hole></collision>")
			+ box("shot", 115, 115, "<velocity><x>0</x><y>-1</y></velocity><collisions><enabled>true</enabled></collisions>"), "<pop />");
		Played play(file.path);
		play.frames(1);
		return 1600 - solidPixels(play.get("wall"));
	};

	const int smooth = cut("");
	const int rough = cut("<rough>0.8</rough>");
	CHECK(smooth == Approx(3.14159 * 144).epsilon(0.1));
	CHECK(rough < smooth);
	CHECK(rough > 0);
}

TEST_CASE("a <hole> is cut only in a picture that stays as it is", "[hole]")
{
	const auto load = [](const std::string& objects)
	{
		const Scratch file(objects, "<pop />");
		Game game(file.path);
	};

	CHECK_THROWS_WITH(load(box("wall", 0, 0, "<collisions><enabled>true</enabled><collision object=\"shot\"><hole><radius>4</radius></hole></collision></collisions>") + box("shot", 50, 50)),
		ContainsSubstring("<hole> cuts a picture that stays as it is"));
	CHECK_THROWS_WITH(load(wall("<collision object=\"shot\"><hole><radius>0</radius></hole></collision>") + box("shot", 300, 300)), ContainsSubstring("<radius> of 0"));
	CHECK_THROWS_WITH(load(wall("<collision object=\"shot\"><hole><radius>3</radius><rough>2</rough></hole></collision>") + box("shot", 300, 300)), ContainsSubstring("<rough> of 2"));
	CHECK_THROWS_WITH(load(wall("<collision object=\"shot\"><hole><radius>3</radius></hole></collision>", "<heading>0</heading>") + box("shot", 300, 300)),
		ContainsSubstring("<hole> cuts a picture"));
}

TEST_CASE("spaceinvaders.xml: shots wear holes in a shield until they pass through it", "[hole][spaceinvaders]")
{
	Played play("descriptions/spaceinvaders.xml");
	play.tap(KeyCode::Space);
	REQUIRE(play.state() == "playing");

	// Four shields, none of them hit yet; the second stands over x 349 to 437.
	Object& shield = play.get("shields.2.1");
	REQUIRE(play.get("shields.4.1").isVisible);
	const auto whole = shield.bitmap;
	CHECK(shield.position.x == Approx(349));

	// Move the ship under the shield's middle and fire up into it.
	Object& ship = play.get("player");
	ship.position.x = shield.position.x + shield.size.x / 2 - ship.size.x / 2;
	bool through = false;
	for (int shot = 0; shot < 20 && !through; ++shot)
	{
		play.tap(KeyCode::Space);
		for (int i = 0; i < 70 && play.get("bullet").isVisible; ++i)
		{
			play.frames(1);
			through = through || play.get("bullet").position.y + play.get("bullet").size.y < shield.position.y;
		}
		ship.position.x = shield.position.x + shield.size.x / 2 - ship.size.x / 2;
	}
	CHECK(play.get("shields.2.1").bitmap != whole);
	CHECK(through);
}
