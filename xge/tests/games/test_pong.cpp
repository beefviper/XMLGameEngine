// test_pong.cpp
// XML Game Engine
// author: beefviper
// date: Oct 15, 2026
//
// Catch2 tests for games/descriptions/pong.xml, played through a real
// xge::Engine with no window and the keys a player would press: each state
// has the objects it shows, and paused and game over draw the game still
// underneath (<show state="playing" />), and the ball goes faster at the
// fourth and twelfth hit, and the computer plays the right paddle, easy or
// smart, and can be beaten. Its deflections and sounds are tested in
// test_deflect.cpp and test_sound.cpp.

#include "engine/engine.h"
#include "game/game.h"
#include "play/verbs.h"
#include "window/null.h"

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
	struct Pong
	{
		Game game{ "descriptions/pong.xml" };
		Engine engine{ game, std::make_unique<NullWindow>() };

		Pong()
		{
			for (auto& object : game.getCurrentObjects())
			{
				object.size = measureShapeSize(object.picture);
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

		// The ball sent from just off the left paddle's face at `from` toward
		// `to` at that speed; then whether the computer sends it back
		// (true) or lets it past for a point (false).
		bool returns(Vector2f from, Vector2f to, float speed)
		{
			Object& ball = object("ball");
			Object& left = object("paddle1");
			const int before = static_cast<int>(left.variable.at("score"));
			ball.position = from;
			const Vector2f way{ to.x - from.x, to.y - from.y };
			const float length = std::hypot(way.x, way.y);
			ball.velocity = { way.x / length * speed, way.y / length * speed };
			for (int i = 0; i < 400; ++i)
			{
				frames(1);
				// A point serves the ball again, maybe to the left: the
				// point first.
				if (static_cast<int>(left.variable.at("score")) != before) { return false; }
				if (ball.velocity.x < 0.0f) { return true; }
			}
			return false;
		}

		// The computer's paddle with its top at y, still.
		void paddleAt(float y)
		{
			Object& right = object("paddle2");
			right.position.y = y;
			right.velocity = {};
		}

		float middle(const std::string& name) { return object(name).position.y + object(name).size.y / 2; }
	};

	// The ball's top left, as it leaves the left paddle's face near the top,
	// and where it goes: the right paddle's face at the bottom.
	const Vector2f fromTop{ 52.0f, 24.0f };
	const Vector2f toBottomCorner{ 1214.0f, 700.0f };
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
	CHECK(measureShapeSize(ball.picture).x == 16.0f);
	CHECK(measureShapeSize(ball.picture).y == 16.0f);
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
		ball.velocity = { speeds.empty() ? -9.0f : -speeds.back(), 0.0f };
		pong.frames(2);
		speeds.push_back(speed());
	}
	CHECK(speeds[2] == Approx(9.0f));
	CHECK(speeds[3] == Approx(9.0f * 1.3f));
	CHECK(speeds[10] == Approx(9.0f * 1.3f));
	CHECK(speeds[11] == Approx(9.0f * 1.3f * 1.3f));
	CHECK(ball.variable.at("hits") == 12);

	ball.position = { 4.0f, 300.0f };
	ball.velocity = { -6.0f, 0.0f };
	pong.frames(3);
	CHECK(pong.object("paddle2").variable.at("score") == 1);
	CHECK(speed() == Approx(9.0f));
	CHECK(ball.variable.at("hits") == 0);
}

TEST_CASE("Space plays the computer, which keeps the right paddle under the ball; 2 is two players", "[pong][computer]")
{
	{
		Pong pong;
		pong.tap(KeyCode::Space);
		REQUIRE(pong.state() == "playing");
		CHECK(pong.object("computer").variable.at("players") == 1);
		Object& ball = pong.object("ball");
		ball.position = { 600.0f, 600.0f };
		ball.velocity = { 3.0f, 0.0f };
		pong.frames(80);
		CHECK(std::abs(pong.middle("paddle2") - pong.middle("ball")) <= 9.0f);
	}
	{
		Pong pong;
		pong.tap(KeyCode::Num2);
		REQUIRE(pong.state() == "playing");
		CHECK(pong.object("computer").variable.at("players") == 2);
		const float y = pong.object("paddle2").position.y;
		Object& ball = pong.object("ball");
		ball.position = { 600.0f, 600.0f };
		ball.velocity = { 3.0f, 0.0f };
		pong.frames(80);
		CHECK(pong.object("paddle2").position.y == y);
	}
}

TEST_CASE("Pong's easy computer is beaten by a ball from the top corner to the bottom one, once it is fast", "[pong][computer]")
{
	Pong pong;
	pong.tap(KeyCode::Space);
	// At the first speed it gets there; at the fourth hit's it does not.
	pong.paddleAt(0.0f);
	CHECK(pong.returns(fromTop, toBottomCorner, 9.0f));
	pong.paddleAt(0.0f);
	CHECK_FALSE(pong.returns(fromTop, toBottomCorner, 9.0f * 1.3f));
}

TEST_CASE("Pong's smart computer goes back to the middle, and is beaten only by the fastest corner to corner", "[pong][computer]")
{
	Pong pong;
	pong.tap(KeyCode::A);
	pong.tap(KeyCode::D);
	CHECK(pong.object("computer").lookName() == "smart");
	pong.tap(KeyCode::Space);
	REQUIRE(pong.object("computer").variable.at("level") == 2);

	// The ball going away: back to the middle.
	pong.paddleAt(0.0f);
	Object& ball = pong.object("ball");
	ball.position = { 1000.0f, 100.0f };
	ball.velocity = { -3.0f, 0.0f };
	pong.frames(80);
	CHECK(std::abs(pong.middle("paddle2") - 360.0f) <= 9.0f);

	// From the middle it meets the corner to corner shot at the fourth hit's
	// speed, but not at the twelfth's.
	CHECK(pong.returns(fromTop, toBottomCorner, 9.0f * 1.3f));
	pong.paddleAt(300.0f);
	CHECK_FALSE(pong.returns(fromTop, toBottomCorner, 9.0f * 1.3f * 1.3f));
}

TEST_CASE("Pong's computer choice is kept from one game to the next", "[pong][computer]")
{
	Pong pong;
	pong.tap(KeyCode::D);
	pong.tap(KeyCode::Space);
	pong.game.setVariable("paddle1", "score", 11.0f);
	pong.frames(2);
	REQUIRE(pong.state() == "gameover");
	pong.tap(KeyCode::Space);
	CHECK(pong.state() == "mainmenu");
	CHECK(pong.object("computer").variable.at("level") == 2);
	CHECK(pong.object("computer").lookName() == "smart");
}
