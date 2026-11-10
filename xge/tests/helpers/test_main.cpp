// test_main.cpp
// XML Game Engine
// author: beefviper
// date: Oct 13, 2026
//
// The tests load the shipped games as descriptions/<game>.xml and their media
// as assets/, relative to the working directory, the way xgecli and xgegui
// play them: from inside the games folder. ctest starts xgetest there; this
// listener moves into it (see engine/folder.h) when xgetest is started from
// anywhere else, such as the top of the repository.
//
// The engine's randomness (<random>, a wander's turns) is seeded from Catch's
// own seed as each test case starts, so a run that fails is played again,
// exactly, with the seed it printed: xgetest --rng-seed N. Without it the
// engine drew its seed from the machine, and a failure seen once could not
// be brought back. A test that fixes a seed of its own still can.

#include "engine/folder.h"
#include "game/random.h"

#include <catch2/catch_get_random_seed.hpp>

#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

namespace
{
	class EnterGamesFolder : public Catch::EventListenerBase
	{
	public:
		using Catch::EventListenerBase::EventListenerBase;

		void testRunStarting(const Catch::TestRunInfo&) override
		{
			xge::enterDataFolder();
		}

		void testCaseStarting(const Catch::TestCaseInfo&) override
		{
			xge::fixRandomSeed(Catch::getSeed());
		}
	};
}

CATCH_REGISTER_LISTENER(EnterGamesFolder)
