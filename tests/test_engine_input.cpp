// test_engine_input.cpp
// XML Game Engine
// author: beefviper
// date: Sept 29, 2026
//
// Catch2 tests for Engine's key handling (Engine::handleKeyPressed and
// handleKeyReleased) against real xge::Games built from games/breakout.xml
// and games/spaceinvaders.xml, driven through a fake Window so no real
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

#include "engine.h"
#include "game.h"

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
	// Just enough of a Window for Engine's constructor: nothing is drawn and
	// no events are ever produced (the tests call handleKey* directly).
	class FakeWindow : public Window
	{
	public:
		bool isOpen() const override { return true; }
		void close() override {}
		void init(std::vector<Object>&) override {}
		std::vector<std::pair<KeyCode, bool>> pollEvents() override { return {}; }
		void clear(const std::string&) override {}
		void draw(Object&) override {}
		void display() override {}
	};

	// A Window that reports the key changes it is given, once, and can be
	// closed: what Engine::pump() and isWindowOpen() are tested with.
	class ScriptedWindow : public FakeWindow
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
	Game game{ "games/breakout.xml" };
	Engine engine(game, std::make_unique<FakeWindow>());
	Object& player = game.getObject("player");

	tap(engine, KeyCode::Space); // mainmenu -> playing

	engine.handleKeyPressed(KeyCode::Left);
	REQUIRE(player.velocity.x == -kStep);

	tap(engine, KeyCode::Space); // pause: "paused" binds only Space
	CHECK(game.getCurrentState().name == "paused");
	CHECK(player.velocity.x == 0.0f);

	tap(engine, KeyCode::Space); // unpause, Left still down
	CHECK(game.getCurrentState().name == "playing");
	CHECK(player.velocity.x == -kStep);

	engine.handleKeyReleased(KeyCode::Left);
	CHECK(player.velocity.x == 0.0f);
}

TEST_CASE("a key pressed in a state that does not bind it starts driving the player once a state that does is entered", "[engine_input]")
{
	Game game{ "games/breakout.xml" };
	Engine engine(game, std::make_unique<FakeWindow>());
	Object& player = game.getObject("player");

	tap(engine, KeyCode::Space); // mainmenu -> playing
	tap(engine, KeyCode::Space); // pause

	engine.handleKeyPressed(KeyCode::Left); // no Left binding in "paused"
	CHECK(player.velocity.x == 0.0f);

	tap(engine, KeyCode::Space); // unpause
	CHECK(player.velocity.x == -kStep);

	engine.handleKeyReleased(KeyCode::Left);
	CHECK(player.velocity.x == 0.0f);
}

TEST_CASE("releasing a key while paused leaves the player stopped after unpausing", "[engine_input]")
{
	Game game{ "games/breakout.xml" };
	Engine engine(game, std::make_unique<FakeWindow>());
	Object& player = game.getObject("player");

	tap(engine, KeyCode::Space); // mainmenu -> playing

	engine.handleKeyPressed(KeyCode::Left);
	tap(engine, KeyCode::Space); // pause
	engine.handleKeyReleased(KeyCode::Left);
	tap(engine, KeyCode::Space); // unpause

	CHECK(player.velocity.x == 0.0f);
}

TEST_CASE("a key pressed and released while paused never moves the player", "[engine_input]")
{
	Game game{ "games/breakout.xml" };
	Engine engine(game, std::make_unique<FakeWindow>());
	Object& player = game.getObject("player");

	tap(engine, KeyCode::Space); // mainmenu -> playing
	tap(engine, KeyCode::Space); // pause

	tap(engine, KeyCode::Left);
	tap(engine, KeyCode::Space); // unpause

	CHECK(player.velocity.x == 0.0f);
}

TEST_CASE("holding a state-changing key through the change does not trigger it again", "[engine_input]")
{
	Game game{ "games/breakout.xml" };
	Engine engine(game, std::make_unique<FakeWindow>());

	// Space is bound to a state change in mainmenu, playing and paused. Held
	// down, it must take mainmenu to playing once, not straight on to paused.
	engine.handleKeyPressed(KeyCode::Space);
	CHECK(game.getCurrentState().name == "playing");

	engine.handleKeyReleased(KeyCode::Space);
	CHECK(game.getCurrentState().name == "playing");
}

TEST_CASE("an ordinary press and release in one state is unchanged", "[engine_input]")
{
	Game game{ "games/breakout.xml" };
	Engine engine(game, std::make_unique<FakeWindow>());
	Object& player = game.getObject("player");

	tap(engine, KeyCode::Space); // mainmenu -> playing

	engine.handleKeyPressed(KeyCode::Right);
	CHECK(player.velocity.x == kStep);
	engine.handleKeyReleased(KeyCode::Right);
	CHECK(player.velocity.x == 0.0f);
}

TEST_CASE("a held fire key does not shoot again when its state returns", "[engine_input]")
{
	Game game{ "games/spaceinvaders.xml" };
	Engine engine(game, std::make_unique<FakeWindow>());
	Object& bullet = game.getObject("bullet");

	tap(engine, KeyCode::Space); // mainmenu -> playing

	engine.handleKeyPressed(KeyCode::Space); // fire
	REQUIRE(bullet.collisionData.enabled);

	// Used up: the shot is gone and the key is still down.
	bullet.collisionData.enabled = false;
	tap(engine, KeyCode::P); // pause
	tap(engine, KeyCode::P); // unpause

	CHECK_FALSE(bullet.collisionData.enabled);
}

TEST_CASE("the key that starts Space Invaders does not also fire a shot", "[engine_input]")
{
	Game game{ "games/spaceinvaders.xml" };
	Engine engine(game, std::make_unique<FakeWindow>());
	Object& bullet = game.getObject("bullet");

	// Space starts the game from the menu and fires in play. Held down through
	// the change, it must only start the game.
	engine.handleKeyPressed(KeyCode::Space);
	CHECK(game.getCurrentState().name == "playing");
	CHECK_FALSE(bullet.collisionData.enabled);

	engine.handleKeyReleased(KeyCode::Space);
	CHECK_FALSE(bullet.collisionData.enabled);
}

TEST_CASE("keys read by pump() wait for the next step", "[engine_input]")
{
	Game game{ "games/breakout.xml" };
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
	Game game{ "games/breakout.xml" };
	auto scripted = std::make_unique<ScriptedWindow>();
	ScriptedWindow& window = *scripted;
	Engine engine(game, std::move(scripted));

	CHECK(engine.isWindowOpen());

	window.open = false;
	CHECK_FALSE(engine.isWindowOpen());
}

TEST_CASE("every shipped game follows the shared key conventions", "[engine_input]")
{
	// Space starts, pauses and plays again (never Enter), and player one is on
	// W, A, S and D (never Q and Z). A game of one screen (pong_min) has nothing
	// for Space to do.
	int checked = 0;
	for (const auto& entry : std::filesystem::directory_iterator("games"))
	{
		if (entry.path().extension() != ".xml") { continue; }

		std::ifstream in(entry.path());
		const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

		INFO(entry.path().string());
		CHECK(text.find("button=\"enter\"") == std::string::npos);
		CHECK(text.find("button=\"q\"") == std::string::npos);
		CHECK(text.find("button=\"z\"") == std::string::npos);
		const bool oneScreen = text.find("<state ") == text.rfind("<state ");
		CHECK((oneScreen || text.find("button=\"space\"") != std::string::npos));
		++checked;
	}
	CHECK(checked >= 10);
}
