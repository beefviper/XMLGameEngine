// generate.h
// XML Game Engine
// author: beefviper
// date: Oct 5, 2026

#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace xge
{
	// A game that could not be generated: no generator for the target, a game
	// the engine will not load, or a file that could not be written or
	// copied. what() says which, with the stylesheet's own message when it
	// stopped.
	class GenerateError : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	struct GenerateRequest
	{
		std::filesystem::path gameFile;    // the game (games/pong.xml)
		std::string target;                // the generator ("windows-cpp")
		std::filesystem::path generators;  // the folder holding a folder of stylesheets per target (generators/)
		std::filesystem::path dataFolder;  // where the game's assets/ and the engine's lib/ are looked for
		std::filesystem::path output;      // the folder the program is written to
	};

	// What was written to the output folder, as paths relative to it: the
	// program's files (those the stylesheet wrote, and the target's modules
	// and the engine's files copied beside them), and the game's assets.
	struct GeneratedProgram
	{
		std::vector<std::filesystem::path> files;
		std::vector<std::filesystem::path> assets;
	};

	// Whether this program was built with libxslt, which --generate needs.
	bool canGenerate() noexcept;

	// The name of the C++ function a generated program's data is: the game
	// file's name in camel case and "Game" (pong_min.xml is pongMinGame).
	std::string gameFunctionName(const std::filesystem::path& gameFile);

	// Turns a game into a program with the target's stylesheet
	// (generators/<target>/generate.xsl), run by libxslt, once the engine has
	// read the game and would play it (a game it will not load is refused).
	// The stylesheet writes the program's files with exsl:document, relative
	// to the output folder, which is made if it is not there (and removed
	// again if it was made and the game cannot be generated). Its own result
	// lists them, the target's modules to copy (from
	// generators/<target>/modules), the engine's own files to copy unchanged
	// (from lib/ in the data folder) and the asset files to copy (from the
	// data folder): <generated><file path /><module path from /><engine path
	// from /><asset path /></generated>. Throws GenerateError.
	GeneratedProgram generateGame(const GenerateRequest& request);
}
