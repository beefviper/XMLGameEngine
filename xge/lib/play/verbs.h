// verbs.h
// XML Game Engine
// author: beefviper
// date: Oct 12, 2026

#pragma once

#include "game/object.h"
#include "game/types.h"
#include "play/geometry.h"

#include <string>
#include <vector>

namespace xge
{
	// What a verb does to an object, once something has decided it should
	// happen: no game file, no rules and no looking anything up, only the
	// object (and what it touched) changed. The engine's CommandExecutor
	// calls these when a game file's rule says so; a generated game calls
	// them straight from its own rules. The arithmetic is play/geometry.h's.

	// An object called `name` showing `sprite` (circleSprite, ...), in play
	// from the start, its collisions on or off (<enabled>). Where it starts
	// and how fast it goes are startAt's.
	Object makeObject(std::string name, std::vector<std::string> sprite, bool collides);

	// Where the object starts and how fast it goes, now and after a reset.
	void startAt(Object& object, Vector2f position, Vector2f velocity);

	// What an object measures: what the window measured, or, before there is
	// one, what its sprite implies.
	Vector2f sizeOf(const Object& object);

	// The middle of what an object measures.
	inline Vector2f middleOf(const Object& object) { return object.position + sizeOf(object) * 0.5f; }

	// The object of that name or group whose middle is nearest `from`'s,
	// leaving `from` out, among those `inPlay` takes; null if there is none.
	// What <chase>, <aim>, <hunt>, <pull>, <seek> and distance= look for.
	template <typename InPlay>
	const Object* nearestInPlay(const std::vector<Object>& objects, const Object& from, const std::string& name, InPlay inPlay)
	{
		const Vector2f here = middleOf(from);
		const Object* nearest = nullptr;
		float nearestApart = 0.0f;
		for (const Object& other : objects)
		{
			if (&other == &from || !other.named(name) || !inPlay(other)) { continue; }
			const Vector2f apart = middleOf(other) - here;
			const float distance = apart.x * apart.x + apart.y * apart.y;
			if (!nearest || distance < nearestApart)
			{
				nearest = &other;
				nearestApart = distance;
			}
		}
		return nearest;
	}

	// How far an object moves in one frame: its own velocity, plus the
	// velocity of whatever it is riding (see Object::riding).
	inline Vector2f motionOf(const Object& object) noexcept
	{
		return object.velocity + object.riding;
	}

	// <bounce />: off the touched edge, the speed kept.
	void bounce(Object& object, Edge edge);

	// <deflect>maxDegrees</deflect>: back off `other` at an angle set by
	// where on the touched side it hit.
	void deflect(Object& object, const Object& other, Edge edge, float maxDegrees);

	// <knock>: the object knocks `other`, both with a mass (see
	// knockedApart); `touch` is the object's own view of it. Two that went
	// into each other are put apart along the touch, the lighter moved
	// more. Whether they were coming together.
	bool knock(Object& object, Object& other, const Touch& touch, float bounce);

	// <hole>: clears the pixels of the object's own picture within `radius`
	// of where `other` struck it - the solid pixel nearest the middle of
	// `other`, within a pixel or two of it, or that middle when there is
	// none - each at the edge kept at random by up to `rough` of the
	// radius. So shot after shot up one line goes deeper and deeper. The
	// first hole keeps the whole picture in Object::unholed for a reset; the
	// picture is copied, never changed where other objects share it.
	void cutHole(Object& object, const Object& other, float radius, float rough);

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

	// <track />: the object's middle goes to `fraction` (0 to 1) of the way
	// across `span` (the window's width, or its height when not
	// `horizontal`), kept wholly inside it. Its speed is left alone.
	void track(Object& object, bool horizontal, float fraction, float span);

	// <track path="...">: the object's middle `fraction` (0 to 1, held to
	// that) of the way along the path, measured along its legs from its
	// <start>; a <home /> leg goes back to the start.
	void trackPath(Object& object, const Path& path, float fraction);

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
	// velocity, and the object faces that way: with a key of the other axis
	// held too, the diagonal between them, which its <fire>s follow.
	void holdMove(Object& object, Direction direction, float step, bool pressed);
}
