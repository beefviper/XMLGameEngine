// test_random_keys.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026
//
// A long test (xgetest-long): every game played for 20,000 frames of random
// presses of the keys it binds, from two seeds, on the null window. Nothing
// may throw, and no position or speed may stop being a finite number. A
// short run of the same is in xgetest (engine/test_vocabulary.cpp).

#include "cli.h"
#include "random_keys.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

TEST_CASE("every game survives 20,000 frames of random keys, from two seeds", "[long][keys]")
{
	for (const std::string& file : played::shippedGames(xge::gameFilesIn("descriptions")))
	{
		for (const unsigned seed : { 1u, 2u }) { played::playRandomKeys(file, seed, 20000); }
	}
}
