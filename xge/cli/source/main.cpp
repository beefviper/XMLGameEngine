// main.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "cli.h"
#include "engine/folder.h"
#include "generate.h"
#include "game/game.h"
#include "game/random.h"
#include "load/xml.h"
#include "engine/engine.h"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

int main(int argc, char* argv[])
{
	std::string filename;
	std::filesystem::path dataFolder;
	xge::CliOptions options;

	try
	{
		const std::vector<std::string> args(argv + 1, argv + argc);
		options = xge::parseCommandLine(args);

		if (options.showHelp)
		{
			std::cout << xge::usageText();
			return EXIT_SUCCESS;
		}

		// The games folder (descriptions/ and assets/) is looked for in the
		// working directory, next to the program, and one folder above it (see
		// engine/folder.h), the same as xgegui does. A game given as a relative
		// path is relative to where the program was started, so it is found
		// before the working directory is changed to the games folder.
		dataFolder = xge::findDataFolder(std::filesystem::current_path(), xge::programDirectory());

		const std::filesystem::path gamesDirectory = dataFolder.empty() ? std::filesystem::path("games/descriptions") : dataFolder / "descriptions";

		// --check: each game loaded, a line each, nothing played.
		if (options.check)
		{
			std::vector<std::string> files;
			int missing = 0;
			if (options.games.empty()) { files = xge::gameFilesIn(gamesDirectory); }
			for (const std::string& game : options.games)
			{
				try
				{
					files.push_back(std::filesystem::absolute(xge::findGameFile(game, gamesDirectory)).string());
				}
				catch (const xge::CliError& error)
				{
					std::cout << game << ": " << error.what() << '\n';
					++missing;
				}
			}
			if (files.empty() && missing == 0) { throw xge::CliError("no games to check in " + gamesDirectory.string()); }
			if (!dataFolder.empty())
			{
				std::error_code ignored;
				std::filesystem::current_path(dataFolder, ignored);
			}
			const int failed = xge::checkGames(files, options.xml, std::cout) + missing;
			return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
		}

		filename = std::filesystem::absolute(xge::findGameFile(options.game, gamesDirectory)).string();

		// Relative to where the program was started, as the game is.
		if (!options.keys.empty())
		{
			options.keys = std::filesystem::absolute(options.keys).string();
		}
		if (!options.dump.empty())
		{
			options.dump = std::filesystem::absolute(options.dump).string();
		}
		if (!options.picture.empty())
		{
			options.picture = std::filesystem::absolute(options.picture).string();
		}
	}
	catch (const xge::CliError& error)
	{
		std::cerr << "Error: " << error.what() << "\n\n" << xge::usageText();
		return EXIT_FAILURE;
	}

	// --generate writes the game out as a program instead of playing it: into
	// the folder given, or <game>-<target> where the program was started.
	if (!options.generate.empty())
	{
		// The generators are in the games folder; the engine's own files are in
		// xge/ in the repository the program was built from.
		const std::filesystem::path base = dataFolder.empty() ? std::filesystem::current_path() / "games" : dataFolder;

		xge::GenerateRequest request;
		request.gameFile = filename;
		request.target = options.generate;
		request.generators = base / "generators";
		request.dataFolder = base;
		request.engineFolder = XGE_ENGINE_FOLDER;
		request.output = std::filesystem::absolute(options.output.empty()
			? std::filesystem::path(std::filesystem::path(filename).stem().string() + "-" + options.generate)
			: std::filesystem::path(options.output));

		try
		{
			const xge::GeneratedProgram program = xge::generateGame(request);

			std::cout << "generated " << filename << " for " << options.generate << " in " << request.output.string() << ":\n";
			for (const auto& file : program.files)
			{
				std::cout << "  " << file.generic_string() << '\n';
			}
			for (const auto& asset : program.assets)
			{
				std::cout << "  " << asset.generic_string() << " (copied)\n";
			}
		}
		catch (const std::exception& error)
		{
			std::cerr << "Error: " << error.what() << '\n';
			return EXIT_FAILURE;
		}

		return EXIT_SUCCESS;
	}

	// The engine reads assets/ relative to the working directory, so that is
	// the games folder.
	if (!dataFolder.empty())
	{
		std::error_code ignored;
		std::filesystem::current_path(dataFolder, ignored);
	}

	std::cout << "file: " << filename << '\n'
		<< "window: " << xge::windowBackendName(options.window) << '\n'
		<< "xml: " << xge::xmlBackendName(options.xml) << '\n'
		<< "audio: " << xge::audioBackendName(options.audio) << "\n\n";

	// --frames: played to the end with no window, nothing printed but how
	// far it got.
	if (options.frames > 0)
	{
		return xge::playFrames(filename, options, std::cout);
	}
	if (options.seed)
	{
		xge::fixRandomSeed(options.seed);
	}

	// A game file that is wrong - an unknown tag, a missing <radius>, a value
	// that will not evaluate - is reported by the exception loading it throws,
	// with where in the file it was found.
	try
	{
		// Game is fully evaluated the moment its constructor returns: every
		// object's position, size, velocity, action and variable is real, with
		// no window (the engine draws text and reads image files itself).
		xge::Game game{ xge::readGame(filename, options.xml) };
		game.printGame();

		xge::Engine engine(game, options.window, options.audio);
		engine.keepIn(xge::keptFileFor(filename).string());
		if (options.record)
		{
			engine.record();
		}
		if (!options.keys.empty())
		{
			engine.replay(xge::readDemo(options.keys, options.xml));
		}

		engine.loop();

		if (options.record)
		{
			std::string text;
			{
				std::ifstream in(filename, std::ios::binary);
				std::ostringstream read;
				read << in.rdbuf();
				text = read.str();
			}
			const xge::RawDemo demo = engine.recorded();
			std::ofstream out(filename, std::ios::binary | std::ios::trunc);
			out << xge::withDemo(text, demo);
			if (!out)
			{
				throw std::runtime_error("could not write the demo into " + filename);
			}
			std::cout << "recorded " << demo.frames << " frames and " << demo.keys.size() << " key changes into " << std::filesystem::absolute(filename).string() << " as its <demo>\n";
		}
	}
	catch (const std::exception& error)
	{
		std::cout << "Error: " << error.what() << '\n';
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
