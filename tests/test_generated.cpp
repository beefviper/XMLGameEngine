// test_generated.cpp
// XML Game Engine
// author: beefviper
// date: Oct 10, 2026
//
// Every generated game against the engine: each game that scripts/shell/windows/
// games-build-all.bat has built (windows-cpp, with BUILD_TESTING) is played with
// no window from the same keys and the same random seed by that program and by
// the engine, and the pictures at the same frames, and the sounds played, must
// match (the frame trace, lib/include/trace.h).
//
// Not run with the other tests: it needs the games generated and built first,
// which takes minutes and a compiler, so it is hidden and asked for by its tag:
//
//     scripts\shell\windows\games-generate-all.bat
//     scripts\shell\windows\games-build-all.bat
//     output\Debug\xgetest "[generated]"
//
// (games-run-all.bat does all of that.) A game that is not built is skipped,
// saying so. The frames of one that differs are kept in the temporary folder, in
// xge-trace/<game>: frameNNNN.bmp is the generated program's and
// frameNNNN.engine.bmp the engine's, stateNNNN.txt and stateNNNN.engine.txt where
// each object is. XGE_GAME=<game> plays one game, XGE_FRAMES=100,101 keeps those
// frames.

#include "engine.h"
#include "game.h"
#include "trace.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace xge;

namespace
{
	namespace fs = std::filesystem;

	// A setting from the environment, if there is one.
	std::optional<std::string> environment(const char* name)
	{
#ifdef _WIN32
		char* value = nullptr;
		std::size_t size = 0;
		if (_dupenv_s(&value, &size, name) != 0 || value == nullptr)
		{
			return std::nullopt;
		}
		std::string text(value);
		std::free(value);
		return text;
#else
		const char* value = std::getenv(name);
		return value ? std::optional<std::string>(value) : std::nullopt;
#endif
	}

	// How long every game is played, in frames.
	constexpr int lastFrame = 2000;

	// What every game is played with: Space to start, then keys pressed and let go
	// of at random, for a long time, so that things meet in every way a game has.
	// The games share their keys (Space starts, then W A S D, the arrows and Space
	// to fire), so one script does for them all. It is worked out from a fixed
	// seed with plain arithmetic, so that it is the same everywhere.
	std::string playedKeys()
	{
		std::mt19937 numbers(11);
		const auto below = [&](unsigned count) { return static_cast<unsigned>(numbers() % count); };

		const char* const names[] = { "space", "a", "d", "w", "s", "left", "right", "up", "down" };
		std::vector<std::pair<int, std::string>> events;
		events.emplace_back(3, "+space");
		events.emplace_back(5, "-space");

		int frame = 20;
		while (frame < lastFrame - 100)
		{
			const std::string key = names[below(9)];
			events.emplace_back(frame, "+" + key);
			events.emplace_back(frame + 1 + static_cast<int>(below(40)), "-" + key);
			frame += 1 + static_cast<int>(below(25));
		}
		std::stable_sort(events.begin(), events.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

		std::string script;
		for (const auto& [at, change] : events)
		{
			script += (script.empty() ? "" : ",") + std::to_string(at) + change;
		}
		return script;
	}

	// The frames that are kept and compared: every hundredth, or the ones named in
	// the environment (XGE_FRAMES=100,101,102), to look at a game closely.
	std::string playedFrames()
	{
		if (const auto named = environment("XGE_FRAMES"))
		{
			return *named;
		}

		std::string list;
		for (int frame = 100; frame <= lastFrame; frame += 100)
		{
			list += (list.empty() ? "" : ",") + std::to_string(frame);
		}
		return list;
	}

	const std::string frames = playedFrames();
	const std::string keys = playedKeys();

	// How far apart a colour may be (the edge of a text is not drawn the same by a
	// picture kept and a text drawn), how many pixels may be further apart than
	// that (the edge of a shape a little differently placed), and how many frames
	// apart a sound may be heard (a touch found a frame earlier or later).
	constexpr int tolerance = 64;
	constexpr std::size_t allow = 100;
	constexpr int slack = 1;

	// The games there are, by name, or the one named in the environment
	// (XGE_GAME=pong).
	std::vector<std::string> gameNames()
	{
		if (const auto named = environment("XGE_GAME"))
		{
			return { *named };
		}

		std::vector<std::string> names;
		for (const auto& entry : fs::directory_iterator("games"))
		{
			if (entry.path().extension() == ".xml")
			{
				names.push_back(entry.path().stem().string());
			}
		}
		std::sort(names.begin(), names.end());
		return names;
	}

	// Where games-build-all.bat builds a game: generated/<game>/output/<config>/
	// beside the tests' own folder, which is named for the configuration too; or
	// in output/ itself where there is no configuration folder.
	fs::path builtProgram(const std::string& game)
	{
		const fs::path built = fs::path("generated") / game / "output";
		const std::string program = game + ".exe";

		for (const fs::path& candidate : {
			built / fs::current_path().filename() / program,
			built / game,
			built / program })
		{
			if (fs::exists(candidate)) { return fs::absolute(candidate); }
		}
		return {};
	}

	// Runs the program in its own folder, which is where its assets are, with its
	// output in a file; its exit status.
	int run(const fs::path& program, const std::string& arguments, const fs::path& log)
	{
		const fs::path previous = fs::current_path();
		fs::current_path(program.parent_path());

		const std::string command = "\"" + program.string() + "\" " + arguments + " > \"" + log.string() + "\" 2>&1";
#ifdef _WIN32
		// cmd keeps the quotes round a command that starts with one only if the whole is quoted
		const int status = std::system(("\"" + command + "\"").c_str());
#else
		const int status = std::system(command.c_str());
#endif

		fs::current_path(previous);
		return status;
	}
}

TEST_CASE("a generated game plays as the engine does", "[.generated]")
{
	const std::string game = GENERATE_COPY(from_range(gameNames()));
	CAPTURE(game);

	const fs::path program = builtProgram(game);
	if (program.empty())
	{
		SKIP("not built: run scripts/shell/windows/games-build-all.bat (and games-generate-all.bat before it)");
	}

	const fs::path folder = fs::temp_directory_path() / "xge-trace" / game;
	const fs::path log = folder.string() + ".log";
	fs::remove_all(folder);
	fs::create_directories(folder.parent_path());

	// the generated program, which writes its frames and sounds to the folder
	const int status = run(program, "--trace \"" + folder.string() + "\" --frames \"" + frames + "\" --keys \"" + keys + "\"", log);
	INFO("the output of the generated program is in " << log.string());
	REQUIRE(status == 0);

	// the engine, played the same way, compared with them
	game_expr::seedRandom(1);
	Game played{ "games/" + game + ".xml" };
	game_expr::unseedRandom();

	auto sounds = std::make_unique<trace::SoundLog>();
	trace::SoundLog* heard = sounds.get();
	Engine engine(played, WindowFactory::createOffscreen(played.getWindowDesc(), WindowBackend::SFML3), std::move(sounds));

	trace::Request request;
	request.frames = trace::parseFrames(frames);
	request.keys = trace::parseKeys(keys);
	request.against = folder;
	request.tolerance = tolerance;
	request.allow = allow;
	request.sounds = heard;
	request.soundSlack = slack;

	std::ostringstream said;
	const int different = trace::run(engine, request, said);
	INFO(said.str());
	INFO("the frames are kept in " << folder.string());
	REQUIRE(different == 0);

	// only a game that differs keeps its frames
	fs::remove_all(folder);
	fs::remove(log);
}
