// verbs.h
// XML Game Engine
// author: beefviper
// date: Oct 12, 2026

#pragma once

#include "engine/resources.h"
#include "game/object.h"
#include "game/parts.h"
#include "game/types.h"
#include "play/geometry.h"

#include <string>
#include <vector>

namespace xge
{
	struct Parts;

	// What more than one place does to an object: several verbs, or a verb
	// and the frame. A verb's own work is in its file in verbs/; the
	// arithmetic is play/geometry.h's.

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
			const float distance = dotOf(apart, apart);
			if (!nearest || distance < nearestApart)
			{
				nearest = &other;
				nearestApart = distance;
			}
		}
		return nearest;
	}

	// How far an object moves in a step `t` seconds long: its own velocity times t (for a stepped
	// animation, only on the step its picture changes, by the time saved up
	// since: Object::Animation::stepped), plus the velocity of whatever it is
	// riding (see Object::riding) times t. With t of 1, how fast it goes.
	inline Vector2f motionOf(const Object& object, float t) noexcept
	{
		return (object.animation.frames.shows().stepped ? object.velocity * object.animation.share : object.velocity * t) + object.riding * t;
	}

	// How far the sweep moves it in a step `t` long: motionOf, less half of
	// what its pull gained it this step (Object::Motion::gained), so that a
	// constant pull moves it v t + a t t / 2 however the time is cut.
	inline Vector2f sweptMotion(const Object& object, float t) noexcept
	{
		if (object.animation.frames.shows().stepped) { return motionOf(object, t); }
		return motionOf(object, t) - object.motion.gained * (t * 0.5f);
	}

	// After `mover` has met `surface`, which does not move of itself (going
	// at `surfaceAt` where they touch, `out` the way out of it to the
	// mover), going at `before` as it came: its <friction> rubs it along the
	// surface and may bring it to rest there (Object::Rest). What a <bounce>
	// and a <knock> share.
	void rub(Object& mover, Object::Rest* rest, const Object& surface, const Vector2f& out, const Vector2f& surfaceAt, const Vector2f& before, float settle);

	// <stick />: pushed back inside the window on that edge's axis, losing
	// only the velocity carrying it into the edge.
	void stick(Object& object, const WindowDesc& window, Edge edge);
}
