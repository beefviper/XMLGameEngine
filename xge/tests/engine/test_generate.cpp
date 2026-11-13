// test_generate.cpp
// XML Game Engine
// author: beefviper
// date: Oct 5, 2026
//
// Catch2 tests for xgecli --generate (xge/cli/source/generate.cpp, running the
// stylesheets in games/generators/engine-cpp): the program it writes is the game
// as the engine's own data, a main() that plays it with the engine's Game and
// Engine on SFML, and the engine's files copied in unchanged, nothing of XML
// or of the other libraries among them. That the data is the same game as
// the file, field by field and frame by frame, is in
// test_generated_games.cpp, which compiles it. The windows-cpp target
// (games/generators/windows-cpp) writes pong_min as games/prototypes has it.

#include "scratch.h"
#include "game/game.h"
#include "load/xml.h"
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
			path(fs::temp_directory_path() / scratchName(name))
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

	// The tests run in the games folder beside the test program, with the
	// games, their assets and the generators in it; the engine's lib/ is in
	// xge/ in the repository (XGE_ENGINE_FOLDER, targets.cmake).
	GenerateRequest requestFor(const fs::path& game, const fs::path& output, const std::string& target = "engine-cpp")
	{
		GenerateRequest request;
		request.gameFile = game;
		request.target = target;
		request.generators = fs::current_path() / "generators";
		request.dataFolder = fs::current_path();
		request.engineFolder = XGE_ENGINE_FOLDER;
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
	const GeneratedProgram program = generateGame(requestFor(fs::current_path() / "descriptions/pong_min.xml", folder.path / "out"));
	const fs::path out = folder.path / "out";

	for (const char* file : { "source/pong_min.cpp", "source/main.cpp", "source/game_parts.h", "CMakeLists.txt", "README.md", ".gitignore" })
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

	// the game is what the engine reads from the file, a line for each tag
	const std::string data = readFile(out / "source/pong_min.cpp");
	CHECK(data.rfind("// pong_min.cpp\n// XML Game Engine\n", 0) == 0);
	CHECK(data.find("#include \"game_parts.h\"") != std::string::npos);
	CHECK(data.find("xge::GameData pongMinGame()\n{") != std::string::npos);
	CHECK(data.find("\t// <object name=\"ball\">\n\tRawObject playingBallObject()\n\t{\n\t\tRawObject object;\n\t\tobject.name = \"ball\";") != std::string::npos);
	CHECK(data.find("\t\tobject.sprite = sprite(\"rectangle\", \"\", [](RawSprite& look)") != std::string::npos);
	CHECK(data.find("\t\tobject.rawCollisionData.enabled = true;") != std::string::npos);
	CHECK(data.find("addEdgeRule(object.rawCollisionData, \"vertical\", { verb(\"bounce\") });") != std::string::npos);
	CHECK(data.find("\tgame.objects = objects();") != std::string::npos);
	CHECK(data.find("\t\tstate.objects = playingObjects();") != std::string::npos);
	CHECK(data.find("std::vector<RawSound> sounds()") == std::string::npos);

	// game_parts.h is the target's own, copied unchanged
	CHECK(readFile(out / "source/game_parts.h") == readFile(fs::current_path() / "generators/engine-cpp/modules/game_parts.h"));

	// the build compiles the engine's files beside them, with the engine's warnings
	const std::string cmake = readFile(out / "CMakeLists.txt");
	CHECK(cmake.find("\tsource/pong_min.cpp\n") != std::string::npos);
	CHECK(cmake.find("\tengine/game/game.cpp\n") != std::string::npos);
	CHECK(cmake.find("\tengine/engine/engine.cpp\n") != std::string::npos);
	CHECK(cmake.find("-Wall -Wextra -Wpedantic -Wshadow -Wconversion") != std::string::npos);
}

TEST_CASE("a generated program carries the engine's files unchanged, and none of its XML or library choosing", "[generate]")
{
	TempFolder folder("xge_test_generate_engine_files");
	const GeneratedProgram program = generateGame(requestFor(fs::current_path() / "descriptions/pong.xml", folder.path / "out"));
	const fs::path engine = folder.path / "out/engine";

	std::set<std::string> copied;
	for (const fs::path& file : program.files)
	{
		const std::string path = file.generic_string();
		if (!path.starts_with("engine/"))
		{
			continue;
		}
		INFO(path);
		// The path inside engine/ is the one inside xge/lib/ and the one the
		// files include each other by ("game/game.h").
		const std::string name = path.substr(7);
		CHECK(readFile(engine / name) == readFile(fs::path(XGE_ENGINE_FOLDER) / "lib" / name));
		copied.insert(name);
	}
	REQUIRE(copied.count("engine/engine.cpp") == 1);
	REQUIRE(copied.count("game/game.cpp") == 1);
	copied.insert("game_parts.h");
	// Each verb's file holds its reader, so what a reader is given comes too
	// (the node every XML library is read through, and the shared readers),
	// but no XML library and nothing that reads a game.
	CHECK(copied.count("xml/document.h") == 1);
	CHECK(copied.count("load/reading.cpp") == 1);
	for (const char* left : { "load/xml.h", "load/xml.cpp", "xml/document.cpp", "load/xsd.h", "window/window.cpp", "audio/audio.cpp", "engine/backends.cpp",
		"window/raylib.h", "window/sdl2.h", "window/opengl.h", "audio/raylib.h", "audio/sdl2.h", "engine/folder.h" })
	{
		INFO(left);
		CHECK(copied.count(left) == 0);
		CHECK_FALSE(fs::exists(engine / left));
	}

	// Every file a copied file includes is copied too, so the program builds
	// from them alone.
	for (const std::string& name : copied)
	{
		std::istringstream lines(readFile(name == "game_parts.h" ? folder.path / "out/source" / name : engine / name));
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
	const GeneratedProgram program = generateGame(requestFor(fs::current_path() / "descriptions/spaceinvaders2.xml", folder.path / "out"));
	const std::set<fs::path> assets(program.assets.begin(), program.assets.end());
	CHECK(assets.count("assets/tuffy.ttf") == 1);
	CHECK(assets.count("assets/Space Invaders Color Sprites.svg") == 1);
	CHECK(fs::exists(folder.path / "out/assets/Space Invaders Color Sprites.svg"));
}

TEST_CASE("a group is written as what it gives its members, then a line for each member or cell", "[generate]")
{
	TempFolder folder("xge_test_generate_group");
	generateGame(requestFor(fs::current_path() / "descriptions/spaceinvaders.xml", folder.path / "out"));
	const std::string data = readFile(folder.path / "out/source/spaceinvaders.cpp");

	const GameData game = readGame((fs::current_path() / "descriptions/spaceinvaders.xml").string());
	// The aliens are the playing state's own objects.
	std::size_t members = 0;
	const auto tally = [&](const std::vector<RawObject>& objects)
	{
		for (const RawObject& object : objects)
		{
			if (!object.groupName.empty())
			{
				++members;
			}
		}
	};
	tally(game.objects);
	for (const RawState& state : game.states) { tally(state.objects); }
	REQUIRE(members > 55);
	CHECK(data.find("\tvoid playingAliensGroup(std::vector<RawObject>& all)") != std::string::npos);
	CHECK(data.find("\t\tMember group;\n") != std::string::npos);
	CHECK(count(data, "\t\taddCell(all, group, ") + count(data, "\t\taddMember(all, group, ") == members);
}

TEST_CASE("a game the engine will not load is not generated, and the folder made for it goes", "[generate]")
{
	TempFolder folder("xge_test_generate_broken");
	fs::create_directories(folder.path);
	std::ofstream(folder.path / "broken.xml") << "<game><window name=\"Broken\">";

	CHECK_THROWS_AS(generateGame(requestFor(folder.path / "broken.xml", folder.path / "out")), GenerateError);
	CHECK_FALSE(fs::exists(folder.path / "out"));

	CHECK_THROWS_AS(generateGame(requestFor(fs::current_path() / "descriptions/pong.xml", folder.path / "other", "amiga-asm")), GenerateError);
	CHECK_FALSE(fs::exists(folder.path / "other"));

	// a stylesheet that stops leaves no half a program behind
	fs::create_directories(folder.path / "generators/stops");
	std::ofstream(folder.path / "generators/stops/generate.xsl")
		<< "<xsl:stylesheet version=\"1.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\">"
		   "<xsl:template match=\"/\"><xsl:message terminate=\"yes\">cannot</xsl:message></xsl:template></xsl:stylesheet>";
	GenerateRequest stops = requestFor(fs::current_path() / "descriptions/pong.xml", folder.path / "stopped", "stops");
	stops.generators = folder.path / "generators";
	CHECK_THROWS_AS(generateGame(stops), GenerateError);
	CHECK_FALSE(fs::exists(folder.path / "stopped"));
}

TEST_CASE("the function a game is written as is named for its file", "[generate]")
{
	CHECK(gameFunctionName("descriptions/pong_min.xml") == "pongMinGame");
	CHECK(gameFunctionName("descriptions/spaceinvaders2.xml") == "spaceinvaders2Game");
	CHECK(gameFunctionName("3d-tennis.xml") == "n3dTennisGame");
}

namespace
{
	// A C++ file with its date line left out, which says when it was
	// written, and its line ends as stored: a checkout on Windows ends the
	// hand-written lines with \r\n (.gitattributes), the generator with \n.
	std::string undated(const std::string& written)
	{
		std::string text;
		for (const char c : written) { if (c != '\r') { text += c; } }
		const std::string mark = "\n// date: ";
		const std::size_t at = text.find(mark);
		if (at == std::string::npos) { return text; }
		return text.substr(0, at) + text.substr(text.find('\n', at + 1));
	}
}

TEST_CASE("windows-cpp writes pong_min as the hand-written one in games/prototypes is written", "[generate]")
{
	TempFolder folder("xge_test_generate_windows_cpp");
	const fs::path out = folder.path / "out";
	generateGame(requestFor(fs::current_path() / "descriptions/pong_min.xml", out, "windows-cpp"));
	const fs::path written = fs::path(XGE_ENGINE_FOLDER).parent_path() / "games/prototypes/pong_min/source";

	for (const char* file : { "pong_min.h", "pong_min.cpp", "main.cpp" })
	{
		INFO(file);
		CHECK(undated(readFile(out / "source" / file)) == undated(readFile(written / file)));
	}

	// its build compiles the engine's small functions, and no Game, loader or XML
	const std::string build = readFile(out / "CMakeLists.txt");
	CHECK(build.find("engine/play/frame.cpp") != std::string::npos);
	CHECK(build.find("SFML::Graphics") != std::string::npos);
	CHECK(build.find("game.cpp") == std::string::npos);
	CHECK(build.find("load/") == std::string::npos);
	CHECK(build.find("xml/") == std::string::npos);
	CHECK(fs::exists(out / "engine/play/verbs.cpp"));
	CHECK_FALSE(fs::exists(out / "engine/game/game.cpp"));
	CHECK(fs::exists(out / "LICENSE"));
}

TEST_CASE("windows-cpp lists every part of a game it has no words for yet, then stops", "[generate]")
{
	TempFolder folder("xge_test_generate_windows_cpp_stops");
	try
	{
		generateGame(requestFor(fs::current_path() / "descriptions/pong.xml", folder.path / "out", "windows-cpp"));
		FAIL("pong.xml was written");
	}
	catch (const GenerateError& error)
	{
		const std::string what = error.what();
		INFO(what);
		CHECK(what.find("windows-cpp cannot write these parts of pong.xml yet; use engine-cpp for this game:\n") != std::string::npos);
		// the whole list, a line each and each once, with the object it is in
		CHECK(what.find("\n  more than one <state>") != std::string::npos);
		CHECK(what.find("\n  <styles> in <game>") != std::string::npos);
		CHECK(what.find("\n  <text> in <sprite> (object 'score1')") != std::string::npos);
		CHECK(what.find("\n  <play> in <collision> (object 'ball')") != std::string::npos);
		CHECK(what.find("\n  the name 'window.width.center' (object 'score1')") != std::string::npos);
		CHECK(count(what, "\n  <styles> in <game>") == 1);
		// a long list comes through whole (libxslt hands it over in one piece)
		CHECK(what.find("\n  <sounds> in <game>") != std::string::npos);
	}
	CHECK_FALSE(fs::exists(folder.path / "out"));
}

TEST_CASE("the games windows-cpp covers: the rest stop with their list", "[generate]")
{
	// A line here for each game it learns to write (plan A5).
	const std::set<std::string> covered{ "pong_min" };

	TempFolder folder("xge_test_generate_windows_cpp_covers");
	std::set<std::string> written;
	for (const auto& entry : fs::directory_iterator(fs::current_path() / "descriptions"))
	{
		if (entry.path().extension() != ".xml") { continue; }
		const std::string game = entry.path().stem().string();
		if (game.find("_scratch") != std::string::npos) { continue; }
		try
		{
			generateGame(requestFor(entry.path(), folder.path / game, "windows-cpp"));
			written.insert(game);
		}
		catch (const GenerateError& error)
		{
			INFO(game << ": " << error.what());
			CHECK(std::string(error.what()).find("use engine-cpp for this game:\n  ") != std::string::npos);
		}
	}
	CHECK(written == covered);
}
