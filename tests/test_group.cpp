// test_group.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026
//
// Catch2 tests for the <group> tag: one description shared by several
// objects, each <member> giving only what is its own. A group is read as one
// ordinary object per member (named group.1, group.2, ... unless the member
// names itself), the group's name still means every member at once, members
// of a group with <lockstep> move as one block, and what a group or member
// gets wrong says where. Then both schema checkers, Xerces's real validation
// and this project's own XsdLiteValidator, on the shipped games written with
// groups and on mistakes in them.
//
// Small games are written to a scratch file and loaded by a real xge::Game;
// none of it needs a window.

#include "game.h"
#include "xml_document.h"
#include "xsd_lite.h"

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
		ScratchFile scratch{ std::filesystem::temp_directory_path() / name };
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
		CHECK(object.baseName == object.name); // an ordinary object that also knows its group
		CHECK(object.collisionData.enabled);
		CHECK(object.collisionData.left.size() == 1);
		CHECK(object.collisionData.right.size() == 1);
		CHECK(object.variable.at("weight") == 3.0f);
	}
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
	CHECK(first.spriteParams == second.spriteParams);

	// The member that says more wins: its own row, its own x for the speed
	// (the group's 0 for y), and its own shape.
	CHECK(odd.position.x == 300.0f);
	CHECK(odd.position.y == 150.0f);
	CHECK(odd.velocity.x == 1.0f);
	CHECK(odd.velocity.y == 0.0f);
	CHECK(odd.spriteParams != first.spriteParams);

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

	SECTION("nothing gives the velocity")
	{
		REQUIRE_THROWS_WITH(Loaded(gameXml(groupWith(kSprite + kPosition + kCollisions, kMember))),
			ContainsSubstring("has no <velocity><x>"));
	}

	SECTION("a group with no collisions")
	{
		REQUIRE_THROWS_WITH(Loaded(gameXml(groupWith(kSprite + kPosition + kVelocity, kMember))),
			ContainsSubstring("group 'lane'") && ContainsSubstring("missing <collisions>"));
	}

	SECTION("something a member cannot say")
	{
		REQUIRE_THROWS_WITH(Loaded(gameXml(groupWith(kSprite + kPosition + kVelocity + kCollisions,
			"<member><collisions><enabled>true</enabled></collisions></member>"))),
			ContainsSubstring("object 'lane.1'") && ContainsSubstring("a member can give a <sprite>, <position> or <velocity>"));
	}

	SECTION("something a group cannot say")
	{
		REQUIRE_THROWS_WITH(Loaded(gameXml(groupWith(kSprite + kPosition + kVelocity + kCollisions + "<bogus />", kMember))),
			ContainsSubstring("group 'lane'") && ContainsSubstring("unknown <bogus>"));
	}

	SECTION("a value that is both text and a tag, in a member")
	{
		REQUIRE_THROWS_WITH(Loaded(gameXml(groupWith(kSprite + kPosition + kVelocity + kCollisions,
			"<member><position><x>5 <random min=\"1\" max=\"2\" /></x></position></member>"))),
			ContainsSubstring("holds both the text"));
	}
}

// ------------------------------------------------------- the shipped games
TEST_CASE("frogger.xml and spacerace.xml have the objects they had before groups", "[group][shipped]")
{
	// Counted from the games as they were written with one <object> for each:
	// Frogger's 59 objects include the lane markings, a 13 by 4 <grid>, so 58
	// objects and 52 cells; Space Race's 37 have no grid.
	const auto census = [](const char* file)
	{
		Game game{ file };
		std::map<std::string, int> classes;
		for (const Object& object : game.getCurrentObjects()) { ++classes[object.objClass]; }
		classes["all"] = static_cast<int>(game.getCurrentObjects().size());
		return classes;
	};

	const auto frogger = census("games/frogger.xml");
	CHECK(frogger.at("all") == 110);
	CHECK(frogger.at("logs") == 13);
	CHECK(frogger.at("hazard") == 19);
	CHECK(frogger.at("pads") == 5);

	const auto race = census("games/spacerace.xml");
	CHECK(race.at("all") == 37);
	CHECK(race.at("debris") == 27);
}

TEST_CASE("every lane of frogger and space race is a group of the members it had", "[group][shipped]")
{
	struct Lane { const char* group; int members; };
	const Lane frogger[] = {
		{ "homes", 5 }, { "pads", 5 }, { "hedges", 6 },
		{ "logrow2", 2 }, { "logrow3", 3 }, { "logrow4", 2 }, { "logrow5", 3 }, { "logrow6", 3 },
		{ "truckrow8", 2 }, { "carrow9", 2 }, { "carrow10", 3 }, { "carrow11", 3 }, { "carrow12", 3 },
	};

	Game game{ "games/frogger.xml" };
	for (const Lane& lane : frogger)
	{
		int found = 0;
		for (const Object& object : game.getCurrentObjects())
		{
			if (object.groupName != lane.group) { continue; }
			++found;
			CHECK(object.name == std::string(lane.group) + "." + std::to_string(found));
		}
		INFO(lane.group);
		CHECK(found == lane.members);
	}

	Game race{ "games/spacerace.xml" };
	for (int lane = 1; lane <= 9; ++lane)
	{
		const std::string group = "debris" + std::to_string(lane);
		const auto members = std::count_if(race.getCurrentObjects().begin(), race.getCurrentObjects().end(),
			[&](const Object& object) { return object.groupName == group; });
		INFO(group);
		CHECK(members == 3);
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
		const ScratchFile scratch{ "games/group_scratch.xml" };
		{
			std::ofstream out(scratch.path);
			out << xml;
		}

		Verdict verdict{};

		auto weak = XmlDocumentFactory::create(XmlBackend::TinyXml2);
		REQUIRE(weak->load(scratch.path.string()));
		XsdLiteValidator validator;
		REQUIRE(validator.loadSchema("assets/xmlgameengine.xsd", XmlBackend::TinyXml2));
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
	for (const char* file : { "games/frogger.xml", "games/spacerace.xml", "games/gemcatcher.xml" })
	{
		DYNAMIC_SECTION(file)
		{
			const Verdict verdict = judge(readFile(file));

			CHECK(verdict.weakAccepts);
			CHECK(verdict.strongAccepts);
		}
	}
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
		{ "a member that gives collisions", "games/spacerace.xml", "<member>\n", "<member><collisions><enabled>true</enabled></collisions>\n", "collisions" },
		{ "a group with something it cannot hold", "games/spacerace.xml", "<velocity>", "<speed /><velocity>", "speed" },
		{ "a member with something it cannot hold", "games/spacerace.xml", "<member>\n", "<member><speed />\n", "speed" },
		{ "a position that is not an x and a y", "games/spacerace.xml", "<y>75</y>", "<z>75</z>", "z" },
		{ "a group with no name", "games/spacerace.xml", "<group name=\"debris1\" ", "<group ", "missing required attribute 'name'" },
		{ "a lockstep flag that is not true or false", "games/spacerace.xml", "<enabled>true</enabled>", "<enabled>true</enabled><lockstep>maybe</lockstep>", "\"maybe\" is not a valid" },
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
