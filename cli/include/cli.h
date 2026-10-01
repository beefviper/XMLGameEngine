// cli.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "window.h"
#include "xml_document.h"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace xge
{
	// A command line that cannot be used: an unknown option, an option with
	// no value, a backend that does not exist, a game file that is not
	// there. what() is the message to show; main() prints it and exits.
	class CliError : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	// What the command line asked for.
	struct CliOptions
	{
		// The game as given (a name like "pong", or a file or path like
		// "pong.xml"), not yet looked for on disk - see findGameFile().
		std::string game = "pong";

		WindowBackend window = WindowBackend::SFML3;
		XmlBackend xml = XmlBackend::Xerces;

		bool showHelp = false;
	};

	// Reads the arguments (without the program name):
	//
	//   pong                   a single bare argument is the game, as it
	//                          always was ("pong", "pong.xml", "a/b/pong.xml")
	//   -g pong  -gpong        the game, same as the bare argument
	//   --game pong
	//   -w sfml3 -wsfml3       the window library: sfml3 (default), raylib, sdl2
	//   --window sfml3
	//   -x xerces -xxerces     the XML library: xerces (default), tinyxml2,
	//   --xml xerces           pugixml, rapidxml
	//   -h  --help             show the usage and exit
	//
	// A short option takes its value attached or after a space; a long option
	// needs the space ("--game=pong" is an error, not accepted). Backend names
	// are not case sensitive. Each option may be given once, and so may the
	// game, whether bare or with -g/--game. With no game at all it is "pong".
	//
	// Throws CliError for anything it cannot use. Does not look at the disk.
	CliOptions parseCommandLine(const std::vector<std::string>& args);

	// Finds the game file for a name or path from the command line. A name
	// with no extension gets ".xml" added ("pong" is "pong.xml"); anything
	// else is used as given. It is looked for, in this order:
	//
	//   1. exactly as given (a path, or a name in the current directory)
	//   2. its file name alone in the working directory (the same place as 1
	//      for a bare name; only differs when a directory was given)
	//   3. its file name alone in the games directory (default "games",
	//      beneath the working directory)
	//
	// Throws CliError, saying which file was not found, if none of them has it.
	std::string findGameFile(const std::string& game,
		const std::filesystem::path& gamesDirectory = "games");

	// The command line name of a backend, as -w and -x take it ("sfml3",
	// "tinyxml2"), for the start message.
	std::string windowBackendName(WindowBackend backend);
	std::string xmlBackendName(XmlBackend backend);

	// The usage text, for --help and after an error.
	std::string usageText();
}
