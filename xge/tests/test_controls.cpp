// test_controls.cpp
// XML Game Engine
// author: beefviper
// date: Oct 16, 2026
//
// Catch2 tests for the mouse and the gamepads: their buttons pressed and
// released as keys are (pad1.a, mouse.left), a pad's stick as its
// directions, and <input axis="..."> with <track /> (Kaboom's bucket on the
// mouse, as the Atari's paddle). Engine::handleControls is given what a
// window would report.

#include "engine/engine.h"
#include "game/game.h"
#include "engine/keys.h"
#include "play/verbs.h"
#include "window/window.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace xge;
using Catch::Matchers::ContainsSubstring;

namespace
{
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

	Controls mouseAt(float x, float y)
	{
		Controls controls;
		controls.pointerInside = true;
		controls.pointerX = x;
		controls.pointerY = y;
		return controls;
	}

	Controls padWith(PadButton button)
	{
		Controls controls;
		controls.pads[0].connected = true;
		controls.pads[0].buttons[static_cast<std::size_t>(button)] = true;
		return controls;
	}

	// A file that is removed again when the test ends.
	struct ScratchGame
	{
		std::filesystem::path path{ "descriptions/controls_scratch.xml" };

		explicit ScratchGame(const std::string& xml)
		{
			std::ofstream out(path);
			out << xml;
		}

		~ScratchGame()
		{
			std::error_code ignored;
			std::filesystem::remove(path, ignored);
		}

		ScratchGame(const ScratchGame&) = delete;
		ScratchGame& operator=(const ScratchGame&) = delete;
	};

	// A one-state game whose <inputs> are `inputs`.
	std::string gameWith(const std::string& inputs)
	{
		return R"(<game>
  <window name="t"><width>200</width><height>100</height><background>color.black</background><fullscreen>false</fullscreen><framerate>60</framerate></window>
  <variables />
  <states>
    <state name="playing">
      <objects>
        <object name="paddle">
          <sprite><rectangle><width>40</width><height>10</height><color>color.white</color></rectangle></sprite>
          <position><x>0</x><y>0</y></position>
          <actions><action name="steer"><track /></action><action name="right"><move direction="right">2</move></action></actions>
        </object>
      </objects>
      <inputs>)" + inputs + R"(</inputs>
    </state>
  </states>
</game>
)";
	}

	std::string loadError(const std::string& xml)
	{
		ScratchGame file(xml);
		try
		{
			Game game(file.path.string());
		}
		catch (const std::exception& error)
		{
			return error.what();
		}
		return {};
	}
}

TEST_CASE("the mouse's and the pads' buttons and the controls that point have names", "[controls]")
{
	CHECK(keyCodeFromString("mouse.left") == KeyCode::MouseLeft);
	CHECK(keyCodeFromString("pad1.a") == KeyCode::Pad1A);
	CHECK(keyCodeFromString("pad2.start") == KeyCode::Pad2Start);
	CHECK(keyCodeToString(padKey(1, PadButton::Left)) == "pad2.left");
	CHECK(axisFromString("mouse.x") == Axis::MouseX);
	CHECK(axisFromString("pad2.y") == Axis::Pad2Y);
	CHECK(axisFromString("mouse.z") == Axis::Count);
	CHECK(axisToString(Axis::Pad1X) == "pad1.x");
}

TEST_CASE("a pad's start button starts Kaboom and its d-pad moves the bucket", "[controls]")
{
	Game game{ "descriptions/kaboom.xml" };
	Engine engine(game, std::make_unique<FakeWindow>());
	Object& bucket = game.getObject("bucket");

	engine.handleControls(Controls{});  // the first look only notes where things are
	engine.handleControls(padWith(PadButton::Start));
	engine.handleControls(Controls{});
	REQUIRE(game.getCurrentState().name == "playing");

	engine.handleControls(padWith(PadButton::Left));
	CHECK(bucket.velocity.x < 0.0f);
	engine.handleControls(Controls{});
	CHECK(bucket.velocity.x == 0.0f);
}

TEST_CASE("a pad's stick pushed past halfway is its direction", "[controls]")
{
	Game game{ "descriptions/kaboom.xml" };
	Engine engine(game, std::make_unique<FakeWindow>());
	Object& bucket = game.getObject("bucket");
	engine.handleControls(Controls{});
	engine.handleKeyPressed(KeyCode::Space);
	engine.handleKeyReleased(KeyCode::Space);
	REQUIRE(game.getCurrentState().name == "playing");

	// Kaboom also tracks pad1.x as a paddle; the stick held right moves the
	// bucket there and holds the right direction down.
	Controls pushed;
	pushed.pads[0].connected = true;
	pushed.pads[0].x = 0.4f;
	engine.handleControls(pushed);
	CHECK(bucket.velocity.x == 0.0f);
	pushed.pads[0].x = 0.9f;
	engine.handleControls(pushed);
	CHECK(bucket.velocity.x > 0.0f);
}

TEST_CASE("the mouse moves Kaboom's bucket across the window as the paddle did", "[controls]")
{
	Game game{ "descriptions/kaboom.xml" };
	Engine engine(game, std::make_unique<FakeWindow>());
	Object& bucket = game.getObject("bucket");
	const float width = game.getWindowDesc().width;
	const float start = bucket.position.x;

	// Resting where it was when the game started, the mouse moves nothing.
	engine.handleControls(mouseAt(30.0f, 300.0f));
	engine.handleKeyPressed(KeyCode::Space);
	engine.handleKeyReleased(KeyCode::Space);
	REQUIRE(game.getCurrentState().name == "playing");
	engine.handleControls(mouseAt(30.0f, 300.0f));
	CHECK(bucket.position.x == start);

	// Moved, the bucket's middle goes to it...
	engine.handleControls(mouseAt(300.0f, 300.0f));
	CHECK(bucket.position.x + sizeOf(bucket).x / 2 == Catch::Approx(300.0f));

	// ...and stays whole inside the window at either side.
	engine.handleControls(mouseAt(1.0f, 300.0f));
	CHECK(bucket.position.x == 0.0f);
	engine.handleControls(mouseAt(width - 1.0f, 300.0f));
	CHECK(bucket.position.x + sizeOf(bucket).x == Catch::Approx(width));

	// Off the window, it is left where it was.
	const float before = bucket.position.x;
	Controls away;
	engine.handleControls(away);
	CHECK(bucket.position.x == before);
}

TEST_CASE("a mouse button is a key", "[controls]")
{
	ScratchGame file(gameWith(R"(<input button="mouse.left"><trigger object="paddle" action="right" /></input>)"));
	Game game(file.path.string());
	Engine engine(game, std::make_unique<FakeWindow>());
	Object& paddle = game.getObject("paddle");

	engine.handleControls(Controls{});
	Controls down;
	down.mouse[0] = true;
	engine.handleControls(down);
	CHECK(paddle.velocity.x == 2.0f);
	engine.handleControls(Controls{});
	CHECK(paddle.velocity.x == 0.0f);
}

TEST_CASE("a vertical control tracks up and down", "[controls]")
{
	ScratchGame file(gameWith(R"(<input axis="mouse.y"><trigger object="paddle" action="steer" /></input>)"));
	Game game(file.path.string());
	Engine engine(game, std::make_unique<FakeWindow>());
	Object& paddle = game.getObject("paddle");

	engine.handleControls(mouseAt(10.0f, 10.0f));
	engine.handleControls(mouseAt(10.0f, 50.0f));
	CHECK(paddle.position.y == Catch::Approx(45.0f));
	CHECK(paddle.position.x == 0.0f);
}

TEST_CASE("a control that points is checked when the game loads", "[controls]")
{
	CHECK_THAT(loadError(gameWith(R"(<input axis="mouse.z"><trigger object="paddle" action="steer" /></input>)")),
		ContainsSubstring("no control named 'mouse.z'"));
	CHECK_THAT(loadError(gameWith(R"(<input axis="mouse.x"><push state="playing" /></input>)")),
		ContainsSubstring("does nothing on a control that points"));
	CHECK_THAT(loadError(gameWith(R"(<input button="space" axis="mouse.x"><trigger object="paddle" action="steer" /></input>)")),
		ContainsSubstring("one of the two"));
	CHECK_THAT(loadError(gameWith(R"(<input button="pad3.a"><trigger object="paddle" action="steer" /></input>)")),
		ContainsSubstring("no key named 'pad3.a'"));
}
