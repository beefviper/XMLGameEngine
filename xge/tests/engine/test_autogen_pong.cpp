// test_autogen_pong.cpp
// XML Game Engine
// author: beefviper
// date: Nov 13, 2026
//
// Catch2 tests that the Pong kept in games/autogen/windows-cpp/pong (what
// the windows-cpp generator writes for pong.xml) does what pong.xml says
// (xge/tests/written/pong.h).

#include "pong.h"

#include "written/pong.h"

TEST_CASE("the pong windows-cpp writes does what pong.xml says", "[autogen][pong]")
{
	pong_plays<pong::Pong>();
}
