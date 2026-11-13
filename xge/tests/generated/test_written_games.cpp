// test_written_games.cpp
// XML Game Engine
// author: beefviper
// date: Nov 13, 2026
//
// Catch2 tests that every game windows-cpp writes (written_games.h, from
// scripts/cmake/tests.cmake) does what its file says: the generated copy is
// compiled in, and the checks for that game (xge/tests/written/<game>.h)
// played on it.

#include "written_games.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("every game windows-cpp writes does what its file says", "[generate][written_games]")
{
	for (const WrittenGame& each : writtenGames)
	{
		INFO(each.game);
		each.plays();
	}
}
