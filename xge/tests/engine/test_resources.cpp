// test_resources.cpp
// XML Game Engine
// author: beefviper
// date: Nov 17, 2026
//
// A game's Resources: each picture held once, however many objects show it.

#include "played.h"
#include "engine/resources.h"
#include "picture/image.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

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
		CHECK(play.get("thing0").drawnPicture() == play.get("thing19").drawnPicture());
	}
	{
		played::Scratch game{ alike(20, image, "<image><path>assets/paddle.jpg</path><flip>horizontal</flip></image>", 1), "" };
		played::Played play{ game.path };
		CHECK(play.game.resources().pictureCount() == 2);
		const std::shared_ptr<const Bitmap> flipped = play.get("thing0").drawnPicture();
		CHECK(flipped->rgba == flipBitmap(*play.get("thing1").drawnPicture(), true, false).rgba);
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
