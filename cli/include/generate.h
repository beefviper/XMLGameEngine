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
	// file that is not XML, or a tag the target cannot generate yet. what() says
	// which, with the stylesheet's own message when it stopped.
	class GenerateError : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	struct GenerateRequest
	{
		std::filesystem::path gameFile;    // the game (games/pong.xml)
		std::string target;                // the generator ("windows-cpp")
		std::filesystem::path generators;  // the folder holding a folder per target (generators/)
		std::filesystem::path dataFolder;  // where the game's assets/ are looked for
		std::filesystem::path output;      // the folder the program is written to
	};

	// What was written to the output folder: the files the stylesheet wrote and
	// the assets copied beside them, as paths relative to the output folder.
	struct GeneratedProgram
	{
		std::vector<std::filesystem::path> files;
		std::vector<std::filesystem::path> assets;
	};

	// Whether this program was built with libxslt, which --generate needs.
	bool canGenerate() noexcept;

	// Turns a game into a program with the target's stylesheet
	// (generators/<target>/generate.xsl), run by libxslt. The stylesheet writes
	// the program's files with exsl:document, relative to the output folder,
	// which is made if it is not there, and its own result is a list of them
	// and of the asset files to copy (<generated><file path /><asset path />),
	// which are copied here from the data folder. Throws GenerateError.
	GeneratedProgram generateGame(const GenerateRequest& request);
}
