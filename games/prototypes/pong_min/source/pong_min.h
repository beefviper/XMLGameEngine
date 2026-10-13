// pong_min.h
// XML Game Engine
// author: beefviper
// date: Oct 12, 2026
//
// Pong, from games/descriptions/pong_min.xml: the game the file describes, written as
// code. This is what the windows-cpp generator should write (a prototype,
// written by hand for now). It decides what happens; the engine's own
// modules do it: playOneFrame (frame.h) runs the frame in the engine's
// order, the verbs (verbs.h) and the arithmetic (geometry.h) move things,
// and SFMLWindow draws them.

#pragma once

#include "frame.h"
#include "keycode.h"
#include "object.h"

#include <cstddef>
#include <vector>

namespace pong_min
{
	class Pong : public xge::FrameRules
	{
	public:
		// The game as it starts.
		Pong();

		// <window>
		xge::WindowDesc window;

		// <objects>, in the order the file has them, which is the order they
		// are moved, swept and drawn in.
		std::vector<xge::Object> objects;

		// A key went down (pressed) or came up: the state's <input>.
		void key(xge::KeyCode key, bool pressed);

		// One frame of play.
		void frame();

		// FrameRules: what pong_min.xml says should happen.
		bool inPlay(const xge::Object& object) override;
		void atEdge(xge::Object& object, xge::Edge edge) override;
		bool sticksAt(const xge::Object& object, xge::Edge edge) override;
		bool canCollide(const xge::Object& a, const xge::Object& b) override;
		void touched(xge::Object& a, xge::Object& b, xge::Edge edgeOfB) override;

	private:
		enum : std::size_t { Ball, Paddle1, Paddle2 };
		xge::Object& ball() { return objects[Ball]; }
		xge::Object& paddle1() { return objects[Paddle1]; }
		xge::Object& paddle2() { return objects[Paddle2]; }

		// The ball's start: its <random> variables drawn, then where it goes
		// and how fast from them. Again for every serve.
		void drawBallStart();

		// <actions>
		void paddle1Up(bool pressed);
		void paddle1Down(bool pressed);
		void paddle2Up(bool pressed);
		void paddle2Down(bool pressed);
	};
}
