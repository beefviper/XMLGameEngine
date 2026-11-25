// test_autogen_breakout.cpp
// XML Game Engine
// author: beefviper
// date: Nov 24, 2026
//
// Catch2 tests that the Breakout kept in games/autogen/windows-cpp/breakout
// (what the windows-cpp generator writes for breakout.xml) does what
// breakout.xml says (xge/tests/written/breakout.h).

#include "breakout.h"

#include "written/breakout.h"

TEST_CASE("the breakout windows-cpp writes does what breakout.xml says", "[autogen][breakout]")
{
	breakout_plays<breakout::Breakout>();
}
