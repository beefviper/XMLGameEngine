// test_pong.cpp
// XML Game Engine
// author: beefviper
// date: Oct 15, 2026
//
// Catch2 tests for games/descriptions/pong.xml, played through a real
// xge::Engine with no window and the keys a player would press: each state
// has the objects it shows, and paused and game over draw the game still
// underneath (<show state="playing" />), and the ball goes faster at the
// fourth and twelfth hit. Its deflections and sounds are tested in
// test_deflect.cpp and test_sound.cpp.

#include "engine/engine.h"
#include "game/game.h"
#include "play/verbs.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace xge;
using Catch::Approx;

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

	struct Pong
	{
		Game game{ "descriptions/pong.xml" };
		Engine engine{ game, std::make_unique<FakeWindow>() };

		Pong()
		{
			for (auto& object : game.getCurrentObjects())
			{
				object.size = measureShapeSize(object.spriteParams, object.shapeKind);
			}
		}

		void tap(KeyCode key)
		{
			engine.handleKeyPressed(key);
			engine.handleKeyReleased(key);
			engine.step();
		}

		void frames(int count)
		{
			for (int i = 0; i < count; ++i) { engine.step(); }
		}

		Object& object(const std::string& name) { return game.getObject(name); }
		bool shown(const std::string& name) { return game.isShown(object(name)); }
		bool still(const std::string& name) { return game.isStill(object(name)); }
		std::string state() { return game.getCurrentState().name; }
	};
}

TEST_CASE("each state of Pong has the objects it shows", "[pong][scenes]")
{
	Pong pong;
	REQUIRE(pong.state() == "mainmenu");
	CHECK(pong.object("title").name == "mainmenu.title");
	CHECK(pong.object("ball").name == "playing.ball");
	CHECK(pong.object("paused").name == "paused.paused");
	CHECK(pong.object("paddle2").name == "playing.paddle2"); // a member of the paddles

	CHECK(pong.shown("title"));
	CHECK_FALSE(pong.shown("ball"));
	CHECK_FALSE(pong.still("ball"));

	pong.tap(KeyCode::Space);
	REQUIRE(pong.state() == "playing");
	for (const char* name : { "ball", "paddle1", "paddle2", "score1", "score2", "net.1.1" })
	{
		INFO(name);
		CHECK(pong.shown(name));
	}
	CHECK_FALSE(pong.shown("title"));
	CHECK_FALSE(pong.shown("paused"));
}

TEST_CASE("paused, Pong draws the game still underneath, and Space plays on", "[pong][scenes][shows]")
{
	Pong pong;
	pong.tap(KeyCode::Space);
	pong.frames(10);

	pong.tap(KeyCode::Space);
	REQUIRE(pong.state() == "paused");
	const Vector2f at = pong.object("ball").position;
	pong.frames(30);
	CHECK(pong.object("ball").position.x == at.x);
	CHECK(pong.object("ball").position.y == at.y);
	CHECK(pong.shown("paused"));
	for (const char* name : { "ball", "paddle1", "paddle2", "score1", "score2" })
	{
		INFO(name);
		CHECK_FALSE(pong.shown(name));
		CHECK(pong.still(name));
	}

	pong.tap(KeyCode::Space);
	REQUIRE(pong.state() == "playing");
	CHECK_FALSE(pong.still("ball"));
	CHECK(pong.object("ball").position.x != at.x);
}

TEST_CASE("at eleven, game over is written over the game still, and Space starts again", "[pong][scenes][shows]")
{
	Pong pong;
	pong.tap(KeyCode::Space);
	pong.object("paddle2").variable["score"] = 11;
	pong.frames(1);
	REQUIRE(pong.state() == "gameover");
	CHECK(pong.shown("gameover"));
	CHECK(pong.shown("continue"));
	CHECK(pong.still("ball"));
	CHECK(pong.still("score2"));

	pong.tap(KeyCode::Space);
	CHECK(pong.state() == "mainmenu");
	CHECK(pong.object("paddle2").variable.at("score") == 0);
	CHECK_FALSE(pong.still("ball"));
}
TEST_CASE("the ball is drawn as big as its own variable says", "[pong]")
{
	// Its <rectangle> reads ball.size, the ball's own <variable>; a size was
	// once worked out before the variable had its value, and drawn as 0.
	Pong pong;
	const Object& ball = pong.object("ball");
	CHECK(measureShapeSize(ball.spriteParams, ball.shapeKind).x == 16.0f);
	CHECK(measureShapeSize(ball.spriteParams, ball.shapeKind).y == 16.0f);
}

TEST_CASE("the fourth and the twelfth hit make Pong's ball faster, and a point serves it at its first speed", "[pong]")
{
	Pong pong;
	pong.tap(KeyCode::Space);
	Object& ball = pong.object("ball");
	const Object& paddle1 = pong.object("paddle1");
	const auto speed = [&] { return std::hypot(ball.velocity.x, ball.velocity.y); };

	std::vector<float> speeds;
	for (int hit = 1; hit <= 12; ++hit)
	{
		ball.position = { paddle1.position.x + paddle1.size.x + 4.0f, paddle1.position.y + paddle1.size.y / 2 - 8.0f };
		ball.velocity = { speeds.empty() ? -6.0f : -speeds.back(), 0.0f };
		pong.frames(2);
		speeds.push_back(speed());
	}
	CHECK(speeds[2] == Approx(6.0f));
	CHECK(speeds[3] == Approx(6.0f * 1.3f));
	CHECK(speeds[10] == Approx(6.0f * 1.3f));
	CHECK(speeds[11] == Approx(6.0f * 1.3f * 1.3f));
	CHECK(ball.variable.at("hits") == 12);

	ball.position = { 4.0f, 300.0f };
	ball.velocity = { -6.0f, 0.0f };
	pong.frames(3);
	CHECK(pong.object("paddle2").variable.at("score") == 1);
	CHECK(speed() == Approx(6.0f));
	CHECK(ball.variable.at("hits") == 0);
}
