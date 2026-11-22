// test_whole_games.cpp
// XML Game Engine
// author: beefviper
// date: Nov 16, 2026
//
// Whole shipped games played from a fresh load at 8, 30, 60, 144 and 240
// steps a second and in uneven steps: nothing throws, everything stays at
// a finite place and speed, and one measure a game, in game seconds,
// agrees with the run at 60 within its range. Hidden in xgetest ([.long]).

#include "played.h"
#include "random_keys.h"
#include "game/random.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using played::Played;
using xge::KeyCode;

namespace
{
	// A game played in steps of one length (or uneven ones, `rate` 0), each
	// step checked for finite places and speeds.
	class Pacer
	{
	public:
		Pacer(Played& game, int stepsASecond) : play(game), rate(stepsASecond), pick(7) {}

		// Plays `seconds` more, cutting the last step so the total is exact;
		// `each` is asked after every step, and may stop the play early by
		// answering true.
		void advance(double seconds, const std::function<bool()>& each = {})
		{
			std::uniform_real_distribution<double> length(1.0 / 240.0, 0.1);
			const double end = played + seconds;
			while (end - played > 1e-6)
			{
				const double step = std::min(end - played, rate > 0 ? 1.0 / rate : length(pick));
				play.engine.input(); play.engine.update(static_cast<float>(step));
				played += step;
				for (const xge::Object& object : play.game.getCurrentObjects())
				{
					if (!played::finite(object))
					{
						FAIL_CHECK(object.name << " is not at a finite place after " << played << " seconds");
						return;
					}
				}
				if (each && each()) { return; }
			}
		}

		// The game seconds played so far.
		double now() const { return played; }

	private:
		Played& play;
		int rate;
		std::mt19937 pick;
		double played = 0.0;
	};

	// The rates every game is played at; 0 is uneven.
	const std::vector<int> kRates{ 8, 30, 60, 144, 240, 0 };

	std::string rateName(int rate) { return rate > 0 ? std::to_string(rate) + " a second" : std::string("uneven"); }

	// The game in `file` played at each rate by `run`, which gives its
	// measure; each must be within `within` of the run at 60 (`share`: as a
	// share of it). The numbers are printed, for the commit message.
	void agreeAtEveryRate(const std::string& file, const std::string& what, double within, bool share,
		const std::function<double(Played&, Pacer&)>& run)
	{
		std::vector<std::pair<int, double>> measured;
		for (const int rate : kRates)
		{
			INFO(file << " at " << rateName(rate));
			xge::fixRandomSeed(1);
			Played play{ file };
			Pacer pacer{ play, rate };
			measured.emplace_back(rate, run(play, pacer));
		}
		const double at60 = std::find_if(measured.begin(), measured.end(), [](const auto& each) { return each.first == 60; })->second;
		std::cout << file << ", " << what << ":";
		for (const auto& [rate, value] : measured) { std::cout << " " << rateName(rate) << " " << value << ";"; }
		std::cout << '\n';
		for (const auto& [rate, value] : measured)
		{
			INFO(file << ", " << what << ", at " << rateName(rate) << ": " << value << ", at 60: " << at60);
			CHECK(std::abs(value - at60) <= (share ? within * std::abs(at60) : within));
		}
	}

	xge::Vector2f middleOf(const xge::Object& object) { return object.position + object.size * 0.5f; }
}

TEST_CASE("Pong's first point comes as soon at every step length", "[.long][steps][pong]")
{
	// Two players, neither pressing a key: the computer's paddle, held to
	// a band about the ball, would send it back at angles that change with
	// where in the band it is at the touch.
	agreeAtEveryRate("descriptions/pong.xml", "seconds from the serve to the first point", 0.10, true, [](Played& play, Pacer& pacer)
	{
		play.tap(KeyCode::Num2);
		REQUIRE(play.state() == "playing");
		const auto scores = [&] { return play.get("paddle1").variable.at("score") + play.get("paddle2").variable.at("score"); };
		const float before = scores();
		const double start = pacer.now();
		pacer.advance(60.0, [&] { return scores() != before; });
		REQUIRE(scores() != before);
		return pacer.now() - start;
	});
}

TEST_CASE("Asteroids' ship goes as far after a second of thrust at every step length", "[.long][steps][asteroids]")
{
	agreeAtEveryRate("descriptions/asteroids.xml", "pixels gone in 3 seconds after 1 second of thrust", 0.05, true, [](Played& play, Pacer& pacer)
	{
		play.tap(KeyCode::Space);
		REQUIRE(play.state() == "playing");
		xge::Object& ship = play.get("ship");
		ship.collisionsOn = false;
		// Gone: each step's move added up, a wrap left out.
		double gone = 0.0;
		xge::Vector2f was = ship.position;
		const auto count = [&]
		{
			const xge::Vector2f moved = ship.position - was;
			const double step = std::hypot(moved.x, moved.y);
			if (step < 200.0) { gone += step; }
			was = ship.position;
			ship.collisionsOn = false;
			return false;
		};
		play.engine.handleKeyPressed(KeyCode::W);
		pacer.advance(1.0, count);
		play.engine.handleKeyReleased(KeyCode::W);
		gone = 0.0;
		pacer.advance(3.0, count);
		return gone;
	});
}

TEST_CASE("Mario's jump goes as high and is in the air as long at every step length", "[.long][steps][mariobros]")
{
	// From the ground between the low left floor and the POW, where nothing
	// is overhead for a whole jump. The landing is worked out inside the
	// step it came in, from where he was before it, his speed and his
	// pull (per frame in this game), so the time is not rounded to a step.
	const auto jump = [](Played& play, Pacer& pacer, bool height)
	{
		play.tap(KeyCode::Num1);
		REQUIRE(play.state() == "playing");
		xge::Object& mario = play.get("mario");
		mario.position.x = 100.0f;
		pacer.advance(0.5);
		const float ground = mario.position.y;
		float top = ground;
		play.tap(KeyCode::W);
		const double start = pacer.now();
		double before = start;
		xge::Vector2f was = mario.position;
		xge::Vector2f speed = mario.velocity;
		double landed = 0.0;
		pacer.advance(3.0, [&]
		{
			top = std::min(top, mario.position.y);
			if (mario.position.y >= ground && pacer.now() - start > 0.1)
			{
				const double pull = mario.acceleration.y;
				const double rise = static_cast<double>(ground - was.y);
				const double going = speed.y;
				const double rest = pull > 0.0 ? (-going + std::sqrt(going * going + 2.0 * pull * rise)) / pull : rise / going;
				landed = before - start + rest;
				return true;
			}
			before = pacer.now();
			was = mario.position;
			speed = mario.velocity;
			return false;
		});
		REQUIRE(landed > 0.0);
		return height ? static_cast<double>(ground - top) : landed;
	};
	agreeAtEveryRate("descriptions/mariobros.xml", "pixels high", 3.0, false, [&](Played& play, Pacer& pacer) { return jump(play, pacer, true); });
	agreeAtEveryRate("descriptions/mariobros.xml", "seconds in the air", 0.05, true, [&](Played& play, Pacer& pacer) { return jump(play, pacer, false); });
}

TEST_CASE("Space Invaders' block is in the same place after 10 seconds at every step length", "[.long][steps][spaceinvaders]")
{
	// Within one of its own steps: 15 pixels a note.
	agreeAtEveryRate("descriptions/spaceinvaders.xml", "the block's x after 10 seconds", 15.0, false, [](Played& play, Pacer& pacer)
	{
		play.game.pushState("playing");
		play.get("player").collisionsOn = false;
		pacer.advance(10.0, [&] { play.get("player").collisionsOn = false; return false; });
		return static_cast<double>(play.get("aliens.1.1").position.x);
	});
}

TEST_CASE("Snake's head has gone as many cells after 2.45 seconds at every step length", "[.long][steps][snake]")
{
	// 8 cells a second: 19 cells, short of the wall the field's right side
	// makes at 22.
	agreeAtEveryRate("descriptions/snake.xml", "cells gone", 0.0, false, [](Played& play, Pacer& pacer)
	{
		play.tap(KeyCode::Space);
		REQUIRE(play.state() == "playing");
		const float from = play.get("head").position.x;
		pacer.advance(2.45);
		return std::round(static_cast<double>(play.get("head").position.x - from) / 20.0);
	});
}

TEST_CASE("Pinball's ball goes as high from a full plunger at every step length", "[.long][steps][pinball]")
{
	agreeAtEveryRate("descriptions/pinball.xml", "pixels the ball rises", 0.05, true, [](Played& play, Pacer& pacer)
	{
		play.tap(KeyCode::Space);
		pacer.advance(0.05);
		play.engine.handleKeyPressed(KeyCode::Space);
		pacer.advance(1.5);
		const xge::Object& ball = play.get("ball");
		const float from = middleOf(ball).y;
		float top = from;
		play.engine.handleKeyReleased(KeyCode::Space);
		pacer.advance(1.5, [&] { top = std::min(top, middleOf(ball).y); return false; });
		return static_cast<double>(from) - static_cast<double>(top);
	});
}
