// test_cli.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026
//
// Catch2 tests for xgecli's command line (xge/cli/source/cli.cpp): the options
// and their short, attached and long forms, the backend names, the errors, and
// the order a game file is looked for in.
//
// Test names must not start with "-" or contain a comma: ctest hands the name to
// Catch2 on its command line, which reads them as an option or a list.

#include "file_text.h"
#include "scratch.h"
#include "cli.h"
#include "engine/engine.h"
#include "game/game.h"
#include "load/xml.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>

#include <filesystem>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
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
			directory(std::filesystem::temp_directory_path() / scratchName("xge_test_cli"))
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

TEST_CASE("generate takes a target and output a folder", "[cli]")
{
	const CliOptions options = parseCommandLine({ "pong", "--generate", "windows-cpp" });
	CHECK(options.game == "pong");
	CHECK(options.generate == "windows-cpp");
	CHECK(options.output.empty());

	CHECK(parseCommandLine({ "--generate", "Windows-CPP" }).generate == "windows-cpp");
	CHECK(parseCommandLine({ "--generate", "windows-cpp", "-o", "out" }).output == "out");
	CHECK(parseCommandLine({ "--generate", "windows-cpp", "-oout" }).output == "out");
	CHECK(parseCommandLine({ "--output", "out", "--generate", "windows-cpp" }).output == "out");

	// -g is still the game, not generate.
	CHECK(parseCommandLine({ "-g", "pong" }).generate.empty());

	CHECK(parseCommandLine({}).generate.empty());
}

TEST_CASE("generate needs a known target and output needs generate", "[cli]")
{
	CHECK_THROWS_AS(parseCommandLine({ "--generate" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--generate", "amiga-asm" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--generate", "windows-cpp", "--generate", "windows-cpp" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "-o", "out" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--generate", "windows-cpp", "-o", "a", "-o", "b" }), CliError);
	CHECK(generateTargets() == std::vector<std::string>{ "windows-cpp" });
}

TEST_CASE("record writes the game played into the game file, once and not when generating", "[cli][demo]")
{
	CHECK_FALSE(parseCommandLine({ "galaxian" }).record);
	CHECK(parseCommandLine({ "galaxian", "--record" }).record);
	CHECK(parseCommandLine({ "-g", "galaxian", "--record" }).record);
	CHECK_THROWS_AS(parseCommandLine({ "-r" }), CliError); // long form only, as --generate
	CHECK(parseCommandLine({ "--record", "galaxian" }).game == "galaxian");
	CHECK_THROWS_AS(parseCommandLine({ "--record", "--record" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--record", "--generate", "windows-cpp" }), CliError);
	CHECK(usageText().find("--record") != std::string::npos);
}

TEST_CASE("check takes any number of games, and nothing that plays", "[cli][check]")
{
	CHECK_FALSE(parseCommandLine({ "pong" }).check);
	const CliOptions none = parseCommandLine({ "--check" });
	CHECK(none.check);
	CHECK(none.games.empty());
	const CliOptions several = parseCommandLine({ "pong", "frogger", "--check", "-x", "tinyxml2" });
	CHECK(several.check);
	CHECK(several.games == std::vector<std::string>{ "pong", "frogger" });
	CHECK(several.xml == XmlBackend::TinyXml2);

	CHECK_THROWS_AS(parseCommandLine({ "-c" }), CliError); // long form only
	CHECK_THROWS_AS(parseCommandLine({ "--check", "--check" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--check", "--record" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--check", "--generate", "windows-cpp" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--check", "-w", "sdl2" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--check", "-a", "none" }), CliError);
	CHECK(usageText().find("--check") != std::string::npos);
}

TEST_CASE("check loads each game as for play and says ok or what is wrong", "[cli][check]")
{
	const std::filesystem::path broken = scratchName("descriptions/check_broken.xml");
	{
		std::ifstream in("descriptions/pong.xml");
		std::string text = fileText(in);
		text.replace(text.find("<width>1280</width>"), 19, "<width>0</width>");
		std::ofstream(broken) << text;
	}

	std::ostringstream out;
	CHECK(checkGames({ "descriptions/pong.xml", broken.string(), "descriptions/frogger.xml" }, XmlBackend::TinyXml2, out) == 1);
	const std::string said = out.str();
	CHECK(said.find("pong.xml: ok\n") != std::string::npos);
	CHECK(said.find("frogger.xml: ok\n") != std::string::npos);
	CHECK(said.find(broken.filename().string() + ": ") != std::string::npos);
	INFO(said);
	CHECK(said.find("width") != std::string::npos);
	CHECK(said.find("Loading file") == std::string::npos);
	std::filesystem::remove(broken);

	// With no game, every one in the folder.
	const std::vector<std::string> all = gameFilesIn("descriptions");
	CHECK(all.size() > 50);
	CHECK(std::is_sorted(all.begin(), all.end()));
}

TEST_CASE("frames plays with no window and no sound and seed fixes the numbers", "[cli][frames]")
{
	const CliOptions played = parseCommandLine({ "pong", "--frames", "600", "--seed", "1" });
	CHECK(played.frames == 600);
	REQUIRE(played.seed);
	CHECK(*played.seed == 1u);
	CHECK(played.window == WindowBackend::None);
	CHECK(played.audio == AudioBackend::None);
	CHECK(parseCommandLine({ "pong", "--frames", "1", "-w", "none", "-a", "none" }).frames == 1);
	CHECK(parseCommandLine({ "pong" }).frames == 0);
	CHECK_FALSE(parseCommandLine({ "pong" }).seed);
	CHECK(parseCommandLine({ "pong", "--seed", "0" }).seed == 0u); // a seed alone plays in a window

	CHECK_THROWS_AS(parseCommandLine({ "--frames" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--frames", "0" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--frames", "ten" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--frames", "10", "--frames", "10" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--seed", "-1" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--seed", "1", "--seed", "2" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--frames", "10", "--record" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--frames", "10", "--check" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--frames", "10", "--generate", "windows-cpp" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--seed", "1", "--check" }), CliError);
	CHECK(usageText().find("--frames") != std::string::npos);
	CHECK(usageText().find("--seed") != std::string::npos);
}

TEST_CASE("frames plays the game to the end and says how far it got", "[cli][frames]")
{
	const CliOptions options = parseCommandLine({ "pong", "--frames", "600", "--seed", "1", "-x", "tinyxml2" });
	std::ostringstream out;
	CHECK(playFrames("descriptions/pong.xml", options, out) == EXIT_SUCCESS);
	INFO(out.str());
	CHECK(out.str().find("played 600 frames of pong.xml") != std::string::npos);

	// A game that will not load is a message and a failure.
	const std::filesystem::path broken = scratchName("descriptions/frames_broken.xml");
	{
		std::ifstream in("descriptions/pong.xml");
		std::string text = fileText(in);
		text.replace(text.find("<width>1280</width>"), 19, "<width>0</width>");
		std::ofstream(broken) << text;
	}
	std::ostringstream failed;
	CHECK(playFrames(broken.string(), options, failed) == EXIT_FAILURE);
	std::filesystem::remove(broken);
	INFO(failed.str());
	CHECK(failed.str().rfind("Error: ", 0) != std::string::npos);
	CHECK(failed.str().find("width") != std::string::npos);
}

TEST_CASE("keys takes a file and goes with neither seed nor record", "[cli][frames]")
{
	CHECK(parseCommandLine({ "pong", "--keys", "play.xml" }).keys == "play.xml");
	CHECK(parseCommandLine({ "pong", "--keys", "play.xml", "--frames", "10" }).frames == 10);
	CHECK_THROWS_AS(parseCommandLine({ "--keys" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--keys", "a.xml", "--keys", "b.xml" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--keys", "a.xml", "--seed", "1" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--keys", "a.xml", "--record" }), CliError);
	CHECK_THROWS_AS(parseCommandLine({ "--keys", "a.xml", "--check" }), CliError);
	CHECK(usageText().find("--keys") != std::string::npos);
}

namespace
{
	// Every object's name, place and numbers, a line each: what two games
	// played alike have alike.
	std::string everything(Game& game)
	{
		std::ostringstream out;
		out << game.getCurrentState().name << '\n';
		for (const Object& object : game.getCurrentObjects())
		{
			out << object.name << ' ' << object.position.x << ' ' << object.position.y;
			for (const auto& [name, value] : object.variable) { out << ' ' << name << '=' << value; }
			out << '\n';
		}
		return out.str();
	}
}

TEST_CASE("keys plays back what record recorded and ends the same", "[cli][frames][demo]")
{
	// Ten seconds of Pong played and recorded, keys pressed between frames
	// as a window would report them.
	RawDemo recording;
	std::string atEnd;
	{
		Game game{ readGame("descriptions/pong.xml", XmlBackend::TinyXml2) };
		Engine engine(game, WindowBackend::None, AudioBackend::None);
		engine.record();
		const std::vector<std::pair<int, std::pair<KeyCode, bool>>> keys = {
			{ 10, { KeyCode::Space, true } }, { 14, { KeyCode::Space, false } },
			{ 60, { KeyCode::W, true } }, { 200, { KeyCode::W, false } },
			{ 250, { KeyCode::S, true } }, { 420, { KeyCode::S, false } },
			{ 450, { KeyCode::W, true } }, { 590, { KeyCode::W, false } },
		};
		std::size_t next = 0;
		for (int frame = 0; frame < 600; ++frame)
		{
			for (; next < keys.size() && keys[next].first == frame; ++next)
			{
				if (keys[next].second.second) { engine.handleKeyPressed(keys[next].second.first); }
				else { engine.handleKeyReleased(keys[next].second.first); }
			}
			engine.step();
		}
		recording = engine.recorded();
		atEnd = everything(game);
	}
	REQUIRE(recording.frames == 600);
	REQUIRE(recording.keys.size() == 8);
	CHECK(recording.keys[2].frame == 60);

	// Written into a copy of the game, as --record writes it, and read back.
	const std::filesystem::path recorded = scratchName("descriptions/keys_recorded.xml");
	{
		std::ifstream in("descriptions/pong.xml");
		std::ofstream(recorded) << withDemo(fileText(in), recording);
	}
	for (const XmlBackend xml : XmlDocumentFactory::availableBackends())
	{
		INFO(XmlDocumentFactory::name(xml));
		const RawDemo read = readDemo(recorded.string(), xml);
		CHECK(read.seed == recording.seed);
		CHECK(read.frames == 600);
		CHECK(read.keys.size() == 8);
	}

	// Played back, it ends where the recording did.
	{
		Game game{ readGame("descriptions/pong.xml", XmlBackend::TinyXml2) };
		Engine engine(game, WindowBackend::None, AudioBackend::None);
		engine.replay(readDemo(recorded.string(), XmlBackend::TinyXml2));
		for (int frame = 0; frame < 600; ++frame) { engine.step(); }
		CHECK(everything(game) == atEnd);
	}

	// The keys made the difference: the same seed with none ends elsewhere.
	{
		Game game{ readGame("descriptions/pong.xml", XmlBackend::TinyXml2) };
		Engine engine(game, WindowBackend::None, AudioBackend::None);
		RawDemo noKeys = recording;
		noKeys.keys.clear();
		engine.replay(noKeys);
		for (int frame = 0; frame < 600; ++frame) { engine.step(); }
		CHECK(everything(game) != atEnd);
	}

	// And by the command line, from a file that is only the <demo>.
	const std::filesystem::path alone = scratchName("descriptions/keys_alone.xml");
	{
		const std::string game = withDemo("<game>\n</game>\n", recording);
		std::ofstream(alone) << game.substr(game.find("<demo>"), game.find("</demo>") + 7 - game.find("<demo>")) << '\n';
	}
	std::ostringstream out;
	CHECK(playFrames("descriptions/pong.xml", parseCommandLine({ "pong", "--frames", "600", "--keys", alone.string() }), out) == EXIT_SUCCESS);
	INFO(out.str());
	CHECK(out.str().find("played 600 frames") != std::string::npos);

	// A file with no <demo> says so.
	std::ostringstream none;
	CHECK(playFrames("descriptions/pong.xml", parseCommandLine({ "pong", "--frames", "10", "--keys", "descriptions/pong.xml" }), none) == EXIT_FAILURE);
	CHECK(none.str().find("has no <demo>") != std::string::npos);

	std::filesystem::remove(recorded);
	std::filesystem::remove(alone);
}

TEST_CASE("describe writes the state and every object in play as XML", "[cli][dump]")
{
	Game game{ readGame("descriptions/pong.xml", XmlBackend::TinyXml2) };
	Engine engine(game, WindowBackend::None, AudioBackend::None);
	engine.handleKeyPressed(KeyCode::Space);
	engine.handleKeyReleased(KeyCode::Space);
	for (int frame = 0; frame < 60; ++frame) { engine.step(); }

	std::ostringstream out;
	game.describe(out, 60);
	const std::string said = out.str();
	INFO(said);
	CHECK(said.rfind("<moment>\n  <frame>60</frame>\n  <state name=\"playing\" />\n", 0) == 0);
	for (const std::string name : { "playing.ball", "playing.paddle1", "playing.paddle2" })
	{
		const Object& object = game.getObject(name);
		std::ostringstream position;
		position << "<position><x>" << object.position.x << "</x><y>" << object.position.y << "</y></position>";
		const auto at = said.find("<object name=\"" + name + "\"");
		REQUIRE(at != std::string::npos);
		CHECK(said.find(position.str(), at) == said.find("<position>", at));
	}
	CHECK(said.find("class=\"paddle\" group=\"playing.paddles\"") != std::string::npos);
	CHECK(said.find("<variable name=\"score\">0</variable>") != std::string::npos);
	CHECK(said.find("playing.title") == std::string::npos); // not in play
	CHECK(said.substr(said.size() - 10) == "</moment>\n");
}

TEST_CASE("the help options ask for the usage", "[cli]")
{
	CHECK(parseCommandLine({ "-h" }).showHelp);
	CHECK(parseCommandLine({ "--help" }).showHelp);
	CHECK(usageText().find("--game") != std::string::npos);
	CHECK(usageText().find("--generate") != std::string::npos);
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
	TempWorkingDirectory::touch("games/descriptions/breakout.xml");

	CHECK(std::filesystem::path(findGameFile("breakout")) == std::filesystem::path("games") / "descriptions" / "breakout.xml");
	CHECK(std::filesystem::path(findGameFile("breakout.xml")) == std::filesystem::path("games") / "descriptions" / "breakout.xml");
}

TEST_CASE("the working directory wins over the games directory", "[cli]")
{
	TempWorkingDirectory work;
	TempWorkingDirectory::touch("pong.xml");
	TempWorkingDirectory::touch("games/descriptions/pong.xml");

	CHECK(findGameFile("pong") == "pong.xml");
}

TEST_CASE("a path as given wins over the working and games directories", "[cli]")
{
	TempWorkingDirectory work;
	TempWorkingDirectory::touch("mine/pong.xml");
	TempWorkingDirectory::touch("pong.xml");
	TempWorkingDirectory::touch("games/descriptions/pong.xml");

	CHECK(std::filesystem::path(findGameFile("mine/pong.xml")) == std::filesystem::path("mine") / "pong.xml");
	CHECK(std::filesystem::path(findGameFile("mine/pong")) == std::filesystem::path("mine") / "pong.xml");
}

TEST_CASE("a path that is not there falls back to the file name", "[cli]")
{
	TempWorkingDirectory work;
	TempWorkingDirectory::touch("pong.xml");
	TempWorkingDirectory::touch("games/descriptions/breakout.xml");

	CHECK(findGameFile("nowhere/pong.xml") == "pong.xml");
	CHECK(std::filesystem::path(findGameFile("nowhere/breakout.xml")) == std::filesystem::path("games") / "descriptions" / "breakout.xml");
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
