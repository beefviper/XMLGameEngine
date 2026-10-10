// main.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "cli.h"
#include "data_folder.h"
#include "generate.h"
#include "game.h"
#include "engine.h"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
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

		// games/ and assets/ are looked for in the working directory, next to
		// the program, and one folder above it (see data_folder.h), the same as
		// xgegui does. A game given as a relative path is relative to where the
		// program was started, so it is found before the working directory is
		// changed to the folder they are in.
		dataFolder = xge::findDataFolder(std::filesystem::current_path(), xge::programDirectory());

		const std::filesystem::path gamesDirectory = dataFolder.empty() ? std::filesystem::path("games") : dataFolder / "games";
		filename = std::filesystem::absolute(xge::findGameFile(options.game, gamesDirectory)).string();
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
		const std::filesystem::path base = dataFolder.empty() ? std::filesystem::current_path() : dataFolder;

		xge::GenerateRequest request;
		request.gameFile = filename;
		request.target = options.generate;
		request.generators = base / "generators";
		request.dataFolder = base;
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
			for (const auto& picture : program.drawn)
			{
				std::cout << "  " << picture.generic_string() << " (drawn from its svg)\n";
			}
		}
		catch (const std::exception& error)
		{
			std::cerr << "Error: " << error.what() << '\n';
			return EXIT_FAILURE;
		}

		return EXIT_SUCCESS;
	}

	// The engine reads assets/ relative to the working directory.
	if (!dataFolder.empty())
	{
		std::error_code ignored;
		std::filesystem::current_path(dataFolder, ignored);
	}

	std::cout << "file: " << filename << '\n'
		<< "window: " << xge::windowBackendName(options.window) << '\n'
		<< "xml: " << xge::xmlBackendName(options.xml) << '\n'
		<< "audio: " << xge::audioBackendName(options.audio) << "\n\n";

	// A game file that is wrong - an unknown tag, a missing <radius>, a value
	// that will not evaluate - is reported by the exception loading it throws,
	// with where in the file it was found.
	try
	{
		// Game is fully evaluated the moment its constructor returns - every
		// object's position (a group's cells included), velocity, action, and
		// variable is real - none of it needs a Window/backend to exist first
		// (see game_expr.cpp, command.cpp's measureShapeSize). The exceptions are
		// Object::size of a text or image (the real rendered footprint, used for
		// drawing/collision), which stays {0,0} until Engine's constructor builds
		// a real Window backend to measure it (see Window::init(), window.h), and
		// any <position> that uses the width or height of such an object (for
		// example title.width), which can only be worked out after that. printGame() knows the
		// difference and prints both as "unknown" instead of a misleading {0,0}
		// if called before Engine exists, so it is called twice: once here to
		// show what is known without a window, and once after Engine.
		xge::Game game{ filename, options.xml };

		std::cout << "=== before Engine: sizes and size-dependent positions not yet known ===\n\n";
		game.printGame();

		xge::Engine engine(game, options.window, options.audio);

		std::cout << "=== after Engine: sizes measured, positions finished ===\n\n";
		game.printGame();

		engine.loop();
	}
	catch (const std::exception& error)
	{
		std::cout << "Error: " << error.what() << '\n';
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
