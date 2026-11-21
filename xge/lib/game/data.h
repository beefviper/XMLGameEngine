// data.h
// XML Game Engine
// author: beefviper
// date: Oct 11, 2026

#pragma once

#include "game/command.h"
#include "game/object.h"
#include "game/states.h"
#include "picture/image.h"

#include <string>
#include <utility>
#include <vector>

namespace xge
{
	// A key of a recorded game: pressed or let go, at a time in seconds
	// from the start of the recording.
	struct DemoKey
	{
		std::string button;
		bool pressed{ true };
		double seconds{ 0 };
	};

	// A <demo>: a game recorded with xgecli --record, which <demo /> plays
	// back (Engine). It starts from a new game, with the random numbers
	// drawn from `seed`, and lasts `seconds`. None when `recorded` is false.
	struct WrittenDemo
	{
		bool recorded{ false };
		unsigned int seed{ 0 };
		double seconds{ 0 };
		std::vector<DemoKey> keys;
	};

	// A game as written, before any of it is worked out: what load/xml reads
	// from a game file (readGame), or what a generated program writes out as
	// C++ (xge/cli/source/generate.cpp). Game works it out (load/expr) and runs
	// it, the same way whichever it came from.
	struct GameData
	{
		WindowDesc window;
		std::vector<std::pair<std::string, WrittenValue>> variables;
		std::vector<WrittenState> states;
		std::vector<WrittenObject> objects;
		std::vector<WrittenSound> sounds;
		std::vector<WrittenPath> paths;

		// <styles>: what a <text> or a <bitmap> can take by style="..." (see applyStyles).
		std::vector<WrittenStyle> styles;

		// <sheets>: the picture files sprites, animations and packs cut their
		// pictures from (see Sheet).
		std::vector<Sheet> sheets;

		// <patterns>: what holders with a <score> are scored by (see WrittenPatterns).
		std::vector<WrittenPatterns> patterns;

		// <keeps>: the objects (or groups) a full <reset /> passes over, as
		// named in the file.
		std::vector<std::string> keeps;

		// <demo>: a recorded game the attract mode plays (see WrittenDemo).
		WrittenDemo demo;


		// How it was checked against xgedef.xsd, as printGame() says it.
		std::string validation;
	};
}
