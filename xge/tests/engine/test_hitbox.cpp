// test_hitbox.cpp
// XML Game Engine
// author: beefviper
// date: Oct 18, 2026
//
// Catch2 tests for what an object touches with: only some characters of
// its bitmap (<solid char>), or a shape of its own (<hitbox>), as Q*bert's
// cubes touch by their tops and Q*bert by his feet.

#include "scratch.h"
#include "engine/engine.h"
#include "game/game.h"
#include "picture/bitmap.h"
#include "play/executor.h"
#include "window/null.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace xge;
using Catch::Matchers::ContainsSubstring;

namespace
{
	// A game played with no window, each shape the size its sprite implies.
	struct Played
	{
		Game game;
		Engine engine;

		explicit Played(const std::string& file) : game(file), engine(game, std::make_unique<NullWindow>())
		{
			for (auto& object : game.getCurrentObjects())
			{
				object.size = measureShapeSize(object.picture);
			}
		}

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { engine.step(); }
		}

		// The probe's action of that name, as a key would run it.
		void probe(const std::string& action)
		{
			CommandExecutor executor(game);
			executor.executeInput(CmdTriggerAction{ "probe", action }, true);
			frames(1);
		}

		float hits() { return game.getObject("target").variable.at("hits"); }
	};

	// A game of one state with these objects, written out to be played.
	class Scratch
	{
	public:
		explicit Scratch(const std::string& objects)
		{
			std::ofstream out(path);
			out << "<game xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"../xgedef.xsd\">"
				"<window name=\"hitbox\"><width>800</width><height>600</height><background>color.black</background>"
				"<framerate>60</framerate></window>"
				"<variables><variable name=\"unused\">0</variable></variables>"
				"<states><state name=\"playing\"><objects>" << objects << "</objects>"
				"<inputs><input button=\"space\"><pop /></input></inputs></state></states></game>";
		}
		~Scratch() { std::filesystem::remove(path); }
		Scratch(const Scratch&) = delete;
		Scratch& operator=(const Scratch&) = delete;

		const std::string path = scratchName("descriptions/hitbox_scratch.xml");
	};

	// A 10 by 10 box at 105, 200 that hops straight up by each of `hops`
	// (actions "1", "2", ...).
	std::string probe(const std::vector<int>& hops)
	{
		std::string actions;
		for (std::size_t i = 0; i < hops.size(); ++i)
		{
			actions += "<action name=\"" + std::to_string(i + 1) + "\"><hop direction=\"up\">" + std::to_string(hops[i]) + "</hop></action>";
		}
		return "<object name=\"probe\"><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite>"
			"<position><x>105</x><y>200</y></position><collisions></collisions><actions>" + actions + "</actions></object>";
	}

	// At 100, 100, counting the probe's touches, with `collisions` before its rule.
	std::string target(const std::string& sprite, const std::string& collisions)
	{
		return "<object name=\"target\"><sprite>" + sprite + "</sprite><position><x>100</x><y>100</y></position>"
			"<collisions>" + collisions + "<collision object=\"probe\"><inc variable=\"target.hits\" /></collision></collisions>"
			"<variables><variable name=\"hits\">0</variable></variables></object>";
	}

	// 20 by 20: a top half of '*' and a bottom half of 'b'.
	const std::string twoInks = "<bitmap><row>****</row><row>****</row><row>bbbb</row><row>bbbb</row><scale>5</scale>"
		"<color>color.yellow</color><color char=\"b\">color.brown</color></bitmap>";

	void load(const std::string& objects)
	{
		const Scratch bad(objects);
		Game game(bad.path);
	}
}

TEST_CASE("a bitmap's touching pixels are those of the characters named solid, flipped and turned with it", "[hitbox]")
{
	const Bitmap picture = rasterizeRows({ "*b", "b*" }, 1, colorFromName("color.white"), { { 'b', colorFromName("color.brown") } }, "*");
	CHECK(picture.solidAt(1, 0));
	CHECK_FALSE(picture.touchesAt(1, 0));
	CHECK(picture.touchesAt(0, 0));
	CHECK(picture.touchesAt(1, 1));

	const Bitmap flipped = flipBitmap(picture, true, false);
	CHECK(flipped.touchesAt(1, 0));
	CHECK_FALSE(flipped.touchesAt(0, 0));

	const Bitmap turned = turnBitmap(rasterizeRows({ "**", "bb" }, 2, colorFromName("color.white"), { { 'b', colorFromName("color.brown") } }, "*"), 180.0f);
	const int bottom = turned.height / 2 + 1;
	CHECK(turned.solidAt(turned.width / 2, bottom));
	CHECK(turned.touchesAt(turned.width / 2, bottom));       // the '*'s, now at the bottom
	CHECK_FALSE(turned.touchesAt(turned.width / 2, bottom - 2));

	// Without any, every drawn pixel touches.
	CHECK(rasterizeRows({ "*b" }, 1, colorFromName("color.white"), { { 'b', colorFromName("color.brown") } }).touchesAt(1, 0));
}

TEST_CASE("<solid char>: only those characters of a pixel-tested bitmap touch", "[hitbox]")
{
	const Scratch file(probe({ 89, 20 }) + target(twoInks, "<type>pixel</type><solid char=\"*\" />"));
	Played play(file.path);

	play.probe("1"); // into the bottom half alone, the brown
	CHECK(play.game.getObject("probe").position.y == 111);
	CHECK(play.hits() == 0);

	play.probe("2"); // up into the top half
	CHECK(play.hits() == 1);

	CHECK_THROWS_WITH(load(target(twoInks, "<solid char=\"*\" />")), ContainsSubstring("needs <type>pixel</type>"));
	CHECK_THROWS_WITH(load(target(twoInks, "<type>pixel</type><solid char=\"q\" />")), ContainsSubstring("<solid char=\"q\"> names no character of the bitmap"));
	CHECK_THROWS_WITH(load(target("<rectangle><width>20</width><height>20</height></rectangle>", "<type>pixel</type><solid char=\"*\" />")),
		ContainsSubstring("every sprite of the object must be one"));
}

TEST_CASE("<hitbox>: an object touches with its own shape, placed from its picture's top left", "[hitbox]")
{
	// A 20 by 20 man who touches only with his feet: 10 by 4 at 5, 16.
	const std::string feet = "<hitbox><rectangle><width>10</width><height>4</height></rectangle><position><x>5</x><y>16</y></position></hitbox>";
	const std::string man = "<rectangle><width>20</width><height>20</height></rectangle>";
	{
		const Scratch file(probe({ 99, 10 }) + target(man, feet));
		Played play(file.path);

		play.probe("1"); // over his head, 101 to 111
		CHECK(play.hits() == 0);
		play.probe("2"); // 91 to 101: off the top of him altogether
		CHECK(play.hits() == 0);
	}
	{
		const Scratch file(probe({ 87 }) + target(man, feet));
		Played play(file.path);

		play.probe("1"); // 113 to 123, over his feet at 116 to 120
		CHECK(play.hits() == 1);
	}
	{
		// A round one: a circle of radius 3 at his middle's corner.
		const Scratch file(probe({ 87 }) + target(man, "<hitbox><circle><radius>3</radius></circle><position><x>0</x><y>0</y></position></hitbox>"));
		Played play(file.path);
		play.probe("1");
		CHECK(play.hits() == 0);
	}

	CHECK_THROWS_WITH(load(target(man, "<type>pixel</type>" + feet)), ContainsSubstring("cannot go with <type>pixel</type>"));
	CHECK_THROWS_WITH(load(target(man, "<hitbox><rectangle><width>0</width><height>4</height></rectangle></hitbox>")), ContainsSubstring("bigger than nothing"));
}

TEST_CASE("<hitbox> pulled in from the sides: the most particular word for a side wins, and left and right swap while mirrored", "[hitbox]")
{
	const std::string man = "<rectangle><width>20</width><height>20</height></rectangle>"; // 100 to 120 each way
	const auto hitsWith = [&](const std::string& hitbox, int hop)
	{
		const Scratch file(probe({ hop }) + target(man, "<hitbox>" + hitbox + "</hitbox>"));
		Played play(file.path);
		play.probe("1");
		return play.hits();
	};

	// Up by 87 the probe covers 113 to 123, his bottom 7 pixels.
	CHECK(hitsWith("<all>2</all>", 87) == 1);
	CHECK(hitsWith("<all>8</all>", 87) == 0);
	CHECK(hitsWith("<vertical>8</vertical>", 87) == 0);
	CHECK(hitsWith("<horizontal>8</horizontal>", 87) == 1);
	CHECK(hitsWith("<all>8</all><bottom>1</bottom>", 87) == 1); // a side over all
	CHECK(hitsWith("<vertical>1</vertical><bottom>8</bottom>", 87) == 0); // a side over vertical
	CHECK(hitsWith("<all>1</all><vertical>8</vertical>", 87) == 0); // vertical over all
	CHECK(hitsWith("<top>15</top>", 87) == 1);
	// Up by 99 it covers 101 to 111, his top 11 pixels.
	CHECK(hitsWith("<top>10</top>", 99) == 1);
	CHECK(hitsWith("<top>12</top>", 99) == 0);

	{
		// The probe at 101 to 111 across; his box from 112 across, or, mirrored, to 108.
		const Scratch file(probe({ 87 }) + target(man, "<hitbox><left>12</left></hitbox>"));
		Played play(file.path);
		play.game.getObject("probe").position.x = 101.0f;
		play.probe("1");
		CHECK(play.hits() == 0);

		const Scratch again(probe({ 87 }) + target(man, "<hitbox><left>12</left></hitbox>"));
		Played mirrored(again.path);
		mirrored.game.getObject("probe").position.x = 101.0f;
		mirrored.game.getObject("target").mirrored = true;
		mirrored.probe("1");
		CHECK(mirrored.hits() == 1);
	}

	CHECK_THROWS_WITH(load(target(man, "<hitbox><all>-2</all></hitbox>")), ContainsSubstring("never out"));
	CHECK_THROWS_WITH(load(target(man, "<hitbox />")), ContainsSubstring("<hitbox>"));
}

TEST_CASE("<hitbox> written more than once: touching any of them is a touch", "[hitbox]")
{
	const std::string man = "<rectangle><width>20</width><height>20</height></rectangle>"; // 100 to 120 each way
	const std::string body = "<hitbox><all>8</all></hitbox>"; // 108 to 112
	const std::string foot = "<hitbox><rectangle><width>10</width><height>4</height></rectangle><position><x>5</x><y>16</y></position></hitbox>";
	const auto hitsWith = [&](const std::string& hitboxes, int hop)
	{
		const Scratch file(probe({ hop }) + target(man, hitboxes));
		Played play(file.path);
		play.probe("1");
		return play.hits();
	};

	// Up by 87 the probe covers 113 to 123: past the body, over the foot at 116 to 120.
	CHECK(hitsWith(body, 87) == 0);
	CHECK(hitsWith(body + foot, 87) == 1);
	CHECK(hitsWith(foot + body, 87) == 1);
	// Up by 99 it covers 101 to 111: the body, not the foot.
	CHECK(hitsWith(foot, 99) == 0);
	CHECK(hitsWith(body + foot, 99) == 1);
}

TEST_CASE("<hitbox>: what is stood on, stopped at or bounced off is met by the whole picture", "[hitbox]")
{
	// Up by 87 the probe covers 113 to 123: the man's bottom 7 pixels, under his hitbox.
	const std::string man = "<rectangle><width>20</width><height>20</height></rectangle>";
	const std::string pulledIn = "<hitbox><bottom>8</bottom></hitbox>";
	{
		const Scratch file(probe({ 87 }) + target(man, pulledIn));
		Played play(file.path);
		play.probe("1");
		CHECK(play.hits() == 0);
	}
	{
		std::string stopping = probe({ 87 });
		const std::string none = "<collisions></collisions>";
		stopping.replace(stopping.find(none), none.size(), "<collisions><collision object=\"target\"><stop /></collision></collisions>");
		const Scratch file(stopping + target(man, pulledIn));
		Played play(file.path);
		play.probe("1");
		CHECK(play.hits() == 1);
	}
}
