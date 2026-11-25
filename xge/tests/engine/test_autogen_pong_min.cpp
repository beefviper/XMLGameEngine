// test_autogen_pong_min.cpp
// XML Game Engine
// author: beefviper
// date: Oct 12, 2026
//
// Catch2 tests that the pong_min kept in games/autogen/windows-cpp/pong_min
// (what the windows-cpp generator writes for pong_min.xml) does what
// pong_min.xml says (xge/tests/written/pong_min.h).

#include "pong_min.h"

#include "written/pong_min.h"

TEST_CASE("the pong_min windows-cpp writes does what pong_min.xml says", "[autogen]")
{
	pong_min_plays<pong_min::Pong>();
}
