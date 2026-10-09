// test_generate.cpp
// XML Game Engine
// author: beefviper
// date: Oct 5, 2026
//
// Catch2 tests for xgecli --generate (cli/source/generate.cpp and the
// stylesheets in generators/windows-cpp): what it writes for pong_min.xml (a
// plain SFML program in the agreed layout, with nothing of the engine in it)
// and for all of pong.xml (screens, texts, pictures, sounds, and the physics
// and sound modules copied beside it), groups as std::vectors (Space Race,
// Freeway, and members of their own), that a game carries only the modules,
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
#include <iterator>
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

TEST_CASE("generating Space Race and Freeway writes each group as a std::vector, and hops", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder race("xge_test_generate_spacerace");
	generateGame(requestFor("games/spacerace.xml", race.path));
	const std::string main = readFile(race.path / "main.cpp");

	// A lane of debris: its shapes, the velocity they share, and each one's place.
	CHECK(main.find("std::vector<sf::RectangleShape> debris1(3);\nsf::Vector2f debris1Velocity; // every one of them") != std::string::npos);
	CHECK(main.find("\tdebris1[1].setPosition({272.0f, 75.0f});") != std::string::npos);
	CHECK(main.find("\tdebris1Velocity = {2.0f, 0.0f};") != std::string::npos);

	// Each one moves and wraps round, in a loop; a rocket looks at each one.
	CHECK(main.find("\tfor (sf::RectangleShape& one : debris1)\n\t{\n\t\tone.move(debris1Velocity);") != std::string::npos);
	CHECK(main.find("\t\tphysics::wrap(one, debris1Velocity, physics::Edge::Left, windowArea);") != std::string::npos);
	CHECK(main.find("\tfor (const sf::RectangleShape& other : debris9)\n\t{\n\t\tif (physics::touching(rocket1, other))\n\t\t{\n\t\t\tstartRocket1();") != std::string::npos);
	CHECK(main.find("\t\t\tfor (const sf::RectangleShape& one : debris1)\n\t\t\t{\n\t\t\t\twindow.draw(one);") != std::string::npos);

	// The object called "start" does not clash with start().
	CHECK(main.find("sf::Text start_(font);") != std::string::npos);

	TempFolder freeway("xge_test_generate_freeway");
	generateGame(requestFor("games/freeway.xml", freeway.path));
	const std::string road = readFile(freeway.path / "main.cpp");

	// A hop is a key pressed, a step at once if it stays in the window.
	CHECK(road.find("if (key == sf::Keyboard::Key::W)\n\t\t{\n\t\t\tphysics::hop(chicken1, {0.0f, -cell}, windowArea);") != std::string::npos);
	CHECK(road.find("isKeyPressed") == std::string::npos);

	// A tunable nothing uses is left out (Freeway's margin).
	CHECK(road.find("const float margin") == std::string::npos);
	CHECK(road.find("const float cell = 50.0f;") != std::string::npos);
}

TEST_CASE("a group whose members move and look each their own way keeps a velocity each", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_group");
	fs::create_directories(folder.path);
	std::ofstream(folder.path / "flock.xml") <<
		"<game>\n"
		"  <window name=\"Flock\"><width>320</width><height>200</height><background>color.black</background><fullscreen>false</fullscreen><framerate>60</framerate></window>\n"
		"  <variables />\n"
		"  <objects>\n"
		"    <group name=\"birds\">\n"
		"      <sprite><circle><radius>4</radius><color>color.yellow</color></circle></sprite>\n"
		"      <position><y>50</y></position>\n"
		"      <velocity><x>1</x><y>0</y></velocity>\n"
		"      <collisions><enabled>true</enabled><collision edge=\"right\"><wrap /></collision></collisions>\n"
		"      <member><position><x>10</x></position></member>\n"
		"      <member><position><x>60</x></position><velocity><x>2</x></velocity></member>\n"
		"      <member><sprite><circle><radius>6</radius><color>color.red</color></circle></sprite><position><x>110</x><y>80</y></position></member>\n"
		"    </group>\n"
		"  </objects>\n"
		"  <states><state name=\"flying\"><shows><show object=\"birds\" /></shows></state></states>\n"
		"</game>\n";
	generateGame(requestFor(folder.path / "flock.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	CHECK(main.find("std::vector<sf::CircleShape> birds(3);\nstd::vector<sf::Vector2f> birdsVelocity;") != std::string::npos);
	CHECK(main.find("\tbirdsVelocity = {{1.0f, 0.0f}, {2.0f, 0.0f}, {1.0f, 0.0f}};") != std::string::npos);
	CHECK(main.find("\tbirds[2].setPosition({110.0f, 80.0f});") != std::string::npos);
	CHECK(main.find("\t// birds.3, a look of its own\n\tbirds[2].setRadius(6.0f);\n\tbirds[2].setFillColor(sf::Color::Red);") != std::string::npos);
	CHECK(main.find("\tfor (std::size_t i = 0; i < birds.size(); ++i)\n\t{\n\t\tbirds[i].move(birdsVelocity[i]);") != std::string::npos);
	CHECK(main.find("\t\tphysics::wrap(birds[i], birdsVelocity[i], physics::Edge::Right, windowArea);") != std::string::npos);
}

TEST_CASE("what can die keeps a flag for being in play, and a touch both sides have rules for is one touch", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_die");
	fs::create_directories(folder.path);
	std::ofstream(folder.path / "catch.xml") <<
		"<game>\n"
		"  <window name=\"Catch\"><width>320</width><height>240</height><background>color.black</background><fullscreen>false</fullscreen><framerate>60</framerate></window>\n"
		"  <variables />\n"
		"  <objects>\n"
		"    <object name=\"ball\">\n"
		"      <sprite><circle><radius>4</radius></circle></sprite>\n"
		"      <position><x>60</x><y>150</y></position>\n"
		"      <velocity><x>0</x><y>-3</y></velocity>\n"
		"      <collisions><enabled>true</enabled><collision edge=\"bottom\"><die /></collision><collision edge=\"all\"><bounce /></collision><collision class=\"bricks\"><bounce /></collision></collisions>\n"
		"    </object>\n"
		"    <object name=\"player\">\n"
		"      <sprite><rectangle><width>40</width><height>8</height></rectangle></sprite>\n"
		"      <position><x>140</x><y>220</y></position>\n"
		"      <velocity><x>0</x><y>0</y></velocity>\n"
		"      <collisions><enabled>true</enabled><collision object=\"drops\"><die /></collision></collisions>\n"
		"      <actions><action name=\"left\"><move direction=\"left\">4</move></action></actions>\n"
		"    </object>\n"
		"    <group name=\"bricks\" class=\"bricks\">\n"
		"      <sprite><rectangle><width>30</width><height>10</height><color>color.red</color></rectangle></sprite>\n"
		"      <position><y>20</y></position>\n"
		"      <velocity><x>0</x><y>0</y></velocity>\n"
		"      <collisions><enabled>true</enabled><collision><die /></collision></collisions>\n"
		"      <member><position><x>10</x></position></member>\n"
		"      <member><position><x>50</x></position></member>\n"
		"    </group>\n"
		"    <group name=\"drops\">\n"
		"      <sprite><circle><radius>3</radius><color>color.cyan</color></circle></sprite>\n"
		"      <position><y>0</y></position>\n"
		"      <velocity><x>0</x><y>1</y></velocity>\n"
		"      <collisions><enabled>true</enabled><collision class=\"bricks\"><die /></collision><collision edge=\"bottom\"><die /></collision></collisions>\n"
		"      <member><position><x>20</x></position></member>\n"
		"      <member><position><x>200</x></position><velocity><y>2</y></velocity></member>\n"
		"    </group>\n"
		"  </objects>\n"
		"  <states><state name=\"playing\"><shows><show object=\"ball\" /><show object=\"player\" /><show object=\"bricks\" /><show object=\"drops\" /></shows>\n"
		"    <inputs><input button=\"a\"><trigger object=\"player\" action=\"left\" /></input><input button=\"space\"><reset /></input></inputs></state></states>\n"
		"</game>\n";
	generateGame(requestFor(folder.path / "catch.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	// a flag each, true again from the start
	CHECK(main.find("bool ballAlive = true; // in play until it dies") != std::string::npos);
	CHECK(main.find("std::vector<bool> bricksAlive; // which of them are still in play") != std::string::npos);
	CHECK(main.find("\tballAlive = true;\n") != std::string::npos);
	CHECK(main.find("\tbricksAlive.assign(bricks.size(), true);") != std::string::npos);
	// drawn, moved by a key and updated only while in play
	CHECK(main.find("\t\tif (ballAlive)\n\t\t{\n\t\t\twindow.draw(ball);") != std::string::npos);
	CHECK(main.find("\t\t\tif (bricksAlive[i])\n\t\t\t{\n\t\t\t\twindow.draw(bricks[i]);") != std::string::npos);
	CHECK(main.find("\t\tif (playerAlive)\n\t\t{\n\t\t\tplayer.move({-4.0f, 0.0f});") != std::string::npos);
	CHECK(main.find("void updateBall()\n{\n\tif (!ballAlive)\n\t{\n\t\treturn;\n\t}") != std::string::npos);
	CHECK(main.find("\tfor (std::size_t i = 0; i < bricks.size(); ++i)\n\t{\n\t\tif (!bricksAlive[i])\n\t\t{\n\t\t\tcontinue;") != std::string::npos);
	// dying ends the rest of its rules this frame; every rule about the bottom
	// is one touch of it, so the ball dies there before the bounce written later
	CHECK(main.find("\t// bottom: die bounce\n\tif (physics::past(ball, physics::Edge::Bottom, windowArea))\n\t{\n\t\tballAlive = false;\n\t\tphysics::bounce(ball, ballVelocity, physics::Edge::Bottom, windowArea);\n\t\treturn;\n") != std::string::npos);
	// the ball's bounce off a brick and the brick's die are one touch, in the ball's update
	CHECK(main.find("\t\tif (bricksAlive[j] && physics::touching(ball, bricks[j]))\n\t\t{\n\t\t\t// bricks, by its own rule: die\n\t\t\tbricksAlive[j] = false;\n\t\t\tphysics::bounceOff(ball, ballVelocity, bricks[j]);") != std::string::npos);
	CHECK(main.find("physics::touching(bricks[i], ball)") == std::string::npos);
	// a member that dies inside a loop over another group leaves that loop
	CHECK(main.find("\t\t\t\tdropsAlive[j] = false;\n\t\t\t\tbricksAlive[i] = false;\n\t\t\t\tbreak;") != std::string::npos);
	CHECK(main.find("physics::touching(drops[i], bricks[j])") == std::string::npos);
}

TEST_CASE("generating Breakout writes its bricks as one group of columns and rows, and wins when none is left", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_breakout");
	generateGame(requestFor(fs::current_path() / "games/breakout.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	CHECK(main.find("// bricks: 54 of them\nstd::vector<sf::RectangleShape> bricks(54);") != std::string::npos);
	// every brick the group's look, then each row its own color
	CHECK(main.find("\tfor (sf::RectangleShape& one : bricks)\n\t{\n\t\tone.setSize({width, height});\n\t\tone.setFillColor(sf::Color::Red);\n\t}") != std::string::npos);
	CHECK(main.find("\t// bricks, row 2\n\tfor (std::size_t i = 9; i < 18; ++i)\n\t{\n\t\tbricks[i].setFillColor(sf::Color(255, 165, 0, 255));\n\t}") != std::string::npos);
	// the top row first, left to right, as the engine lays them out
	CHECK(main.find("\t// bricks: 9 columns by 6 rows\n\tfor (std::size_t row = 0; row < 6; ++row)\n\t{\n\t\tfor (std::size_t column = 0; column < 9; ++column)\n\t\t{\n"
		"\t\t\tbricks[row * 9 + column].setPosition({margin + static_cast<float>(column) * (width + 5.0f), margin / 2.0f + height + 5.0f + static_cast<float>(row) * (height + 5.0f)});") != std::string::npos);
	// the bricks never move, so their bounce off the sides is left out
	CHECK(main.find("physics::past(bricks") == std::string::npos);
	// every rule about the bottom in one touch of it: the ball bounces and dies
	CHECK(main.find("\t// bottom: bounce die\n\tif (physics::past(ball, physics::Edge::Bottom, windowArea))\n\t{\n\t\tphysics::bounce(ball, ballVelocity, physics::Edge::Bottom, windowArea);\n\t\tballAlive = false;\n\t\treturn;") != std::string::npos);
	// the class counts both groups of bricks
	CHECK(main.find("\t// bricks: none left\n\tif (std::count(strongAlive.begin(), strongAlive.end(), true) + std::count(bricksAlive.begin(), bricksAlive.end(), true) == 0)\n\t{\n\t\tscreens.push_back(Screen::Youwin);") != std::string::npos);
	CHECK(main.find("\t// ball: none left\n\tif (!ballAlive)\n\t{\n\t\tscreens.push_back(Screen::Gameover);") != std::string::npos);
	CHECK(main.find("#include <algorithm>") != std::string::npos);
}

TEST_CASE("generating Breakout gives the top row two looks, whole and cracked, and a rule for each", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_breakout_looks");
	generateGame(requestFor(fs::current_path() / "games/breakout.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	// an enum of its looks, and the one each brick shows
	CHECK(main.find("enum class StrongLook { Whole, Cracked };\nstd::vector<StrongLook> strongLook(9);") != std::string::npos);
	CHECK(main.find("void becomeStrong(std::size_t i, StrongLook look)\n{\n\tstrongLook[i] = look;\n\tswitch (look)\n\t{\n\tcase StrongLook::Whole:\n") != std::string::npos);
	CHECK(main.find("\tcase StrongLook::Cracked:\n\t\tstrong[i].setSize({width, height});\n\t\tstrong[i].setFillColor(sf::Color(64, 64, 64, 255));\n\t\tbreak;") != std::string::npos);
	// start() shows the first look on every brick
	CHECK(main.find("\tfor (std::size_t i = 0; i < strong.size(); ++i)\n\t{\n\t\tbecomeStrong(i, StrongLook::Whole);\n\t}") != std::string::npos);
	// the rules in the order written, each looking at the look as it comes:
	// a cracked brick goes, a whole one cracks, and a hit does only one
	CHECK(main.find("\t\t\tif (strongLook[j] == StrongLook::Cracked)\n\t\t\t{\n\t\t\t\tstrongAlive[j] = false;\n\t\t\t}\n"
		"\t\t\tif (strongLook[j] == StrongLook::Whole)\n\t\t\t{\n\t\t\t\tbecomeStrong(j, StrongLook::Cracked);\n\t\t\t}\n"
		"\t\t\tphysics::bounceOff(ball, ballVelocity, strong[j]);") != std::string::npos);
	// its rules are all about the ball, written with the ball's: nothing left of its own each frame
	CHECK(main.find("void updateStrong()") == std::string::npos);
	CHECK(main.find("\t// strong: 9 columns by 1 row\n") != std::string::npos);
}

TEST_CASE("an edge rule with sprite= is an if on the look, inside the touch of the edge", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_edge_looks");
	fs::create_directories(folder.path);
	std::ofstream(folder.path / "puck.xml") <<
		"<game>\n"
		"  <window name=\"Puck\"><width>320</width><height>240</height><background>color.black</background><fullscreen>false</fullscreen><framerate>60</framerate></window>\n"
		"  <variables />\n"
		"  <objects>\n"
		"    <object name=\"puck\">\n"
		"      <sprite name=\"whole\"><rectangle><width>10</width><height>10</height><color>color.grey</color></rectangle></sprite>\n"
		"      <sprite name=\"cracked\"><rectangle><width>10</width><height>10</height><color>color.darkgrey</color></rectangle></sprite>\n"
		"      <position><x>20</x><y>100</y></position>\n"
		"      <velocity><x>-4</x><y>0</y></velocity>\n"
		"      <collisions><enabled>true</enabled>\n"
		"        <collision edge=\"left\" sprite=\"cracked\"><die /></collision>\n"
		"        <collision edge=\"left\" sprite=\"whole\"><become sprite=\"cracked\" /><bounce /></collision>\n"
		"        <collision edge=\"horizontal\"><inc variable=\"puck.hits\" /></collision>\n"
		"        <collision edge=\"vertical\" sprite=\"cracked\"><wrap /></collision>\n"
		"      </collisions>\n"
		"      <variables><variable name=\"hits\">0</variable></variables>\n"
		"    </object>\n"
		"  </objects>\n"
		"  <states><state name=\"playing\"><shows><show object=\"puck\" /></shows><inputs /><conditions /></state></states>\n"
		"</game>\n";
	generateGame(requestFor(folder.path / "puck.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	// each rule in the order written, the look looked at as it comes; the
	// die under a look may not have happened, so the way out asks
	CHECK(main.find("\tif (physics::past(puck, physics::Edge::Left, windowArea))\n\t{\n"
		"\t\tif (puckLook == PuckLook::Cracked)\n\t\t{\n\t\t\tpuckAlive = false;\n\t\t}\n"
		"\t\tif (puckLook == PuckLook::Whole)\n\t\t{\n\t\t\tbecomePuck(PuckLook::Cracked);\n\t\t\tphysics::bounce(puck, puckVelocity, physics::Edge::Left, windowArea);\n\t\t}\n"
		"\t\tpuckHits += 1.0f;\n\t\tif (!puckAlive)\n\t\t{\n\t\t\treturn;\n\t\t}\n\t}") != std::string::npos);
	// a wrap with sprite= wraps only while it shows that look
	CHECK(main.find("\t// top: wrap, while it shows cracked\n\tif (puckLook == PuckLook::Cracked)\n\t{\n\t\tphysics::wrap(puck, puckVelocity, physics::Edge::Top, windowArea);\n\t}") != std::string::npos);
}

TEST_CASE("a rule with unless= is passed over while touching that class, through one function for the class", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_unless");
	fs::create_directories(folder.path);
	std::ofstream(folder.path / "river.xml") <<
		"<game>\n"
		"  <window name=\"River\"><width>320</width><height>240</height><background>color.black</background><fullscreen>false</fullscreen><framerate>60</framerate></window>\n"
		"  <variables />\n"
		"  <objects>\n"
		"    <object name=\"water\" class=\"water\">\n"
		"      <sprite><rectangle><width>320</width><height>60</height><color>color.blue</color></rectangle></sprite>\n"
		"      <position><x>0</x><y>80</y></position><velocity><x>0</x><y>0</y></velocity>\n"
		"      <collisions><enabled>true</enabled></collisions>\n"
		"    </object>\n"
		"    <group name=\"logs\" class=\"logs\">\n"
		"      <sprite><rectangle><width>40</width><height>20</height><color>color.brown</color></rectangle></sprite>\n"
		"      <velocity><x>2</x><y>0</y></velocity>\n"
		"      <collisions><enabled>true</enabled><collision edge=\"horizontal\"><wrap /></collision></collisions>\n"
		"      <member><position><x>20</x><y>100</y></position></member>\n"
		"      <member><position><x>200</x><y>100</y></position></member>\n"
		"    </group>\n"
		"    <object name=\"frog\">\n"
		"      <sprite><rectangle><width>10</width><height>10</height><color>color.green</color></rectangle></sprite>\n"
		"      <position><x>150</x><y>10</y></position><velocity><x>0</x><y>1</y></velocity>\n"
		"      <collisions><enabled>true</enabled>\n"
		"        <collision class=\"water\" unless=\"logs\"><inc variable=\"frog.wet\" /></collision>\n"
		"        <collision edge=\"bottom\" unless=\"water\"><bounce /></collision>\n"
		"      </collisions>\n"
		"      <variables><variable name=\"wet\">0</variable></variables>\n"
		"    </object>\n"
		"  </objects>\n"
		"  <states><state name=\"playing\"><shows><show object=\"water\" /><show object=\"logs\" /><show object=\"frog\" /></shows><inputs /><conditions /></state></states>\n"
		"</game>\n";
	generateGame(requestFor(folder.path / "river.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	// the touch, and not on a log other than the water it touches
	CHECK(main.find("\tif (physics::touching(frog, water) && !touchingLogs(frog, &water))\n\t{\n\t\tfrogWet += 1.0f;") != std::string::npos);
	// an edge has no other: nothing to leave out
	CHECK(main.find("\t\tif (!touchingWater(frog, nullptr))\n\t\t{\n\t\t\tphysics::bounce(frog, frogVelocity, physics::Edge::Bottom, windowArea);") != std::string::npos);
	// one function for each class named, going through everything of it
	CHECK(main.find("template <typename Shape>\nbool touchingLogs(const Shape& one, const void* other);") != std::string::npos);
	CHECK(main.find("\tfor (const sf::RectangleShape& each : logs)\n\t{\n\t\tif (counts(each))\n\t\t{\n\t\t\treturn true;") != std::string::npos);
}

TEST_CASE("generating Frogger lets the frog ride a log, and the river costs a life unless it is on one", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_frogger");
	generateGame(requestFor(fs::current_path() / "games/frogger.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	// what it rides is worked out again every frame, from the rules
	CHECK(main.find("sf::Vector2f frogRiding; // the velocity of what it rides (<ride />), worked out every frame") != std::string::npos);
	// before anything on the screen moves, so a log updated before the frog counts
	CHECK(main.find("void updatePlaying()\n{\n\tfrogRiding = {}; // what it rides, worked out again this frame\n\tupdatePads();") != std::string::npos);
	CHECK(main.find("\t\tif (physics::touching(frog, other))\n\t\t{\n\t\t\tfrogRiding = logrow6Velocity;\n\t\t}") != std::string::npos);
	// and moves it after them, for the frame it is touching, as in the engine
	CHECK(main.find("\t// it rides along: moved as well by what it touches, this frame\n\tfrog.move(frogRiding);\n}") != std::string::npos);
	CHECK(main.find("\tif (physics::touching(frog, water) && !touchingLogs(frog, &water))\n") != std::string::npos);
	// a reset gets off
	CHECK(main.find("void startFrog()\n{\n\tfrog.setPosition({6.0f * cell + inset, 13.0f * cell + inset});\n\tfrogRiding = {};\n}") != std::string::npos);
}

TEST_CASE("generating Depth Charge fires a charge from the ship, one at a time, and it dies where it hits", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_depthcharge");
	generateGame(requestFor(fs::current_path() / "games/depthcharge.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	// a projectile is out of play until it is fired
	CHECK(main.find("bool chargeAlive = false; // in play from when it is fired until it dies") != std::string::npos);
	CHECK(main.find("\tchargeAlive = false; // until it is fired") != std::string::npos);
	// fired from the middle of the ship's top, at its own velocity, only when it is not out already
	CHECK(main.find("\t\t\t// fire charge, if it is not out already\n\t\t\tif (!chargeAlive)\n\t\t\t{\n\t\t\t\tcharge.setPosition({physics::left(ship) + physics::width(ship) / 2.0f - physics::width(charge) / 2.0f, physics::top(ship)});\n\t\t\t\tchargeVelocity = {0.0f, 5.0f};\n\t\t\t\tchargeAlive = true;") != std::string::npos);
	// a sub it hits sinks by the sub's own rule, in the same touch
	CHECK(main.find("\t\tif (subs1Alive[j] && physics::touching(charge, subs1[j]))\n\t\t{\n\t\t\t// subs1, by its own rule: die\n\t\t\tsubs1Alive[j] = false;\n\t\t\tshipSunk += 1.0f;") != std::string::npos);
	CHECK(main.find("\t// bottom: dec die\n\tif (physics::past(charge, physics::Edge::Bottom, windowArea))\n\t{\n\t\tshipCharges -= 1.0f;") != std::string::npos);
}

TEST_CASE("generating Astrosmash puts one rock back at a time, its fall drawn anew", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_astrosmash");
	generateGame(requestFor(fs::current_path() / "games/astrosmash.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	// each rock falls at a speed of its own, drawn for it, as in the engine
	CHECK(main.find("std::vector<sf::Vector2f> bouldersVelocity(4);") != std::string::npos);
	// one member starts again on its own: what they share written once, the rest a list
	CHECK(main.find("void startBoulders(std::size_t i)\n{\n\tconst float xs[] = {50.0f, 230.0f, 410.0f, 590.0f};\n\tboulders[i].setPosition({xs[i], randomBetween(-500.0f, -40.0f)});\n\tbouldersVelocity[i] = {0.0f, randomBetween(1.0f, 2.0f)};\n}") != std::string::npos);
	CHECK(main.find("\tfor (std::size_t i = 0; i < boulders.size(); ++i)\n\t{\n\t\tstartBoulders(i);\n\t}") != std::string::npos);
	CHECK(main.find("\t\t// bottom: dec reset\n\t\tif (physics::past(boulders[i], physics::Edge::Bottom, windowArea))\n\t\t{\n\t\t\tshipLives -= 1.0f;\n\t\t\tshowLives();\n\t\t\tstartBoulders(i);") != std::string::npos);
}

TEST_CASE("generating Kaboom counts the bomber's timers down, and a missed bomb puts every bomb back", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_kaboom");
	generateGame(requestFor(fs::current_path() / "games/kaboom.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	// a timer is the frames until it goes off, worked out again (a <random> drawn anew) when it gets there
	CHECK(main.find("int bomber1Timer1 = 0; // the frames until its timer goes off") != std::string::npos);
	CHECK(main.find("\t// timer 1: reverse\n\tif (bomber1Timer1 == 0)\n\t{\n\t\tbomber1Timer1 = framesFor(randomBetween(0.3f, 1.4f));\n\t}\n\tif (--bomber1Timer1 == 0)\n\t{\n\t\tbomber1Velocity = -bomber1Velocity;\n\t}") != std::string::npos);
	CHECK(main.find("int framesFor(float seconds)") != std::string::npos);
	// the timers go off before anything moves, on the screens that show the bomber
	CHECK(main.find("\t// the timers first, before anything moves\n\ttickBomber1();\n\n\tupdateBomber1();") != std::string::npos);
	// he faces down: a bomb leaves from under him, at its own speed
	CHECK(main.find("physics::fireFrom(bomber1, bombs1[i], bombs1Velocity[i], physics::Facing::Down, sf::Vector2f{0.0f, fall1}.length());") != std::string::npos);
	// the bombs are hidden until he drops one, and <reset object> puts every one back
	CHECK(main.find("\tbombs1Alive.assign(bombs1.size(), false); // hidden until they are brought in") != std::string::npos);
	CHECK(main.find("\t\t// bottom: dec play reset\n\t\tif (physics::past(bombs1[i], physics::Edge::Bottom, windowArea))\n\t\t{\n\t\t\tbucketBuckets -= 1.0f;\n\t\t\tshowBucketsvalue();\n\t\t\tboomSound.play();\n\t\t\tresetBombs1();") != std::string::npos);
	CHECK(main.find("void resetBombs1()\n{") != std::string::npos);
	// the waves share one set of keys
	CHECK(main.find("\tcase Screen::Wave3:\n\t\tif (key == sf::Keyboard::Key::Space || key == sf::Keyboard::Key::P)") != std::string::npos);
}

TEST_CASE("generating Demon Attack gives each demon a timer of its own, firing from the pool", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_demonattack");
	generateGame(requestFor(fs::current_path() / "games/demonattack.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	CHECK(main.find("std::vector<int> aliensTimer1; // for each of them, the frames until its timer goes off") != std::string::npos);
	CHECK(main.find("\t\tif (--aliensTimer1[i] == 0)\n\t\t{\n\t\t\t// fire the first of demonshots that is not out already\n\t\t\tfor (std::size_t k = 0; k < demonshots.size(); ++k)") != std::string::npos);
	// each demon bounces off the sides on its own
	CHECK(main.find("physics::bounce(aliens[i], aliensVelocity[i], physics::Edge::Left, windowArea);") != std::string::npos);
	// the cannon, moved by keys alone, wraps round once it is right off
	CHECK(main.find("\tphysics::wrap(player, physics::Edge::Left, windowArea);") != std::string::npos);
	// none left: all of them back, their timers too
	CHECK(main.find("void resetAliens()") != std::string::npos);
	CHECK(main.find("\taliensTimer1.assign(aliens.size(), 0);") != std::string::npos);
}

TEST_CASE("generating Megamania keeps the energy no screen shows, run down by a timer of each wave", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_megamania");
	generateGame(requestFor(fs::current_path() / "games/megamania.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	CHECK(main.find("// energy: never shown, its variables kept\nfloat energyLevel = 0.0f;") != std::string::npos);
	CHECK(main.find("int wave1Timer1 = 0;") != std::string::npos);
	CHECK(main.find("\t// this screen's timer 1: dec\n\tif (wave1Timer1 == 0)\n\t{\n\t\twave1Timer1 = framesFor(1.0f);\n\t}\n\tif (--wave1Timer1 == 0)\n\t{\n\t\tenergyLevel -= 1.0f;\n\t\tshowEnergybar();\n\t}") != std::string::npos);
	CHECK(main.find("void resetEnergy()\n{\n\tenergyLevel = 45.0f;\n\tshowEnergybar();\n}") != std::string::npos);
	// the laser, shown on every wave, only touches the wave showing
	CHECK(main.find("\tif (screens.back() == Screen::Wave2)\n\t{\n\t\t// cookies: die") != std::string::npos);
}

TEST_CASE("generating Asteroids turns the ship's picture with its heading, and a broken rock releases two smaller ones", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_asteroids");
	generateGame(requestFor(fs::current_path() / "games/asteroids.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	// a key held turns it, and thrust pushes it the way it faces; either key of an action, once a frame
	CHECK(main.find("\tif (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))\n\t{\n\t\tshipHeading -= turnrate;\n\t\tturnShip();\n\t}") != std::string::npos);
	CHECK(main.find("\t\tshipVelocity += physics::ahead(shipHeading) * thrustpower;") != std::string::npos);
	// its picture is drawn again when the whole degree changes, and that is what its touches test
	CHECK(main.find("\t\tconst sf::Image picture = pictures::turnedLines(shipLines, static_cast<float>(degrees));\n\t\tshipPicture.update(picture);\n\t\tshipPixels = picture;") != std::string::npos);
	CHECK(main.find("\tshipVelocity *= 1.0f - drag;\n\tship.move(shipVelocity);") != std::string::npos);
	CHECK(main.find("physics::fireAhead(ship, shots[i], shotsVelocity[i], shipHeading, sf::Vector2f{0.0f, -shotspeed}.length());") != std::string::npos);
	// rocks of several looks find their pixels by their picture; a touch is swept along the step
	CHECK(main.find("physics::touchingPixels(shots[i], nullptr, bigrocks[j], &pixelsOf(bigrocks[j].getTexture()), shotsVelocity[i] - bigrocksVelocity[j])") != std::string::npos);
	CHECK(main.find("const sf::Image& pixelsOf(const sf::Texture& picture)") != std::string::npos);
	CHECK(main.find("for (std::size_t k = 0, released = 0; k < mediumrocks.size() && released < 2; ++k)") != std::string::npos);
	CHECK(main.find("mediumrocks[k].setPosition(bigrocks[j].getGlobalBounds().getCenter() - mediumrocks[k].getGlobalBounds().size / 2.0f);") != std::string::npos);
	CHECK(main.find("\tphysics::wrap(ship, shipVelocity, physics::Edge::Left, windowArea);") != std::string::npos);
}

TEST_CASE("generating Combat drives each tank along its heading, a turned picture of rows", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_combat");
	generateGame(requestFor(fs::current_path() / "games/combat.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	CHECK(main.find("\t\ttank1Velocity += physics::ahead(tank1Heading) * back;") != std::string::npos);
	CHECK(main.find("pictures::turned(pictures::rows(tank1Rows, 3, sf::Color::Yellow), static_cast<float>(degrees))") != std::string::npos);
	CHECK(main.find("\tif (shell2Alive && physics::touchingPixels(tank1, &tank1Pixels, shell2, nullptr, tank1Velocity - shell2Velocity))") != std::string::npos);
	CHECK(main.find("\t\tif (physics::touchingPixels(tank1, &tank1Pixels, other, nullptr, tank1Velocity))\n\t\t{\n\t\t\tphysics::bounceOff(tank1, tank1Velocity, other);") != std::string::npos);
}

TEST_CASE("generating Lunar Lander pulls the lander down, burns fuel while a thruster is held, and lands by speed", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_lunarlander");
	generateGame(requestFor(fs::current_path() / "games/lunarlander.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	CHECK(main.find("\tlanderAcceleration = {0.0f, gravity};") != std::string::npos);
	CHECK(main.find("\tlanderVelocity += landerAcceleration;\n\tlander.move(landerVelocity);") != std::string::npos);
	CHECK(main.find("\tif (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))\n\t{\n\t\t// burning fuel, while there is any\n\t\tif (landerFuel > 0.0f)\n\t\t{\n\t\t\tlanderFuel -= 1.0f;\n\t\t\tshowFuelvalue();\n\t\t\tlanderVelocity.y -= thrust;\n\t\t}\n\t}") != std::string::npos);
	// its speed is taken once, before the first rule about the pad, as the engine takes it
	CHECK(main.find("\tconst float landerSpeedAtPad = landerVelocity.length();") != std::string::npos);
	CHECK(main.find("physics::touchingPixels(lander, &landerPixels, pad, &padPixels, landerVelocity) && landerSpeedAtPad < safespeed)") != std::string::npos);
	CHECK(main.find("\t\tlanderVelocity = {};\n\t\tlanderAcceleration = {}; // and its pull, until it is reset") != std::string::npos);
}

TEST_CASE("generating Frostbite jumps Bailey a row at a time, touching nothing in the air", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_frostbite");
	generateGame(requestFor(fs::current_path() / "games/frostbite.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	// a key starts a jump, if none is under way and it lands on the screen
	CHECK(main.find("\t\t\tphysics::jump(bailey, {0.0f, -rowgap}, framesFor(leap), baileyJumpStep, baileyJumpFrames, windowArea);") != std::string::npos);
	CHECK(main.find("\tphysics::jumping(bailey, baileyJumpStep, baileyJumpFrames, windowArea);") != std::string::npos);
	// in the air it touches nothing
	CHECK(main.find("if (baileyJumpFrames == 0 && physics::touching(row1[i], bailey))") != std::string::npos);
	// a game variable named like a group keeps a name of its own
	CHECK(main.find("const float row1_ = 172.0f; // named like the group row1") != std::string::npos);
	CHECK(main.find("\tbaileyJumpFrames = 0;") != std::string::npos);
}

TEST_CASE("generating Air-Sea Battle swings each gun with keys, and fires along its heading", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_airseabattle");
	generateGame(requestFor(fs::current_path() / "games/airseabattle.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	CHECK(main.find("physics::fireAhead(gun1, shots1[i], shots1Velocity[i], gun1Heading, sf::Vector2f{0.0f, -shellspeed}.length());") != std::string::npos);
	CHECK(main.find("\t\tgun1Heading -= swing;") != std::string::npos);
	// a name C++ or its library has already is kept apart
	CHECK(main.find("sf::Text clock_(font);") != std::string::npos);
	CHECK(main.find("const float sea_ = 420.0f; // named like the object sea") != std::string::npos);
}

TEST_CASE("generating Donkey Kong walks Jumpman by the keys, lands him on girders, leaps and climbs", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_donkeykong");
	generateGame(requestFor(fs::current_path() / "games/donkeykong.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	// the keys held give the way across, kept in a leap
	CHECK(main.find("\tmanWalk = 0.0f; // and the keys held below\n\tmanClimb = 0.0f;") != std::string::npos);
	CHECK(main.find("\t\tmanWalk -= walk;") != std::string::npos);
	CHECK(main.find("\tif (!manLeaping)\n\t{\n\t\tmanVelocity.x = manWalk;\n\t}") != std::string::npos);
	// on a ladder nothing pulls him; what he stands on is found again every frame
	CHECK(main.find("\tphysics::climb(man, manVelocity, manClimb, manClimbing, manGrounded, ladderAt(man));\n\tif (!manClimbing)\n\t{\n\t\tmanVelocity += manAcceleration; // on a ladder nothing pulls it\n\t}\n\tmanGrounded = false; // until it lands again, this frame\n\tman.move(manVelocity);") != std::string::npos);
	CHECK(main.find("\t\t\tif (!manClimbing && physics::land(man, manVelocity, other))\n\t\t\t{\n\t\t\t\tmanGrounded = true;\n\t\t\t\tmanLeaping = false;") != std::string::npos);
	CHECK(main.find("\t\t\tif (manGrounded && !manClimbing)\n\t\t\t{\n\t\t\t\tmanVelocity.y = -std::sqrt(2.0f * manAcceleration.y * leapheight);") != std::string::npos);
	CHECK(main.find("std::optional<sf::FloatRect> ladderAt(const Shape& one)\n{\n\tfor (const sf::RectangleShape& each : ladders)") != std::string::npos);
	// the barrels share one pull, each with a velocity of its own, and land too
	CHECK(main.find("\t\tbarrelsVelocity[i] += barrelsAcceleration;") != std::string::npos);
	CHECK(main.find("\t\t\t\tphysics::land(barrels[i], barrelsVelocity[i], other);") != std::string::npos);
}

TEST_CASE("generating Pitfall! makes each scorpion an object of its screen, and a ladder only where it is shown", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_pitfall");
	generateGame(requestFor(fs::current_path() / "games/pitfall.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	// a member shown by its own name on a screen of its own is an object
	CHECK(main.find("sf::Sprite scorpion2(") != std::string::npos);
	CHECK(main.find("std::vector<sf::Sprite> scorpions") == std::string::npos);
	CHECK(main.find("\tif (screens.back() == Screen::Screen4 && physics::atLadder(one, ladder))") != std::string::npos);
	// running off a side moves him round to the other
	CHECK(main.find("\t\tstatusScreen += 1.0f;\n\t\tharry.move({-(windowRight - 2.0f * physics::width(harry)), 0.0f});") != std::string::npos);
}

TEST_CASE("generating Missile Command aims the base at the sight, and each missile chases the nearest city", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_missilecommand");
	generateGame(requestFor(fs::current_path() / "games/missilecommand.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	// an aim is kept, and the shots go along it
	CHECK(main.find("\t\tif (const std::optional<sf::Vector2f> way = physics::aimAt(base, nearestSight()))\n\t\t{\n\t\t\tbaseAim = *way;\n\t\t\tbaseAimed = true;") != std::string::npos);
	CHECK(main.find("if (baseAimed)\n\t\t\t\t\t{\n\t\t\t\t\t\tphysics::fireAlong(base, abms[i], abmsVelocity[i], baseAim, sf::Vector2f{0.0f, -abmspeed}.length());") != std::string::npos);
	// each missile looks for the nearest city still standing
	CHECK(main.find("physics::chase(missiles1[i], missiles1Velocity[i], nearestCities(missiles1[i]), speed1, 0.0f);") != std::string::npos);
	CHECK(main.find("std::optional<sf::Vector2f> nearestCities(const Shape& one)\n{\n\tconst sf::Vector2f here = one.getGlobalBounds().getCenter();") != std::string::npos);
	CHECK(main.find("\t\tif (!citiesAlive[i])\n\t\t{\n\t\t\tcontinue;") != std::string::npos);
}

TEST_CASE("generating Berserk turns the man with the keys he walks by, and fires the way he faces", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_berserk");
	generateGame(requestFor(fs::current_path() / "games/berserk.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	// a key that walks him, pressed, turns him and his look; held, it walks him
	CHECK(main.find("physics::Facing playerFacing = physics::Facing::Right;") != std::string::npos);
	CHECK(main.find("\t\tif (key == sf::Keyboard::Key::Up || key == sf::Keyboard::Key::W)\n\t\t{\n\t\t\tplayerFacing = physics::Facing::Up;\n\t\t\tbecomePlayer(PlayerLook::Up);") != std::string::npos);
	CHECK(main.find("physics::fireFrom(player, bullet, bulletVelocity, playerFacing, sf::Vector2f{0.0f, -8.0f}.length());") != std::string::npos);
	// a robot faces the way it aims; Otto chases the man
	CHECK(main.find("\t\t\t\trobots1leftAimed[i] = true;\n\t\t\t\trobots1leftFacing[i] = physics::facingOf(*way);") != std::string::npos);
	CHECK(main.find("physics::chase(otto, ottoVelocity, nearestPlayer(), 1.1f, 0.0f);") != std::string::npos);
}

TEST_CASE("generating Galaxian flies the fleet in on its paths, one behind another, and each alien on its dive", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_galaxian");
	generateGame(requestFor(fs::current_path() / "games/galaxian.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	// the paths are a table, a leg of a step or home
	CHECK(main.find("enum class Path { Fromtop, Fromtopleft, Fromtopright, Fromleft, Fromright, Dive };") != std::string::npos);
	CHECK(main.find("\t{divespeed, std::nullopt, {\n\t\t{{-24.0f, -24.0f}}, // and play") != std::string::npos);
	CHECK(main.find("\t\t{{0.0f, windowBottom + alienheight - 496.0f}},\n\t\t{{}, true}}}\n};") != std::string::npos);
	// a follower keeps its flight and its home, and flies before anything moves
	CHECK(main.find("std::vector<physics::Flight<Path>> flagshipsFlight(2);") != std::string::npos);
	CHECK(main.find("\t// the paths, a frame on, before anything moves\n\tflyFlagships();") != std::string::npos);
	CHECK(main.find("physics::fly(flagships[i], flagshipsVelocity[i], flagshipsFlight[i], flagshipsHome[i], routes, [&](Path path, std::size_t leg)") != std::string::npos);
	CHECK(main.find("\t\t\t\telse if (path == Path::Dive && leg == 2)\n\t\t\t\t{\n\t\t\t\t\t// fire the first of bombs") != std::string::npos);
	// sent off one every stagger, or its own dive
	CHECK(main.find("physics::follow(flagships[i], flagshipsVelocity[i], flagshipsFlight[i], Path::Fromtop, static_cast<int>(std::lround(static_cast<float>(setOff) * spacing * static_cast<float>(framerate))), routes);") != std::string::npos);
	CHECK(main.find("physics::follow(escorts[i], escortsVelocity[i], escortsFlight[i], Path::Dive, 0, routes);") != std::string::npos);
}

TEST_CASE("generating Space Invaders animates the block, each row its own pictures, through one become", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_spaceinvaders");
	generateGame(requestFor(fs::current_path() / "games/spaceinvaders.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	// a look is the group's picture, or the rows' that change it
	CHECK(main.find("\tcase AliensLook::B:\n\t\tif (i >= 11 && i <= 32) // aliens, rows 2, 3\n\t\t{\n\t\t\taliens[i].setTexture(aliensRow23BPicture, true);\n\t\t}\n\t\telse if (i >= 33) // aliens, rows 4, 5") != std::string::npos);
	CHECK(main.find("\t\telse\n\t\t{\n\t\t\taliens[i].setTexture(aliensBPicture, true);") != std::string::npos);
	// the animation goes through the looks, each alien in play on its own count
	CHECK(main.find("const std::vector<AliensLook> aliensFrames = {AliensLook::A, AliensLook::B};") != std::string::npos);
	CHECK(main.find("\t\tif (++aliensShown[i] < framesFor(animationSeconds))\n\t\t{\n\t\t\tcontinue;\n\t\t}\n\t\taliensShown[i] = 0;\n\t\taliensFrame[i] = (aliensFrame[i] + 1) % aliensFrames.size();\n\t\tbecomeAliens(i, aliensFrames[aliensFrame[i]]);") != std::string::npos);
	CHECK(main.find("\t// the animations a frame on\n\tanimateAliens();\n\n\t// the timers first") != std::string::npos);
	CHECK(main.find("\taliensFrame.assign(aliens.size(), 0);\n\taliensShown.assign(aliens.size(), 0);") != std::string::npos);
}

TEST_CASE("a group in lockstep moves as one block, and turns as one off a side", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_lockstep");
	fs::create_directories(folder.path);
	std::ofstream(folder.path / "block.xml") <<
		"<game>\n"
		"  <window name=\"Block\"><width>320</width><height>240</height><background>color.black</background><fullscreen>false</fullscreen><framerate>60</framerate></window>\n"
		"  <variables />\n"
		"  <objects>\n"
		"    <group name=\"aliens\" class=\"aliens\">\n"
		"      <columns>6</columns><rows>2</rows><padding><x>4</x><y>4</y></padding>\n"
		"      <sprite><circle><radius>5</radius><color>color.green</color></circle></sprite>\n"
		"      <position><x>20</x><y>20</y></position>\n"
		"      <velocity><x>2</x><y>0</y></velocity>\n"
		"      <collisions><enabled>true</enabled><lockstep>true</lockstep><collision edge=\"horizontal\"><bounce /></collision><collision object=\"shot\"><die /></collision></collisions>\n"
		"      <row number=\"even\"><sprite><circle><color>color.red</color></circle></sprite></row>\n"
		"    </group>\n"
		"    <object name=\"shot\">\n"
		"      <sprite><rectangle><width>2</width><height>6</height></rectangle></sprite>\n"
		"      <position><x>40</x><y>200</y></position>\n"
		"      <velocity><x>0</x><y>-4</y></velocity>\n"
		"      <collisions><enabled>true</enabled><collision class=\"aliens\"><die /></collision><collision edge=\"top\"><die /></collision></collisions>\n"
		"    </object>\n"
		"  </objects>\n"
		"  <states><state name=\"playing\"><shows><show object=\"aliens\" /><show object=\"shot\" /></shows>\n"
		"    <conditions><condition class=\"aliens\"><remaining>11</remaining><reset /></condition></conditions></state></states>\n"
		"</game>\n";
	generateGame(requestFor(folder.path / "block.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	// 6 columns by 2 rows, the top row first as the engine lays them out; every even row red
	CHECK(main.find("std::vector<sf::CircleShape> aliens(12);") != std::string::npos);
	CHECK(main.find("\t\t\taliens[row * 6 + column].setPosition({20.0f + static_cast<float>(column) * (2.0f * 5.0f + 4.0f), 20.0f + static_cast<float>(row) * (2.0f * 5.0f + 4.0f)});") != std::string::npos);
	CHECK(main.find("\t// aliens, every even row\n\tfor (std::size_t i = 6; i < 12; ++i)\n\t{\n\t\taliens[i].setFillColor(sf::Color::Red);\n\t}") != std::string::npos);
	// all of the block moved first, then its rules
	CHECK(main.find("void updateAliens()\n{\n\tfor (sf::CircleShape& one : aliens)\n\t{\n\t\tone.move(aliensVelocity);\n\t}\n\n\tfor (std::size_t i = 0; i < aliens.size(); ++i)") != std::string::npos);
	CHECK(main.find("\t\t\taliensVelocity.x = -aliensVelocity.x;\n\t\t\tfor (sf::CircleShape& each : aliens)\n\t\t\t{\n\t\t\t\teach.move({aliensVelocity.x, 0.0f});") != std::string::npos);
	CHECK(main.find("\t// aliens: no more than 11 left\n\tif (std::count(aliensAlive.begin(), aliensAlive.end(), true) <= 11)\n\t{\n\t\tstart();") != std::string::npos);

	// a row's variables, and looks of a row's own in a group of one look, are not written yet
	const auto refused = [&](const std::string& from, const std::string& to)
	{
		std::string xml = readFile(folder.path / "block.xml");
		xml.replace(xml.find(from), from.size(), to);
		std::ofstream(folder.path / "changed.xml") << xml;
		try
		{
			generateGame(requestFor(folder.path / "changed.xml", folder.path / "out2"));
		}
		catch (const GenerateError& error)
		{
			return std::string(error.what());
		}
		return std::string("generated it");
	};
	CHECK(refused("<row number=\"even\"><sprite><circle><color>color.red</color></circle></sprite></row>", "<row number=\"even\"><variables><variable name=\"points\">2</variable></variables></row>").find("cannot generate <variables> of a <row> yet") != std::string::npos);
	CHECK(refused("<circle><color>color.red</color></circle></sprite>", "<circle><color>color.red</color></circle></sprite><sprite name=\"hit\"><circle><radius>2</radius></circle></sprite>").find("cannot generate a <member>, <row>, <column> or <cell> with several sprites in a group of one yet") != std::string::npos);
}

TEST_CASE("a group's rows, columns and cells change what they pick, and each cell is where the engine puts it", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_cells");
	fs::create_directories(folder.path);
	std::ofstream(folder.path / "cells.xml") <<
		"<game>\n"
		"  <window name=\"Cells\"><width>400</width><height>300</height><background>color.black</background><fullscreen>false</fullscreen><framerate>60</framerate></window>\n"
		"  <variables><variable name=\"gap\">3</variable></variables>\n"
		"  <objects>\n"
		"    <group name=\"dots\">\n"
		"      <columns>5</columns><rows>4</rows><padding><x>gap</x><y>2</y></padding>\n"
		"      <sprite><circle><radius>5</radius><color>color.green</color></circle></sprite>\n"
		"      <position><x>10</x><y>20</y></position>\n"
		"      <velocity><x>1</x><y>0</y></velocity>\n"
		"      <collisions><enabled>false</enabled></collisions>\n"
		"      <row number=\"2\"><sprite><circle><radius>7</radius></circle></sprite><padding><y>6</y></padding></row>\n"
		"      <row number=\"4\"><velocity><x>-1</x></velocity></row>\n"
		"      <column number=\"odd\"><velocity><y>0.5</y></velocity></column>\n"
		"      <column number=\"3\"><padding><x>10</x></padding></column>\n"
		"      <cell row=\"3\" column=\"4\"><sprite><circle><radius>3</radius><color>color.yellow</color></circle></sprite><velocity><x>0</x></velocity></cell>\n"
		"    </group>\n"
		"    <group name=\"bars\">\n"
		"      <columns>4</columns><rows>3</rows><padding><x>2</x><y>4</y></padding>\n"
		"      <sprite><rectangle><width>20</width><height>10</height></rectangle></sprite>\n"
		"      <position><x>10</x><y>150</y></position>\n"
		"      <velocity><x>0</x><y>0</y></velocity>\n"
		"      <collisions><enabled>false</enabled></collisions>\n"
		"      <column number=\"even\"><sprite><rectangle><color>color.red</color></rectangle></sprite></column>\n"
		"    </group>\n"
		"    <group name=\"invaders\">\n"
		"      <columns>3</columns><rows>2</rows><padding><x>4</x><y>4</y></padding>\n"
		"      <sprite><bitmap><row>.**.</row><row>****</row><row>*..*</row><scale>2</scale><color>color.green</color></bitmap></sprite>\n"
		"      <position><x>200</x><y>20</y></position>\n"
		"      <velocity><x>0</x><y>0</y></velocity>\n"
		"      <collisions><enabled>false</enabled></collisions>\n"
		"      <row number=\"2\"><sprite><bitmap><row>*....*</row><row>.****.</row></bitmap></sprite></row>\n"
		"      <cell row=\"1\" column=\"3\"><sprite><bitmap><color>color.red</color></bitmap></sprite></cell>\n"
		"    </group>\n"
		"    <group name=\"birds\">\n"
		"      <sprite><circle><radius>4</radius><color>color.yellow</color></circle></sprite>\n"
		"      <position><y>250</y></position>\n"
		"      <velocity><x>0</x><y>0</y></velocity>\n"
		"      <collisions><enabled>false</enabled></collisions>\n"
		"      <member><position><x>200</x></position></member>\n"
		"      <member><sprite><circle><radius>6</radius></circle></sprite><position><x>230</x></position></member>\n"
		"    </group>\n"
		"  </objects>\n"
		"  <states><state name=\"playing\"><shows><show object=\"dots\" /><show object=\"bars\" /><show object=\"invaders\" /><show object=\"birds\" /></shows>\n"
		"    <inputs><input button=\"space\"><reset /></input></inputs></state></states>\n"
		"</game>\n";
	generateGame(requestFor(folder.path / "cells.xml", folder.path / "out"));
	const std::string main = readFile(folder.path / "out/main.cpp");

	// the group's look on every one, then only what a row or cell changes
	CHECK(main.find("\tfor (sf::CircleShape& one : dots)\n\t{\n\t\tone.setRadius(5.0f);\n\t\tone.setFillColor(sf::Color::Green);\n\t}") != std::string::npos);
	CHECK(main.find("\t// dots, row 2\n\tfor (std::size_t i = 5; i < 10; ++i)\n\t{\n\t\tdots[i].setRadius(7.0f);\n\t}") != std::string::npos);
	CHECK(main.find("\t// dots, column 4, row 3\n\tdots[13].setRadius(3.0f);\n\tdots[13].setFillColor(sf::Color::Yellow);") != std::string::npos);
	// whole columns in a loop over every row
	CHECK(main.find("\t// bars, every even column\n\tfor (std::size_t row = 0; row < 3; ++row)\n\t{\n\t\tfor (std::size_t column : {1u, 3u})\n\t\t{\n\t\t\tbars[row * 4 + column].setFillColor(sf::Color::Red);") != std::string::npos);
	// a member's sprite changes only what it gives
	CHECK(main.find("\t// birds.2, a look of its own\n\tbirds[1].setRadius(6.0f);\n") != std::string::npos);

	// rows of other sizes, one at a time; a column's gap before it, a cell at a time
	CHECK(main.find("\t// dots: 5 columns by 4 rows, each row as big as its look\n\tfloat dotsTop = 20.0f;\n\t// row 1\n\tfloat dotsLeft = 10.0f;\n\tdots[0].setPosition({dotsLeft, dotsTop});\n"
		"\tdotsLeft += 2.0f * 5.0f + gap;\n\tdots[1].setPosition({dotsLeft, dotsTop});\n\tdotsLeft += 2.0f * 5.0f + 10.0f;\n\tdots[2].setPosition({dotsLeft, dotsTop});") != std::string::npos);
	CHECK(main.find("\tdotsTop += 2.0f * 5.0f + 6.0f;\n\t// row 2\n\tdotsLeft = 10.0f;\n") != std::string::npos);
	// a smaller cell in the middle of its place
	CHECK(main.find("\t// dots, column 4, row 3: in the middle of its place\n\tdots[13].move({(2.0f * 5.0f - 2.0f * 3.0f) / 2.0f, (2.0f * 5.0f - 2.0f * 3.0f) / 2.0f});") != std::string::npos);
	// rows alike: two loops, as before
	CHECK(main.find("\t\t\tbars[row * 4 + column].setPosition({10.0f + static_cast<float>(column) * (20.0f + 2.0f), 150.0f + static_cast<float>(row) * (10.0f + 4.0f)});") != std::string::npos);

	// a velocity each: the group's, then each other on the cells that have it
	CHECK(main.find("std::vector<sf::Vector2f> dotsVelocity;") != std::string::npos);
	CHECK(main.find("\tdotsVelocity.assign(20, {1.0f, 0.0f});\n\t// dots, every odd column\n\tfor (std::size_t i : {0u, 2u, 4u, 5u, 7u, 9u, 10u, 12u, 14u})\n\t{\n\t\tdotsVelocity[i] = {1.0f, 0.5f};") != std::string::npos);
	CHECK(main.find("\t// dots, row 4, every odd column\n\tfor (std::size_t i : {15u, 17u, 19u})\n\t{\n\t\tdotsVelocity[i] = {-1.0f, 0.5f};") != std::string::npos);

	// a picture for each look; a row's new rows, and a cell's color on the group's rows
	CHECK(main.find("!invadersColumn3Row1Picture.loadFromImage(pictures::rows(invadersRows, 2, sf::Color::Red))") != std::string::npos);
	CHECK(main.find("!invadersRow2Picture.loadFromImage(pictures::rows(invadersRow2Rows, 2, sf::Color::Green))") != std::string::npos);
	CHECK(main.find("invadersColumn3Row1Rows") == std::string::npos);
	CHECK(main.find("\tinvadersTop += static_cast<float>(invadersPicture.getSize().y) + 4.0f;") != std::string::npos);
}

TEST_CASE("a sprite of rows, of lines or from an SVG is a picture the program has, drawn as the engine draws it", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_pictures");
	fs::create_directories(folder.path);
	std::ofstream(folder.path / "gallery.xml") <<
		"<game>\n"
		"  <window name=\"Gallery\"><width>320</width><height>240</height><background>color.black</background><fullscreen>false</fullscreen><framerate>60</framerate></window>\n"
		"  <variables><variable name=\"zoom\">2</variable></variables>\n"
		"  <objects>\n"
		"    <object name=\"cannon\">\n"
		"      <sprite><svg><path>assets/Space Invaders Color Sprites.svg</path><x>4</x><y>3.5</y><width>24</width><height>26</height><scale>zoom</scale><hide>backdrop</hide><flip>vertical</flip></svg></sprite>\n"
		"      <position><x>140</x><y>4</y></position>\n"
		"      <velocity><x>0</x><y>0</y></velocity>\n"
		"      <collisions><enabled>false</enabled></collisions>\n"
		"    </object>\n"
		"    <group name=\"aliens\">\n"
		"      <columns>4</columns><rows>2</rows><padding><x>6</x><y>6</y></padding>\n"
		"      <sprite><bitmap><row>..*..*..</row><row>.******.</row><row>**.**.**</row><row>********</row><row>.*....*.</row><scale>2</scale><color>color.green</color></bitmap></sprite>\n"
		"      <position><x>20</x><y>70</y></position>\n"
		"      <velocity><x>1</x><y>0</y></velocity>\n"
		"      <collisions><enabled>true</enabled><lockstep>true</lockstep><collision edge=\"horizontal\"><bounce /></collision></collisions>\n"
		"    </group>\n"
		"    <object name=\"player\">\n"
		"      <sprite><bitmap><row>...*...</row><row>.*****.</row><row>*******</row><scale>3</scale><color>color.cyan</color></bitmap></sprite>\n"
		"      <position><x>150</x><y>170</y></position>\n"
		"      <velocity><x>0</x><y>0</y></velocity>\n"
		"      <collisions><enabled>true</enabled><collision edge=\"horizontal\"><stick /></collision></collisions>\n"
		"      <actions><action name=\"left\"><move direction=\"left\">3</move></action><action name=\"right\"><move direction=\"right\">3</move></action></actions>\n"
		"    </object>\n"
		"    <object name=\"ground\">\n"
		"      <sprite>\n"
		"        <line><from><x>0</x><y>20</y></from><to><x>80</x><y>4</y></to><color>color.lightgrey</color><thickness>2</thickness></line>\n"
		"        <line><from><x>80</x><y>4</y></from><to><x>200</x><y>30</y></to><color>color.lightgrey</color><thickness>2</thickness></line>\n"
		"        <line><from><x>200</x><y>30</y></from><to><x>318</x><y>10</y></to></line>\n"
		"      </sprite>\n"
		"      <position><x>0</x><y>200</y></position>\n"
		"      <velocity><x>0</x><y>0</y></velocity>\n"
		"      <collisions><enabled>false</enabled></collisions>\n"
		"    </object>\n"
		"  </objects>\n"
		"  <states><state name=\"playing\"><shows><show object=\"cannon\" /><show object=\"aliens\" /><show object=\"player\" /><show object=\"ground\" /></shows>\n"
		"    <inputs><input button=\"left\"><trigger object=\"player\" action=\"left\" /></input><input button=\"right\"><trigger object=\"player\" action=\"right\" /></input></inputs></state></states>\n"
		"</game>\n";
	const GeneratedProgram program = generateGame(requestFor(folder.path / "gallery.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	CHECK(main.find("#include \"pictures.h\"") != std::string::npos);
	CHECK(fs::exists(folder.path / "out/pictures.h"));
	// a bitmap's rows written out one under another, so it looks like itself
	CHECK(main.find("const std::vector<std::string> playerRows = {\n\t\"...*...\",\n\t\".*****.\",\n\t\"*******\"\n};\nsf::Texture playerPicture;") != std::string::npos);
	CHECK(main.find("!playerPicture.loadFromImage(pictures::rows(playerRows, 3, sf::Color::Cyan))") != std::string::npos);
	// lines, white and 1 thick when they do not say
	CHECK(main.find("\t{{200.0f, 30.0f}, {318.0f, 10.0f}, sf::Color::White, 1}\n};") != std::string::npos);
	CHECK(main.find("!groundPicture.loadFromImage(pictures::lines(groundLines))") != std::string::npos);
	// a group of cells of a bitmap: the picture's size from one cell to the next
	CHECK(main.find("std::vector<sf::Sprite> aliens(8, sf::Sprite(aliensPicture));") != std::string::npos);
	CHECK(main.find("static_cast<float>(column) * (static_cast<float>(aliensPicture.getSize().x) + 6.0f)") != std::string::npos);
	// an svg drawn by xgecli into a picture, flipped as an image is
	CHECK(main.find("!cannonPicture.loadFromFile(\"assets/drawn/cannon.png\")") != std::string::npos);
	CHECK(main.find("cannon.setTextureRect({{0, static_cast<int>(cannonPicture.getSize().y)}") != std::string::npos);
	CHECK(main.find("const float zoom") == std::string::npos); // only the svg used it
	REQUIRE(program.drawn.size() == 1);
	CHECK(program.drawn.front() == fs::path("assets/drawn/cannon.png"));

	// a PNG of the part of the drawing taken, at 2 pixels a unit: 24 by 26 units
	std::ifstream pngFile(folder.path / "out/assets/drawn/cannon.png", std::ios::binary);
	const std::string png((std::istreambuf_iterator<char>(pngFile)), std::istreambuf_iterator<char>());
	REQUIRE(png.size() > 24);
	CHECK(png.compare(0, 8, "\x89PNG\r\n\x1a\n") == 0);
	const auto bigEndian = [&](std::size_t at)
	{
		return (static_cast<unsigned int>(static_cast<unsigned char>(png[at])) << 24) | (static_cast<unsigned int>(static_cast<unsigned char>(png[at + 1])) << 16)
			| (static_cast<unsigned int>(static_cast<unsigned char>(png[at + 2])) << 8) | static_cast<unsigned int>(static_cast<unsigned char>(png[at + 3]));
	};
	CHECK(bigEndian(16) == 48);
	CHECK(bigEndian(20) == 52);
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
	CHECK(main.find("const float tinyStep") == std::string::npos); // nothing uses it
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

	CHECK(refusal("<collision edge=\"vertical\"><aim object=\"box\" /></collision>", "", "").find("cannot generate <aim object=\"box\"> of something not an object or group, or of itself yet (in game > objects > object box") != std::string::npos);
	CHECK(refusal("<collision edge=\"vertical\"><land /></collision>", "", "").find("cannot generate <land /> outside a <collision> with other objects") != std::string::npos);
	CHECK(refusal("<collision edge=\"vertical\"><wrap /><play sound=\"boom\" /></collision>", "", "").find("cannot generate <wrap /> with other commands in the same <collision> yet") != std::string::npos);
	CHECK(refusal("<collision edge=\"top\"><inc variable=\"lives\" /></collision>", "<variable name=\"lives\">3</variable>", "").find("cannot generate <inc variable=\"lives\"> (it counts an object variable, as paddle1.score) yet") != std::string::npos);
	CHECK(refusal("<collision edge=\"top\"><play sound=\"boom\" /></collision>", "", "").find("cannot generate <play sound=\"boom\">, which is not a <sound> yet") != std::string::npos);
	CHECK(refusal("", "", "<state name=\"paused\"><inputs><input button=\"space\"><push state=\"nowhere\" /></input></inputs></state>").find("<push state=\"nowhere\">, which is not a <state>") != std::string::npos);
	CHECK(refusal("<collision edge=\"top\"><reset object=\"nothing\" /></collision>", "", "").find("cannot generate <reset object=\"nothing\">, which is not an object or group a screen shows yet") != std::string::npos);
	CHECK(refusal("", "", "<state name=\"won\"><conditions><condition object=\"box\"><remaining>half</remaining><reset /></condition></conditions></state>").find("cannot generate <remaining> that is not a whole number yet") != std::string::npos);
	CHECK(refusal("<collision edge=\"top\"><fire object=\"box\" /></collision>", "", "").find("cannot generate <fire> outside the <action> or <timer> of an object yet") != std::string::npos);
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
