// test_conditions.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026
//
// Game::checkConditions and its matchesClassOrObjectFilter helper live in an
// anonymous namespace inside game.cpp (internal linkage - not callable from
// outside that translation unit), so this is a small, dependency-free
// reimplementation of just the matching/threshold logic - copied from
// Game::checkConditions and matchesClassOrObjectFilter in source/game.cpp -
// kept in sync with that code and re-verified here rather than re-derived by
// hand each time it changes. (Exposing the real helper - e.g. moving it out
// of the anonymous namespace into a header-declared free function - would let
// this test call it directly instead; that's a production-code change beyond
// what this test conversion is doing.)

#include <catch2/catch_test_macros.hpp>

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace
{
	struct Candidate
	{
		std::string objClass;
		std::string name;
		std::map<std::string, float> variable;
	};

	struct Condition
	{
		std::string filterClass;
		std::string filterObject;
		std::string variableName;
		float value{};
	};

	// Copied verbatim (modulo the Object -> Candidate rename) from
	// source/game.cpp's anonymous namespace.
	bool matchesClassOrObjectFilter(const std::string& filterClass, const std::string& filterObject, const Candidate& candidate)
	{
		if (!filterClass.empty() && filterClass != candidate.objClass) { return false; }
		if (!filterObject.empty() && filterObject != candidate.name) { return false; }
		return true;
	}

	// Mirrors Game::checkConditions' inner search: the first candidate whose
	// variable has reached the threshold wins; returns its name, or nullopt
	// if the condition doesn't fire against any candidate.
	std::optional<std::string> firstMatch(const Condition& condition, const std::vector<Candidate>& candidates)
	{
		for (const auto& candidate : candidates)
		{
			if (!matchesClassOrObjectFilter(condition.filterClass, condition.filterObject, candidate))
			{
				continue;
			}

			auto it = candidate.variable.find(condition.variableName);
			if (it == candidate.variable.end() || it->second < condition.value)
			{
				continue;
			}

			return candidate.name;
		}
		return std::nullopt;
	}

	// pong.xml's actual shape: two paddles sharing class "paddle", each with
	// its own "score" variable.
	const std::vector<Candidate> kPaddles = {
		{ "paddle", "paddle1", { { "score", 12.0f } } },
		{ "paddle", "paddle2", { { "score", 15.0f } } },
	};
}

TEST_CASE("a class filter fires on the first candidate past the threshold", "[conditions]")
{
	Condition winByClass{ "paddle", "", "score", 15.0f };
	auto hit = firstMatch(winByClass, kPaddles);

	REQUIRE(hit.has_value());
	CHECK(*hit == "paddle2"); // 15 >= 15; paddle1's 12 doesn't qualify
}

TEST_CASE("a class filter doesn't fire before any candidate reaches the threshold", "[conditions]")
{
	Condition notYet{ "paddle", "", "score", 20.0f };
	CHECK_FALSE(firstMatch(notYet, kPaddles).has_value());
}

TEST_CASE("an object filter only ever checks that one object", "[conditions]")
{
	Condition byObject{ "", "paddle1", "score", 10.0f };
	auto hit = firstMatch(byObject, kPaddles);

	REQUIRE(hit.has_value());
	CHECK(*hit == "paddle1"); // even though paddle2 also qualifies
}

TEST_CASE("class and object filters combine with AND, not OR", "[conditions]")
{
	SECTION("a mismatched combination matches nothing")
	{
		// object must be *of that class* AND be *that specific object* -
		// paddle1 isn't class 'enemy', so this matches nothing.
		Condition bothMismatched{ "enemy", "paddle1", "score", 10.0f };
		CHECK_FALSE(firstMatch(bothMismatched, kPaddles).has_value());
	}

	SECTION("a matching combination fires normally")
	{
		Condition bothMatching{ "paddle", "paddle1", "score", 10.0f };
		auto hit = firstMatch(bothMatching, kPaddles);

		REQUIRE(hit.has_value());
		CHECK(*hit == "paddle1");
	}
}

TEST_CASE("no filters at all matches any candidate, like an unfiltered collision rule", "[conditions]")
{
	Condition unfiltered{ "", "", "score", 15.0f };
	auto hit = firstMatch(unfiltered, kPaddles);

	REQUIRE(hit.has_value());
	CHECK(*hit == "paddle2");
}

TEST_CASE("a variable name no candidate declares never fires, and never crashes", "[conditions]")
{
	Condition missingVariable{ "paddle", "", "lives", 1.0f };
	CHECK_FALSE(firstMatch(missingVariable, kPaddles).has_value());
}
