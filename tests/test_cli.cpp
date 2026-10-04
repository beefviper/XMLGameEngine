// test_cli.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026
//
// Catch2 tests for xgecli's command line (cli/source/cli.cpp): the options
// and their short, attached and long forms, the backend names, the errors, and
// the order a game file is looked for in.
//
// Test names must not start with "-" or contain a comma: ctest hands the name to
// Catch2 on its command line, which reads them as an option or a list.

#include "cli.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace xge;

namespace
{
	using Args = std::vector<std::string>;

	// Runs the body with the working directory set to a fresh empty one, and
	// puts it back afterwards.
	class TempWorkingDirectory
	{
	public:
		TempWorkingDirectory() :
			previous(std::filesystem::current_path()),
			directory(std::filesystem::temp_directory_path() / "xge_test_cli")
		{
			std::filesystem::remove_all(directory);
			std::filesystem::create_directories(directory);
			std::filesystem::current_path(directory);
		}

		~TempWorkingDirectory()
		{
			std::filesystem::current_path(previous);
			std::filesystem::remove_all(directory);
		}

		static void touch(const std::filesystem::path& file)
		{
			if (file.has_parent_path())
			{
				std::filesystem::create_directories(file.parent_path());
			}
			std::ofstream(file) << "<game />\n";
		}

	private:
		std::filesystem::path previous;
		std::filesystem::path directory;
	};
}

TEST_CASE("no arguments means pong with the default backends", "[cli]")
{
	const CliOptions options = parseCommandLine({});

	CHECK(options.game == "pong");
	CHECK(options.window == WindowBackend::SFML3);
	CHECK(options.xml == XmlBackend::Xerces);
	CHECK(options.audio == AudioBackend::SFML3);
	CHECK_FALSE(options.showHelp);
}

TEST_CASE("a single bare argument is the game", "[cli]")
{
	CHECK(parseCommandLine({ "breakout" }).game == "breakout");
	CHECK(parseCommandLine({ "pong.xml" }).game == "pong.xml");
	CHECK(parseCommandLine({ "some/dir/pong.xml" }).window == WindowBackend::SFML3);
}

TEST_CASE("the game option names the game in its short and attached and long forms", "[cli]")
{
	CHECK(parseCommandLine({ "-g", "pong" }).game == "pong");
	CHECK(parseCommandLine({ "-gbreakout" }).game == "breakout");
	CHECK(parseCommandLine({ "--game", "kaboom.xml" }).game == "kaboom.xml");
}

TEST_CASE("a long option needs a space and not an equals sign", "[cli]")
{
	CHECK_THROWS_AS(parseCommandLine({ "--game=pong" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--gamepong" }), CliError);
}

TEST_CASE("the game options fail with no game", "[cli]")
{
	CHECK_THROWS_AS(parseCommandLine({ "-g" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--game" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "-g", "-w", "sdl2" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "-g", "" }), CliError);
}

TEST_CASE("the window option picks the window library", "[cli]")
{
	CHECK(parseCommandLine({ "-w", "sfml3" }).window == WindowBackend::SFML3);
	CHECK(parseCommandLine({ "-wraylib" }).window == WindowBackend::Raylib);
	CHECK(parseCommandLine({ "--window", "sdl2" }).window == WindowBackend::SDL2);
	CHECK(parseCommandLine({ "-w", "SDL2" }).window == WindowBackend::SDL2);
	CHECK(parseCommandLine({ "-wopengl" }).window == WindowBackend::OpenGL);
	CHECK(parseCommandLine({ "--window", "OpenGL" }).window == WindowBackend::OpenGL);
}

TEST_CASE("the xml option picks the XML library", "[cli]")
{
	CHECK(parseCommandLine({ "-x", "xerces" }).xml == XmlBackend::Xerces);
	CHECK(parseCommandLine({ "-xtinyxml2" }).xml == XmlBackend::TinyXml2);
	CHECK(parseCommandLine({ "--xml", "pugixml" }).xml == XmlBackend::PugiXml);
	CHECK(parseCommandLine({ "-x", "RapidXml" }).xml == XmlBackend::RapidXml);
}

TEST_CASE("the audio option picks the sound library, or none", "[cli]")
{
	CHECK(parseCommandLine({ "-a", "sfml3" }).audio == AudioBackend::SFML3);
	CHECK(parseCommandLine({ "-araylib" }).audio == AudioBackend::Raylib);
	CHECK(parseCommandLine({ "--audio", "sdl2" }).audio == AudioBackend::SDL2);
	CHECK(parseCommandLine({ "-a", "None" }).audio == AudioBackend::None);
	CHECK_THROWS_AS(parseCommandLine({ "-a" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--audio", "openal" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "-anone", "-asdl2" }), CliError);

	const CliOptions all = parseCommandLine({ "-wraylib", "breakout", "--audio", "raylib", "-xpugixml" });
	CHECK(all.game == "breakout");
	CHECK(all.window == WindowBackend::Raylib);
	CHECK(all.audio == AudioBackend::Raylib);
	CHECK(all.xml == XmlBackend::PugiXml);
}

TEST_CASE("all three options together in any order and either form", "[cli]")
{
	const CliOptions attached = parseCommandLine({ "-gpong", "-wsdl2", "-xtinyxml2" });
	CHECK(attached.game == "pong");
	CHECK(attached.window == WindowBackend::SDL2);
	CHECK(attached.xml == XmlBackend::TinyXml2);

	const CliOptions spaced = parseCommandLine({ "-x", "pugixml", "-w", "raylib", "-g", "breakout" });
	CHECK(spaced.game == "breakout");
	CHECK(spaced.window == WindowBackend::Raylib);
	CHECK(spaced.xml == XmlBackend::PugiXml);

	const CliOptions mixed = parseCommandLine({ "--xml", "rapidxml", "-wsfml3", "kaboom" });
	CHECK(mixed.game == "kaboom");
	CHECK(mixed.window == WindowBackend::SFML3);
	CHECK(mixed.xml == XmlBackend::RapidXml);
}

TEST_CASE("a bare game can come with options", "[cli]")
{
	const CliOptions options = parseCommandLine({ "-wsdl2", "breakout" });

	CHECK(options.game == "breakout");
	CHECK(options.window == WindowBackend::SDL2);
	CHECK(options.xml == XmlBackend::Xerces);
}

TEST_CASE("a missing or unknown backend is an error", "[cli]")
{
	CHECK_THROWS_AS(parseCommandLine({ "-w" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--xml" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "-w", "-x", "xerces" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "-wvulkan" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--xml", "expat" }), CliError);
}

TEST_CASE("unknown options and repeats are errors", "[cli]")
{
	CHECK_THROWS_AS(parseCommandLine({ "-q" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--verbose" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "-hx" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "-wsdl2", "-wraylib" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "-xtinyxml2", "--xml", "pugixml" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "pong", "breakout" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "pong", "-g", "breakout" }), CliError);
}

TEST_CASE("backend names read back the way the options take them", "[cli]")
{
	CHECK(windowBackendName(WindowBackend::SFML3) == "sfml3");
	CHECK(windowBackendName(WindowBackend::Raylib) == "raylib");
	CHECK(windowBackendName(WindowBackend::SDL2) == "sdl2");
	CHECK(windowBackendName(WindowBackend::OpenGL) == "opengl");
	CHECK(xmlBackendName(XmlBackend::Xerces) == "xerces");
	CHECK(xmlBackendName(XmlBackend::TinyXml2) == "tinyxml2");
	CHECK(xmlBackendName(XmlBackend::PugiXml) == "pugixml");
	CHECK(xmlBackendName(XmlBackend::RapidXml) == "rapidxml");
	CHECK(audioBackendName(AudioBackend::SFML3) == "sfml3");
	CHECK(audioBackendName(AudioBackend::Raylib) == "raylib");
	CHECK(audioBackendName(AudioBackend::SDL2) == "sdl2");
	CHECK(audioBackendName(AudioBackend::None) == "none");
}

TEST_CASE("the help options ask for the usage", "[cli]")
{
	CHECK(parseCommandLine({ "-h" }).showHelp);
	CHECK(parseCommandLine({ "--help" }).showHelp);
	CHECK(usageText().find("--game") != std::string::npos);
}

TEST_CASE("a bare name gets .xml and is found in the working directory", "[cli]")
{
	TempWorkingDirectory work;
	TempWorkingDirectory::touch("pong.xml");

	CHECK(findGameFile("pong") == "pong.xml");
	CHECK(findGameFile("pong.xml") == "pong.xml");
}

TEST_CASE("a game is found in the games directory last", "[cli]")
{
	TempWorkingDirectory work;
	TempWorkingDirectory::touch("games/breakout.xml");

	CHECK(std::filesystem::path(findGameFile("breakout")) == std::filesystem::path("games") / "breakout.xml");
	CHECK(std::filesystem::path(findGameFile("breakout.xml")) == std::filesystem::path("games") / "breakout.xml");
}

TEST_CASE("the working directory wins over the games directory", "[cli]")
{
	TempWorkingDirectory work;
	TempWorkingDirectory::touch("pong.xml");
	TempWorkingDirectory::touch("games/pong.xml");

	CHECK(findGameFile("pong") == "pong.xml");
}

TEST_CASE("a path as given wins over the working and games directories", "[cli]")
{
	TempWorkingDirectory work;
	TempWorkingDirectory::touch("mine/pong.xml");
	TempWorkingDirectory::touch("pong.xml");
	TempWorkingDirectory::touch("games/pong.xml");

	CHECK(std::filesystem::path(findGameFile("mine/pong.xml")) == std::filesystem::path("mine") / "pong.xml");
	CHECK(std::filesystem::path(findGameFile("mine/pong")) == std::filesystem::path("mine") / "pong.xml");
}

TEST_CASE("a path that is not there falls back to the file name", "[cli]")
{
	TempWorkingDirectory work;
	TempWorkingDirectory::touch("pong.xml");
	TempWorkingDirectory::touch("games/breakout.xml");

	CHECK(findGameFile("nowhere/pong.xml") == "pong.xml");
	CHECK(std::filesystem::path(findGameFile("nowhere/breakout.xml")) == std::filesystem::path("games") / "breakout.xml");
}

TEST_CASE("a game that is nowhere is an error naming the file", "[cli]")
{
	TempWorkingDirectory work;

	CHECK_THROWS_AS(findGameFile("missing"), CliError);

	try
	{
		findGameFile("missing");
	}
	catch (const CliError& error)
	{
		CHECK(std::string(error.what()).find("missing.xml") != std::string::npos);
	}
}
