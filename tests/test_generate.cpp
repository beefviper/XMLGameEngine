// test_generate.cpp
// XML Game Engine
// author: beefviper
// date: Oct 5, 2026
//
// Catch2 tests for xgecli --generate (cli/source/generate.cpp and the
// windows-cpp stylesheets in generators/): what it writes for Pong, that a game
// carries only the parts of the runtime it uses, and that a tag the target
// cannot generate yet stops it with a message naming the tag. Whether the
// program it writes builds and plays is not tested here (it needs a compiler
// and a window): see docs/designs/01-vision-and-format.md.

#include "generate.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
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
		std::ifstream in(file);
		std::stringstream text;
		text << in.rdbuf();
		return text.str();
	}

	// The games, assets and generators are next to the test program (assets.cmake).
	GenerateRequest requestFor(const fs::path& game, const fs::path& output)
	{
		GenerateRequest request;
		request.gameFile = game;
		request.target = "windows-cpp";
		request.generators = fs::current_path() / "generators";
		request.dataFolder = fs::current_path();
		request.output = output;
		return request;
	}

	// The smallest game: one rectangle on one screen.
	void writeTinyGame(const fs::path& file, const std::string& extra)
	{
		fs::create_directories(file.parent_path());
		std::ofstream(file) <<
			"<game>\n"
			"  <window name=\"Tiny\"><width>320</width><height>200</height><background>color.black</background><fullscreen>false</fullscreen><framerate>60</framerate></window>\n"
			"  <objects>\n"
			"    <object name=\"box\">\n"
			"      <sprite><rectangle><width>10</width><height>10</height><color>color.white</color></rectangle></sprite>\n"
			"      <position><x>window.width.center</x><y>window.height.center</y></position>\n"
			"      <velocity><x>1</x><y>0</y></velocity>\n"
			"      <collisions><enabled>true</enabled><collision edge=\"horizontal\"><bounce /></collision>" << extra << "</collisions>\n"
			"    </object>\n"
			"  </objects>\n"
			"  <states><state name=\"playing\"><shows><show object=\"box\" /></shows></state></states>\n"
			"</game>\n";
	}
}

TEST_CASE("generating Pong writes its program and copies its assets", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder output("xge_test_generate_pong");
	const GeneratedProgram program = generateGame(requestFor("games/pong.xml", output.path));

	CHECK(program.files.size() == 3);
	CHECK(fs::exists(output.path / "main.cpp"));
	CHECK(fs::exists(output.path / "CMakeLists.txt"));
	CHECK(fs::exists(output.path / "README.md"));
	CHECK(fs::exists(output.path / "assets/tuffy.ttf"));
	CHECK(fs::exists(output.path / "assets/paddle.jpg"));
	CHECK_FALSE(fs::exists(output.path / "manifest.xml"));

	// The rules are plain statements about Pong's own objects.
	const std::string main = readFile(output.path / "main.cpp");
	CHECK(main.find("deflect(o_ball, o_paddle1, edgeOfFirst, (45.0f));") != std::string::npos);
	CHECK(main.find("v_paddle2_score += 1.0f;") != std::string::npos);
	CHECK(main.find("if (v_paddle1_score >= (15.0f) || v_paddle2_score >= (15.0f))") != std::string::npos);
	CHECK(main.find("case sf::Keyboard::Key::W:") != std::string::npos);

	// The title's <equation> is a lambda of its steps.
	CHECK(main.find("const float s_half = xge::divide(v_title_width, 2.0f);") != std::string::npos);

	CHECK(readFile(output.path / "CMakeLists.txt").find("add_executable(pong main.cpp)") != std::string::npos);
}

TEST_CASE("a generated game carries only the verbs it uses", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_tiny");
	writeTinyGame(folder.path / "tiny.xml", "");
	generateGame(requestFor(folder.path / "tiny.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	CHECK(main.find("void bounceOff(") != std::string::npos);
	CHECK(main.find("void deflect(") == std::string::npos);
	CHECK(main.find("void stick(") == std::string::npos);
	CHECK(main.find("struct Voice") == std::string::npos);
}

TEST_CASE("a tag the target cannot generate yet is named in the error", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_refused");
	writeTinyGame(folder.path / "tiny.xml", "<collision edge=\"vertical\"><wrap /></collision>");

	try
	{
		generateGame(requestFor(folder.path / "tiny.xml", folder.path / "out"));
		FAIL("generated a game with <wrap />");
	}
	catch (const GenerateError& error)
	{
		const std::string message = error.what();
		CHECK(message.find("cannot generate <wrap> yet") != std::string::npos);
		CHECK(message.find("object box") != std::string::npos);
	}
}

TEST_CASE("an unknown target or a missing game is an error", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder output("xge_test_generate_errors");

	GenerateRequest unknown = requestFor("games/pong.xml", output.path);
	unknown.target = "amiga-asm";
	CHECK_THROWS_AS(generateGame(unknown), GenerateError);

	CHECK_THROWS_AS(generateGame(requestFor("games/nosuchgame.xml", output.path)), GenerateError);
}
