// verify_conditions.cpp
// XML Game Engine
// author: beefviper
// date: Sept 27, 2026
//
// Not part of the build (see CMakeLists.txt - it isn't listed there).
// Game::checkConditions and its matchesClassOrObjectFilter helper live
// inside game.cpp/game.h, which pull in Object (SFML graphics) and can't be
// linked in this sandbox (see verify_collision_geometry.cpp's header for
// why). This is a small, dependency-free reimplementation of just the
// matching/threshold logic - copied from Game::checkConditions and
// matchesClassOrObjectFilter in source/game.cpp - kept in sync with that
// code and re-verified here rather than re-derived by hand each time it
// changes. Build and run it with:
//   g++ -std=c++20 tests/verify_conditions.cpp -o verify_conditions && ./verify_conditions

#include <cstdio>
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

	int failures = 0;

	void expect(bool ok, const std::string& description)
	{
		std::printf("%s: %s\n", ok ? "PASS" : "FAIL", description.c_str());
		if (!ok) { ++failures; }
	}
}

int main()
{
	// pong_full.xml's actual shape: two paddles sharing class "paddle",
	// each with its own "score" variable.
	const std::vector<Candidate> paddles = {
		{ "paddle", "paddle1", { { "score", 12.0f } } },
		{ "paddle", "paddle2", { { "score", 15.0f } } },
	};

	{
		Condition winByClass{ "paddle", "", "score", 15.0f };
		auto hit = firstMatch(winByClass, paddles);
		expect(hit.has_value() && *hit == "paddle2", "class=paddle, value=15 fires on paddle2 (15 >= 15), skips paddle1 (12 < 15)");
	}
	{
		Condition notYet{ "paddle", "", "score", 20.0f };
		expect(!firstMatch(notYet, paddles).has_value(), "class=paddle, value=20 doesn't fire - neither paddle has reached it yet");
	}
	{
		Condition byObject{ "", "paddle1", "score", 10.0f };
		auto hit = firstMatch(byObject, paddles);
		expect(hit.has_value() && *hit == "paddle1", "object=paddle1 only ever checks paddle1, even though paddle2 also qualifies");
	}
	{
		// Both filters given: object must be *of that class* AND be *that
		// specific object* - a mismatched combination matches nothing.
		Condition bothMismatched{ "enemy", "paddle1", "score", 10.0f };
		expect(!firstMatch(bothMismatched, paddles).has_value(), "class=enemy + object=paddle1 matches nothing - paddle1 isn't class 'enemy'");
	}
	{
		Condition bothMatching{ "paddle", "paddle1", "score", 10.0f };
		auto hit = firstMatch(bothMatching, paddles);
		expect(hit.has_value() && *hit == "paddle1", "class=paddle + object=paddle1 (both correct) fires on paddle1");
	}
	{
		// No filters at all matches any candidate - same as an unfiltered
		// collision "basic" rule.
		Condition unfiltered{ "", "", "score", 15.0f };
		auto hit = firstMatch(unfiltered, paddles);
		expect(hit.has_value() && *hit == "paddle2", "no filters at all matches anything - still finds paddle2's 15");
	}
	{
		Condition missingVariable{ "paddle", "", "lives", 1.0f };
		expect(!firstMatch(missingVariable, paddles).has_value(), "a variable name neither paddle declares never fires (no crash, no false positive)");
	}

	std::printf("\n%d failure(s)\n", failures);
	return failures == 0 ? 0 : 1;
}
