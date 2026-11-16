// test_names.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026
//
// Catch2 tests that a name which matches nothing stops the load, saying
// what was meant: a misspelt class or object in a rule would never match,
// and the game would quietly do nothing.

#include "file_text.h"
#include "played.h"
#include "scratch.h"
#include "game/game.h"
#include "load/xml.h"
#include "xml/document.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace xge;
using Catch::Matchers::ContainsSubstring;
using played::Scratch;

namespace
{
	// A frog of class "frog", a log of class "log", and the frog's rules.
	std::string pond(const std::string& rules)
	{
		return "<object name=\"frog\" class=\"frog\"><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite>"
			"<position><x>100</x><y>100</y></position><collisions>" + rules + "</collisions></object>"
			+ "<object name=\"log\" class=\"log\"><sprite><rectangle><width>40</width><height>10</height></rectangle></sprite>"
			"<position><x>100</x><y>100</y></position></object>";
	}

	std::string errorOf(const std::string& objects)
	{
		const Scratch file(objects, "<pop />");
		try
		{
			Game game(file.path);
		}
		catch (const std::exception& error)
		{
			return error.what();
		}
		return "";
	}
}

TEST_CASE("a collision rule's class, object, unless and through name something the game has", "[names]")
{
	CHECK(errorOf(pond("<collision class=\"log\"><ride /></collision>")).empty());
	CHECK(errorOf(pond("<collision object=\"log\"><ride /></collision>")).empty());

	CHECK_THAT(errorOf(pond("<collision class=\"lgo\"><ride /></collision>")),
		ContainsSubstring("object 'playing.frog' > <collision class=\"lgo\">: no object has the class 'lgo'") && ContainsSubstring("did you mean 'log'"));
	CHECK_THAT(errorOf(pond("<collision object=\"lgo\"><ride /></collision>")),
		ContainsSubstring("<collision object=\"lgo\">: there is no object or group named 'lgo'") && ContainsSubstring("did you mean 'log'"));
	CHECK_THAT(errorOf(pond("<collision class=\"log\" unless=\"flog\"><ride /></collision>")),
		ContainsSubstring("<collision unless=\"flog\">: no object has the class 'flog'"));
	CHECK_THAT(errorOf(pond("<collision edge=\"bottom\" unless=\"lgo\"><reset /></collision>")),
		ContainsSubstring("<collision unless=\"lgo\">: no object has the class 'lgo'"));
}

TEST_CASE("a condition's class and object name something the game has", "[names]")
{
	const auto stateWith = [](const std::string& condition)
	{
		const Scratch file(pond(""), "<pop />", "<conditions>" + condition + "</conditions>");
		try
		{
			Game game(file.path);
		}
		catch (const std::exception& error)
		{
			return std::string(error.what());
		}
		return std::string();
	};
	CHECK(stateWith("<condition class=\"log\" total=\"count\"><atmost>0</atmost><pop /></condition>").empty());
	CHECK_THAT(stateWith("<condition class=\"lgo\" total=\"count\"><atmost>0</atmost><pop /></condition>"),
		ContainsSubstring("class=\"lgo\": no object has that class") && ContainsSubstring("did you mean 'log'"));
	CHECK_THAT(stateWith("<condition object=\"forg\" variable=\"x\"><atmost>0</atmost><pop /></condition>"),
		ContainsSubstring("object=\"forg\" names no object or group") && ContainsSubstring("did you mean 'frog'"));
	CHECK_THAT(stateWith("<condition><any><condition object=\"frog\" variable=\"x\"><atleast>0</atleast></condition>"
		"<condition class=\"lgo\" total=\"count\"><atleast>1</atleast></condition></any><pop /></condition>"),
		ContainsSubstring("class=\"lgo\""));
}

TEST_CASE("a condition in an action or a rule's check names something the game has too", "[names]")
{
	CHECK_THAT(errorOf(pond("") + played::box("hero", 300, 300, "<actions><action name=\"go\"><condition class=\"lgo\" total=\"count\"><atleast>1</atleast><reverse /></condition></action></actions>")),
		ContainsSubstring("class=\"lgo\": no object has that class"));
	CHECK_THAT(errorOf(pond("<collision class=\"log\"><condition object=\"forg\" variable=\"x\"><atleast>0</atleast></condition><ride /></collision>")),
		ContainsSubstring("object=\"forg\" names no object or group"));
}

TEST_CASE("a condition's variable is one that what it looks at has, or a number the engine keeps", "[names]")
{
	const auto stateWith = [](const std::string& condition)
	{
		const Scratch file(pond("") + played::box("status", 0, 0, "<variables><variable name=\"lives\">3</variable></variables>"), "<pop />", "<conditions>" + condition + "</conditions>");
		try
		{
			Game game(file.path);
		}
		catch (const std::exception& error)
		{
			return std::string(error.what());
		}
		return std::string();
	};
	CHECK(stateWith("<condition object=\"status\" variable=\"lives\"><atmost>0</atmost><pop /></condition>").empty());
	CHECK(stateWith("<condition object=\"frog\" variable=\"x\"><atmost>0</atmost><pop /></condition>").empty());
	CHECK_THAT(stateWith("<condition object=\"status\" variable=\"lifes\"><atmost>0</atmost><pop /></condition>"),
		ContainsSubstring("variable=\"lifes\": nothing the condition looks at has a variable or number of that name")
		&& ContainsSubstring("did you mean 'lives'") && ContainsSubstring("its variables are: lives"));
}

TEST_CASE("a state shows what the game has, and a trigger's from= is a look it has", "[names]")
{
	const auto loads = [](const std::string& objects, const std::string& onSpace, const std::string& shows = {})
	{
		const std::string path = scratchName("descriptions/names_scratch.xml");
		{
			std::ofstream out(path);
			out << "<game xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"../xgedef.xsd\">"
				"<window name=\"scratch\"><width>800</width><height>600</height><background>color.black</background>"
				"<framerate>60</framerate></window>"
				"<variables><variable name=\"unused\">0</variable></variables>"
				"<states><state name=\"playing\"><objects>" << objects << "</objects>" << shows
				<< "<inputs><input button=\"space\">" << onSpace << "</input></inputs></state></states></game>";
		}
		std::string message;
		try
		{
			Game game(path);
		}
		catch (const std::exception& error)
		{
			message = error.what();
		}
		std::error_code ignored;
		std::filesystem::remove(path, ignored);
		return message;
	};
	CHECK_THAT(loads(pond(""), "<pop />", "<shows><show object=\"forg\" /></shows>"),
		ContainsSubstring("<show object=\"forg\" />: there is no object or group of that name") && ContainsSubstring("did you mean 'frog'"));

	const std::string lamp = "<object name=\"lamp\"><sprite name=\"off\"><rectangle><width>10</width><height>10</height></rectangle></sprite>"
		"<sprite name=\"on\"><rectangle><width>10</width><height>10</height></rectangle></sprite><position><x>0</x><y>0</y></position>"
		"<actions><action name=\"flip\"><become sprite=\"on\" /></action></actions></object>";
	CHECK(loads(lamp, "<trigger object=\"lamp\" action=\"flip\" from=\"off\" />").empty());
	CHECK_THAT(loads(lamp, "<trigger object=\"lamp\" action=\"flip\" from=\"of\" />"),
		ContainsSubstring("from=\"of\">: 'playing.lamp' has no look of that name") && ContainsSubstring("did you mean 'off'") && ContainsSubstring("its looks are: off, on"));
}

TEST_CASE("what a holder keeps its cards in order by is a variable some object has", "[names]")
{
	std::ifstream in("descriptions/hearts.xml", std::ios::binary);
	std::string text = fileText(in);
	const std::string by = "<by variable=\"suit\" />";
	REQUIRE(text.find(by) != std::string::npos);
	text.replace(text.find(by), by.size(), "<by variable=\"siut\" />");
	const std::string path = scratchName("descriptions/names_scratch.xml");
	std::ofstream(path, std::ios::binary | std::ios::trunc) << text;
	std::string message;
	try
	{
		Game game(path);
	}
	catch (const std::exception& error)
	{
		message = error.what();
	}
	std::error_code ignored;
	std::filesystem::remove(path, ignored);
	CHECK_THAT(message, ContainsSubstring("<by variable=\"siut\">: no object has a variable or number of that name") && ContainsSubstring("did you mean 'suit'"));
}

TEST_CASE("an object, group, map or state may not take a name the engine's words start with, and nothing is a number", "[names]")
{
	CHECK_THAT(errorOf(played::box("self", 0, 0)), ContainsSubstring("an object may not be called 'self'"));
	CHECK_THAT(errorOf(played::box("window", 0, 0)), ContainsSubstring("an object may not be called 'window'"));
	CHECK_THAT(errorOf(played::box("7", 0, 0)), ContainsSubstring("an object may not be called '7': a number reads that way"));
	CHECK_THAT(errorOf(played::box("o", 0, 0, "<variables><variable name=\"2\">0</variable></variables>")), ContainsSubstring("a variable may not be called '2'"));
	// A variable is only read after a dot, so a word is free there.
	CHECK(errorOf(played::box("card", 0, 0, "<variables><variable name=\"color\">1</variable><variable name=\"top\">0</variable></variables>")).empty());
}

TEST_CASE("a misspelt name says the line of what holds it, under every XML library", "[names][line_numbers]")
{
	// No schema is named, so only the checks after reading see these; each
	// case misspells one name of a game that otherwise loads.
	const std::string file = scratchName("descriptions/names_lines.xml");
	const auto write = [&](const std::string& rule, const std::string& triggered, const std::string& condition, const std::string& shown)
	{
		std::ofstream(file) << "<game>\n"
			"  <window name=\"lines\"><width>800</width><height>600</height><background>color.black</background><framerate>60</framerate></window><variables><variable name=\"unused\">0</variable></variables>\n"
			"  <states>\n"
			"    <state name=\"playing\">\n"
			"      <objects>\n"
			"        <object name=\"frog\" class=\"frog\"><sprite><rectangle><width>10</width><height>10</height></rectangle></sprite><position><x>100</x><y>100</y></position>\n"
			"          <collisions>\n"
			"            <collision class=\"" << rule << "\"><ride /></collision>\n"
			"          </collisions>\n"
			"          <actions><action name=\"go\">\n"
			"            <trigger object=\"" << triggered << "\" action=\"sink\" />\n"
			"          </action></actions>\n"
			"        </object>\n"
			"        <object name=\"log\" class=\"log\"><sprite><rectangle><width>40</width><height>10</height></rectangle></sprite><position><x>100</x><y>100</y></position>\n"
			"          <actions><action name=\"sink\"><stop /></action></actions>\n"
			"        </object>\n"
			"      </objects>\n"
			"      <inputs><input button=\"space\"><pop /></input></inputs>\n"
			"      <conditions>\n"
			"        <condition object=\"frog\" variable=\"" << condition << "\"><atmost>0</atmost><pop /></condition>\n"
			"      </conditions>\n"
			"      <shows><show object=\"" << shown << "\" /></shows>\n"
			"    </state>\n"
			"  </states>\n"
			"</game>\n";
	};
	const auto errorOf = [&](XmlBackend backend)
	{
		try
		{
			Game game(readGame(file, backend));
		}
		catch (const std::exception& error)
		{
			return std::string(error.what());
		}
		return std::string();
	};
	const auto endsWith = [](const std::string& text, const std::string& end)
	{
		return text.size() >= end.size() && text.compare(text.size() - end.size(), end.size(), end) == 0;
	};

	struct Case
	{
		const char* what;
		const char* rule;
		const char* triggered;
		const char* condition;
		const char* shown;
		const char* says;
		const char* line;
	};
	const Case cases[] = {
		{ "a rule's class", "lgo", "log", "x", "frog", "no object has the class 'lgo'", " (line 8)" },
		{ "a command's object, in an action: the object's line", "log", "lgo", "x", "frog", "an action of 'lgo' is asked for", " (line 6)" },
		{ "a condition's variable", "log", "log", "z", "frog", "variable=\"z\"", " (line 20)" },
		{ "a state's show", "log", "log", "x", "forg", "<show object=\"forg\" />", " (line 4)" },
	};
	for (const XmlBackend backend : XmlDocumentFactory::availableBackends())
	{
		INFO(XmlDocumentFactory::name(backend));
		write("log", "log", "x", "frog");
		CHECK(errorOf(backend).empty());
		for (const Case& each : cases)
		{
			INFO(each.what);
			write(each.rule, each.triggered, each.condition, each.shown);
			const std::string message = errorOf(backend);
			INFO(message);
			CHECK_THAT(message, ContainsSubstring(each.says));
			CHECK(endsWith(message, each.line));
		}
	}
	std::filesystem::remove(file);
}
