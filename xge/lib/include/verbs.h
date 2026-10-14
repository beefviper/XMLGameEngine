// verbs.h
// XML Game Engine
// author: beefviper
// date: Oct 12, 2026

#pragma once

#include "object.h"
#include "types.h"

#include <string>
#include <vector>

namespace xge
{
	// What a verb does to an object, once something has decided it should
	// happen: no game file, no rules and no looking anything up, only the
	// object (and what it touched) changed. The engine's CommandExecutor
	// calls these when a game file's rule says so; a generated game calls
	// them straight from its own rules. The arithmetic is geometry.h's.

	// An object called `name` showing `sprite` (circleSprite, ...), in play
	// from the start, its collisions on or off (<enabled>). Where it starts
	// and how fast it goes are startAt's.
	Object makeObject(std::string name, std::vector<std::string> sprite, bool collides);

	// Where the object starts and how fast it goes, now and after a reset.
	void startAt(Object& object, Vector2f position, Vector2f velocity);

	// What an object measures: what the window measured, or, before there is
	// one, what its sprite implies.
	Vector2f sizeOf(const Object& object);

	// How far an object moves in one frame: its own velocity, plus the
	// velocity of whatever it is riding (see Object::riding).
	Vector2f motionOf(const Object& object) noexcept;

	// <bounce />: off the touched edge, the speed kept.
	void bounce(Object& object, Edge edge);

	// <deflect>maxDegrees</deflect>: back off `other` at an angle set by
	// where on the touched side it hit.
	void deflect(Object& object, const Object& other, Edge edge, float maxDegrees);

	// <stick />: pushed back inside the window on that edge's axis, losing
	// only the velocity carrying it into the edge.
	void stick(Object& object, const WindowDesc& window, Edge edge);

	// <wrap />: once right off through `edge`, back in from the other side.
	void wrap(Object& object, const WindowDesc& window, Edge edge);

	// <ride />: moves with `other` for this frame.
	void ride(Object& object, const Object& other);

	// <carry />: picks `item` up, held where it is now, putting down what
	// it carried before; nothing if it was just put down and is still
	// being touched.
	void carry(Object& object, const Object& item);

	// <drop />: puts down what it carries.
	void drop(Object& object);

	// <die />: out of play; not drawn, moved or collided with.
	void die(Object& object);

	// <stop />: at rest, with nothing left to move it.
	void stop(Object& object);

	// A bare <reset /> on an object: back where it started, facing and going
	// the way it started; a held key still decides its velocity. The start
	// is positionOriginal and velocityOriginal, so a start that draws a
	// <random> is drawn again before this.
	void restart(Object& object);

	// A direction held at `step` (0: let go), recombined with the others held
	// into the velocity: Left/Right and Up/Down are separate axes, and in a
	// <leap> the way across waits for the landing.
	void holdStep(Object& object, Direction direction, float step);

	// A <move> in an action, as its key goes down (pressed) or up: that
	// direction held at `step`, recombined with the others held into the
	// velocity, and the object faces that way.
	void holdMove(Object& object, Direction direction, float step, bool pressed);
}
