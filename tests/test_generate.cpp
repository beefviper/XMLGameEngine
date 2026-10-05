// test_generate.cpp
// XML Game Engine
// author: beefviper
// date: Oct 5, 2026
//
// Catch2 tests for xgecli --generate (cli/source/generate.cpp and the
// stylesheets in generators/). windows-cpp: what it writes for pong_min.xml (a
// plain SFML program in the agreed layout, with nothing of the engine in it),
// that a game carries only the helper functions it uses, and that what it cannot
// generate yet stops it with a message naming the tag. windows-cpp-full: what it
// writes for Pong, and its runtime parts. And pong_min.xml itself, played by the
// engine. Whether a written program builds and plays is not tested here (it needs
// a compiler and a window): see docs/designs/01-vision-and-format.md.

#include "command.h"
#include "game.h"
#include "generate.h"

#include <catch2/catch_approx.hpp>
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
	GenerateRequest requestFor(const fs::path& game, const fs::path& output, const std::string& target = "windows-cpp")
	{
		GenerateRequest request;
		request.gameFile = game;
		request.target = target;
		request.generators = fs::current_path() / "generators";
		request.dataFolder = fs::current_path();
		request.output = output;
		return request;
	}

	// The smallest game: one rectangle on one screen, with `variables` (an
	// object's <variable>s), `extra` (more collision rules) and `states` (more
	// states) put in.
	void writeTinyGame(const fs::path& file, const std::string& extra, const std::string& variables = "", const std::string& states = "")
	{
		fs::create_directories(file.parent_path());
		std::ofstream(file) <<
			"<game>\n"
			"  <window name=\"Tiny\"><width>320</width><height>200</height><background>color.black</background><fullscreen>false</fullscreen><framerate>60</framerate></window>\n"
			"  <variables><variable name=\"tiny.step\">2</variable></variables>\n"
			"  <objects>\n"
			"    <object name=\"box\">\n"
			"      <sprite><rectangle><width>10</width><height>10</height><color>color.white</color></rectangle></sprite>\n"
			"      <position><x>window.width.center</x><y>window.height.center</y></position>\n"
			"      <velocity><x>1</x><y>0</y></velocity>\n"
			"      <collisions><enabled>true</enabled><collision edge=\"horizontal\"><bounce /></collision>" << extra << "</collisions>\n"
			"      <variables>" << variables << "</variables>\n"
			"    </object>\n"
			"  </objects>\n"
			"  <states><state name=\"playing\"><shows><show object=\"box\" /></shows></state>" << states << "</states>\n"
			"</game>\n";
	}
}

// ------------------------------------------------------------- windows-cpp

TEST_CASE("generating pong_min writes a plain SFML program, laid out as by hand", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder output("xge_test_generate_pong_min");
	const GeneratedProgram program = generateGame(requestFor("games/pong_min.xml", output.path));

	CHECK(program.files.size() == 3);
	CHECK(program.assets.empty());
	CHECK_FALSE(fs::exists(output.path / "manifest.xml"));

	const std::string main = readFile(output.path / "main.cpp");

	// The header, then includes, globals, objects, declarations, main and the
	// definitions, in that order.
	const std::size_t header = main.find("// main.cpp\n// XML Game Engine\n// author: beefviper\n// date: ");
	const std::size_t include = main.find("#include <SFML/Graphics.hpp>");
	const std::size_t window = main.find("const float windowWidth = 1280.0f;");
	const std::size_t tunables = main.find("const float ballRadius = 10.0f;");
	const std::size_t objects = main.find("sf::CircleShape ball;");
	const std::size_t declarations = main.find("void updateBall();");
	const std::size_t mainFunction = main.find("int main()");
	const std::size_t loop = main.find("while (window.isOpen())");
	const std::size_t definition = main.find("void updateBall()\n{");
	CHECK(header == 0);
	CHECK(include < window);
	CHECK(window < tunables);
	CHECK(tunables < objects);
	CHECK(objects < declarations);
	CHECK(declarations < mainFunction);
	CHECK(mainFunction < loop);
	CHECK(loop < definition);
	CHECK(definition != std::string::npos);

	// The rules are statements about the game's own objects, by their names.
	CHECK(main.find("deflect(ball, ballVelocity, paddle1, 45.0f);") != std::string::npos);
	CHECK(main.find("if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))\n\t{\n\t\tpaddle1.move({0.0f, -step});") != std::string::npos);
	CHECK(main.find("ballVelocity.y = std::abs(ballVelocity.y);") != std::string::npos);
	CHECK(main.find("paddle1.setPosition({windowLeft + margin, windowHeightCenter - height / 2.0f});") != std::string::npos);

	// Nothing of the engine.
	CHECK(main.find("xge::") == std::string::npos);
	CHECK(main.find("namespace") == std::string::npos);

	const std::string cmake = readFile(output.path / "CMakeLists.txt");
	CHECK(cmake.find("add_executable(pong_min main.cpp)") != std::string::npos);
	CHECK(cmake.find("set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT pong_min)") != std::string::npos);
	CHECK(cmake.find("Audio") == std::string::npos);
}

TEST_CASE("a generated game carries only the helper functions and headers it uses", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_tiny_helpers");
	writeTinyGame(folder.path / "tiny.xml", "");
	generateGame(requestFor(folder.path / "tiny.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	CHECK(main.find("float left(const sf::Shape& shape);") != std::string::npos);
	CHECK(main.find("float right(const sf::Shape& shape);") != std::string::npos);
	CHECK(main.find("float top(") == std::string::npos);
	CHECK(main.find("touching(") == std::string::npos);
	CHECK(main.find("deflect(") == std::string::npos);
	CHECK(main.find("randomBetween(") == std::string::npos);
	CHECK(main.find("#include <random>") == std::string::npos);
	CHECK(main.find("const float windowWidthCenter = windowWidth / 2.0f;") != std::string::npos);
	CHECK(main.find("const float windowLeft") == std::string::npos);
	CHECK(main.find("const float tinyStep = 2.0f;") != std::string::npos);
}

TEST_CASE("what windows-cpp cannot generate yet is named in the error", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	const auto refusal = [](const std::string& extra, const std::string& variables, const std::string& states)
	{
		TempFolder folder("xge_test_generate_min_refused");
		writeTinyGame(folder.path / "tiny.xml", extra, variables, states);
		try
		{
			generateGame(requestFor(folder.path / "tiny.xml", folder.path / "out"));
		}
		catch (const GenerateError& error)
		{
			return std::string(error.what());
		}
		return std::string("generated it");
	};

	CHECK(refusal("<collision edge=\"vertical\"><wrap /></collision>", "", "").find("cannot generate <wrap> yet (in game > objects > object box") != std::string::npos);
	CHECK(refusal("", "<variable name=\"lives\">3</variable>", "").find("cannot generate <variables> in an <object> yet") != std::string::npos);
	CHECK(refusal("", "", "<state name=\"paused\" />").find("cannot generate a second <state>") != std::string::npos);
	CHECK(refusal("<collision edge=\"top\"><reset object=\"box\" /></collision>", "", "").find("cannot generate <reset object=") != std::string::npos);
}

TEST_CASE("pong_min plays in the engine: the ball deflects off a paddle, and a point puts it back in the middle", "[generate][pong_min]")
{
	Game game{ "games/pong_min.xml" };
	game.setCurrentState(0);
	for (auto& object : game.getCurrentObjects())
	{
		object.size = measureShapeSize(object.spriteParams, object.shapeKind);
	}

	Object& ball = game.getObject("ball");
	Object& paddle1 = game.getObject("paddle1");
	REQUIRE(game.getCurrentObjects().size() == 3);
	CHECK(paddle1.position.x == Catch::Approx(30.0f));
	CHECK(ball.position.x == Catch::Approx(630.0f));

	// Straight at paddle1's middle: back out the way it came.
	ball.velocity = { -7.0f, 0.0f };
	for (int frame = 0; frame < 90; ++frame) { game.updateObjects(); }
	CHECK(ball.velocity.x > 0.0f);

	// Past paddle1, off the left: back in the middle, going the way it was.
	ball.position = { 300.0f, 50.0f };
	ball.velocity = { -7.0f, 0.0f };
	int frames = 0;
	while (ball.position.x < 400.0f && frames < 100)
	{
		game.updateObjects();
		++frames;
	}
	CHECK(frames < 100);
	CHECK(ball.position.x == Catch::Approx(ball.positionOriginal.x).margin(7.0f));
	CHECK(ball.position.y == Catch::Approx(ball.positionOriginal.y));
	CHECK(ball.velocity.x == Catch::Approx(-7.0f));
}

// -------------------------------------------------------- windows-cpp-full

TEST_CASE("generating Pong in full writes its program and copies its assets", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder output("xge_test_generate_pong");
	const GeneratedProgram program = generateGame(requestFor("games/pong.xml", output.path, "windows-cpp-full"));

	CHECK(program.files.size() == 3);
	CHECK(fs::exists(output.path / "main.cpp"));
	CHECK(fs::exists(output.path / "CMakeLists.txt"));
	CHECK(fs::exists(output.path / "README.md"));
	CHECK(fs::exists(output.path / "assets/tuffy.ttf"));
	CHECK(fs::exists(output.path / "assets/paddle.jpg"));
	CHECK_FALSE(fs::exists(output.path / "manifest.xml"));

	// The rules are plain statements about Pong's own objects.
	const std::string main = readFile(output.path / "main.cpp");
	CHECK(main.find("deflect(o_ball, o_paddle1, edgeOfFirst, 45.0f);") != std::string::npos);
	CHECK(main.find("v_paddle2_score += 1.0f;") != std::string::npos);
	CHECK(main.find("if (v_paddle1_score >= 15.0f || v_paddle2_score >= 15.0f)") != std::string::npos);
	CHECK(main.find("case sf::Keyboard::Key::W:") != std::string::npos);

	// The title's <equation> is plain arithmetic, its step written where it is used.
	CHECK(main.find("(v_window_width_center - v_title_width / 2.0f)") != std::string::npos);
	CHECK(main.find("xge::divide") == std::string::npos);

	const std::string cmake = readFile(output.path / "CMakeLists.txt");
	CHECK(cmake.find("add_executable(pong main.cpp)") != std::string::npos);
	CHECK(cmake.find("set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT pong)") != std::string::npos);
}

TEST_CASE("a game generated in full carries only the verbs it uses", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_tiny");
	writeTinyGame(folder.path / "tiny.xml", "");
	generateGame(requestFor(folder.path / "tiny.xml", folder.path / "out", "windows-cpp-full"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	CHECK(main.find("void bounceOff(") != std::string::npos);
	CHECK(main.find("void deflect(") == std::string::npos);
	CHECK(main.find("void stick(") == std::string::npos);
	CHECK(main.find("struct Voice") == std::string::npos);
}

TEST_CASE("a formula or equation is written as C++ arithmetic, bracketed only where needed", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_formulas");
	writeTinyGame(folder.path / "tiny.xml", "",
		"<variable name=\"inner\"><formula><subtract><minuend>10</minuend><subtrahend><subtract><minuend>4</minuend><subtrahend>3</subtrahend></subtract></subtrahend></subtract></formula></variable>"
		"<variable name=\"sum\"><formula><multiply><multiplicand><add><augend>1</augend><addend>2</addend></add></multiplicand><multiplier>3</multiplier></multiply></formula></variable>"
		"<variable name=\"chain\"><formula><subtract><minuend>9</minuend><subtrahend>1</subtrahend><subtrahend>2</subtrahend></subtract></formula></variable>"
		"<variable name=\"half\"><formula><add><augend>box.width</augend><addend><divide><dividend>window.width</dividend><divisor>2</divisor></divide></addend></add></formula></variable>"
		"<variable name=\"steps\"><equation><add name=\"both\" augend=\"1\" addend=\"2\" /><divide dividend=\"12\" divisor=\"both\" /></equation></variable>");
	generateGame(requestFor(folder.path / "tiny.xml", folder.path / "out", "windows-cpp-full"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	CHECK(main.find("v_box_inner_start = (10.0f - (4.0f - 3.0f));") != std::string::npos);
	CHECK(main.find("v_box_sum_start = ((1.0f + 2.0f) * 3.0f);") != std::string::npos);
	CHECK(main.find("v_box_chain_start = (9.0f - 1.0f - 2.0f);") != std::string::npos);
	CHECK(main.find("v_box_half_start = (v_box_width + v_window_width / 2.0f);") != std::string::npos);
	CHECK(main.find("v_box_steps_start = (12.0f / (1.0f + 2.0f));") != std::string::npos);
}

TEST_CASE("a tag windows-cpp-full cannot generate yet is named in the error", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_refused");
	writeTinyGame(folder.path / "tiny.xml", "<collision edge=\"vertical\"><wrap /></collision>");

	try
	{
		generateGame(requestFor(folder.path / "tiny.xml", folder.path / "out", "windows-cpp-full"));
		FAIL("generated a game with <wrap />");
	}
	catch (const GenerateError& error)
	{
		const std::string message = error.what();
		CHECK(message.find("cannot generate <wrap> yet") != std::string::npos);
		CHECK(message.find("object box") != std::string::npos);
	}
}

// ------------------------------------------------------------------ either

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
