// verbs.cpp
// XML Game Engine
// author: beefviper
// date: Oct 12, 2026

#include "play/verbs.h"
#include "game/parts.h"

#include "game/command.h"
#include "game/random.h"
#include "play/geometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace xge
{
	Vector2f sizeOf(const Object& object)
	{
		if (object.size.x != 0.0f || object.size.y != 0.0f) { return object.size; }
		return measureShapeSize(object.drawn.picture);
	}

	// After `mover` has met `surface`, which does not move of itself
	// (going at `surfaceAt` where they touch, `out` the way out of it to
	// the mover), going at `before` as it came: its <friction>.
	// - Rubbing: its speed along the surface is cut by up to friction
	//   times the speed the touch took off straight into it.
	// - Settling: a touch slower than `settle` (knockedApart's) is it
	//   lying there. If what it lies on is still, and either its own pull
	//   cannot slide it along that surface (the pull along it no more than
	//   friction times the pull into it) or it lay on another surface a
	//   frame or two ago and its pull points between the two (wedged in a
	//   corner), it comes to rest: still, and not pulled, until something
	//   moves it or what it rests on moves (applyAcceleration).
	void rub(Object& mover, Object::Rest* rest, const Object& surface, const Vector2f& out, const Vector2f& surfaceAt, const Vector2f& before, float settle)
	{
		if (!rest || !(mover.motion.friction > 0.0f)) { return; }

		const Vector2f cameIn = before - surfaceAt;
		Vector2f now = mover.velocity - surfaceAt;
		const float pushed = dotOf(now, out) - dotOf(cameIn, out);
		const Vector2f along = now - out * dotOf(now, out);
		const float sliding = lengthOf(along);
		if (sliding > 0.0f && pushed > 0.0f)
		{
			now = now - along * (std::min(sliding, mover.motion.friction * pushed) / sliding);
		}
		mover.velocity = surfaceAt + now;

		if (!(-dotOf(cameIn, out) < settle)) { return; }

		// Settled on another within the last two sixtieths of a second.
		const bool another = rest->settledAge <= 2.0f / 60.0f + kTimeSlack && dotOf(rest->settledNormal, out) < 0.95f;
		const Vector2f other = rest->settledNormal;
		const Object::Support otherSupport = rest->settledOn;
		const Object::Support support{ surface.name, surface.position, surface.heading };
		rest->settledOn = support;
		rest->settledNormal = out;
		rest->settledAge = 0.0f;

		const bool still = surface.turning.spin == 0.0f && surfaceAt.x == 0.0f && surfaceAt.y == 0.0f;
		if (!still || !(lengthOf(now) < settle)) { return; }

		const Vector2f pull = mover.acceleration;
		const float into = -dotOf(pull, out);
		if (!(into > 0.0f)) { return; }
		rest->on.clear();
		if (lengthOf(pull + out * into) > mover.motion.friction * into)
		{
			if (!another) { return; }
			const Vector2f up = pull * -1.0f;
			const float first = crossOf(other, up);
			const float second = crossOf(up, out);
			if (!(dotOf(pull, other) < 0.0f && first != 0.0f && second != 0.0f && (first > 0.0f) == (second > 0.0f))) { return; }
			rest->on.push_back(otherSupport);
		}
		rest->on.push_back(support);

		mover.velocity = {};
		rest->resting = true;
		rest->at = mover.position;
	}

	// Only the touched edge's own axis is affected, so an object pressed
	// against the bottom wall still slides left or right, and one moving away
	// from the wall is not held back.
	void stick(Object& object, const WindowDesc& window, Edge edge)
	{
		stickInside(object.position, object.velocity, object.size, { window.width, window.height }, edge);
	}

}
