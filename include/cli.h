// cli.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include <string>

namespace xge
{
	// Resolves which game XML file to load from the command line.
	//
	// No argument: defaults to the game name "pong". Given one argument, it's
	// taken as the game name (e.g. "pong") or the game's filename (e.g.
	// "pong.xml", "breakout.xml") - a name with no '.' gets ".xml" appended,
	// anything else is used exactly as given.
	//
	// The resulting filename is looked for first in the current directory,
	// then in a "games" directory beneath it (std::filesystem::exists()
	// naturally returns false there too if "games" doesn't exist, so there's
	// no separate directory-existence check needed). If neither has it, this
	// prints an error and terminates the program - main() never has to
	// handle a "not found" case itself.
	std::string resolveGameFilename(int argc, char* argv[]);
}
