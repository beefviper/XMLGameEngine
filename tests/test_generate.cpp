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

TEST_CASE("generating Breakout writes its rows of bricks as grids, and wins when none is left", "[generate]")
{
	if (!canGenerate())
	{
		SKIP("built without libxslt");
	}

	TempFolder folder("xge_test_generate_breakout");
	generateGame(requestFor(fs::current_path() / "games/breakout.xml", folder.path / "out"));

	const std::string main = readFile(folder.path / "out/main.cpp");
	CHECK(main.find("// bricks: 54 of them\nstd::vector<sf::RectangleShape> bricks(54);") != std::string::npos);
	CHECK(main.find("\t// bricks.2, a grid of 9 by 1\n\tfor (std::size_t i = 9; i < 18; ++i)\n\t{\n\t\tbricks[i].setSize({width, height});") != std::string::npos);
	CHECK(main.find("\tfor (std::size_t column = 0; column < 9; ++column)\n\t{\n\t\tbricks[9 + column].setPosition({margin + static_cast<float>(column) * (width + 5.0f), ") != std::string::npos);
	// the bricks never move, so their bounce off the sides is left out
	CHECK(main.find("physics::past(bricks") == std::string::npos);
	// every rule about the bottom in one touch of it: the ball bounces and dies
	CHECK(main.find("\t// bottom: bounce die\n\tif (physics::past(ball, physics::Edge::Bottom, windowArea))\n\t{\n\t\tphysics::bounce(ball, ballVelocity, physics::Edge::Bottom, windowArea);\n\t\tballAlive = false;\n\t\treturn;") != std::string::npos);
	CHECK(main.find("\t// bricks: none left\n\tif (std::count(bricksAlive.begin(), bricksAlive.end(), true) == 0)\n\t{\n\t\tscreens.push_back(Screen::Youwin);") != std::string::npos);
	CHECK(main.find("\t// ball: none left\n\tif (!ballAlive)\n\t{\n\t\tscreens.push_back(Screen::Gameover);") != std::string::npos);
	CHECK(main.find("#include <algorithm>") != std::string::npos);
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
		"      <sprite><grid><columns>3</columns><rows>2</rows><padding><x>4</x><y>4</y></padding><circle><radius>5</radius><color>color.green</color></circle></grid></sprite>\n"
		"      <position><x>20</x></position>\n"
		"      <velocity><x>2</x><y>0</y></velocity>\n"
		"      <collisions><enabled>true</enabled><lockstep>true</lockstep><collision edge=\"horizontal\"><bounce /></collision><collision object=\"shot\"><die /></collision></collisions>\n"
		"      <member><position><y>20</y></position></member>\n"
		"      <member><position><x>200</x><y>20</y></position></member>\n"
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
	// two members, each a grid of 3 by 2, in column order as the engine names them
	CHECK(main.find("std::vector<sf::CircleShape> aliens(12);") != std::string::npos);
	CHECK(main.find("\t\t\taliens[6 + column * 2 + row].setPosition({200.0f + static_cast<float>(column) * (2.0f * 5.0f + 4.0f), 20.0f + static_cast<float>(row) * (2.0f * 5.0f + 4.0f)});") != std::string::npos);
	// all of the block moved first, then its rules
	CHECK(main.find("void updateAliens()\n{\n\tfor (sf::CircleShape& one : aliens)\n\t{\n\t\tone.move(aliensVelocity);\n\t}\n\n\tfor (std::size_t i = 0; i < aliens.size(); ++i)") != std::string::npos);
	CHECK(main.find("\t\t\taliensVelocity.x = -aliensVelocity.x;\n\t\t\tfor (sf::CircleShape& each : aliens)\n\t\t\t{\n\t\t\t\teach.move({aliensVelocity.x, 0.0f});") != std::string::npos);
	CHECK(main.find("\t// aliens: no more than 11 left\n\tif (std::count(aliensAlive.begin(), aliensAlive.end(), true) <= 11)\n\t{\n\t\tstart();") != std::string::npos);
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

	CHECK(refusal("<collision edge=\"vertical\"><stop /></collision>", "", "").find("cannot generate <stop> yet (in game > objects > object box") != std::string::npos);
	CHECK(refusal("<collision edge=\"vertical\"><wrap /><play sound=\"boom\" /></collision>", "", "").find("cannot generate <wrap /> with other commands in the same <collision> yet") != std::string::npos);
	CHECK(refusal("<collision edge=\"top\"><inc variable=\"lives\" /></collision>", "<variable name=\"lives\">3</variable>", "").find("cannot generate <inc variable=\"lives\"> (it counts an object variable, as paddle1.score) yet") != std::string::npos);
	CHECK(refusal("<collision edge=\"top\"><play sound=\"boom\" /></collision>", "", "").find("cannot generate <play sound=\"boom\">, which is not a <sound> yet") != std::string::npos);
	CHECK(refusal("", "", "<state name=\"paused\"><inputs><input button=\"space\"><push state=\"nowhere\" /></input></inputs></state>").find("<push state=\"nowhere\">, which is not a <state>") != std::string::npos);
	CHECK(refusal("<collision edge=\"top\"><reset object=\"box\" /></collision>", "", "").find("cannot generate <reset object=") != std::string::npos);
	CHECK(refusal("", "", "<state name=\"won\"><conditions><condition object=\"box\"><remaining>half</remaining><reset /></condition></conditions></state>").find("cannot generate <remaining> that is not a whole number yet") != std::string::npos);
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
