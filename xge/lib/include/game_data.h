// game_data.h
// XML Game Engine
// author: beefviper
// date: Oct 11, 2026

#pragma once

#include "command.h"
#include "object.h"
#include "states.h"

#include <string>
#include <utility>
#include <vector>

namespace xge
{
	// A game as written, before any of it is worked out: what game_xml reads
	// from a game file (readGame), or what a generated program writes out as
	// C++ (xge/cli/source/generate.cpp). Game works it out (game_expr) and runs
	// it, the same way whichever it came from.
	struct GameData
	{
		WindowDesc window;
		std::vector<std::pair<std::string, RawValue>> variables;
		std::vector<RawState> states;
		std::vector<RawObject> objects;
		std::vector<RawSound> sounds;
		std::vector<RawPath> paths;

		// <keeps>: the objects (or groups) a full <reset /> passes over, as
		// named in the file.
		std::vector<std::string> keeps;

		// How it was checked against xgedef.xsd, as printGame() says it.
		std::string validation;
	};
}
