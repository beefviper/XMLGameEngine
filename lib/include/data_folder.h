// data_folder.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace xge
{
	// Where the games and the assets are, shared by every program that runs a
	// game (XGECLI and XGEGUI), so they look in the same places. The engine reads
	// assets/ (the font, images) relative to the working directory, and the
	// games are in games/; the build puts a copy of both in the build directory,
	// which is one folder above the programs in a Visual Studio build
	// (build/Debug), so a program started from there, or from anywhere else,
	// has to find them.

	// The folder the running program is in, or an empty path if the system
	// does not say.
	std::filesystem::path programDirectory();

	// The first of these that has both a games/ and an assets/ folder in it:
	// the working directory, the program's own folder, and the folder above
	// that. An empty path if none does. The result is a clean absolute path.
	std::filesystem::path findDataFolder(const std::filesystem::path& workingDirectory,
		const std::filesystem::path& programDirectory);

	// Looks for the folder as above (from the current working directory and
	// programDirectory()) and, if found, makes it the working directory, so the
	// program can be started from anywhere. Returns the folder, or an empty
	// path, and changes nothing, when there is none. A game named by a relative
	// path has to be found (see locateGameFile) before this is called.
	std::filesystem::path enterDataFolder();

	// A game name or path from the user with the extension it needs: a name
	// with none gets ".xml" ("pong" is "pong.xml"); anything else is as given.
	std::filesystem::path gameFileGiven(const std::string& game);

	// Finds a game file for a name or path, in this order:
	//
	//   1. exactly as given (a path, or a name in the current directory)
	//   2. its file name alone in the working directory (the same place as 1
	//      for a bare name; only differs when a directory was given)
	//   3. its file name alone in gamesDirectory
	//
	// Returns the path as it was found, or nothing.
	std::optional<std::filesystem::path> locateGameFile(const std::string& game,
		const std::filesystem::path& gamesDirectory);
}
