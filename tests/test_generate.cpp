// test_generate.cpp
// XML Game Engine
// author: beefviper
// date: Oct 5, 2026
//
// Catch2 tests for xgecli --generate (cli/source/generate.cpp): the program
// it writes is the game as the engine's own data, a main() that plays it with
// the engine's Game and Engine on SFML, and the engine's files copied in
// unchanged, nothing of XML or of the other libraries among them. That the
// data is the same game as the file, frame by frame, is in
// test_generated_games.cpp, which compiles it.

#include "game.h"
#include "game_xml.h"
#include "generate.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>

using namespace xge;

namespace
{
	namespace fs = std::filesystem;

	// A fresh empty folder for one test's output, removed afterwards.
	class TempFolder
	{
	public:
		explicit TempFolder(const std::string& name) :
			path(fs::temp_directory_path() / name)
		{
			fs::remove_all(path);
		}

		~TempFolder()
		{
			std::error_code ignored;
			fs::remove_all(path, ignored);
		}

		TempFolder(const TempFolder&) = delete;
		TempFolder& operator=(const TempFolder&) = delete;

		fs::path path;
	};

	std::string readFile(const fs::path& file)
	{
		std::ifstream in(file, std::ios::binary);
		std::stringstream text;
		text << in.rdbuf();
		return text.str();
	}

	// The games, their assets and the engine's lib/ are next to the test
	// program (assets.cmake).
	GenerateRequest requestFor(const fs::path& game, const fs::path& output, const std::string& target = "windows-cpp")
	{
		GenerateRequest request;
		request.gameFile = game;
		request.target = target;
		request.dataFolder = fs::current_path();
		request.output = output;
		return request;
	}

	std::size_t count(const std::string& text, const std::string& part)
	{
		std::size_t found = 0;
		for (std::size_t at = text.find(part); at != std::string::npos; at = text.find(part, at + part.size()))
		{
			++found;
		}
		return found;
	}
}

TEST_CASE("generating pong_min writes the game as data, a main that runs the engine on it, and the engine's own files", "[generate]")
{
	TempFolder folder("xge_test_generate_pong_min");
	const GeneratedProgram program = generateGame(requestFor(fs::current_path() / "games/pong_min.xml", folder.path / "out"));
	const fs::path out = folder.path / "out";

	for (const char* file : { "source/pong_min.cpp", "source/main.cpp", "CMakeLists.txt", "README.md", ".gitignore" })
	{
		INFO(file);
		CHECK(fs::exists(out / file));
	}
	CHECK(program.assets.empty());

	// main plays the game with the engine's Game and Engine, on SFML's window and sound
	const std::string main = readFile(out / "source/main.cpp");
	CHECK(main.rfind("// main.cpp\n// XML Game Engine\n// author: beefviper\n// date: ", 0) == 0);
	CHECK(main.find("xge::GameData pongMinGame();") != std::string::npos);
	CHECK(main.find("xge::Game game{ pongMinGame() };") != std::string::npos);
	CHECK(main.find("std::make_unique<xge::SFMLWindow>(game.getWindowDesc())") != std::string::npos);
	CHECK(main.find("audio = std::make_unique<xge::SFMLAudio>();") != std::string::npos);
	CHECK(main.find("engine.loop();") != std::string::npos);

	// the game is what the engine reads from the file, field by field
	const std::string data = readFile(out / "source/pong_min.cpp");
	CHECK(data.rfind("// pong_min.cpp\n// XML Game Engine\n", 0) == 0);
	CHECK(data.find("#include \"game_data.h\"") != std::string::npos);
	CHECK(data.find("xge::GameData pongMinGame()\n{") != std::string::npos);
	CHECK(data.find("\t// <object name=\"ball\">\n\tRawObject ballObject()\n\t{\n\t\tRawObject object;\n\t\tobject.name = \"ball\";") != std::string::npos);
	CHECK(data.find("\t\tobject.sprite.kind = \"circle\";") != std::string::npos);
	CHECK(data.find("object.rawCollisionData.enabled = true;") != std::string::npos);
	CHECK(data.find("verb(\"bounce\")") != std::string::npos);
	CHECK(data.find("\tgame.objects = objects();") != std::string::npos);
	// only the helpers it uses: pong_min has no <random>, <equation> or <sound>
	CHECK(data.find("RawValue value(std::string text)") != std::string::npos);
	CHECK(data.find("RawValue arithmetic(") == std::string::npos);
	CHECK(data.find("std::vector<RawSound> sounds()") == std::string::npos);

	// the build compiles the engine's files beside them, with the engine's warnings
	const std::string cmake = readFile(out / "CMakeLists.txt");
	CHECK(cmake.find("\tsource/pong_min.cpp\n") != std::string::npos);
	CHECK(cmake.find("\tengine/game.cpp\n") != std::string::npos);
	CHECK(cmake.find("\tengine/engine.cpp\n") != std::string::npos);
	CHECK(cmake.find("-Wall -Wextra -Wpedantic -Wshadow -Wconversion") != std::string::npos);
}

TEST_CASE("a generated program carries the engine's files unchanged, and none of its XML or library choosing", "[generate]")
{
	TempFolder folder("xge_test_generate_engine_files");
	generateGame(requestFor(fs::current_path() / "games/pong.xml", folder.path / "out"));
	const fs::path engine = folder.path / "out/engine";

	std::set<std::string> copied;
	for (const fs::path& file : engineFiles())
	{
		INFO(file.generic_string());
		CHECK(readFile(engine / file.filename()) == readFile(fs::current_path() / file));
		copied.insert(file.filename().string());
	}
	for (const char* left : { "game_xml.h", "game_xml.cpp", "xml_document.h", "xsd_lite.h", "window.cpp", "audio.cpp", "engine_backends.cpp",
		"window_raylib.h", "window_sdl2.h", "window_opengl.h", "audio_raylib.h", "audio_sdl2.h", "data_folder.h" })
	{
		INFO(left);
		CHECK(copied.count(left) == 0);
		CHECK_FALSE(fs::exists(engine / left));
	}

	// Every file a copied file includes is copied too, so the program builds
	// from them alone.
	for (const std::string& name : copied)
	{
		std::istringstream lines(readFile(engine / name));
		for (std::string line; std::getline(lines, line);)
		{
			if (line.rfind("#include \"", 0) != 0)
			{
				continue;
			}
			const std::string included = line.substr(10, line.find('"', 10) - 10);
			INFO(name << " includes " << included);
			CHECK(copied.count(included) == 1);
		}
	}
}

TEST_CASE("a generated program copies the font and pictures its game uses", "[generate]")
{
	TempFolder folder("xge_test_generate_assets");
	const GeneratedProgram program = generateGame(requestFor(fs::current_path() / "games/pong.xml", folder.path / "out"));
	const std::set<fs::path> assets(program.assets.begin(), program.assets.end());
	CHECK(assets.count("assets/tuffy.ttf") == 1);
	CHECK(assets.count("assets/paddle.jpg") == 1);
	CHECK(fs::exists(folder.path / "out/assets/paddle.jpg"));
}

TEST_CASE("a group is written as its members, each as what it changes of the one before", "[generate]")
{
	TempFolder folder("xge_test_generate_group");
	generateGame(requestFor(fs::current_path() / "games/spaceinvaders.xml", folder.path / "out"));
	const std::string data = readFile(folder.path / "out/source/spaceinvaders.cpp");

	const GameData game = readGame((fs::current_path() / "games/spaceinvaders.xml").string());
	std::size_t members = 0;
	for (const RawObject& object : game.objects)
	{
		if (!object.groupName.empty())
		{
			++members;
		}
	}
	REQUIRE(members > 55);
	CHECK(data.find("std::vector<RawObject> aliensGroup()") != std::string::npos);
	CHECK(count(data, "\t\tmember.name = ") == members);
	CHECK(count(data, "\t\tmembers.push_back(member);\n") >= members);
}

TEST_CASE("a game the engine will not load is not generated, and the folder made for it goes", "[generate]")
{
	TempFolder folder("xge_test_generate_broken");
	fs::create_directories(folder.path);
	std::ofstream(folder.path / "broken.xml") << "<game><window name=\"Broken\">";

	CHECK_THROWS_AS(generateGame(requestFor(folder.path / "broken.xml", folder.path / "out")), GenerateError);
	CHECK_FALSE(fs::exists(folder.path / "out"));

	CHECK_THROWS_AS(generateGame(requestFor(fs::current_path() / "games/pong.xml", folder.path / "other", "amiga-asm")), GenerateError);
	CHECK_FALSE(fs::exists(folder.path / "other"));
}

TEST_CASE("the function a game is written as is named for its file", "[generate]")
{
	CHECK(gameFunctionName("games/pong_min.xml") == "pongMinGame");
	CHECK(gameFunctionName("games/spaceinvaders2.xml") == "spaceinvaders2Game");
	CHECK(gameFunctionName("3d-tennis.xml") == "n3dTennisGame");
}
