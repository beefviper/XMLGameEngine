// generate.h
// XML Game Engine
// author: beefviper
// date: Oct 5, 2026

#pragma once

#include "game_data.h"

#include <filesystem>
#include <iosfwd>
#include <stdexcept>
#include <string>
#include <vector>

namespace xge
{
	// A game that could not be generated: a target there is no generator for,
	// a game file that does not load, or a file that could not be written or
	// copied. what() says which.
	class GenerateError : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	struct GenerateRequest
	{
		std::filesystem::path gameFile;    // the game (games/pong.xml)
		std::string target;                // the generator ("windows-cpp")
		std::filesystem::path dataFolder;  // where the game's assets/ and the engine's lib/ are looked for
		std::filesystem::path output;      // the folder the program is written to
	};

	// What was written to the output folder, as paths relative to it: the
	// program's own files and the engine's files copied beside them, and the
	// game's assets.
	struct GeneratedProgram
	{
		std::vector<std::filesystem::path> files;
		std::vector<std::filesystem::path> assets;
	};

	// The engine's files a generated program compiles, as they are in lib/:
	// the whole engine but for reading XML and choosing among libraries, with
	// SFML's window and sound.
	const std::vector<std::filesystem::path>& engineFiles();

	// Writes `game` as C++: a function named `function` that returns it,
	// field by field, with the helpers it uses. What source/<game>.cpp of a
	// generated program holds; `from` is the game file, for its comments.
	void writeGameData(std::ostream& out, const GameData& game, const std::string& function, const std::string& from);

	// The name of the function writeGameData writes for a game file:
	// pong_min.xml is pongMinGame.
	std::string gameFunctionName(const std::filesystem::path& gameFile);

	// Turns a game into a program of its own (target windows-cpp): the game
	// read as the engine reads it and written out as C++ data
	// (writeGameData), a main() that plays it with the engine's own Game and
	// Engine on SFML, the engine's files copied in unchanged (engineFiles),
	// the assets it uses, and its build. The output folder is made if it is
	// not there, and removed again if it was made and the game cannot be
	// generated. Throws GenerateError.
	GeneratedProgram generateGame(const GenerateRequest& request);
}
