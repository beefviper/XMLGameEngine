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
	// game (xgecli and xgegui), so they look in the same places. Both are in
	// games/: the game files in games/descriptions/ and their media in
	// games/assets/. The engine reads assets/ (the font, images) relative to
	// the working directory, so games/ is made the working directory. The build
	// puts a copy of games/ in the build directory, which is one folder above
	// the programs in a Visual Studio build (build/Debug), so a program started
	// from there, or from anywhere else, has to find it.

	// The folder the running program is in, or an empty path if the system
	// does not say.
	std::filesystem::path programDirectory();

	// The games folder: the first folder with both a descriptions/ and an
	// assets/ folder in it, looking at the working directory, the program's
	// own folder and the folder above that, each itself and then its games/.
	// An empty path if none has one. The result is a clean absolute path.
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
	//   3. its file name alone in gamesDirectory (descriptions/ in the games
	//      folder)
	//
	// Returns the path as it was found, or nothing.
	std::optional<std::filesystem::path> locateGameFile(const std::string& game,
		const std::filesystem::path& gamesDirectory);

	// Where what a game <keeps> lasts between runs (Engine::keepIn):
	// saves/<game>.xml, in the working directory (the games folder).
	std::filesystem::path keptFileFor(const std::filesystem::path& gameFile);

	// The schema a game file names (its xsi:noNamespaceSchemaLocation),
	// relative to the game's folder: there when it is there, else a file of
	// that name in the game's folder or the nearest folder above it. A game
	// names ../../xge/xgedef.xsd, right in the repository; in output/<config>
	// the schema is at the top (assets.cmake), two folders above the game.
	// The path as named when it is found nowhere.
	std::filesystem::path findSchema(const std::filesystem::path& gameFile, const std::string& schemaLocation);
}
