// test_xml_format.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026
//
// Catch2 tests for the shape of a game file: values that are an expression or
// a value tag (<random>), commands written as tags in the order given, sprites
// as nested shape tags, collision rules with and without an edge, conditions
// with their test tag, and what each error in a file says. Then the two
// schema checkers against the same broken files: Xerces's real validation and
// this project's own XsdLiteValidator must both turn away the same mistakes.
//
// Small games are written to a scratch file and loaded by a real xge::Game;
// none of it needs a window.

#include "scratch.h"
#include "file_text.h"
#include "engine/folder.h"
#include "game/game.h"
#include "load/expr.h"
#include "load/xml.h"
#include "xml/document.h"
#include "load/xsd.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
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

	// One object with every required part; the parts a test cares about are
	// passed in, the rest are dull.
	struct ObjectXml
	{
		std::string name = "o";
		std::string attributes;
		std::string sprite = "<circle><radius>5</radius></circle>";
		std::string x = "0";
		std::string y = "0";
		std::string velocityX = "0";
		std::string velocityY = "0";
		std::string collisions = "<enabled>false</enabled>";
		std::string extra;
	};

	std::string objectXml(const ObjectXml& o)
	{
		return "<object name=\"" + o.name + "\" " + o.attributes + ">"
			"<sprite>" + o.sprite + "</sprite>"
			"<position><x>" + o.x + "</x><y>" + o.y + "</y></position>"
			"<velocity><x>" + o.velocityX + "</x><y>" + o.velocityY + "</y></velocity>"
			"<collisions>" + o.collisions + "</collisions>" + o.extra + "</object>";
	}

	// A state that does nothing, for the commands under test to name.
	std::string idleState(const std::string& name)
	{
		return "<state name=\"" + name + "\"><shows><show object=\"o\" /></shows>"
			"<inputs><input button=\"space\"><pop /></input></inputs></state>";
	}

	std::string gameXml(const std::string& variables, const std::string& objects, const std::string& conditions = {},
		const std::string& moreStates = {})
	{
		return "<game>"
			"<window name=\"test\"><width>800</width><height>600</height><background>color.black</background>"
			"<fullscreen>false</fullscreen><framerate>60</framerate></window>"
			"<variables>" + variables + "</variables>"
			"<objects>" + objects + "</objects>"
			"<states><state name=\"playing\"><shows><show object=\"o\" /></shows>"
			"<inputs><input button=\"space\"><pop /></input></inputs>" + conditions + "</state>" + moreStates + "</states>"
			"</game>";
	}

	// Loads a game made of the given parts (no schema named, so nothing is
	// validated but what load/xml itself checks).
	struct Loaded
	{
		ScratchFile file;
		Game game;

		Loaded(const std::string& xml, const std::string& name = "xge_test_game.xml") :
			file(writeScratch(name, xml)),
			game(file.path.string())
		{
		}
	};

	template <typename Alternative>
	const Alternative& as(const Command& command)
	{
		REQUIRE(std::holds_alternative<Alternative>(command));
		return std::get<Alternative>(command);
	}
}

TEST_CASE("a value is an expression written as text", "[xml_format]")
{
	ObjectXml o;
	o.x = "2 * 3 + window.width.center";
	o.velocityX = "-(1 + 1)";
	Loaded loaded(gameXml("<variable name=\"step\">2</variable>", objectXml(o)));

	const Object& object = loaded.game.getObject("o");
	CHECK(object.start.position.x == 406.0f); // 800 wide window
	CHECK(object.start.velocity.x == -2.0f);
}

TEST_CASE("<random> gives a number between its two ends, and a different one each time", "[xml_format][random]")
{
	std::string objects;
	for (int i = 0; i < 40; ++i)
	{
		ObjectXml o;
		o.name = "r" + std::to_string(i);
		o.velocityX = "<random><min>-7</min><max>7</max></random>";
		o.velocityY = "<random><min>window.width.center</min><max>window.width.center - 10</max></random>"; // expressions, either order
		objects += objectXml(o);
	}
	Loaded loaded(gameXml("", objects));

	std::set<float> seen;
	for (int i = 0; i < 40; ++i)
	{
		const Object& object = loaded.game.getObject("r" + std::to_string(i));
		CHECK(object.start.velocity.x >= -7.0f);
		CHECK(object.start.velocity.x <= 7.0f);
		CHECK(object.start.velocity.y >= 390.0f);
		CHECK(object.start.velocity.y <= 400.0f);
		seen.insert(object.start.velocity.x);
	}
	CHECK(seen.size() > 1);
}

TEST_CASE("a value tag works anywhere a value does", "[xml_format][random]")
{
	ObjectXml o;
	o.sprite = "<rectangle><width><random><min>30</min><max>30</max></random></width><height>10</height></rectangle>";
	o.extra = "<variables><variable name=\"score\"><random><min>4</min><max>4</max></random></variable></variables>";
	Loaded loaded(gameXml("<variable name=\"g\"><random><min>9</min><max>9</max></random></variable>", objectXml(o)));

	CHECK(loaded.game.getObject("o").picture.size.x == 30.0f);
	CHECK(loaded.game.getObject("o").variable.at("score") == 4.0f);
	CHECK(loaded.game.getVariable("g") == 9.0f);
}

TEST_CASE("a window's <scale> shows it that many times larger; with none it is 1", "[xml_format][window]")
{
	const std::string plain = gameXml("", objectXml({}));
	std::string scaled = plain;
	scaled.replace(scaled.find("<background>"), 0, "<scale>3</scale>");

	Loaded one(plain);
	CHECK(one.game.getWindowDesc().scale == 1);

	Loaded three(scaled, "xge_test_scaled.xml");
	CHECK(three.game.getWindowDesc().scale == 3);
	CHECK(three.game.getWindowDesc().width == 800.0f); // the game's own size, in its own pixels

	std::string zero = plain;
	zero.replace(zero.find("<background>"), 0, "<scale>0</scale>");
	CHECK_THROWS_WITH(Loaded(zero, "xge_test_scale_zero.xml"), ContainsSubstring("<scale>"));
}

TEST_CASE("a window's background, fullscreen and framerate, and a <collisions>' enabled, may be left out", "[xml_format][window]")
{
	ObjectXml on;
	on.name = "on";
	on.collisions = "";
	ObjectXml none;
	none.name = "none";
	std::string bare = gameXml("", objectXml({}) + objectXml(on) + objectXml(none));
	for (const std::string tag : { "<background>color.black</background>", "<fullscreen>false</fullscreen>", "<framerate>60</framerate>" })
	{
		bare.erase(bare.find(tag), tag.size());
	}
	// An object with no <collisions> at all touches nothing.
	bare.replace(bare.find("</objects>"), 0, "<object name=\"plain\"><sprite><circle><radius>5</radius></circle></sprite><position><x>0</x><y>0</y></position></object>");

	Loaded loaded(bare, "xge_test_defaults.xml");
	const WindowDesc& window = loaded.game.getWindowDesc();
	CHECK(window.background == "color.black");
	CHECK(window.fullscreen == "false");
	CHECK(window.framerate == 60);

	CHECK(loaded.game.getObject("on").collisionsOn);
	CHECK_FALSE(loaded.game.getObject("none").collisionsOn);
	CHECK_FALSE(loaded.game.getObject("plain").collisionsOn);
}

TEST_CASE("a variable can use the ones declared before it", "[xml_format]")
{
	Loaded loaded(gameXml("<variable name=\"a\">10</variable><variable name=\"b\">a * 2</variable>", objectXml({})));

	CHECK(loaded.game.getVariable("b") == 20.0f);
}

TEST_CASE("a variable declared twice keeps the later value", "[xml_format]")
{
	Loaded loaded(gameXml("<variable name=\"a\">1</variable><variable name=\"a\">2</variable>", objectXml({})));

	CHECK(loaded.game.getVariable("a") == 2.0f);
}

TEST_CASE("a value with both text and a tag, or an unknown tag, is turned away", "[xml_format][errors]")
{
	ObjectXml both;
	both.x = "5 <random><min>1</min><max>2</max></random>";
	REQUIRE_THROWS_WITH(Loaded(gameXml("", objectXml(both))),
		ContainsSubstring("object 'o' > <position> > <x>") && ContainsSubstring("both the text \"5\" and a <random> tag"));

	ObjectXml unknown;
	unknown.x = "<dice sides=\"6\" />";
	REQUIRE_THROWS_WITH(Loaded(gameXml("", objectXml(unknown))), ContainsSubstring("unknown value tag <dice>"));

	ObjectXml empty;
	empty.x = "";
	REQUIRE_THROWS_WITH(Loaded(gameXml("", objectXml(empty))), ContainsSubstring("has no value"));

	ObjectXml incomplete;
	incomplete.x = "<random><min>1</min></random>";
	REQUIRE_THROWS_WITH(Loaded(gameXml("", objectXml(incomplete))), ContainsSubstring("missing <max>"));
}

TEST_CASE("an expression that does not compile says whose it is", "[xml_format][errors]")
{
	ObjectXml o;
	o.x = "nosuchname + 1";
	REQUIRE_THROWS_WITH(Loaded(gameXml("", objectXml(o))), ContainsSubstring("object 'o'") && ContainsSubstring("nosuchname + 1"));
}

TEST_CASE("commands run in the order they are written", "[xml_format][commands]")
{
	ObjectXml o;
	o.collisions = "<enabled>true</enabled>"
		"<collision edge=\"left\"><inc variable=\"o.score\" /><reset /><die /></collision>";
	o.extra = "<variables><variable name=\"score\">0</variable></variables>";
	Loaded loaded(gameXml("", objectXml(o)));

	const auto& left = loaded.game.getObject("o").collisionData.edges[edgeIndex(Edge::Left)].commands;
	REQUIRE(left.size() == 3);
	CHECK(as<CmdIncrement>(left[0]).target.written() == "o.score");
	as<CmdReset>(left[1]);
	as<CmdDie>(left[2]);
}

TEST_CASE("an edge rule reaches the edges it names, and several rules on one edge all run", "[xml_format][commands]")
{
	ObjectXml o;
	o.collisions = "<enabled>true</enabled>"
		"<collision edge=\"all\"><bounce /></collision>"
		"<collision edge=\"left\"><die /></collision>"
		"<collision edge=\"vertical\"><stick /></collision>";
	Loaded loaded(gameXml("", objectXml(o)));

	const auto& data = loaded.game.getObject("o").collisionData;
	CHECK(data.edges[edgeIndex(Edge::Top)].commands.size() == 2);    // all, vertical
	CHECK(data.edges[edgeIndex(Edge::Bottom)].commands.size() == 2); // all, vertical
	CHECK(data.edges[edgeIndex(Edge::Left)].commands.size() == 2);   // all, left
	CHECK(data.edges[edgeIndex(Edge::Right)].commands.size() == 1);  // all
	as<CmdBounce>(data.edges[edgeIndex(Edge::Left)].commands[0]);
	as<CmdDie>(data.edges[edgeIndex(Edge::Left)].commands[1]);
	CHECK(data.others.empty());
}

TEST_CASE("a collision without an edge is about another object; naming nothing means anything", "[xml_format][commands]")
{
	ObjectXml o;
	o.collisions = "<enabled>true</enabled>"
		"<collision><bounce /></collision>"
		"<collision class=\"bricks\"><die /></collision>"
		"<collision object=\"wall\" unless=\"logs\"><reverse /></collision>";
	Loaded loaded(gameXml("", objectXml(o)));

	const auto& others = loaded.game.getObject("o").collisionData.others;
	REQUIRE(others.size() == 3);

	CHECK(others[0].filterClass.empty());
	CHECK(others[0].filterObject.empty());
	as<CmdBounce>(others[0].commands.at(0));

	CHECK(others[1].filterClass == "bricks");
	as<CmdDie>(others[1].commands.at(0));

	CHECK(others[2].filterObject == "wall");
	CHECK(others[2].unlessClass == "logs");
	as<CmdReverse>(others[2].commands.at(0));

	const auto& data = loaded.game.getObject("o").collisionData;
	CHECK(data.edges[edgeIndex(Edge::Top)].commands.empty());
	CHECK(data.edges[edgeIndex(Edge::Left)].commands.empty());
}

TEST_CASE("move and hop take a direction and a value", "[xml_format][commands]")
{
	ObjectXml o;
	o.extra = "<actions>"
		"<action name=\"go\"><move direction=\"left\">step * 2</move><hop direction=\"up\"><random><min>7</min><max>7</max></random></hop></action>"
		"</actions>";
	Loaded loaded(gameXml("<variable name=\"step\">3</variable>", objectXml(o)));

	const auto& go = loaded.game.getObject("o").action.at("go");
	REQUIRE(go.size() == 2);
	CHECK(as<CmdMove>(go[0]).direction == Direction::Left);
	CHECK(as<CmdMove>(go[0]).step == 6.0f);
	CHECK(as<CmdHop>(go[1]).direction == Direction::Up);
	CHECK(as<CmdHop>(go[1]).distance == 7.0f);
}

TEST_CASE("state commands: push, pop, trigger and reset with and without an object", "[xml_format][commands]")
{
	const std::string states =
		"<states><state name=\"playing\"><shows><show object=\"o\" /></shows><inputs>"
		"<input button=\"a\"><push state=\"paused\" /><pop /><pop state=\"paused\" /></input>"
		"<input button=\"b\"><trigger object=\"o\" action=\"go\" /></input>"
		"<input button=\"c\"><reset /><reset object=\"o\" /></input>"
		"</inputs></state>" + idleState("paused") + "</states>";
	ObjectXml o;
	o.extra = "<actions><action name=\"go\"><move direction=\"left\">1</move></action></actions>";
	std::string xml = gameXml("", objectXml(o));
	xml.replace(xml.find("<states>"), xml.find("</states>") + 9 - xml.find("<states>"), states);
	Loaded loaded(xml);
	loaded.game.setCurrentState("playing");

	const State state = loaded.game.getCurrentState();
	const auto& a = state.input.at(keyCodeFromString("a"));
	const auto& b = state.input.at(keyCodeFromString("b"));
	const auto& c = state.input.at(keyCodeFromString("c"));

	REQUIRE(a.size() == 3);
	CHECK(as<CmdPushState>(a[0]).name == "paused");
	CHECK(as<CmdPopState>(a[1]).name.empty());
	CHECK(as<CmdPopState>(a[2]).name == "paused");

	REQUIRE(b.size() == 1);
	CHECK(as<CmdTriggerAction>(b[0]).object == "o");
	CHECK(as<CmdTriggerAction>(b[0]).action == "go");

	REQUIRE(c.size() == 2);
	as<CmdReset>(c[0]);
	CHECK(as<CmdResetObject>(c[1]).target == "o");
}

TEST_CASE("an unknown command, or one missing its attribute, says where", "[xml_format][errors]")
{
	ObjectXml unknown;
	unknown.collisions = "<enabled>true</enabled><collision edge=\"left\"><explode /></collision>";
	REQUIRE_THROWS_WITH(Loaded(gameXml("", objectXml(unknown))),
		ContainsSubstring("object 'o' > <collisions> > <collision>") && ContainsSubstring("unknown command <explode>"));

	ObjectXml missing;
	missing.collisions = "<enabled>true</enabled><collision edge=\"left\"><inc /></collision>";
	REQUIRE_THROWS_WITH(Loaded(gameXml("", objectXml(missing))), ContainsSubstring("<inc> needs variable="));

	ObjectXml badEdge;
	badEdge.collisions = "<enabled>true</enabled><collision edge=\"middle\"><bounce /></collision>";
	REQUIRE_THROWS_WITH(Loaded(gameXml("", objectXml(badEdge))), ContainsSubstring("edge=\"middle\""));

	ObjectXml badDirection;
	badDirection.extra = "<actions><action name=\"a\"><move direction=\"sideways\">1</move></action></actions>";
	REQUIRE_THROWS_WITH(Loaded(gameXml("", objectXml(badDirection))), ContainsSubstring("direction=\"sideways\""));
}

TEST_CASE("<pop state> puts that state in place of this one, so the stack does not grow", "[xml_format][commands]")
{
	const std::string states =
		"<states><state name=\"menu\"><shows><show object=\"o\" /></shows><inputs><input button=\"space\"><push state=\"wave1\" /></input></inputs></state>"
		"<state name=\"wave1\"><shows><show object=\"o\" /></shows><inputs><input button=\"space\"><pop state=\"wave2\" /></input></inputs></state>"
		"<state name=\"wave2\"><shows><show object=\"o\" /></shows><inputs><input button=\"space\"><pop /></input></inputs></state>"
		"</states>";
	std::string xml = gameXml("", objectXml(ObjectXml{}));
	xml.replace(xml.find("<states>"), xml.find("</states>") + 9 - xml.find("<states>"), states);
	Loaded loaded(xml);
	Game& game = loaded.game;
	game.setCurrentState("menu");

	game.pushState("wave1");
	game.popState("wave2");
	CHECK(game.getCurrentState().name == "wave2");

	// wave1 is gone: a plain pop goes back to the menu, not to wave1.
	game.popState();
	CHECK(game.getCurrentState().name == "menu");
}

TEST_CASE("a mistake the schema lets through stops the load, saying what to write instead", "[xml_format][errors]")
{
	const auto errorOf = [](const std::string& xml)
	{
		try
		{
			Loaded loaded(xml);
		}
		catch (const std::exception& error)
		{
			return std::string(error.what());
		}
		return std::string("loaded");
	};
	const auto withStates = [](std::string xml, const std::string& states)
	{
		xml.replace(xml.find("<states>"), xml.find("</states>") + 9 - xml.find("<states>"), states);
		return xml;
	};

	SECTION("a key the engine does not have")
	{
		const std::string error = errorOf(withStates(gameXml("", objectXml(ObjectXml{})),
			"<states><state name=\"playing\"><shows><show object=\"o\" /></shows><inputs><input button=\"spcae\"><pop /></input></inputs></state></states>"));
		CHECK_THAT(error, ContainsSubstring("state 'playing' > <input button=\"spcae\">: there is no key named 'spcae' (did you mean 'space'?)"));
		CHECK_THAT(error, ContainsSubstring("The keys are: a, b, c"));
	}

	SECTION("a color that is not a color. name")
	{
		ObjectXml o;
		o.sprite = "<circle><radius>5</radius><color>white</color></circle>";
		CHECK_THAT(errorOf(gameXml("", objectXml(o))), ContainsSubstring("there is no color named 'white'") && ContainsSubstring("color.white"));

		ObjectXml typo;
		typo.sprite = "<circle><radius>5</radius><color>color.gren</color></circle>";
		CHECK_THAT(errorOf(gameXml("", objectXml(typo))), ContainsSubstring("(did you mean 'color.green'?)"));
	}

	SECTION("a command where the engine does nothing with it")
	{
		ObjectXml o;
		o.extra = "<actions><action name=\"go\"><move direction=\"left\">1</move></action></actions>";
		const std::string error = errorOf(withStates(gameXml("", objectXml(o)),
			"<states><state name=\"playing\"><shows><show object=\"o\" /></shows><inputs><input button=\"a\"><move direction=\"left\">1</move></input></inputs></state></states>"));
		CHECK_THAT(error, ContainsSubstring("state 'playing' > <input button=\"a\">: <move> does nothing in an <input>"));
		CHECK_THAT(error, ContainsSubstring("<trigger object=\"...\" action=\"...\" />"));

		ObjectXml edge;
		edge.collisions = "<enabled>true</enabled><collision edge=\"left\"><push state=\"playing\" /></collision>";
		CHECK_THAT(errorOf(gameXml("", objectXml(edge))), ContainsSubstring("object 'o' > <collision edge>: <push> does nothing in a screen-edge <collision>"));

		ObjectXml touch;
		touch.collisions = "<enabled>true</enabled><collision><wrap /></collision>";
		CHECK_THAT(errorOf(gameXml("", objectXml(touch))), ContainsSubstring("<wrap /> does nothing in a <collision> with another object"));
	}

	SECTION("a leap with nothing to pull it down or stand on, a climb of no class")
	{
		ObjectXml floats;
		floats.extra = "<actions><action name=\"jump\"><leap>40</leap></action></actions>";
		CHECK_THAT(errorOf(gameXml("", objectXml(floats))), ContainsSubstring("object 'o': action 'jump' has a <leap>, and nothing pulls the object down")
			&& ContainsSubstring("<acceleration> with a <y> above 0"));

		ObjectXml falls;
		falls.velocityY = "0</y></velocity><acceleration><x>0</x><y>0.5</y></acceleration><velocity><x>0</x><y>0";
		std::string xml = gameXml("", objectXml(falls));
		xml.replace(xml.find("<velocity><x>0</x><y>0</y></velocity><collisions>"), std::string("<velocity><x>0</x><y>0</y></velocity>").size(), "");
		xml.replace(xml.find("</collisions>"), 13, "</collisions><actions><action name=\"jump\"><leap>40</leap></action></actions>");
		CHECK_THAT(errorOf(xml), ContainsSubstring("action 'jump' has a <leap>, and the object never stands on anything") && ContainsSubstring("<land />"));

		ObjectXml climber;
		climber.attributes = "class=\"ladder\"";
		climber.extra = "<actions><action name=\"up\"><climb direction=\"up\" class=\"ladders\">2</climb></action></actions>";
		CHECK_THAT(errorOf(gameXml("", objectXml(climber))), ContainsSubstring("action 'up' climbs class=\"ladders\", and no object has that class (did you mean 'ladder'?)"));

		ObjectXml sideways;
		sideways.extra = "<actions><action name=\"up\"><climb direction=\"left\" class=\"x\">2</climb></action></actions>";
		CHECK_THAT(errorOf(gameXml("", objectXml(sideways))), ContainsSubstring("left"));
	}

	SECTION("a variable no object has")
	{
		ObjectXml o;
		o.collisions = "<enabled>true</enabled><collision edge=\"left\"><inc variable=\"score\" /></collision>";
		CHECK_THAT(errorOf(gameXml("<variable name=\"score\">0</variable>", objectXml(o))),
			ContainsSubstring("<inc variable=\"score\" />: 'score' is no variable; name one as owner.variable") && ContainsSubstring("the game's own <variables> are fixed numbers"));

		ObjectXml owned;
		owned.collisions = "<enabled>true</enabled><collision edge=\"left\"><dec variable=\"o.lifes\" /></collision>";
		owned.extra = "<variables><variable name=\"lives\">3</variable></variables>";
		CHECK_THAT(errorOf(gameXml("", objectXml(owned))),
			ContainsSubstring("'o' has no variable named 'lifes' (did you mean 'lives'?); its variables are: lives"));
	}

	SECTION("a state, an object or an action spelt wrong")
	{
		ObjectXml o;
		o.extra = "<actions><action name=\"left\"><move direction=\"left\">1</move></action></actions>";
		CHECK_THAT(errorOf(withStates(gameXml("", objectXml(o)),
			"<states><state name=\"playing\"><shows><show object=\"o\" /></shows><inputs><input button=\"a\"><push state=\"playng\" /></input></inputs></state></states>")),
			ContainsSubstring("(did you mean 'playing'?)"));
		CHECK_THAT(errorOf(withStates(gameXml("", objectXml(o)),
			"<states><state name=\"playing\"><shows><show object=\"o\" /></shows><inputs><input button=\"a\"><trigger object=\"o\" action=\"lfet\" /></input></inputs></state></states>")),
			ContainsSubstring("'o' has no action named 'lfet' (did you mean 'left'?); its actions are: left"));
	}
}

TEST_CASE("sprites: each shape's parts", "[xml_format][sprite]")
{
	std::string objects;

	ObjectXml circle; circle.name = "circle";
	circle.sprite = "<circle><radius>4</radius><color>color.red</color></circle>";
	objects += objectXml(circle);

	ObjectXml rectangle; rectangle.name = "rectangle";
	rectangle.sprite = "<rectangle><width>30</width><height>6</height></rectangle>";
	objects += objectXml(rectangle);

	ObjectXml label; label.name = "label";
	label.sprite = "<text><content>HELLO</content><size>32</size><color>color.blue</color></text>";
	objects += objectXml(label);

	ObjectXml image; image.name = "image";
	image.sprite = "<image><path>assets/paddle.jpg</path><flip>horizontal</flip></image>";
	objects += objectXml(image);

	Loaded loaded(gameXml("", objects));
	Game& game = loaded.game;

	CHECK(game.getObject("circle").picture.kind == ShapeKind::Circle);
	CHECK(game.getObject("circle").picture.radius == 4.0f);
	CHECK(game.getObject("circle").picture.color == "color.red");
	CHECK(game.getObject("rectangle").picture.size == Vector2f{ 30.0f, 6.0f });
	CHECK(game.getObject("rectangle").picture.color == "color.white"); // no <color>: white
	CHECK(game.getObject("label").picture.text == "HELLO");
	CHECK(game.getObject("label").picture.textSize == 32);
	CHECK(game.getObject("label").picture.color == "color.blue");
	CHECK(game.getObject("image").picture.file == "assets/paddle.jpg");
	CHECK(game.getObject("image").picture.flipHorizontal);
	CHECK(game.getObject("circle").picture.kind == ShapeKind::Circle);
	CHECK(game.getObject("image").picture.kind == ShapeKind::Image);
}

TEST_CASE("a text's <number> stays live only when it is a lone owner.variable", "[xml_format][sprite]")
{
	ObjectXml owner; owner.name = "paddle";
	owner.extra = "<variables><variable name=\"score\">3</variable></variables>";

	ObjectXml bound; bound.name = "bound";
	bound.sprite = "<text><number>paddle.score</number><size>20</size></text>";

	ObjectXml sum; sum.name = "sum";
	sum.sprite = "<text><number>paddle.score + 1</number><size>20</size></text>";

	ObjectXml label; label.name = "label";
	label.sprite = "<text><content>paddle.score</content><size>20</size></text>";

	Loaded loaded(gameXml("", objectXml(owner) + objectXml(bound) + objectXml(sum) + objectXml(label)));
	Game& game = loaded.game;

	CHECK(game.getObject("bound").showsVariable("paddle", "score"));
	CHECK(game.getObject("bound").picture.text == "3");

	CHECK(game.getObject("sum").textPieces.empty());
	CHECK(game.getObject("sum").picture.text == "4");

	CHECK(game.getObject("label").textPieces.empty());
	CHECK(game.getObject("label").picture.text == "paddle.score"); // a label is text, not a lookup
}

TEST_CASE("a shape with a part missing says which", "[xml_format][errors]")
{
	ObjectXml noRadius;
	noRadius.sprite = "<circle><color>color.red</color></circle>";
	REQUIRE_THROWS_WITH(Loaded(gameXml("", objectXml(noRadius))), ContainsSubstring("<circle>") && ContainsSubstring("missing <radius>"));

	ObjectXml noLabel;
	noLabel.sprite = "<text><size>20</size></text>";
	REQUIRE_THROWS_WITH(Loaded(gameXml("", objectXml(noLabel))), ContainsSubstring("needs a <content>"));

	ObjectXml unknownShape;
	unknownShape.sprite = "<triangle />";
	REQUIRE_THROWS_WITH(Loaded(gameXml("", objectXml(unknownShape))), ContainsSubstring("unknown shape <triangle>"));
}

TEST_CASE("a condition is a test tag, then the commands it runs", "[xml_format][conditions]")
{
	const std::string conditions =
		"<conditions>"
		"<condition class=\"paddle\" variable=\"score\"><atleast>15</atleast><push state=\"gameover\" /></condition>"
		"<condition object=\"frog\" variable=\"lives\"><atmost>0</atmost><push state=\"a\" /><push state=\"b\" /></condition>"
		"<condition class=\"aliens\"><remaining>window.width.center / 400</remaining><pop /></condition>"
		"</conditions>";
	Loaded loaded(gameXml("", objectXml({}), conditions, idleState("gameover") + idleState("a") + idleState("b")));
	loaded.game.setCurrentState("playing");

	const auto& all = loaded.game.getCurrentState().conditions;
	REQUIRE(all.size() == 3);

	CHECK(all[0].filterClass == "paddle");
	CHECK(all[0].variableName == "score");
	CHECK(all[0].value == 15.0f);
	CHECK(all[0].compare == Compare::AtLeast);
	CHECK_FALSE(all[0].remaining);
	CHECK(as<CmdPushState>(all[0].commands.at(0)).name == "gameover");

	CHECK(all[1].filterObject == "frog");
	CHECK(all[1].compare == Compare::AtMost);
	CHECK(all[1].value == 0.0f);
	CHECK(all[1].commands.size() == 2);

	CHECK(all[2].filterClass == "aliens");
	REQUIRE(all[2].remaining);
	CHECK(*all[2].remaining == 1.0f);
	CHECK(all[2].variableName.empty());
}

TEST_CASE("a condition compares with a number or with another variable, in five ways", "[xml_format][conditions]")
{
	// Two objects, each with a score and a best; the state "playing" moves on
	// to "won" when the first test holds.
	const auto finish = [](const std::string& test, float a, float b, float best)
	{
		const std::string variables = "<variables><variable name=\"score\">0</variable><variable name=\"best\">0</variable></variables>";
		ObjectXml one; one.extra = variables;
		ObjectXml two; two.name = "p"; two.extra = variables;
		Loaded loaded(gameXml("", objectXml(one) + objectXml(two),
			"<conditions><condition object=\"o\" variable=\"score\">" + test + "<push state=\"won\" /></condition></conditions>",
			idleState("won")));
		loaded.game.setCurrentState("playing");
		loaded.game.setVariable("o", "score", a);
		loaded.game.setVariable("p", "score", b);
		loaded.game.setVariable("o", "best", best);
		loaded.game.updateObjects();
		return loaded.game.getCurrentState().name == "won";
	};

	// Another object's variable, as it is when the condition is tested.
	CHECK(finish("<above>p.score</above>", 3, 2, 0));
	CHECK_FALSE(finish("<above>p.score</above>", 2, 2, 0));
	CHECK(finish("<below>p.score</below>", 1, 2, 0));
	CHECK_FALSE(finish("<below>p.score</below>", 2, 2, 0));
	CHECK(finish("<equal>p.score</equal>", 2, 2, 0));
	CHECK_FALSE(finish("<equal>p.score</equal>", 2, 3, 0));
	CHECK(finish("<atleast>p.score</atleast>", 2, 2, 0));
	CHECK_FALSE(finish("<atleast>p.score</atleast>", 1, 2, 0));
	CHECK(finish("<atmost>p.score</atmost>", 2, 2, 0));
	CHECK_FALSE(finish("<atmost>p.score</atmost>", 3, 2, 0));

	// The tested object's own other variable, and arithmetic on one.
	CHECK(finish("<above>o.best</above>", 5, 0, 4));
	CHECK_FALSE(finish("<above>o.best</above>", 4, 0, 4));
	CHECK(finish("<atleast>p.score + 2</atleast>", 5, 3, 0));
	CHECK_FALSE(finish("<atleast>p.score + 2</atleast>", 4, 3, 0));

	// And the new tests with a plain number.
	CHECK(finish("<above>2</above>", 3, 0, 0));
	CHECK_FALSE(finish("<above>2</above>", 2, 0, 0));
	CHECK(finish("<below>2</below>", 1, 0, 0));
	CHECK(finish("<equal>2</equal>", 2, 0, 0));
	CHECK_FALSE(finish("<equal>2</equal>", 2.5f, 0, 0));
}

TEST_CASE("a comparison with a variable the game does not have is turned away", "[xml_format][conditions][errors]")
{
	const auto fails = [](const std::string& test, const std::string& expected)
	{
		INFO(test);
		ObjectXml one; one.extra = "<variables><variable name=\"score\">0</variable></variables>";
		CHECK_THROWS_WITH(Loaded(gameXml("", objectXml(one),
			"<conditions><condition object=\"o\" variable=\"score\">" + test + "<pop /></condition></conditions>")),
			ContainsSubstring(expected));
	};

	fails("<above>q.score</above>", "q.score");
	fails("<above>o.scroe</above>", "o.scroe");
	fails("<above object=\"o\" variable=\"score\" />", "takes its number as content, which may name a variable: <above>o.score</above>");
	fails("<remaining variable=\"score\" />", "takes its number as content");
	fails("<over>1</over>", "expected <atleast>, <atmost>, <above>, <below>, <equal> or <remaining>");
}

TEST_CASE("a condition with no test, a wrong first tag, or no variable to read is turned away", "[xml_format][errors]")
{
	REQUIRE_THROWS_WITH(Loaded(gameXml("", objectXml({}), "<conditions><condition class=\"c\" variable=\"v\" /></conditions>")),
		ContainsSubstring("needs an <atleast>, <atmost>, <above>, <below>, <equal> or <remaining>"));

	REQUIRE_THROWS_WITH(Loaded(gameXml("", objectXml({}), "<conditions><condition class=\"c\" variable=\"v\"><pop /></condition></conditions>")),
		ContainsSubstring("starts with <pop>"));

	REQUIRE_THROWS_WITH(Loaded(gameXml("", objectXml({}), "<conditions><condition class=\"c\"><atleast>1</atleast><pop /></condition></conditions>")),
		ContainsSubstring("needs variable="));
}

// ------------------------------------------------------------------ schema
namespace
{
	std::string readFile(const std::string& path)
	{
		std::ifstream in(path);
		REQUIRE(in.good());
		std::string text = fileText(in);
		text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
		return text;
	}

	// pong.xml with the first occurrence of `from` replaced by `to`.
	std::string pongWith(const std::string& from, const std::string& to)
	{
		std::string xml = readFile("descriptions/pong.xml");
		const auto at = xml.find(from);
		REQUIRE(at != std::string::npos);
		xml.replace(at, from.size(), to);
		return xml;
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
		const ScratchFile scratch{ scratchName("descriptions/xml_format_scratch.xml") };
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
}

TEST_CASE("both schema checkers accept a good game", "[xml_format][schema]")
{
	const Verdict verdict = judge(readFile("descriptions/pong.xml"));

	CHECK(verdict.weakAccepts);
	CHECK(verdict.strongAccepts);
}

TEST_CASE("both schema checkers turn away the same mistakes", "[xml_format][schema]")
{
	struct Mistake
	{
		const char* what;
		const char* from;
		const char* to;
		const char* weakSays; // part of the weak validator's message
	};

	const Mistake mistakes[] = {
		{ "a direction that is not one", "<move direction=\"up\">step</move>", "<move direction=\"sideways\">step</move>", "direction" },
		{ "an unknown tag where a value goes", "<x>margin</x>", "<x><bogus /></x>", "unexpected element <bogus>" },
		{ "a rectangle with no width", "<width>ball.size</width>", "", "expected <width>" },
		{ "text in an element that holds elements", "<object name=\"title\">", "<object name=\"title\">oops", "unexpected text \"oops\"" },
		{ "a flag that is not true or false", "<height>720</height>", "<height>720</height><fullscreen>maybe</fullscreen>", "\"maybe\" is not a valid" },
		{ "a command missing its attribute", "<push state=\"playing\" />", "<push />", "missing required attribute 'state'" },
		{ "a command that does not exist", "<bounce />", "<explode />", "explode" },
		{ "a window width that is not a number", "<width>1280</width>", "<width>huge</width>", "not a valid" },
		{ "a window scale of nothing", "<height>720</height>", "<height>720</height><scale>0</scale>", "not a valid" },
		{ "an edge that is not one", "<collision edge=\"vertical\">", "<collision edge=\"middle\">", "edge" },
		{ "a condition with no test (only a set=\"...\" condition has none)", "<atleast>11</atleast>", "", "unexpected element <push>" },
		{ "a <random> with no max", "<random><min>-30</min><max>30</max></random>", "<random><min>-30</min></random>", "max" },
		{ "several keys in one button=", "<input button=\"w\">", "<input button=\"w up\">", "not a valid" },
	};

	for (const Mistake& mistake : mistakes)
	{
		DYNAMIC_SECTION(mistake.what)
		{
			const Verdict verdict = judge(pongWith(mistake.from, mistake.to));

			CHECK_FALSE(verdict.weakAccepts);
			CHECK_THAT(verdict.weakMessage, ContainsSubstring(mistake.weakSays));
			CHECK_FALSE(verdict.strongAccepts);
		}
	}
}

TEST_CASE("an input names one key by button=, or several by a <button> each", "[xml_format][inputs]")
{
	const auto withInput = [](const std::string& input)
	{
		ObjectXml o;
		std::string xml = gameXml("", objectXml(o));
		const std::string written = "<input button=\"space\"><pop /></input>";
		xml.replace(xml.find(written), written.size(), input);
		return xml;
	};

	Loaded several(withInput("<input><button>space</button><button>p</button><pop /></input>"));
	const auto& input = several.game.getStates().front().input;
	CHECK(input.count(KeyCode::Space) == 1);
	CHECK(input.count(KeyCode::P) == 1);

	REQUIRE_THROWS_WITH(Loaded(withInput("<input button=\"space p\"><pop /></input>")),
		ContainsSubstring("button=\"space p\" names several; write each as a <button> of its own"));
	REQUIRE_THROWS_WITH(Loaded(withInput("<input button=\"space\"><button>p</button><pop /></input>")),
		ContainsSubstring("has button=\"space\" and a <button>; use one or the other"));
	REQUIRE_THROWS_WITH(Loaded(withInput("<input><pop /></input>")),
		ContainsSubstring("names a button"));
}

TEST_CASE("a command that names a state, object or action the game does not have stops the load", "[xml_format][commands]")
{
	const auto withInput = [](const std::string& commands, const std::string& extra = {})
	{
		ObjectXml o;
		o.extra = extra;
		std::string xml = gameXml("", objectXml(o));
		xml.replace(xml.find("<pop />"), 7, commands);
		return xml;
	};

	REQUIRE_THROWS_WITH(Loaded(withInput("<push state=\"pasued\" />")),
		ContainsSubstring("state 'playing'") && ContainsSubstring("pasued"));
	REQUIRE_THROWS_WITH(Loaded(withInput("<trigger object=\"nobody\" action=\"go\" />")),
		ContainsSubstring("no object of that name"));
	REQUIRE_THROWS_WITH(Loaded(withInput("<trigger object=\"o\" action=\"jump\" />")),
		ContainsSubstring("'o' has no action named 'jump'"));
	REQUIRE_THROWS_WITH(Loaded(withInput("<fire object=\"bullet\" />")),
		ContainsSubstring("bullet"));
	REQUIRE_THROWS_WITH(Loaded(withInput("<reset object=\"nobody\" />")),
		ContainsSubstring("nobody"));

	// A condition is checked the same way as a key.
	REQUIRE_THROWS_WITH(Loaded(gameXml("", objectXml({}), "<conditions><condition object=\"o\" variable=\"v\"><atleast>1</atleast><push state=\"over\" /></condition></conditions>")),
		ContainsSubstring("over"));

	// The names that are there load as before.
	CHECK_NOTHROW(Loaded(withInput("<trigger object=\"o\" action=\"go\" />",
		"<actions><action name=\"go\"><move direction=\"left\">1</move></action></actions>")));
}

TEST_CASE("the state a game starts in is never popped", "[xml_format][states]")
{
	Loaded loaded(gameXml("", objectXml({}), {}, idleState("paused")));
	loaded.game.setCurrentState(0);
	loaded.game.pushState("paused");

	loaded.game.popState();
	CHECK(loaded.game.getCurrentState().name == "playing");

	const unsigned long changes = loaded.game.stateChangeCount();
	loaded.game.popState();
	CHECK(loaded.game.getCurrentState().name == "playing");
	CHECK(loaded.game.stateChangeCount() == changes);

	CHECK_THROWS_AS(loaded.game.pushState("nowhere"), std::out_of_range);
}
