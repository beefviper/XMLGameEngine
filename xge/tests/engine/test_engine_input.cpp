// test_engine_input.cpp
// XML Game Engine
// author: beefviper
// date: Sept 29, 2026
//
// Catch2 tests for Engine's key handling (Engine::handleKeyPressed and
// handleKeyReleased) against real xge::Games built from games/descriptions/breakout.xml
// and games/descriptions/spaceinvaders.xml, driven through a fake Window so no real
// window is opened.
//
// The current state decides what a held key means. Breakout's "paused"
// state shows the player but binds only Space, so while it is up the
// player must not respond to a held Left, and must pick Left back up on
// return to "playing" if it is still down - and the other way round for a
// key first pressed during the pause. Regressions covered: a key's release
// used to be looked up in whichever state was active when it was let go,
// and a held key kept driving the player through a state that did not bind
// it.

#include "engine/engine.h"
#include "file_text.h"
#include "game/game.h"
#include "window/null.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace xge;

namespace
{
	// A Window that reports the key changes it is given, once, and can be
	// closed: what Engine::pump() and isWindowOpen() are tested with.
	class ScriptedWindow : public NullWindow
	{
	public:
		bool isOpen() const override { return open; }
		std::vector<std::pair<KeyCode, bool>> pollEvents() override { return std::exchange(queued, {}); }

		bool open{ true };
		std::vector<std::pair<KeyCode, bool>> queued;
	};

	// breakout.xml's <variable name="step">8</variable>.
	constexpr float kStep = 8.0f;

	void tap(Engine& engine, KeyCode key)
	{
		engine.handleKeyPressed(key);
		engine.handleKeyReleased(key);
	}
}

TEST_CASE("a held key stops driving the player while a state without it is up, and resumes on return", "[engine_input]")
{
	Game game{ "descriptions/breakout.xml" };
	Engine engine(game, std::make_unique<NullWindow>());
	Object& player = game.getObject("player");

	tap(engine, KeyCode::Space); // title -> playing

	engine.handleKeyPressed(KeyCode::Left);
	REQUIRE(player.velocity.x == -kStep);

	tap(engine, KeyCode::Pause); // pause: "paused" binds only Pause and the pad's Start
	CHECK(game.getCurrentState().name == "paused");
	CHECK(player.velocity.x == 0.0f);

	tap(engine, KeyCode::Pause); // unpause, Left still down
	CHECK(game.getCurrentState().name == "playing");
	CHECK(player.velocity.x == -kStep);

	engine.handleKeyReleased(KeyCode::Left);
	CHECK(player.velocity.x == 0.0f);
}

TEST_CASE("a key pressed in a state that does not bind it starts driving the player once a state that does is entered", "[engine_input]")
{
	Game game{ "descriptions/breakout.xml" };
	Engine engine(game, std::make_unique<NullWindow>());
	Object& player = game.getObject("player");

	tap(engine, KeyCode::Space); // title -> playing
	tap(engine, KeyCode::Pause); // pause

	engine.handleKeyPressed(KeyCode::Left); // no Left binding in "paused"
	CHECK(player.velocity.x == 0.0f);

	tap(engine, KeyCode::Pause); // unpause
	CHECK(player.velocity.x == -kStep);

	engine.handleKeyReleased(KeyCode::Left);
	CHECK(player.velocity.x == 0.0f);
}

TEST_CASE("releasing a key while paused leaves the player stopped after unpausing", "[engine_input]")
{
	Game game{ "descriptions/breakout.xml" };
	Engine engine(game, std::make_unique<NullWindow>());
	Object& player = game.getObject("player");

	tap(engine, KeyCode::Space); // title -> playing

	engine.handleKeyPressed(KeyCode::Left);
	tap(engine, KeyCode::Pause); // pause
	engine.handleKeyReleased(KeyCode::Left);
	tap(engine, KeyCode::Pause); // unpause

	CHECK(player.velocity.x == 0.0f);
}

TEST_CASE("a key pressed and released while paused never moves the player", "[engine_input]")
{
	Game game{ "descriptions/breakout.xml" };
	Engine engine(game, std::make_unique<NullWindow>());
	Object& player = game.getObject("player");

	tap(engine, KeyCode::Space); // title -> playing
	tap(engine, KeyCode::Pause); // pause

	tap(engine, KeyCode::Left);
	tap(engine, KeyCode::Pause); // unpause

	CHECK(player.velocity.x == 0.0f);
}

TEST_CASE("holding a state-changing key through the change does not trigger it again", "[engine_input]")
{
	Game game{ "descriptions/breakout.xml" };
	Engine engine(game, std::make_unique<NullWindow>());

	// Space is bound to a state change in title and in the end screen. Held
	// down, it must take title to playing once, and do nothing more.
	engine.handleKeyPressed(KeyCode::Space);
	CHECK(game.getCurrentState().name == "playing");

	engine.handleKeyReleased(KeyCode::Space);
	CHECK(game.getCurrentState().name == "playing");
}

TEST_CASE("an ordinary press and release in one state is unchanged", "[engine_input]")
{
	Game game{ "descriptions/breakout.xml" };
	Engine engine(game, std::make_unique<NullWindow>());
	Object& player = game.getObject("player");

	tap(engine, KeyCode::Space); // title -> playing

	engine.handleKeyPressed(KeyCode::Right);
	CHECK(player.velocity.x == kStep);
	engine.handleKeyReleased(KeyCode::Right);
	CHECK(player.velocity.x == 0.0f);
}

TEST_CASE("a held fire key does not shoot again when its state returns", "[engine_input]")
{
	Game game{ "descriptions/spaceinvaders.xml" };
	Engine engine(game, std::make_unique<NullWindow>());
	Object& bullet = game.getObject("bullet");

	tap(engine, KeyCode::Space); // title -> playing

	engine.handleKeyPressed(KeyCode::Space); // fire
	REQUIRE(bullet.collisionsOn);

	// Used up: the shot is gone and the key is still down.
	bullet.collisionsOn = false;
	tap(engine, KeyCode::Pause); // pause
	tap(engine, KeyCode::Pause); // unpause

	CHECK_FALSE(bullet.collisionsOn);
}

TEST_CASE("the key that starts Space Invaders does not also fire a shot", "[engine_input]")
{
	Game game{ "descriptions/spaceinvaders.xml" };
	Engine engine(game, std::make_unique<NullWindow>());
	Object& bullet = game.getObject("bullet");

	// Space starts the game from the menu and fires in play. Held down through
	// the change, it must only start the game.
	engine.handleKeyPressed(KeyCode::Space);
	CHECK(game.getCurrentState().name == "playing");
	CHECK_FALSE(bullet.collisionsOn);

	engine.handleKeyReleased(KeyCode::Space);
	CHECK_FALSE(bullet.collisionsOn);
}

TEST_CASE("keys read by pump() wait for the next step", "[engine_input]")
{
	Game game{ "descriptions/breakout.xml" };
	auto scripted = std::make_unique<ScriptedWindow>();
	ScriptedWindow& window = *scripted;
	Engine engine(game, std::move(scripted));

	// Space starts the game from the menu. Read while the game is paused, it
	// must neither start it nor be lost.
	window.queued = { { KeyCode::Space, true }, { KeyCode::Space, false } };
	engine.pump();
	CHECK(game.getCurrentState().name != "playing");

	engine.step();
	CHECK(game.getCurrentState().name == "playing");
}

TEST_CASE("the engine knows when its window has been closed", "[engine_input]")
{
	Game game{ "descriptions/breakout.xml" };
	auto scripted = std::make_unique<ScriptedWindow>();
	ScriptedWindow& window = *scripted;
	Engine engine(game, std::move(scripted));

	CHECK(engine.isWindowOpen());

	window.open = false;
	CHECK_FALSE(engine.isWindowOpen());
}

TEST_CASE("every shipped game follows the shared key conventions", "[engine_input]")
{
	// Space starts and plays again (never Enter), the Pause key (or the pad's
	// Start) pauses and plays on, and player one is on W, A, S and D (never Z;
	// Q turns or cycles back beside E). A game of one screen (pong_min) has
	// nothing for Space to do.
	int checked = 0;
	for (const auto& entry : std::filesystem::directory_iterator("descriptions"))
	{
		if (entry.path().extension() != ".xml") { continue; }
		// Other tests write scratch games here while this one runs, side by
		// side under ctest, and one half written can look like anything.
		if (entry.path().stem().string().find("_scratch") != std::string::npos) { continue; }

		std::ifstream in(entry.path());
		const std::string text = fileText(in);

		INFO(entry.path().string());
		// A key is button="name", or <button>name</button> among several.
		const auto names = [&](const std::string& key)
		{
			return text.find("button=\"" + key + "\"") != std::string::npos || text.find("<button>" + key + "</button>") != std::string::npos;
		};
		CHECK_FALSE(names("enter"));
		CHECK_FALSE(names("z"));
		const bool oneScreen = text.find("<state ") == text.rfind("<state ");
		CHECK((oneScreen || names("space")));
		// P and Escape pause nothing: the one pause key is Pause.
		CHECK_FALSE(names("p"));
		CHECK_FALSE(names("escape"));
		// The keys and what they do, however they are laid out.
		std::string packed = text;
		std::erase_if(packed, [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; });
		const std::string pauseKeys = "<input><button>pause</button><button>pad1.start</button>";
		if (text.find("<state name=\"paused\">") != std::string::npos)
		{
			CHECK(packed.find(pauseKeys + "<pop/>") != std::string::npos);
			CHECK(packed.find(pauseKeys + "<pushstate=\"paused\"/>") != std::string::npos);
		}
		++checked;
	}
	CHECK(checked >= 10);
}
