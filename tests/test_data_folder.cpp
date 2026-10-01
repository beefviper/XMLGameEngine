// test_data_folder.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026
//
// Catch2 tests for lib/source/data_folder.cpp: where XGECLI and XGEGUI look for
// games/ and assets/ (the working directory, the program's folder, the folder
// above it) and for a game file.
//
// Test names must not start with "-" or contain a comma: ctest hands the name to
// Catch2 on its command line, which reads them as an option or a list.

#include "data_folder.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <string>

using namespace xge;

namespace
{
	namespace fs = std::filesystem;

	// A fresh empty folder under the system temp folder, removed afterwards.
	// Nothing here changes the working directory, so the tests pass the
	// folders to findDataFolder() directly.
	class TempRoot
	{
	public:
		TempRoot() :
			root(fs::weakly_canonical(fs::temp_directory_path()) / "xge_test_data_folder")
		{
			fs::remove_all(root);
			fs::create_directories(root);
		}

		~TempRoot()
		{
			fs::remove_all(root);
		}

		const fs::path& path() const { return root; }

		// Makes games/ and assets/ in the folder.
		void makeData(const fs::path& folder) const
		{
			fs::create_directories(root / folder / "games");
			fs::create_directories(root / folder / "assets");
		}

		void touch(const fs::path& file) const
		{
			fs::create_directories((root / file).parent_path());
			std::ofstream(root / file) << "<game />\n";
		}

	private:
		fs::path root;
	};
}

TEST_CASE("the data folder is the working directory when it has games and assets", "[data_folder]")
{
	TempRoot temp;
	temp.makeData("work");
	fs::create_directories(temp.path() / "program");

	CHECK(findDataFolder(temp.path() / "work", temp.path() / "program") == temp.path() / "work");
}

TEST_CASE("the data folder is the program folder when the working directory has none", "[data_folder]")
{
	TempRoot temp;
	temp.makeData("program");
	fs::create_directories(temp.path() / "elsewhere");

	CHECK(findDataFolder(temp.path() / "elsewhere", temp.path() / "program") == temp.path() / "program");
}

TEST_CASE("the data folder is one above the program folder, as in a Debug build", "[data_folder]")
{
	TempRoot temp;
	temp.makeData("build");
	fs::create_directories(temp.path() / "build" / "Debug");
	fs::create_directories(temp.path() / "elsewhere");

	// The result is clean, with no trailing "Debug/.."
	CHECK(findDataFolder(temp.path() / "elsewhere", temp.path() / "build" / "Debug") == temp.path() / "build");
}

TEST_CASE("the working directory is preferred over the program folder", "[data_folder]")
{
	TempRoot temp;
	temp.makeData("work");
	temp.makeData("program");

	CHECK(findDataFolder(temp.path() / "work", temp.path() / "program") == temp.path() / "work");
}

TEST_CASE("the program folder is preferred over the one above it", "[data_folder]")
{
	TempRoot temp;
	temp.makeData("build");
	temp.makeData("build/Debug");
	fs::create_directories(temp.path() / "elsewhere");

	CHECK(findDataFolder(temp.path() / "elsewhere", temp.path() / "build" / "Debug") == temp.path() / "build" / "Debug");
}

TEST_CASE("a folder needs both games and assets to be the data folder", "[data_folder]")
{
	TempRoot temp;
	fs::create_directories(temp.path() / "work" / "games");
	fs::create_directories(temp.path() / "program" / "assets");

	CHECK(findDataFolder(temp.path() / "work", temp.path() / "program").empty());
}

TEST_CASE("no data folder gives an empty path", "[data_folder]")
{
	TempRoot temp;
	fs::create_directories(temp.path() / "work");

	CHECK(findDataFolder(temp.path() / "work", temp.path() / "program").empty());
	CHECK(findDataFolder(temp.path() / "work", {}).empty());
}

TEST_CASE("the program folder is a folder that exists", "[data_folder]")
{
	const fs::path program = programDirectory();

	REQUIRE_FALSE(program.empty());
	CHECK(fs::is_directory(program));
}

TEST_CASE("a game name with no extension gets xml added", "[data_folder]")
{
	CHECK(gameFileGiven("pong") == fs::path("pong.xml"));
	CHECK(gameFileGiven("pong.xml") == fs::path("pong.xml"));
	CHECK(gameFileGiven("games/pong") == fs::path("games") / "pong.xml");
	CHECK(gameFileGiven("mine.txt") == fs::path("mine.txt"));
}

TEST_CASE("a game is found in the games directory", "[data_folder]")
{
	TempRoot temp;
	temp.touch("data/games/pong.xml");

	const auto found = locateGameFile("pong", temp.path() / "data" / "games");

	REQUIRE(found.has_value());
	CHECK(*found == temp.path() / "data" / "games" / "pong.xml");
	CHECK(locateGameFile("pong.xml", temp.path() / "data" / "games").has_value());
}

TEST_CASE("a game given by path is found as given", "[data_folder]")
{
	TempRoot temp;
	temp.touch("mine/custom.xml");
	temp.touch("data/games/custom.xml");

	// As given wins over the same name in the games directory.
	const auto found = locateGameFile((temp.path() / "mine" / "custom.xml").string(), temp.path() / "data" / "games");

	REQUIRE(found.has_value());
	CHECK(*found == temp.path() / "mine" / "custom.xml");
}

TEST_CASE("a game given by a wrong folder is found by its file name in the games directory", "[data_folder]")
{
	TempRoot temp;
	temp.touch("data/games/breakout.xml");

	const auto found = locateGameFile("nowhere/breakout.xml", temp.path() / "data" / "games");

	REQUIRE(found.has_value());
	CHECK(*found == temp.path() / "data" / "games" / "breakout.xml");
}

TEST_CASE("a game that is nowhere is not found", "[data_folder]")
{
	TempRoot temp;
	fs::create_directories(temp.path() / "data" / "games");

	CHECK_FALSE(locateGameFile("missing", temp.path() / "data" / "games").has_value());
	CHECK_FALSE(locateGameFile("missing", temp.path() / "no_such_folder").has_value());
}
