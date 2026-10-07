// test_generate.cpp
// XML Game Engine
// author: beefviper
// date: Oct 5, 2026
//
// Catch2 tests for xgecli --generate (cli/source/generate.cpp and the
// stylesheets in generators/windows-cpp): what it writes for pong_min.xml (a
// plain SFML program in the agreed layout, with nothing of the engine in it)
// and for all of pong.xml (screens, texts, pictures, sounds, and the physics
// and sound modules copied beside it), that a game carries only the modules,
// helpers and headers it uses, arithmetic as plain C++, and that what it
// cannot generate yet stops it with a message naming the tag. And pong_min.xml
// itself, played by the engine. Whether a written program builds and plays is
// not tested here (it needs a compiler and a window): see
// docs/designs/01-vision-and-format.md.

#include "command.h"
#include "game.h"
#include "generate.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
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

	// main.cpp, CMakeLists.txt, README.md and the physics module; no sound.
	CHECK(program.files.size() == 4);
	CHECK(program.assets.empty());
	CHECK(fs::exists(output.path / "physics.h"));
	CHECK_FALSE(fs::exists(output.path / "sound.h"));
	CHECK_FALSE(fs::exists(output.path / "manifest.xml"));

	const std::string main = readFile(output.path / "main.cpp");

	// The header, then includes, globals, objects, declarations, main and the
	// definitions, in that order.
	const std::size_t header = main.find("// main.cpp\n// XML Game Engine\n// author: beefviper\n// date: ");
	const std::size_t module = main.find("#include \"physics.h\"");
	const std::size_t include = main.find("#include <SFML/Graphics.hpp>");
	const std::size_t window = main.find("const float windowWidth = 1280.0f;");
	const std::size_t tunables = main.find("const float ballRadius = 10.0f;");
	const std::size_t objects = main.find("sf::CircleShape ball;");
	const std::size_t declarations = main.find("void updateBall();");
	const std::size_t mainFunction = main.find("int main()");
	const std::size_t loop = main.find("while (window.isOpen())");
	const std::size_t definition = main.find("void updateBall()\n{");
	CHECK(header == 0);
	CHECK(module < include);
	CHECK(include < window);
	CHECK(window < tunables);
	CHECK(tunables < objects);
	CHECK(objects < declarations);
	CHECK(declarations < mainFunction);
	CHECK(mainFunction < loop);
	CHECK(loop < definition);
	CHECK(definition != std::string::npos);

	// The rules are statements about the game's own objects, by their names.
	// The physics is the module's, called by name.
	CHECK(main.find("physics::deflect(ball, ballVelocity, paddle1, 45.0f);") != std::string::npos);
	CHECK(main.find("if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))\n\t{\n\t\tpaddle1.move({0.0f, -step});") != std::string::npos);
	CHECK(main.find("if (physics::past(ball, physics::Edge::Top, windowArea))\n\t{\n\t\tphysics::bounce(ball, ballVelocity, physics::Edge::Top, windowArea);") != std::string::npos);
	CHECK(main.find("paddle1.setPosition({windowLeft + margin, windowHeightCenter - height / 2.0f});") != std::string::npos);

	// Nothing of the engine.
	CHECK(main.find("xge::") == std::string::npos);
	CHECK(main.find("namespace") == std::string::npos);

	const std::string cmake = readFile(output.path / "CMakeLists.txt");
	CHECK(cmake.find("add_executable(pong_min main.cpp physics.h)") != std::string::npos);
	CHECK(cmake.find("set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT pong_min)") != std::string::npos);
	CHECK(cmake.find("Audio") == std::string::npos);
}

TEST_CASE("generating pong writes every screen, text, picture and sound, with the modules beside it", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder output("xge_test_generate_pong");
	const GeneratedProgram program = generateGame(requestFor("games/pong.xml", output.path));

	// main.cpp, CMakeLists.txt, README.md, physics.h, sound.h and sound.cpp,
	// and the font and the paddle's picture.
	CHECK(program.files.size() == 6);
	CHECK(program.assets.size() == 2);
	CHECK(fs::exists(output.path / "sound.cpp"));
	CHECK(fs::exists(output.path / "assets/tuffy.ttf"));
	CHECK(fs::exists(output.path / "assets/paddle.jpg"));

	const std::string main = readFile(output.path / "main.cpp");

	// The screens, a stack of them, and what each does.
	CHECK(main.find("enum class Screen { Mainmenu, Settings, Playing, Paused, Gameover };") != std::string::npos);
	CHECK(main.find("screens.assign(1, Screen::Mainmenu);") != std::string::npos);
	CHECK(main.find("case Screen::Mainmenu:\n\t\tif (key == sf::Keyboard::Key::Space)\n\t\t{\n\t\t\tscreens.push_back(Screen::Playing);\n\t\t\tstartSound.play();") != std::string::npos);
	CHECK(main.find("if (paddle1Score >= 15.0f || paddle2Score >= 15.0f)\n\t{\n\t\tscreens.push_back(Screen::Gameover);") != std::string::npos);

	// Texts, a number kept up to date, and a picture flipped.
	CHECK(main.find("sf::Text title(font);") != std::string::npos);
	CHECK(main.find("title.setPosition({(windowWidthCenter - physics::width(title) / 2.0f), margin});") != std::string::npos);
	CHECK(main.find("paddle2Score += 1.0f;\n\t\tshowScore2();") != std::string::npos);
	CHECK(main.find("sf::Sprite paddle2(paddleTexture);") != std::string::npos);
	CHECK(main.find("// flipped left to right") != std::string::npos);

	// A name C++ has already (the object "continue") gets an underscore; a
	// sound's or picture's name does not need one.
	CHECK(main.find("sf::Text continue_(font);") != std::string::npos);
	CHECK(main.find("sound::Sound startSound;") != std::string::npos);

	// Sounds by their notes.
	CHECK(main.find("{sound::Wave::Triangle, sound::pitch(\"E5\"), sound::pitch(\"E3\"), 0.35f}") != std::string::npos);
	CHECK(main.find("sound::rest(0.1f),") != std::string::npos);

	const std::string cmake = readFile(output.path / "CMakeLists.txt");
	CHECK(cmake.find("find_package(SFML 3 COMPONENTS Graphics Audio QUIET)") != std::string::npos);
	CHECK(cmake.find("add_executable(pong main.cpp physics.h sound.h sound.cpp)") != std::string::npos);
	CHECK(cmake.find("PRIVATE SFML::Graphics SFML::Audio)") != std::string::npos);
	CHECK(cmake.find("/assets") != std::string::npos);
}

TEST_CASE("a generated game carries only the modules, helper functions and headers it uses", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_tiny_helpers");
	writeTinyGame(folder.path / "tiny.xml", "");
	generateGame(requestFor(folder.path / "tiny.xml", folder.path / "out"));

	const GeneratedProgram program = generateGame(requestFor(folder.path / "tiny.xml", folder.path / "out2"));
	CHECK(program.files.size() == 4);
	CHECK_FALSE(fs::exists(folder.path / "out2/sound.h"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	CHECK(main.find("#include \"physics.h\"") != std::string::npos);
	CHECK(main.find("#include \"sound.h\"") == std::string::npos);
	CHECK(main.find("physics::past(box, physics::Edge::Left, windowArea)") != std::string::npos);
	CHECK(main.find("physics::touching(") == std::string::npos);
	CHECK(main.find("randomBetween(") == std::string::npos);
	CHECK(main.find("#include <random>") == std::string::npos);
	CHECK(main.find("#include <vector>") == std::string::npos);
	CHECK(main.find("enum class Screen") == std::string::npos);
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
	CHECK(refusal("<collision edge=\"top\"><inc variable=\"lives\" /></collision>", "<variable name=\"lives\">3</variable>", "").find("cannot generate <inc variable=\"lives\"> (it counts an object variable, as paddle1.score) yet") != std::string::npos);
	CHECK(refusal("<collision edge=\"top\"><play sound=\"boom\" /></collision>", "", "").find("cannot generate <play sound=\"boom\">, which is not a <sound> yet") != std::string::npos);
	CHECK(refusal("", "", "<state name=\"paused\"><inputs><input button=\"space\"><push state=\"nowhere\" /></input></inputs></state>").find("<push state=\"nowhere\">, which is not a <state>") != std::string::npos);
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

	// Past paddle1, off the left: back in the middle with a new serve, at the
	// ball's speed and within 30 degrees of straight left or right.
	ball.position = { 300.0f, 50.0f };
	ball.velocity = { -7.0f, 0.0f };
	int frames = 0;
	while (ball.position.x < 400.0f && frames < 100)
	{
		game.updateObjects();
		++frames;
	}
	CHECK(frames < 100);
	CHECK(std::abs(ball.position.x - ball.positionOriginal.x) <= 6.01f);
	CHECK(std::hypot(ball.velocity.x, ball.velocity.y) == Catch::Approx(6.0f));
	CHECK(std::abs(ball.velocity.y) <= std::abs(ball.velocity.x) * std::tan(30.0f * 3.14159265f / 180.0f) + 0.001f);
}

TEST_CASE("pong_min serves to either side, a new draw for every point", "[generate][pong_min]")
{
	Game game{ "games/pong_min.xml" };
	game.setCurrentState(0);
	Object& ball = game.getObject("ball");

	bool left = false;
	bool right = false;
	for (int point = 0; point < 40; ++point)
	{
		ball.position = { -100.0f, ball.positionOriginal.y };
		game.updateObjects();
		CHECK(std::hypot(ball.velocity.x, ball.velocity.y) == Catch::Approx(6.0f));
		(ball.velocity.x < 0.0f ? left : right) = true;
	}
	CHECK(left);
	CHECK(right);
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
		"<variable name=\"half\"><formula><add><augend>tiny.step</augend><addend><divide><dividend>window.width</dividend><divisor>2</divisor></divide></addend></add></formula></variable>"
		"<variable name=\"steps\"><equation><add name=\"both\" augend=\"1\" addend=\"2\" /><divide dividend=\"12\" divisor=\"both\" /></equation></variable>");
	generateGame(requestFor(folder.path / "tiny.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	CHECK(main.find("boxInner = (10.0f - (4.0f - 3.0f));") != std::string::npos);
	CHECK(main.find("boxSum = ((1.0f + 2.0f) * 3.0f);") != std::string::npos);
	CHECK(main.find("boxChain = (9.0f - 1.0f - 2.0f);") != std::string::npos);
	CHECK(main.find("boxHalf = (tinyStep + windowWidth / 2.0f);") != std::string::npos);
	CHECK(main.find("boxSteps = (12.0f / (1.0f + 2.0f));") != std::string::npos);
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
