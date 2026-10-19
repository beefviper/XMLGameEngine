// frame.h
// XML Game Engine
// author: beefviper
// date: Oct 12, 2026

#pragma once

#include "game/object.h"
#include "game/types.h"
#include "play/geometry.h"

#include <cstddef>
#include <string>
#include <vector>

namespace xge
{
	// What a game says should happen, asked by playOneFrame as each moment
	// of the frame comes. The engine's Game answers by looking in the game
	// file (its rules, states and conditions); a generated game answers with
	// code written from the same file. Everything not asked here (the order,
	// the moving, the sweep, the pull, the arithmetic) is playOneFrame's own,
	// so the two cannot play differently except by answering differently.
	class FrameRules
	{
	public:
		virtual ~FrameRules() = default;

		// Whether the object is in play: shown in the current state and not
		// dead. Nothing else is animated, moved, pulled or touched.
		virtual bool inPlay(const Object& object) = 0;

		// The timers that go off this frame, before anything moves, so a bomb
		// dropped this frame starts falling this frame.
		virtual void timers() {}

		// Every object on a path takes this frame's step of it.
		virtual void followPaths() {}

		// A thruster burns one unit of the object's `variable`; false
		// (and nothing burnt) when there is none left. `verb` names the tag,
		// for a warning.
		virtual bool burn(Object& object, const std::string& variable, const char* verb)
		{
			(void)object; (void)variable; (void)verb;
			return true;
		}

		// The object, moving, is at or past this edge of the window, before
		// the frame's move: its rules for that edge.
		virtual void atEdge(Object& object, Edge edge) { (void)object; (void)edge; }

		// After the move: whether the object has a stick for this edge that
		// holds right now (it is put back inside even if it stopped moving).
		virtual bool sticksAt(const Object& object, Edge edge) { (void)object; (void)edge; return false; }

		// Whether a rule of a's answers to b, or one of b's to a. Asked only
		// of two that can touch at all (both colliding, neither in the air,
		// not one lockstep block). The answer may depend only on what an
		// object keeps for the whole game (its name, class, groups and rules):
		// playOneFrame asks once for each two objects and remembers it (see
		// partners).
		virtual bool canCollide(const Object& a, const Object& b) { (void)a; (void)b; return false; }

		// a and b have met during the move (contact.edgeOfSecond is the edge
		// of b that was hit, so a's is the opposite one; the rest of the
		// contact is where and how): both their rules for it.
		virtual void touched(Object& a, Object& b, const Contact& contact) { (void)a; (void)b; (void)contact; }

		// After everything has moved: the state's conditions.
		virtual void conditions() {}

		// For each object, the later ones (by index) a rule of either answers
		// to (canCollide), asked once for the whole game rather than for every
		// two objects every frame: most pairs (a wall and a wall) never can.
		// Asked again when `objects` is a different list.
		const std::vector<std::vector<std::size_t>>& partners(const std::vector<Object>& objects);

	private:
		std::vector<std::vector<std::size_t>> partnerLists;
		const Object* partnersOf{ nullptr };
		std::size_t partnersCount{ 0 };
	};

	// Plays one frame of a game, in the one order every game runs in:
	// animations, timers, paths, climbing, pull and thrust, the window edges
	// (before the move), the move (hops, then every pair that can touch swept
	// through the frame and met in the order they meet), sticks, what is
	// carried put where it is held, conditions.
	void playOneFrame(std::vector<Object>& objects, const WindowDesc& window, FrameRules& rules);
}
