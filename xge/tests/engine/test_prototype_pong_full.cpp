// test_prototype_pong_full.cpp
// XML Game Engine
// author: beefviper
// date: Nov 13, 2026
//
// Catch2 tests that the stand-alone Pong (games/prototypes/pong, what the
// windows-cpp generator writes for pong.xml) does what pong.xml says
// (xge/tests/written/pong.h).

#include "pong.h"

#include "written/pong.h"

TEST_CASE("the hand-written pong does what pong.xml says", "[prototype][pong]")
{
	pong_plays<pong::Pong>();
}
