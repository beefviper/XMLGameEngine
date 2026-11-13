// pong_min.h
// XML Game Engine
// author: beefviper
// date: Oct 12, 2026
//
// Pong, from pong_min.xml.
//
// Written by xgecli --generate windows-cpp as a person would write it. It
// decides what happens; the engine's own small functions do it: playOneFrame
// (play/frame.h) runs the frame in the engine's order and the verbs
// (play/verbs.h) move things. No Game, no reader, no XML.

#pragma once

#include "engine/keys.h"
#include "game/object.h"
#include "play/frame.h"

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

		// Its objects, in the file's order, which is the order they are
		// moved, met and drawn in.
		std::vector<xge::Object> objects;

		// A key went down (pressed) or came up: the state's <inputs>.
		void key(xge::KeyCode key, bool pressed);

		// One frame of play.
		void frame();

		// What pong_min.xml says should happen, as playOneFrame asks.
		bool inPlay(const xge::Object& object) override;
		void atEdge(xge::Object& object, xge::Edge edge) override;
		bool sticksAt(const xge::Object& object, xge::Edge edge) override;
		bool canCollide(const xge::Object& a, const xge::Object& b) override;
		bool hasRules(const xge::Object& object) override;
		void touched(xge::Object& a, xge::Object& b, const xge::Contact& contact) override;

	private:
		xge::Object& ball() { return objects[0]; }
		xge::Object& paddle1() { return objects[1]; }
		xge::Object& paddle2() { return objects[2]; }
		bool isBall(const xge::Object& object) const { return &object == &objects[0]; }
		bool inPaddles(const xge::Object& object) const { return &object == &objects[1] || &object == &objects[2]; }

		// Where the ball starts: its <random>s drawn, then its place and way.
		void ballStart();
	};
}
