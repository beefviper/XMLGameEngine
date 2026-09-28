// cli.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "cli.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <span>

namespace xge
{
	std::string resolveGameFilename(int argc, char* argv[])
	{
		const std::span<char*> args(argv, argc);

		const std::string gameName = (args.size() > 1) ? std::string(args[1]) : std::string("pong");

		// A bare name like "pong" gets ".xml" appended; a name that already
		// has an extension (e.g. "pong.xml") is used exactly as given.
		const std::string filename = (gameName.find('.') == std::string::npos)
			? gameName + ".xml"
			: gameName;

		if (std::filesystem::exists(filename))
		{
			return filename;
		}

		const std::string gamesPathFilename = "games/" + filename;
		if (std::filesystem::exists(gamesPathFilename))
		{
			return gamesPathFilename;
		}

		std::cout << "File not found: " << filename << '\n';
		exit(EXIT_FAILURE);
	}
}
