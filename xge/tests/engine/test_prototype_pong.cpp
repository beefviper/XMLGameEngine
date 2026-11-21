// test_prototype_pong.cpp
// XML Game Engine
// author: beefviper
// date: Oct 12, 2026
//
// Catch2 tests that the hand-written pong_min (games/prototypes/pong_min,
// what the windows-cpp generator writes) does what pong_min.xml says
// (xge/tests/written/pong_min.h).

#include "pong_min.h"

#include "written/pong_min.h"

TEST_CASE("the hand-written pong_min does what pong_min.xml says", "[prototype]")
{
	pong_min_plays<pong_min::Pong>();
}
