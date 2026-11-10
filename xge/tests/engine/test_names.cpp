// test_names.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026
//
// Catch2 tests that a name which matches nothing stops the load, saying
// what was meant: a misspelt class or object in a rule would never match,
// and the game would quietly do nothing.

#include "played.h"
#include "game/game.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

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
			"<position><x>100</x><y>100</y></position><collisions><enabled>true</enabled>" + rules + "</collisions></object>"
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
