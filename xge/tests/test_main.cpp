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

#include "engine/folder.h"

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
	};
}

CATCH_REGISTER_LISTENER(EnterGamesFolder)
