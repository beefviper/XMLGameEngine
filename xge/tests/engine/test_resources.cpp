// test_resources.cpp
// XML Game Engine
// author: beefviper
// date: Nov 17, 2026
//
// A game's Resources: each picture held once, however many objects show it.

#include "played.h"
#include "engine/engine.h"
#include "engine/resources.h"
#include "window/null.h"
#include "picture/image.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <memory>
#include <string>

using Catch::Matchers::ContainsSubstring;
using namespace xge;

namespace
{
	// `count` objects in a row showing the same sprite, the first `odd`
	// times with `oddSprite` in its place.
	std::string alike(int count, const std::string& sprite, const std::string& oddSprite = "", int odd = 0)
	{
		std::string objects;
		for (int i = 0; i < count; ++i)
		{
			objects += "<object name=\"thing" + std::to_string(i) + "\"><sprite>" + (i < odd ? oddSprite : sprite) + "</sprite>"
				"<position><x>" + std::to_string(i * 20) + "</x><y>10</y></position></object>";
		}
		return objects;
	}
}

TEST_CASE("a picture file is read once, whoever asks for it", "[resources]")
{
	Resources resources;
	const PictureId paddle = resources.pictureFile("assets/paddle.jpg");
	CHECK(paddle);
	CHECK(resources.pictureFile("assets/paddle.jpg") == paddle);
	CHECK(resources.pictureCount() == 1);
	CHECK(resources.bitmap(paddle).width == 30);
	CHECK(resources.pictureBytes() == 30u * 150u * 4u);

	const PictureId ball = resources.pictureFile("assets/cards.png");
	CHECK(ball != paddle);
	CHECK(resources.pictureCount() == 2);

	CHECK_THROWS_WITH(resources.pictureFile("assets/no-such-picture.png"), ContainsSubstring("assets/no-such-picture.png"));
	CHECK(resources.pictureCount() == 2);
}

TEST_CASE("objects showing one image file share one picture, and a flip is another", "[resources]")
{
	const std::string image = "<image><path>assets/paddle.jpg</path></image>";
	{
		played::Scratch game{ alike(20, image), "" };
		played::Played play{ game.path };
		CHECK(play.game.resources().pictureCount() == 1);
		CHECK(play.get("thing0").drawn.picture->image == play.get("thing19").drawn.picture->image);
	}
	{
		played::Scratch game{ alike(20, image, "<image><path>assets/paddle.jpg</path><flip>horizontal</flip></image>", 1), "" };
		played::Played play{ game.path };
		CHECK(play.game.resources().pictureCount() == 2);
		const PictureId flipped = play.get("thing0").drawn.picture->image;
		CHECK(flipped->rgba == flipBitmap(*play.get("thing1").drawn.picture->image, true, false).rgba);
	}
}

TEST_CASE("objects drawn alike share one picture, and a colour makes another", "[resources]")
{
	struct Kind
	{
		const char* name;
		std::string sprite;
		std::string recoloured;
	};
	const Kind kinds[] = {
		{ "rows", "<bitmap><row>*.*</row><row>.*.</row><scale>2</scale><color>color.red</color></bitmap>",
			"<bitmap><row>*.*</row><row>.*.</row><scale>2</scale><color>color.blue</color></bitmap>" },
		{ "lines", "<line><from><x>0</x><y>0</y></from><to><x>6</x><y>4</y></to><color>color.red</color></line>",
			"<line><from><x>0</x><y>0</y></from><to><x>6</x><y>4</y></to><color>color.blue</color></line>" },
		{ "polygons", "<polygon><point><x>0</x><y>0</y></point><point><x>8</x><y>0</y></point><point><x>4</x><y>6</y></point><color>color.red</color></polygon>",
			"<polygon><point><x>0</x><y>0</y></point><point><x>8</x><y>0</y></point><point><x>4</x><y>6</y></point><color>color.blue</color></polygon>" },
	};
	for (const Kind& kind : kinds)
	{
		INFO(kind.name);
		{
			played::Scratch game{ alike(20, kind.sprite), "" };
			played::Played play{ game.path };
			CHECK(play.game.resources().pictureCount() == 1);
			CHECK(play.get("thing0").drawn.bitmap == play.get("thing19").drawn.bitmap);
		}
		{
			played::Scratch game{ alike(20, kind.sprite, kind.recoloured, 1), "" };
			played::Played play{ game.path };
			CHECK(play.game.resources().pictureCount() == 2);
		}
	}
}

TEST_CASE("objects turned to one heading share one turned picture", "[resources]")
{
	std::string objects;
	for (int i = 0; i < 20; ++i)
	{
		objects += "<object name=\"ship" + std::to_string(i) + "\"><sprite><bitmap><row>.*.</row><row>***</row><scale>2</scale></bitmap></sprite>"
			"<position><x>" + std::to_string(40 + i * 20) + "</x><y>40</y></position><heading>90</heading></object>";
	}
	played::Scratch game{ objects, "" };
	played::Played play{ game.path };
	CHECK(play.get("ship0").drawn.bitmap == play.get("ship19").drawn.bitmap);
	// The drawing, it at heading 0 (as it loads) and at 90.
	CHECK(play.game.resources().pictureCount() == 3);
}

TEST_CASE("a <mirror>'s flipped picture is made once, whoever shows it", "[resources]")
{
	std::string objects;
	for (int i = 0; i < 20; ++i)
	{
		objects += "<object name=\"guy" + std::to_string(i) + "\"><sprite><bitmap><row>**.</row><row>*..</row><scale>2</scale></bitmap></sprite>"
			"<position><x>" + std::to_string(40 + i * 20) + "</x><y>40</y></position><mirror>left</mirror></object>";
	}
	played::Scratch game{ objects, "" };
	played::Played play{ game.path };
	// The picture and it flipped.
	CHECK(play.game.resources().pictureCount() == 2);
	CHECK(*play.get("guy0").looks.set->flips == *play.get("guy19").looks.set->flips);
}

TEST_CASE("a hole makes one shield's picture its own, and a reset lets it go", "[resources][hole]")
{
	std::string rows;
	for (int i = 0; i < 10; ++i) { rows += "<row>**********</row>"; }
	const auto shield = [&](const std::string& name, int x)
	{
		return "<object name=\"" + name + "\"><sprite><bitmap>" + rows + "<scale>4</scale><color>color.green</color></bitmap></sprite>"
			"<position><x>" + std::to_string(x) + "</x><y>100</y></position>"
			"<collisions><type>pixel</type><collision object=\"shot\"><hole>8</hole></collision></collisions></object>";
	};
	const std::string shot = played::box("shot", 115, 300, "<velocity><x>0</x><y>-480</y></velocity><collisions>"
		"<collision object=\"left\"><reset /></collision></collisions>");
	played::Scratch file{ shield("left", 100) + shield("right", 300) + shot, "<reset object=\"left\" />" };
	played::Played play{ file.path };
	const PictureId whole = play.get("right").drawn.bitmap;
	REQUIRE(play.get("left").drawn.bitmap == whole);
	const std::size_t held = play.game.resources().pictureCount();

	play.frames(30);
	CHECK(play.get("left").drawn.bitmap != whole);
	CHECK(play.get("right").drawn.bitmap == whole);
	CHECK(play.game.resources().pictureCount() == held + 1);

	play.tap(KeyCode::Space);
	CHECK(play.get("left").drawn.bitmap == whole);
	CHECK(play.game.resources().pictureCount() == held);
}

TEST_CASE("a window makes one texture for a picture twenty objects show", "[resources][window]")
{
	played::Scratch game{ alike(20, "<bitmap><row>*.*</row><row>.*.</row><scale>2</scale><color>color.red</color></bitmap>"), "" };
	Game loaded{ game.path };
	auto made = std::make_unique<NullWindow>();
	NullWindow& window = *made;
	Engine engine{ loaded, std::move(made) };
	for (int frame = 0; frame < 10; ++frame) { engine.step(kSixtieth); engine.render(); }
	CHECK(window.texturesMade() == 1);
}

TEST_CASE("a sheet is opened once to cut its cells, and let go once the game has loaded", "[resources][sheet]")
{
	Resources resources;
	Sheet cards;
	cards.name = "cards";
	cards.path = "assets/cards.png";
	cards.columns = 13;
	cards.rows = 5;
	const SheetPicture& opened = resources.sheet(cards);
	CHECK(&resources.sheet(cards) == &opened);
	CHECK(resources.sheetsOpen() == 1);
	resources.letGoOfSheets();
	CHECK(resources.sheetsOpen() == 0);

	Game klondike{ "descriptions/klondike.xml" };
	CHECK(klondike.resources().sheetsOpen() == 0);
	CHECK(klondike.resources().pictureCount() > 52);   // the cards were cut from it
}
