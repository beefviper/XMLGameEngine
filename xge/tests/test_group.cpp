// test_group.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026
//
// Catch2 tests for the <group> tag: one description shared by several
// objects, each <member> giving only what is its own, or each cell of its
// <columns> and <rows>, changed by the <row>, <column> or <cell> that picks it.
// A group is read as one ordinary object per member (named group.1, group.2,
// ... unless the member names itself) or cell (group.column.row), the group's
// name still means every member at once, members of a group with <lockstep>
// move as one block, and what a group, member or cell gets wrong says where. Then both schema checkers, Xerces's real validation
// and this project's own XsdLiteValidator, on the shipped games written with
// groups and on mistakes in them.
//
// Small games are written to a scratch file and loaded by a real xge::Game;
// none of it needs a window.

#include "scratch.h"
#include "engine/folder.h"
#include "game/game.h"
#include "xml/document.h"
#include "load/xsd.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace xge;
using Catch::Matchers::ContainsSubstring;

namespace
{
	// Removes a scratch game file when it goes out of scope, pass or fail.
	struct ScratchFile
	{
		std::filesystem::path path;

		// Moved, never copied: a copy left behind by returning one from a function
		// (which Debug builds do not elide) would delete the file it was written to.
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
		ScratchFile scratch{ std::filesystem::temp_directory_path() / scratchName(name) };
		std::ofstream out(scratch.path);
		out << xml;
		return scratch;
	}

	// A game whose one state shows `shows` (a list of names), around the objects given.
	std::string gameXml(const std::string& objects, const std::string& shows = "<show object=\"lane\" />")
	{
		return "<game>"
			"<window name=\"test\"><width>800</width><height>600</height><background>color.black</background>"
			"<fullscreen>false</fullscreen><framerate>60</framerate></window>"
			"<variables><variable name=\"row\">100</variable></variables>"
			"<objects>" + objects + "</objects>"
			"<states><state name=\"playing\"><shows>" + shows + "</shows>"
			"<inputs><input button=\"space\"><pop /></input></inputs></state></states>"
			"</game>";
	}

	struct Loaded
	{
		ScratchFile file;
		Game game;

		explicit Loaded(const std::string& xml, const std::string& name = "xge_test_group.xml") :
			file(writeScratch(name, xml)),
			game(file.path.string())
		{
		}
	};

	// A lane of three: two members that give only an x, and one with a name and
	// its own row, speed and shape.
	const std::string kLane =
		"<group name=\"lane\" class=\"logs\">"
		"<sprite><rectangle><width>40</width><height>20</height><color>color.brown</color></rectangle></sprite>"
		"<position><y>row</y></position>"
		"<velocity><x>-2</x><y>0</y></velocity>"
		"<collisions><enabled>true</enabled><collision edge=\"horizontal\"><wrap /></collision></collisions>"
		"<variables><variable name=\"weight\">3</variable></variables>"
		"<member><position><x>10</x></position></member>"
		"<member><position><x>200</x></position></member>"
		"<member name=\"odd\">"
		"<sprite><rectangle><width>80</width><height>20</height></rectangle></sprite>"
		"<position><x>300</x><y>150</y></position>"
		"<velocity><x>1</x></velocity>"
		"</member>"
		"</group>";

	// One thing to say about a group, with a complete lane around it to break.
	std::string groupWith(const std::string& parts, const std::string& members)
	{
		return "<group name=\"lane\">" + parts + members + "</group>";
	}

	const std::string kSprite = "<sprite><circle><radius>5</radius></circle></sprite>";
	const std::string kPosition = "<position><x>0</x><y>0</y></position>";
	const std::string kVelocity = "<velocity><x>0</x><y>0</y></velocity>";
	const std::string kCollisions = "<collisions><enabled>false</enabled></collisions>";
	const std::string kMember = "<member />";
}

TEST_CASE("a group is read as one object for each member", "[group]")
{
	Loaded loaded{ gameXml(kLane) };
	const auto& objects = loaded.game.getCurrentObjects();

	REQUIRE(objects.size() == 3);
	CHECK(objects[0].name == "lane.1");
	CHECK(objects[1].name == "lane.2");
	CHECK(objects[2].name == "odd");

	for (const Object& object : objects)
	{
		CHECK(object.objClass == "logs");
		CHECK(object.groupName == "lane");
		CHECK(object.collisionData.enabled);
		CHECK(object.collisionData.edges[edgeIndex(Edge::Left)].commands.size() == 1);
		CHECK(object.collisionData.edges[edgeIndex(Edge::Right)].commands.size() == 1);
		CHECK(object.variable.at("weight") == 3.0f);
	}
}

TEST_CASE("what a game leaves out is nothing: still, and touching nothing", "[group]")
{
	// No <velocity> and no <collisions>, in a group and in an object; a
	// member gives one half of a velocity, an object the other.
	Loaded loaded{ gameXml(groupWith(kSprite + kPosition, "<member /><member><velocity><x>2</x></velocity></member>") +
		"<object name=\"still\">" + kSprite + kPosition + "</object>"
		"<object name=\"falling\">" + kSprite + kPosition + "<velocity><y>3</y></velocity></object>",
		"<show object=\"lane\" /><show object=\"still\" /><show object=\"falling\" />") };

	for (const char* name : { "lane.1", "still" })
	{
		const Object& object = loaded.game.getObject(name);
		CHECK(object.velocity.x == 0.0f);
		CHECK(object.velocity.y == 0.0f);
		CHECK_FALSE(object.collisionData.enabled);
	}
	CHECK(loaded.game.getObject("lane.2").velocity.x == 2.0f);
	CHECK(loaded.game.getObject("lane.2").velocity.y == 0.0f);
	CHECK(loaded.game.getObject("falling").velocity.x == 0.0f);
	CHECK(loaded.game.getObject("falling").velocity.y == 3.0f);
	CHECK_FALSE(loaded.game.getObject("falling").collisionData.enabled);
}

TEST_CASE("a member takes what it leaves out from its group", "[group]")
{
	Loaded loaded{ gameXml(kLane) };
	const Object& first = loaded.game.getObject("lane.1");
	const Object& second = loaded.game.getObject("lane.2");
	const Object& odd = loaded.game.getObject("odd");

	// The group gives the row and the speed, the member its x.
	CHECK(first.position.x == 10.0f);
	CHECK(first.position.y == 100.0f);
	CHECK(second.position.x == 200.0f);
	CHECK(second.position.y == 100.0f);
	CHECK(first.velocity.x == -2.0f);
	CHECK(first.velocity.y == 0.0f);
	CHECK(first.picture == second.picture);

	// The member that says more wins: its own row, its own x for the speed
	// (the group's 0 for y), and its own shape.
	CHECK(odd.position.x == 300.0f);
	CHECK(odd.position.y == 150.0f);
	CHECK(odd.velocity.x == 1.0f);
	CHECK(odd.velocity.y == 0.0f);
	CHECK(odd.picture != first.picture);

	// And where they started is where a reset puts them back.
	CHECK(odd.positionOriginal.y == 150.0f);
	CHECK(odd.velocityOriginal.x == 1.0f);
}

TEST_CASE("the group's name means every member", "[group]")
{
	Loaded loaded{ gameXml(kLane + "<object name=\"other\">" + kSprite + kPosition + kVelocity + kCollisions + "</object>") };
	Game& game = loaded.game;
	game.setCurrentState("playing");

	// A state that shows the group shows all three, and not the object beside it.
	for (const char* name : { "lane.1", "lane.2", "odd" }) { CHECK(game.isShown(game.getObject(name))); }
	CHECK_FALSE(game.isShown(game.getObject("other")));

	// The name alone finds the first member.
	CHECK(&game.getObject("lane") == &game.getObject("lane.1"));

	// <reset object="lane" /> puts every member back where it began.
	for (const char* name : { "lane.1", "lane.2", "odd" })
	{
		game.getObject(name).position.x += 55.0f;
		game.getObject(name).velocity.x = 9.0f;
	}
	game.resetObject("lane");
	CHECK(game.getObject("lane.1").position.x == 10.0f);
	CHECK(game.getObject("lane.2").position.x == 200.0f);
	CHECK(game.getObject("odd").position.x == 300.0f);
	CHECK(game.getObject("odd").velocity.x == 1.0f);

	// One member can be reset, or shown, by its own name.
	game.getObject("lane.2").position.x = 1.0f;
	game.resetObject("lane.2");
	CHECK(game.getObject("lane.2").position.x == 200.0f);
}

TEST_CASE("a state can show one member of a group", "[group]")
{
	Loaded loaded{ gameXml(kLane, "<show object=\"odd\" />") };
	Game& game = loaded.game;
	game.setCurrentState("playing");

	CHECK(game.isShown(game.getObject("odd")));
	CHECK_FALSE(game.isShown(game.getObject("lane.1")));
	CHECK_FALSE(game.isShown(game.getObject("lane.2")));
}

TEST_CASE("groups and plain objects keep the order they are written in", "[group]")
{
	const auto plain = [](const std::string& name)
	{
		return "<object name=\"" + name + "\">" + kSprite + kPosition + kVelocity + kCollisions + "</object>";
	};
	Loaded loaded{ gameXml(plain("first") + kLane + plain("last")) };

	std::vector<std::string> names;
	for (const Object& object : loaded.game.getCurrentObjects()) { names.push_back(object.name); }

	CHECK(names == std::vector<std::string>{ "first", "lane.1", "lane.2", "odd", "last" });
}

TEST_CASE("a group can share actions with every member", "[group]")
{
	const std::string lane = groupWith(kSprite + kPosition + kVelocity + kCollisions
		+ "<actions><action name=\"up\"><move direction=\"up\">3</move></action></actions>",
		"<member name=\"a\" /><member name=\"b\" />");
	Loaded loaded{ gameXml(lane) };

	CHECK(loaded.game.getObject("a").action.count("up") == 1);
	CHECK(loaded.game.getObject("b").action.count("up") == 1);
}

TEST_CASE("members of a group with lockstep move as one block", "[group][lockstep]")
{
	const auto lane = [](const std::string& name, const std::string& lockstep)
	{
		return "<group name=\"" + name + "\">" + kSprite + kPosition + kVelocity
			+ "<collisions><enabled>true</enabled>" + lockstep + "</collisions>"
			"<member /><member /></group>";
	};
	const std::string plainLockstep = "<object name=\"alone\">" + kSprite + kPosition + kVelocity
		+ "<collisions><enabled>true</enabled><lockstep>true</lockstep></collisions></object>";

	Loaded loaded{ gameXml(lane("a", "<lockstep>true</lockstep>") + lane("b", "<lockstep>true</lockstep>")
		+ lane("c", "") + plainLockstep) };

	const int a1 = loaded.game.getObject("a.1").collisionData.lockstep;
	const int a2 = loaded.game.getObject("a.2").collisionData.lockstep;
	const int b1 = loaded.game.getObject("b.1").collisionData.lockstep;
	const int b2 = loaded.game.getObject("b.2").collisionData.lockstep;
	const int alone = loaded.game.getObject("alone").collisionData.lockstep;

	// One number for each block, nothing shared between blocks, none for a
	// group that did not ask.
	CHECK(a1 > 0);
	CHECK(a1 == a2);
	CHECK(b1 > 0);
	CHECK(b1 == b2);
	CHECK(a1 != b1);
	CHECK(alone > 0);
	CHECK(alone != a1);
	CHECK(alone != b1);
	CHECK(loaded.game.getObject("c.1").collisionData.lockstep == 0);
	CHECK(loaded.game.getObject("c.2").collisionData.lockstep == 0);
}

TEST_CASE("a group or member that is incomplete says where", "[group][errors]")
{
	SECTION("a group with no members")
	{
		REQUIRE_THROWS_WITH(Loaded(gameXml(groupWith(kSprite + kPosition + kVelocity + kCollisions, ""))),
			ContainsSubstring("group 'lane'") && ContainsSubstring("has no <member>"));
	}

	SECTION("a group with no name")
	{
		REQUIRE_THROWS_WITH(Loaded(gameXml("<group>" + kSprite + kPosition + kVelocity + kCollisions + kMember + "</group>")),
			ContainsSubstring("<group>") && ContainsSubstring("name="));
	}

	SECTION("nothing gives the sprite")
	{
		REQUIRE_THROWS_WITH(Loaded(gameXml(groupWith(kPosition + kVelocity + kCollisions, kMember))),
			ContainsSubstring("object 'lane.1'") && ContainsSubstring("has no <sprite>, and neither does its group"));
	}

	SECTION("nothing gives one half of the position")
	{
		REQUIRE_THROWS_WITH(Loaded(gameXml(groupWith(kSprite + "<position><y>0</y></position>" + kVelocity + kCollisions,
			"<member /><member><position><x>4</x></position></member>"))),
			ContainsSubstring("object 'lane.1'") && ContainsSubstring("<position><x>"));
	}

	SECTION("something a member cannot say")
	{
		REQUIRE_THROWS_WITH(Loaded(gameXml(groupWith(kSprite + kPosition + kVelocity + kCollisions,
			"<member><collisions><enabled>true</enabled></collisions></member>"))),
			ContainsSubstring("object 'lane.1'") && ContainsSubstring("a member can give <sprite>s, an <animation>, a <position> or a <velocity>"));
	}

	SECTION("something a group cannot say")
	{
		REQUIRE_THROWS_WITH(Loaded(gameXml(groupWith(kSprite + kPosition + kVelocity + kCollisions + "<bogus />", kMember))),
			ContainsSubstring("group 'lane'") && ContainsSubstring("unknown <bogus>"));
	}

	SECTION("a value that is both text and a tag, in a member")
	{
		REQUIRE_THROWS_WITH(Loaded(gameXml(groupWith(kSprite + kPosition + kVelocity + kCollisions,
			"<member><position><x>5 <random><min>1</min><max>2</max></random></x></position></member>"))),
			ContainsSubstring("holds both the text"));
	}
}

// ---------------------------------------------------------------- cells
namespace
{
	// A wall of 3 columns and 2 rows of 20 by 10 red bricks from (100, 50),
	// 5 apart across and 7 down, and whatever rows, columns and cells change.
	std::string wallWith(const std::string& changes, const std::string& rows = "2", const std::string& padding = "<padding><x>5</x><y>7</y></padding>")
	{
		return "<group name=\"wall\" class=\"bricks\"><columns>3</columns><rows>" + rows + "</rows>" + padding +
			"<sprite><rectangle><width>20</width><height>10</height><color>color.red</color></rectangle></sprite>"
			"<position><x>100</x><y>50</y></position><velocity><x>0</x><y>0</y></velocity>"
			"<collisions><enabled>true</enabled></collisions>"
			"<variables><variable name=\"points\">1</variable></variables>" + changes + "</group>";
	}

	const std::string kShowWall = "<show object=\"wall\" />";

	std::string colorOf(const Object& object) { return object.picture.color; }
}

TEST_CASE("a group in columns and rows is a cell for each place, named for its column and row", "[group][cells]")
{
	Loaded loaded{ gameXml(wallWith(""), kShowWall) };
	Game& game = loaded.game;
	const auto& objects = game.getCurrentObjects();

	// The top row first, left to right.
	REQUIRE(objects.size() == 6);
	const std::vector<std::string> names = { "wall.1.1", "wall.2.1", "wall.3.1", "wall.1.2", "wall.2.2", "wall.3.2" };
	for (std::size_t i = 0; i < names.size(); ++i) { CHECK(objects[i].name == names[i]); }
	for (const Object& cell : objects)
	{
		CHECK(cell.groupName == "wall");
		CHECK(cell.objClass == "bricks");
		CHECK(cell.variable.at("points") == 1.0f);
	}

	// A cell's size apart, plus the padding.
	CHECK(game.getObject("wall.1.1").position.x == 100.0f);
	CHECK(game.getObject("wall.1.1").position.y == 50.0f);
	CHECK(game.getObject("wall.3.2").position.x == 100.0f + 2 * (20.0f + 5.0f));
	CHECK(game.getObject("wall.3.2").position.y == 50.0f + 10.0f + 7.0f);
	CHECK(game.tryGetObject("wall.4.1") == nullptr);
	CHECK(game.getObject("wall").name == "wall.1.1");
}

TEST_CASE("a row, a column or a cell changes only what it gives", "[group][cells]")
{
	Loaded loaded{ gameXml(wallWith(
		"<row number=\"2\"><sprite><rectangle><color>color.green</color></rectangle></sprite>"
		"<variables><variable name=\"points\">5</variable></variables></row>"
		"<column number=\"3\"><velocity><x>2</x></velocity></column>"
		"<cell row=\"1\" column=\"2\" name=\"boss\"><sprite><rectangle><width>30</width><height>16</height><color>color.blue</color></rectangle></sprite></cell>"),
		kShowWall) };
	Game& game = loaded.game;

	// The row: green, worth more, the size of the group's bricks.
	CHECK(colorOf(game.getObject("wall.1.2")) == "color.green");
	CHECK(game.getObject("wall.1.2").picture.size.x == 20.0f);
	CHECK(game.getObject("wall.1.2").variable.at("points") == 5.0f);
	CHECK(colorOf(game.getObject("wall.1.1")) == "color.red");
	CHECK(game.getObject("wall.1.1").variable.at("points") == 1.0f);

	// The column moves; the rest do not.
	CHECK(game.getObject("wall.3.1").velocity.x == 2.0f);
	CHECK(game.getObject("wall.3.2").velocity.x == 2.0f);
	CHECK(game.getObject("wall.1.1").velocity.x == 0.0f);

	// The cell has a name of its own, and is bigger, in the middle of its place.
	CHECK(game.tryGetObject("wall.2.1") == nullptr);
	const Object& boss = game.getObject("boss");
	CHECK(boss.groupName == "wall");
	CHECK(colorOf(boss) == "color.blue");
	CHECK(boss.position.x == 100.0f + 25.0f - 5.0f);
	CHECK(boss.position.y == 50.0f - 3.0f);

	// Its neighbours are where they would be without it.
	CHECK(game.getObject("wall.3.1").position.x == 100.0f + 50.0f);
}

TEST_CASE("the padding of a row or column is the gap before it", "[group][cells]")
{
	Loaded loaded{ gameXml(wallWith(
		"<row number=\"2\"><padding><y>20</y></padding></row>"
		"<column number=\"3\"><padding><x>15</x></padding></column>", "3"),
		kShowWall) };
	Game& game = loaded.game;

	CHECK(game.getObject("wall.1.2").position.y == 50.0f + 10.0f + 20.0f);
	CHECK(game.getObject("wall.1.3").position.y == 50.0f + 10.0f + 20.0f + 10.0f + 7.0f); // the group's gap again
	CHECK(game.getObject("wall.2.1").position.x == 100.0f + 25.0f);
	CHECK(game.getObject("wall.3.1").position.x == 100.0f + 25.0f + 20.0f + 15.0f);
}

TEST_CASE("a row or column picks by number, several numbers, odd or even", "[group][cells]")
{
	Loaded loaded{ gameXml(wallWith(
		"<row number=\"odd\"><sprite><rectangle><color>color.blue</color></rectangle></sprite></row>"
		"<row number=\"2 4\"><sprite><rectangle><color>color.green</color></rectangle></sprite></row>", "5"),
		kShowWall) };
	Game& game = loaded.game;

	CHECK(colorOf(game.getObject("wall.1.1")) == "color.blue");
	CHECK(colorOf(game.getObject("wall.2.2")) == "color.green");
	CHECK(colorOf(game.getObject("wall.3.3")) == "color.blue");
	CHECK(colorOf(game.getObject("wall.1.4")) == "color.green");
	CHECK(colorOf(game.getObject("wall.1.5")) == "color.blue");
}

TEST_CASE("a member's sprite of the same shape as its group's gives only what changes", "[group]")
{
	Loaded loaded{ gameXml(groupWith(
		"<sprite><rectangle><width>40</width><height>20</height><color>color.brown</color></rectangle></sprite>" + kPosition + kVelocity + kCollisions,
		"<member name=\"short\"><sprite><rectangle><width>10</width></rectangle></sprite></member>"
		"<member name=\"round\"><sprite><circle><radius>4</radius></circle></sprite></member>"), "<show object=\"lane\" />") };
	Game& game = loaded.game;

	CHECK(game.getObject("short").picture.kind == ShapeKind::Rectangle);
	CHECK(game.getObject("short").picture.size.x == 10.0f);
	CHECK(game.getObject("short").picture.size.y == 20.0f);
	CHECK(game.getObject("short").picture.color == "color.brown");
	CHECK(game.getObject("round").picture.kind == ShapeKind::Circle);
	CHECK(game.getObject("round").picture.radius == 4.0f);
	CHECK(game.getObject("round").picture.color == "color.white");
}

TEST_CASE("a member's tether names its two ends again and keeps the rest; a member's text can choose", "[group]")
{
	Loaded loaded{ gameXml(
		"<object name=\"a\">" + kSprite + kPosition + "</object><object name=\"b\">" + kSprite + kPosition + "</object>"
		"<group name=\"lines\"><sprite><tether from=\"a\" to=\"b\"><thickness>3</thickness><color>color.red</color></tether></sprite>" + kPosition +
		"<member name=\"one\" /><member name=\"two\"><sprite><tether from=\"b\" to=\"a\" /></sprite></member></group>"
		"<group name=\"signs\"><sprite><text><content>go</content><size>10</size></text></sprite>" + kPosition +
		"<member name=\"sign\"><sprite><text><choose><by>1</by><word>left</word><word>right</word></choose></text></sprite></member></group>",
		"<show object=\"lines\" /><show object=\"signs\" /><show object=\"a\" /><show object=\"b\" />") };
	Game& game = loaded.game;

	const Object& two = game.getObject("two");
	CHECK(two.tetherFrom == "b");
	CHECK(two.tetherTo == "a");
	CHECK(two.tetherThickness == 3);
	CHECK(two.tetherColor.r == 255);
	CHECK(two.tetherColor.g == 0);
	CHECK(game.getObject("sign").picture.kind == ShapeKind::Text);
	CHECK(game.getObject("sign").picture.text == "right");
	CHECK(game.getObject("sign").picture.textSize == 10);
	CHECK(game.getObject("sign").picture.color == "color.white");
}

TEST_CASE("cells that cannot be laid out, or changed two ways at once, say where", "[group][cells][errors]")
{
	const auto refused = [](const std::string& group) { Loaded loaded(gameXml(group, kShowWall)); };

	SECTION("a row and a column that change the same thing in one cell")
	{
		REQUIRE_THROWS_WITH(refused(wallWith(
			"<row number=\"1\"><sprite><rectangle><color>color.green</color></rectangle></sprite></row>"
			"<column number=\"2\"><sprite><rectangle><color>color.blue</color></rectangle></sprite></column>")),
			ContainsSubstring("object 'wall.2.1'") && ContainsSubstring("both change its <sprite>") && ContainsSubstring("<cell>"));
	}

	SECTION("two rows that change the same variable in one cell")
	{
		REQUIRE_THROWS_WITH(refused(wallWith(
			"<row number=\"odd\"><variables><variable name=\"points\">2</variable></variables></row>"
			"<row number=\"1\"><variables><variable name=\"points\">3</variable></variables></row>")),
			ContainsSubstring("<variable name=\"points\">"));
	}

	SECTION("a row that is not there")
	{
		REQUIRE_THROWS_WITH(refused(wallWith("<row number=\"3\" />")), ContainsSubstring("\"3\" is not one of 1 to 2"));
	}

	SECTION("a cell that is not there")
	{
		REQUIRE_THROWS_WITH(refused(wallWith("<cell row=\"1\" column=\"4\" />")), ContainsSubstring("is not a place in the group"));
	}

	SECTION("two cells in one place")
	{
		REQUIRE_THROWS_WITH(refused(wallWith("<cell row=\"1\" column=\"1\" /><cell row=\"1\" column=\"1\" />")), ContainsSubstring("the same place"));
	}

	SECTION("a column's gap above it")
	{
		REQUIRE_THROWS_WITH(refused(wallWith("<column number=\"2\"><padding><y>4</y></padding></column>")), ContainsSubstring("gives only an <x>"));
	}

	SECTION("rows that are not a whole number")
	{
		REQUIRE_THROWS_WITH(refused(wallWith("", "two")), ContainsSubstring("<rows>") && ContainsSubstring("a whole number"));
	}

	SECTION("columns and members both")
	{
		REQUIRE_THROWS_WITH(refused(wallWith("<member />")), ContainsSubstring("has no <member>s"));
	}

	SECTION("a row with no columns and rows to pick from")
	{
		REQUIRE_THROWS_WITH(refused(groupWith(kSprite + kPosition + kVelocity + kCollisions, "<row number=\"1\" />")),
			ContainsSubstring("no <columns> and <rows>"));
	}
}

// ------------------------------------------------------- the shipped games
namespace
{
	// Two lanes inside one group: the outer gives the shape, a wrap at the
	// sides and a class; the first lane changes the sprite's color, the
	// second adds a rule of its own and another class.
	const std::string kNested =
		"<group name=\"lanes\" class=\"logs\">"
		"<sprite><rectangle><width>30</width><height>10</height><color>color.brown</color></rectangle></sprite>"
		"<collisions><enabled>true</enabled><collision edge=\"horizontal\"><wrap /></collision></collisions>"
		"<variables><variable name=\"weight\">3</variable></variables>"
		"<group name=\"slow\">"
		"<sprite><rectangle><color>color.red</color></rectangle></sprite>"
		"<position><y>20</y></position><velocity><x>1</x></velocity>"
		"<member><position><x>10</x></position></member>"
		"<member><position><x>200</x></position></member>"
		"</group>"
		"<group name=\"fast\" class=\"hazard\">"
		"<position><y>40</y></position><velocity><x>-3</x></velocity>"
		"<collisions><enabled>true</enabled><collision class=\"frog\"><die /></collision></collisions>"
		"<variables><variable name=\"weight\">5</variable></variables>"
		"<member><position><x>50</x></position></member>"
		"</group>"
		"</group>";
}

TEST_CASE("a group inside a group takes what it leaves out from the one around it", "[group][nested]")
{
	Loaded loaded{ gameXml(kNested, "<show object=\"lanes\" />") };
	Game& game = loaded.game;
	game.setCurrentState("playing");
	const auto& objects = game.getCurrentObjects();

	// Members are named after their own group, in the order written.
	REQUIRE(objects.size() == 3);
	CHECK(objects[0].name == "slow.1");
	CHECK(objects[1].name == "slow.2");
	CHECK(objects[2].name == "fast.1");

	const Object& slow = game.getObject("slow.2");
	CHECK(slow.groupName == "slow");
	CHECK(slow.outerGroups == std::vector<std::string>{ "lanes" });
	CHECK(slow.objClass == "logs");
	CHECK(slow.position.x == 200.0f);
	CHECK(slow.position.y == 20.0f);
	CHECK(slow.velocity.x == 1.0f);
	CHECK(measureShapeSize(slow.picture).x == 30.0f);
	CHECK(slow.picture.color == "color.red");
	CHECK(slow.collisionData.enabled);
	CHECK(slow.collisionData.edges[edgeIndex(Edge::Left)].commands.size() == 1);
	CHECK(slow.collisionData.others.empty());
	CHECK(slow.variable.at("weight") == 3.0f);

	// The inner group's rules and variables add to the outer's, its class replaces it.
	const Object& fast = game.getObject("fast.1");
	CHECK(fast.objClass == "hazard");
	CHECK(fast.collisionData.edges[edgeIndex(Edge::Left)].commands.size() == 1);
	CHECK(fast.collisionData.others.size() == 1);
	CHECK(fast.variable.at("weight") == 5.0f);
	CHECK(fast.velocity.x == -3.0f);

	// The outer name means every member of every group inside it; each inner name, its own.
	for (const Object& object : objects) { CHECK(game.isShown(object)); }
	CHECK(&game.getObject("lanes") == &game.getObject("slow.1"));
	CHECK(&game.getObject("fast") == &game.getObject("fast.1"));
	game.getObject("slow.1").position.x = 99.0f;
	game.getObject("fast.1").position.x = 99.0f;
	game.resetObject("lanes");
	CHECK(game.getObject("slow.1").position.x == 10.0f);
	CHECK(game.getObject("fast.1").position.x == 50.0f);
}

TEST_CASE("a group's <heading> is where its members face, and a group inside it can give its own", "[group][nested][heading]")
{
	// Combat's tanks: one group, each player's a group inside it.
	const std::string tanks =
		"<group name=\"tanks\" class=\"tank\">"
		"<sprite><rectangle><width>10</width><height>10</height></rectangle></sprite>"
		"<heading>45</heading>"
		"<group name=\"left\"><position><x>10</x><y>10</y></position><member /></group>"
		"<group name=\"right\"><position><x>90</x><y>10</y></position><heading>270</heading><member /></group>"
		"</group>";
	Loaded loaded{ gameXml(tanks, "<show object=\"tanks\" />") };
	Game& game = loaded.game;
	game.setCurrentState("playing");

	CHECK(game.getObject("left.1").heading == 45.0f);
	CHECK(game.getObject("right.1").heading == 270.0f);

	// A reset turns it back to the group's heading.
	game.getObject("right.1").heading = 90.0f;
	game.resetObject("tanks");
	CHECK(game.getObject("right.1").heading == 270.0f);
}

TEST_CASE("groups inside a group can each be laid out in cells from the outer's columns", "[group][nested][cells]")
{
	// The outer gives the columns, the gap across and the brick; each inner
	// group its rows and where it starts.
	const std::string wall =
		"<group name=\"wall\" class=\"bricks\">"
		"<columns>3</columns><padding><x>5</x></padding>"
		"<sprite name=\"whole\"><rectangle><width>20</width><height>10</height><color>color.red</color></rectangle></sprite>"
		"<position><x>100</x></position>"
		"<group name=\"top\"><rows>1</rows>"
		"<sprite name=\"whole\"><rectangle><color>color.grey</color></rectangle></sprite>"
		"<position><y>10</y></position></group>"
		"<group name=\"block\"><rows>2</rows><padding><y>7</y></padding>"
		"<position><y>50</y></position>"
		"<collisions><enabled>true</enabled><lockstep>true</lockstep></collisions></group>"
		"</group>";
	Loaded loaded{ gameXml(wall, "<show object=\"wall\" />") };
	Game& game = loaded.game;

	REQUIRE(game.getCurrentObjects().size() == 3 + 6);
	CHECK(game.getObject("top.3.1").position.x == 100.0f + 2 * (20.0f + 5.0f));
	CHECK(game.getObject("top.3.1").position.y == 10.0f);
	CHECK(game.getObject("block.2.2").position.x == 100.0f + 20.0f + 5.0f);
	CHECK(game.getObject("block.2.2").position.y == 50.0f + 10.0f + 7.0f);
	CHECK(game.getObject("top.1.1").objClass == "bricks");
	CHECK_FALSE(game.getObject("top.1.1").collisionData.enabled);
	CHECK(game.getObject("block.1.1").collisionData.enabled);

	// Only the inner group with lockstep moves as one block.
	CHECK(game.getObject("block.1.1").collisionData.lockstep != 0);
	CHECK(game.getObject("block.1.1").collisionData.lockstep == game.getObject("block.3.2").collisionData.lockstep);
	CHECK(game.getObject("top.1.1").collisionData.lockstep == 0);
}

TEST_CASE("a group with groups inside it has no members of its own", "[group][nested][errors]")
{
	REQUIRE_THROWS_WITH(Loaded(gameXml(
		"<group name=\"lanes\">" + kSprite + kPosition + "<member /><group name=\"inner\"><member /></group></group>",
		"<show object=\"lanes\" />")),
		ContainsSubstring("group 'lanes'") && ContainsSubstring("has <group>s inside it, so it has no <member>s"));
}

TEST_CASE("frogger.xml and spacerace.xml are made of the groups they are written as", "[group][shipped]")
{
	const auto census = [](const char* file)
	{
		Game game{ file };
		std::map<std::string, int> classes;
		for (const Object& object : game.getCurrentObjects()) { ++classes[object.objClass]; }
		classes["all"] = static_cast<int>(game.getCurrentObjects().size());
		return classes;
	};

	// Frogger: logs and turtles (class logs), trucks and cars and the hedge
	// (hazard), and the five bays.
	const auto frogger = census("descriptions/frogger.xml");
	CHECK(frogger.at("logs") == 8 + 7);
	CHECK(frogger.at("hazard") == 14 + 1);
	CHECK(frogger.at("pads") == 5);

	// Space Race's debris is a group of 8 columns and 12 rows.
	const auto race = census("descriptions/spacerace.xml");
	CHECK(race.at("all") == 107);
	CHECK(race.at("debris") == 96);
	CHECK(race.at("rocket") == 2);
}

TEST_CASE("every lane of frogger is a group of the members it had, and space race's debris a cell for each place", "[group][shipped]")
{
	// Frogger's lanes are laid out in columns, one row each: playing.logrow2.3.1
	// is the third log of the lane, inside the river group and the lanes group.
	struct Lane { const char* group; int members; };
	const Lane frogger[] = {
		{ "pads", 5 },
		{ "logrow2", 3 }, { "turtlerow3", 4 }, { "logrow4", 2 }, { "logrow5", 3 }, { "turtlerow6", 3 },
		{ "truckrow8", 2 }, { "carrow9", 2 }, { "carrow10", 4 }, { "carrow11", 3 }, { "carrow12", 3 },
	};

	Game game{ "descriptions/frogger.xml" };
	for (const Lane& lane : frogger)
	{
		int found = 0;
		for (const Object& object : game.getCurrentObjects())
		{
			if (object.groupName != "playing." + std::string(lane.group)) { continue; }
			++found;
			CHECK(object.name == "playing." + std::string(lane.group) + "." + std::to_string(found) + ".1");
		}
		INFO(lane.group);
		CHECK(found == lane.members);
	}
	int lanes = 0;
	for (const Object& object : game.getCurrentObjects())
	{
		if (object.inGroup("playing.lanes")) { ++lanes; }
	}
	CHECK(lanes == 15 + 14);

	Game race{ "descriptions/spacerace.xml" };
	for (int row = 1; row <= 12; ++row)
	{
		const auto cells = std::count_if(race.getCurrentObjects().begin(), race.getCurrentObjects().end(),
			[&](const Object& object) { return object.groupName == "playing.debris"
				&& object.name.substr(object.name.rfind('.') + 1) == std::to_string(row); });
		INFO(row);
		CHECK(cells == 8);
	}
}

// ------------------------------------------------------------------ schema
namespace
{
	std::string readFile(const std::string& path)
	{
		std::ifstream in(path);
		REQUIRE(in.good());
		std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
		text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
		return text;
	}

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
		const ScratchFile scratch{ scratchName("descriptions/group_scratch.xml") };
		{
			std::ofstream out(scratch.path);
			out << xml;
		}

		Verdict verdict{};

		auto weak = XmlDocumentFactory::create(XmlBackend::TinyXml2);
		REQUIRE(weak->load(scratch.path.string()));
		XsdLiteValidator validator;
		REQUIRE(validator.loadSchema(findSchema("descriptions/pong.xml", "../xgedef.xsd").string(), XmlBackend::TinyXml2));
		verdict.weakAccepts = validator.validate(*weak->getRootElement());
		verdict.weakMessage = validator.getErrorMessage();

		auto strong = XmlDocumentFactory::create(XmlBackend::Xerces);
		verdict.strongAccepts = strong->load(scratch.path.string());

		return verdict;
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

TEST_CASE("both schema checkers accept the games written with groups", "[group][schema]")
{
	for (const char* file : { "descriptions/frogger.xml", "descriptions/spacerace.xml", "descriptions/kaboom.xml", "descriptions/freeway.xml", "descriptions/depthcharge.xml", "descriptions/astrosmash.xml",
		"descriptions/breakout.xml", "descriptions/spaceinvaders.xml", "descriptions/spaceinvaders2.xml", "descriptions/pong.xml", "descriptions/pong_min.xml",
		"descriptions/combat.xml", "descriptions/tennis.xml", "descriptions/trickshot.xml", "descriptions/airseabattle.xml" })
	{
		DYNAMIC_SECTION(file)
		{
			const Verdict verdict = judge(readFile(file));

			CHECK(verdict.weakAccepts);
			CHECK(verdict.strongAccepts);
		}
	}
}

TEST_CASE("both schema checkers take every object part on a group on a map", "[group][schema][map]")
{
	const Verdict verdict = judge(gameWith("descriptions/pacman.xml", "<color>color.magenta</color></bitmap></sprite>",
		"<color>color.magenta</color></bitmap></sprite><heading>0</heading><drag>1</drag><mass>1</mass>"
		"<pull object=\"pacman\">0</pull><seek object=\"pacman\">0</seek><facing>left</facing>"));

	CHECK(verdict.weakAccepts);
	CHECK(verdict.strongAccepts);
}

TEST_CASE("both schema checkers turn away the same mistakes in a group", "[group][schema]")
{
	struct Mistake
	{
		const char* what;
		const char* file;
		const char* from;
		const char* to;
		const char* weakSays; // part of the weak validator's message
	};

	const Mistake mistakes[] = {
		{ "a member that gives collisions", "descriptions/combat.xml", "<member><position>", "<member><collisions><enabled>true</enabled></collisions><position>", "collisions" },
		{ "a group with something it cannot hold", "descriptions/spacerace.xml", "<columns>8</columns>", "<columns>8</columns><speed />", "speed" },
		{ "a member with something it cannot hold", "descriptions/combat.xml", "<member><position>", "<member><speed /><position>", "speed" },
		{ "a position that is not an x and a y", "descriptions/spacerace.xml", "<y>50</y>", "<z>50</z>", "z" },
		{ "a group with no name", "descriptions/combat.xml", "<group name=\"yellow\">", "<group>", "missing required attribute 'name'" },
		{ "a lockstep flag that is not true or false", "descriptions/spacerace.xml", "<collisions>", "<collisions><lockstep>maybe</lockstep>", "\"maybe\" is not a valid" },
		{ "a member in a group of cells", "descriptions/spacerace.xml", "<row number=\"1 5 9\">", "<member /><row number=\"1 5 9\">", "row" },
		{ "a member beside a group inside a group", "descriptions/combat.xml", "<group name=\"yellow\">", "<member /><group name=\"yellow\">", "group" },
		{ "a member's tether that leaves out an end", "descriptions/fishingderby.xml", "<member name=\"rod1\"><position><x>206</x></position></member>",
			"<member name=\"rod1\"><sprite><tether from=\"rod1\" /></sprite><position><x>206</x></position></member>", "missing required attribute 'to'" },
		{ "a cell with no column", "descriptions/spacerace.xml", "<row number=\"1 5 9\">", "<cell row=\"1\" /><row number=\"1 5 9\">", "missing required attribute 'column'" },
	};

	for (const Mistake& mistake : mistakes)
	{
		DYNAMIC_SECTION(mistake.what)
		{
			const Verdict verdict = judge(gameWith(mistake.file, mistake.from, mistake.to));

			CHECK_FALSE(verdict.weakAccepts);
			CHECK_THAT(verdict.weakMessage, ContainsSubstring(mistake.weakSays));
			CHECK_FALSE(verdict.strongAccepts);
		}
	}
}
