// test_prototype_breakout.cpp
// XML Game Engine
// author: beefviper
// date: Nov 24, 2026
//
// Catch2 tests that the stand-alone Breakout (games/prototypes/breakout,
// what the windows-cpp generator writes for breakout.xml) does what
// breakout.xml says (xge/tests/written/breakout.h).

#include "breakout.h"

#include "written/breakout.h"

TEST_CASE("the hand-written breakout does what breakout.xml says", "[prototype][breakout]")
{
	breakout_plays<breakout::Breakout>();
}
