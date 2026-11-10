// file_text.h
// XML Game Engine
// author: beefviper
// date: Oct 29, 2026
//
// All that is left to read of a stream, as one string: how a test reads a
// game file whole. Read through its buffer, not character by character
// (an istreambuf_iterator pair, which GCC's -Wnull-dereference flags).

#pragma once

#include <istream>
#include <sstream>
#include <string>

inline std::string fileText(std::istream& in)
{
	std::ostringstream text;
	text << in.rdbuf();
	return text.str();
}
