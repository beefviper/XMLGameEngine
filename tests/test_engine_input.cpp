// test_engine_input.cpp
// XML Game Engine
// author: beefviper
// date: Sept 29, 2026
//
// Catch2 tests for Engine's key handling (Engine::handleKeyPressed and
// handleKeyReleased) against a real xge::Game built from games/breakout.xml,
// driven through a fake Window so no real window is opened.
//
// Regression covered: a key's release used to be looked up in whichever
// state was active when it was let go, so holding Left, pausing, releasing
// Left and unpausing left the paddle moving forever - the paused state has
// no Left binding to hear the release.

#include "engine.h"
#include "game.h"

#include <catch2/catch_test_macros.hpp>

#include <memory>

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

	// breakout.xml's <variable name="step" value="2" />.
	constexpr float kStep = 2.0f;

	void tap(Engine& engine, KeyCode key)
	{
		engine.handleKeyPressed(key);
		engine.handleKeyReleased(key);
	}
}

TEST_CASE("releasing a key while paused still stops the paddle", "[engine_input]")
{
	Game game{ "games/breakout.xml" };
	Engine engine(game, std::make_unique<FakeWindow>());
	Object& player = game.getObject("player");

	tap(engine, KeyCode::Space); // mainmenu -> playing

	engine.handleKeyPressed(KeyCode::Left);
	REQUIRE(player.velocity.x == -kStep);

	tap(engine, KeyCode::Space); // pause
	engine.handleKeyReleased(KeyCode::Left);
	tap(engine, KeyCode::Space); // unpause

	CHECK(player.velocity.x == 0.0f);
}

TEST_CASE("a key held straight through pause and unpause keeps moving until released", "[engine_input]")
{
	Game game{ "games/breakout.xml" };
	Engine engine(game, std::make_unique<FakeWindow>());
	Object& player = game.getObject("player");

	tap(engine, KeyCode::Space); // mainmenu -> playing

	engine.handleKeyPressed(KeyCode::Left);
	tap(engine, KeyCode::Space); // pause
	tap(engine, KeyCode::Space); // unpause
	CHECK(player.velocity.x == -kStep);

	engine.handleKeyReleased(KeyCode::Left);
	CHECK(player.velocity.x == 0.0f);
}

TEST_CASE("a key pressed while paused does nothing, and its release is harmless", "[engine_input]")
{
	Game game{ "games/breakout.xml" };
	Engine engine(game, std::make_unique<FakeWindow>());
	Object& player = game.getObject("player");

	tap(engine, KeyCode::Space); // mainmenu -> playing
	tap(engine, KeyCode::Space); // pause

	engine.handleKeyPressed(KeyCode::Left); // no Left binding in "paused"
	engine.handleKeyReleased(KeyCode::Left);
	tap(engine, KeyCode::Space); // unpause

	CHECK(player.velocity.x == 0.0f);
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
