// cli.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "audio/audio.h"
#include "window/window.h"
#include "xml/document.h"

#include <filesystem>
#include <optional>
#include <ostream>
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

		// What this program was built with, the first of them (SFML 3, Xerces and
		// SFML 3's sound unless they were left out; see options.cmake).
		WindowBackend window = WindowFactory::defaultBackend();
		XmlBackend xml = XmlDocumentFactory::defaultBackend();
		AudioBackend audio = AudioFactory::defaultBackend();

		// --generate: the target to write the game out for instead of playing
		// it ("engine-cpp"), or empty to play it; and -o/--output, the folder
		// to write it to (empty: <game>-<target> in the working directory).
		std::string generate;
		std::string output;

		// --record: play the game and write what was played into the game
		// file as its <demo> when the window closes.
		bool record = false;

		// --check: load each game (`games`; none given, every one in
		// games/descriptions/) as for play, print ok or what is wrong, and
		// play nothing (checkGames).
		bool check = false;
		std::vector<std::string> games;

		// --frames N: play that many frames as fast as they go, with no window
		// and no sound, then stop (playFrames); 0 plays in a window as usual.
		// --seed S: the random numbers start from S, from the moment the game
		// loads, so the same command plays the same game every time.
		int frames = 0;
		std::optional<unsigned int> seed;

		// --keys file: play back the <demo> in that file (readDemo) from its
		// own seed, its keys pressed in the frames they were recorded in.
		std::string keys;

		// --dump file: with --frames, the game as it is (Game::describe)
		// written into that file at the end; --dump-every K, every K frames
		// as well. games/xgedump.xsd is the file's schema.
		std::string dump;
		int dumpEvery = 0;

		bool showHelp = false;
	};

	// Reads the arguments (without the program name):
	//
	//   pong                   a single bare argument is the game, as it
	//                          always was ("pong", "pong.xml", "a/b/pong.xml")
	//   -g pong  -gpong        the game, same as the bare argument
	//   --game pong
	//   -w sfml3 -wsfml3       the window library: sfml3 (default), raylib, sdl2, opengl,
	//                          direct3d9, direct3d11 (Windows)
	//   --window sfml3
	//   -x xerces -xxerces     the XML library: xerces (default), tinyxml2,
	//   --xml xerces           pugixml, rapidxml
	//   -a sfml3 -asfml3       the sound library: sfml3 (default), raylib, sdl2,
	//   --audio sfml3          none (silent)
	//   Only the libraries this program was built with can be named; another is an
	//   error that says which there are (a default build has SFML 3 and Xerces).
	//   --generate engine-cpp write the game out as a program for that target
	//                          instead of playing it (no short form: -g is the game)
	//   -o out -oout           the folder --generate writes to
	//   --output out
	//   --record               play and write what was played into the game
	//                          file as its <demo>, for its attract mode
	//   --frames 600           play that many frames with no window and no
	//                          sound, then stop
	//   --seed 1               the random numbers start from this seed
	//   --keys play.xml        play back the <demo> in that file
	//   --dump state.xml       with --frames, write the game's state there at
	//                          the end
	//   --dump-every 60        and every 60 frames on the way
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
	//   3. its file name alone in the games directory (default
	//      "games/descriptions", beneath the working directory; xgecli passes
	//      the descriptions/ of the games folder findDataFolder() found - see
	//      engine/folder.h)
	//
	// Throws CliError, saying which file was not found, if none of them has it.
	std::string findGameFile(const std::string& game,
		const std::filesystem::path& gamesDirectory = "games/descriptions");

	// The command line name of a backend, as -w, -x and -a take it ("sfml3",
	// "tinyxml2"), for the start message.
	std::string windowBackendName(WindowBackend backend);
	std::string xmlBackendName(XmlBackend backend);
	std::string audioBackendName(AudioBackend backend);

	// The targets --generate can write a game out for ("engine-cpp",
	// "windows-cpp").
	std::vector<std::string> generateTargets();

	// The usage text, for --help and after an error.
	std::string usageText();

	// Every .xml file in the folder, sorted: what --check with no game checks.
	std::vector<std::string> gameFilesIn(const std::filesystem::path& gamesDirectory);

	// Loads each game file as for play (the reader, then the engine with a
	// null window and no sound, so sizes and the positions made from them are
	// worked out too), and writes a line each to `out`: "pong.xml: ok", or
	// "pong.xml: " and what stopped it. What the libraries say while reading is
	// not shown; warnings still go to std::cerr. Gives back how many failed.
	int checkGames(const std::vector<std::string>& files, XmlBackend xml, std::ostream& out);

	// --frames: loads the game file (with options.xml, from options.seed if
	// there is one, playing back options.keys if given), plays options.frames frames with a null window and no
	// sound, writes options.dump if asked, and writes one line to `out` saying how far it got. A game that
	// will not load, or throws while it plays, is "Error: " and what it said.
	// Gives back the program's exit code: EXIT_SUCCESS or EXIT_FAILURE.
	int playFrames(const std::string& file, const CliOptions& options, std::ostream& out);
}
