// verbs.cpp
// XML Game Engine
// author: beefviper
// date: Oct 12, 2026

#include "play/verbs.h"

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
		return measureShapeSize(object.picture);
	}

	void cutHole(Object& object, const Object& other, float radius, float rough)
	{
		if (!object.bitmap) { return; }

		auto cut = std::make_shared<Bitmap>(*object.bitmap);

		// Where it struck: the solid pixel nearest the other's middle, near
		// enough the other to be touched by it.
		const Vector2f otherSize = sizeOf(other);
		Vector2f middle = other.position + otherSize * 0.5f - object.position;
		{
			const Vector2f from = other.position - object.position;
			const int reach = 2;
			float best = -1.0f;
			Vector2f nearest = middle;
			const int x0 = std::max(0, static_cast<int>(std::floor(from.x)) - reach);
			const int y0 = std::max(0, static_cast<int>(std::floor(from.y)) - reach);
			const int x1 = std::min(cut->width - 1, static_cast<int>(std::ceil(from.x + otherSize.x)) + reach);
			const int y1 = std::min(cut->height - 1, static_cast<int>(std::ceil(from.y + otherSize.y)) + reach);
			for (int y = y0; y <= y1; ++y)
			{
				for (int x = x0; x <= x1; ++x)
				{
					if (!cut->solidAt(x, y)) { continue; }
					const Vector2f at{ static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f };
					const float away = (at.x - middle.x) * (at.x - middle.x) + (at.y - middle.y) * (at.y - middle.y);
					if (best < 0.0f || away < best) { best = away; nearest = at; }
				}
			}
			middle = nearest;
		}
		const int left = std::max(0, static_cast<int>(std::floor(middle.x - radius)));
		const int top = std::max(0, static_cast<int>(std::floor(middle.y - radius)));
		const int right = std::min(cut->width - 1, static_cast<int>(std::ceil(middle.x + radius)));
		const int bottom = std::min(cut->height - 1, static_cast<int>(std::ceil(middle.y + radius)));
		bool changed = false;
		for (int y = top; y <= bottom; ++y)
		{
			for (int x = left; x <= right; ++x)
			{
				const float dx = static_cast<float>(x) + 0.5f - middle.x;
				const float dy = static_cast<float>(y) + 0.5f - middle.y;
				const float reach = rough > 0.0f ? radius * (1.0f - rough * randomBetween(0.0f, 1.0f)) : radius;
				if (dx * dx + dy * dy > reach * reach || !cut->solidAt(x, y)) { continue; }
				cut->rgba[(static_cast<std::size_t>(y) * static_cast<std::size_t>(cut->width) + static_cast<std::size_t>(x)) * 4 + 3] = 0;
				changed = true;
			}
		}
		if (!changed) { return; }

		if (!object.unholed) { object.unholed = object.bitmap; }
		object.bitmap = std::move(cut);
		object.visualDirty = true;
	}

	namespace
	{
		float dotOf(const Vector2f& a, const Vector2f& b) noexcept { return a.x * b.x + a.y * b.y; }
		float crossOf(const Vector2f& a, const Vector2f& b) noexcept { return a.x * b.y - a.y * b.x; }
		float lengthOf(const Vector2f& a) noexcept { return std::hypot(a.x, a.y); }

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
		void rub(Object& mover, const Object& surface, const Vector2f& out, const Vector2f& surfaceAt, const Vector2f& before, float settle)
		{
			if (!(mover.friction > 0.0f)) { return; }

			const Vector2f cameIn = before - surfaceAt;
			Vector2f now = mover.velocity - surfaceAt;
			const float pushed = dotOf(now, out) - dotOf(cameIn, out);
			const Vector2f along = now - out * dotOf(now, out);
			const float sliding = lengthOf(along);
			if (sliding > 0.0f && pushed > 0.0f)
			{
				now = now - along * (std::min(sliding, mover.friction * pushed) / sliding);
			}
			mover.velocity = surfaceAt + now;

			if (!(-dotOf(cameIn, out) < settle)) { return; }

			const bool another = mover.settledAge <= 2 && dotOf(mover.settledNormal, out) < 0.95f;
			const Vector2f other = mover.settledNormal;
			const Object::Support otherSupport = mover.settledOn;
			const Object::Support support{ surface.name, surface.position, surface.heading };
			mover.settledOn = support;
			mover.settledNormal = out;
			mover.settledAge = 0;

			const bool still = surface.spin == 0.0f && surfaceAt.x == 0.0f && surfaceAt.y == 0.0f;
			if (!still || !(lengthOf(now) < settle)) { return; }

			const Vector2f pull = mover.acceleration;
			const float into = -dotOf(pull, out);
			if (!(into > 0.0f)) { return; }
			mover.restingOn.clear();
			if (lengthOf(pull + out * into) > mover.friction * into)
			{
				if (!another) { return; }
				const Vector2f up = pull * -1.0f;
				const float first = crossOf(other, up);
				const float second = crossOf(up, out);
				if (!(dotOf(pull, other) < 0.0f && first != 0.0f && second != 0.0f && (first > 0.0f) == (second > 0.0f))) { return; }
				mover.restingOn.push_back(otherSupport);
			}
			mover.restingOn.push_back(support);

			mover.velocity = {};
			mover.resting = true;
			mover.restedAt = mover.position;
		}
	}

	bool knock(Object& object, Object& other, const Touch& touch, float bounce)
	{
		// Each one's speed at the point they touch: a thing turning about
		// its pivot (a flipper swinging) moves faster the further out the
		// point lies, and lends that speed (Object::velocityAt).
		const Vector2f way = touch.normal * -1.0f;
		const Vector2f objectAt = object.velocityAt(touch.point);
		const Vector2f otherAt = other.velocityAt(touch.point);
		// Against one that does not move, the one that does settles on it.
		const float settle = other.mass <= 0.0f ? settleSpeed(object.acceleration, way)
			: object.mass <= 0.0f ? settleSpeed(other.acceleration, way * -1.0f) : 0.0f;
		const std::optional<Knocked> after = knockedApart(objectAt, object.mass, otherAt, other.mass, way, bounce, settle);

		// Gone into each other: apart again, each by its share, whether or not
		// they are still closing (a ball the frame's sweep left inside a
		// flipper turning away from it is not left there).
		if (touch.depth > 0.0f && (object.mass > 0.0f || other.mass > 0.0f))
		{
			const float objectShare = object.mass <= 0.0f ? 0.0f : other.mass <= 0.0f ? 1.0f : other.mass / (object.mass + other.mass);
			object.position -= way * (touch.depth * objectShare);
			other.position += way * (touch.depth * (1.0f - objectShare));
		}

		if (!after)
		{
			return false;
		}

		const Vector2f objectWas = object.velocity;
		const Vector2f otherWas = other.velocity;
		object.velocity += after->first - objectAt;
		other.velocity += after->second - otherAt;

		// One against something that does not move: its friction.
		if (other.mass <= 0.0f && object.mass > 0.0f) { rub(object, other, touch.normal, otherAt, objectWas, settle); }
		else if (object.mass <= 0.0f && other.mass > 0.0f) { rub(other, object, way, objectAt, otherWas, settle); }

		return true;
	}

	void bounce(Object& object, const Object& other, const Touch& touch, float share)
	{
		// The way from the object into the other, a surface standing still:
		// only the object's speed along that way changes. (A surface that
		// moves and lends its speed is a <knock>.)
		const Vector2f way = touch.normal * -1.0f;
		const float settle = settleSpeed(object.acceleration, way);
		const Vector2f was = object.velocity;
		if (const std::optional<Knocked> after = knockedApart(object.velocity, 1.0f, Vector2f{}, 0.0f, way, share, settle))
		{
			object.velocity = after->first;
			rub(object, other, touch.normal, Vector2f{}, was, settle);
		}
	}

	void bounceOffEdge(Object& object, Edge edge, float share)
	{
		object.velocity = bounced(object.velocity, edge, share);
	}

	// How far the object's middle is from the middle of the touched side, as
	// a share of half that side's length (-1 at one end, 1 at the other, held
	// there past the ends), times maxDegrees, is the angle away from straight
	// back out, towards the end it hit nearer to: off a Pong paddle's top
	// half the ball goes back up, off its bottom half down.
	void deflect(Object& object, const Object& other, Edge edge, float maxDegrees)
	{
		object.velocity = deflected(object.velocity, { object.position, sizeOf(object) }, { other.position, sizeOf(other) }, edge, maxDegrees);
	}

	// Only the touched edge's own axis is affected, so an object pressed
	// against the bottom wall still slides left or right, and one moving away
	// from the wall is not held back.
	void stick(Object& object, const WindowDesc& window, Edge edge)
	{
		stickInside(object.position, object.velocity, object.size, { window.width, window.height }, edge);
	}

	void stickAgainst(Object& object, Edge side)
	{
		Vector2f& velocity = object.velocity;
		if ((side == Edge::Top && velocity.y < 0.0f) || (side == Edge::Bottom && velocity.y > 0.0f)) { velocity.y = 0.0f; }
		if ((side == Edge::Left && velocity.x < 0.0f) || (side == Edge::Right && velocity.x > 0.0f)) { velocity.x = 0.0f; }
	}

	// Until it has gone completely, nothing happens: an object still partly
	// in view slides out as normal. It comes back one screen and one object's
	// width along, so anything spaced along a lane keeps its spacing.
	void wrap(Object& object, const WindowDesc& window, Edge edge)
	{
		wrapAround(object.position, object.size, object.velocity + object.riding, { window.width, window.height }, edge);
	}

	// Not kept: worked out again every frame from whatever is still being
	// touched, so the moment it is off the other one, it is at rest.
	void ride(Object& object, const Object& other)
	{
		object.riding = other.velocity;
	}

	void carry(Object& object, const Object& item)
	{
		if (item.name == object.carrying || item.name == object.dropped || &item == &object)
		{
			return;
		}

		object.carrying = item.name;
		object.carryOffset = item.position - object.position;
	}

	void drop(Object& object)
	{
		if (object.carrying.empty())
		{
			return;
		}

		object.dropped = object.carrying;
		object.carrying.clear();
	}

	void track(Object& object, bool horizontal, float fraction, float span)
	{
		const Vector2f size = sizeOf(object);
		const float extent = horizontal ? size.x : size.y;
		const float room = std::max(span - extent, 0.0f);
		const float place = std::clamp(fraction * span - extent * 0.5f, 0.0f, room);
		(horizontal ? object.position.x : object.position.y) = place;
	}

	void trackPath(Object& object, const Path& path, float fraction)
	{
		std::vector<Vector2f> corners{ path.start };
		for (const PathStep& step : path.steps)
		{
			corners.push_back(step.home ? path.start : corners.back() + step.by);
		}

		float total = 0.0f;
		for (std::size_t i = 1; i < corners.size(); ++i) { total += std::hypot(corners[i].x - corners[i - 1].x, corners[i].y - corners[i - 1].y); }

		Vector2f at = corners.front();
		float left = std::clamp(fraction, 0.0f, 1.0f) * total;
		for (std::size_t i = 1; i < corners.size(); ++i)
		{
			const Vector2f leg = corners[i] - corners[i - 1];
			const float length = std::hypot(leg.x, leg.y);
			if (left <= length && length > 0.0f)
			{
				at = corners[i - 1] + leg * (left / length);
				break;
			}
			left -= length;
			at = corners[i];
		}

		object.position = at - sizeOf(object) * 0.5f;
	}

	// Everything else about it is left as it was, so whatever brings it back
	// (fire() re-launching a bullet, reset()) starts from a known place.
	void die(Object& object)
	{
		object.collisionsOn = false;
		object.isVisible = false;
		object.followPath.clear();
	}

	// Neither its own pull nor a thruster still held is left (a reset gives
	// them back). Being in motion is what makes collisions run, so an object
	// that has stopped is no longer asked about what it is resting on.
	void stop(Object& object)
	{
		object.velocity = {};
		object.acceleration = {};
		object.activeMoveStep = {};
		object.activeClimb = {};
		object.climbing = false;
		object.leaping = false;
		object.activeThrust = {};
		object.activeThrustBurn = {};
		object.activeTurn = {};
		object.activeThrustAheadBy.clear();
		object.followPath.clear();
		object.walk = {};
	}

	void restart(Object& object)
	{
		object.position = object.start.position;
		object.climbing = false;
		object.leaping = false;
		object.grounded = false;
		object.hasAim = false;
		const bool keyHeld = std::any_of(object.activeMoveStep.begin(), object.activeMoveStep.end(), [](float step) { return step != 0.0f; });
		if (!keyHeld)
		{
			object.velocity = object.start.velocity;
		}
		object.heading = object.start.heading;
		object.showHeading();
	}

	// Each direction's step is kept (Object::activeMoveStep), so holding Down
	// and tapping Up stops, and letting go of Up moves down again.
	void holdStep(Object& object, Direction direction, float step)
	{
		object.activeMoveStep[static_cast<std::size_t>(direction)] = step;

		// With a <grip> below 1 the velocity goes there a frame at a time
		// (playOneFrame), on the ground; a pull still has the up and down.
		const bool slides = object.grip < 1.0f;
		if (direction == Direction::Left || direction == Direction::Right)
		{
			if (!object.leaping && !slides)
			{
				object.velocity.x = object.activeMoveStep[static_cast<std::size_t>(Direction::Right)]
					- object.activeMoveStep[static_cast<std::size_t>(Direction::Left)];
			}
		}
		else if (!slides || object.acceleration.y != 0.0f)
		{
			object.velocity.y = object.activeMoveStep[static_cast<std::size_t>(Direction::Down)]
				- object.activeMoveStep[static_cast<std::size_t>(Direction::Up)];
		}
	}

	void holdMove(Object& object, Direction direction, float step, bool pressed)
	{
		holdStep(object, direction, pressed ? step : 0.0f);
		const auto held = [&](Direction way) { return object.activeMoveStep[static_cast<std::size_t>(way)] != 0.0f; };
		const float across = (held(Direction::Right) ? 1.0f : 0.0f) - (held(Direction::Left) ? 1.0f : 0.0f);
		const float upDown = (held(Direction::Down) ? 1.0f : 0.0f) - (held(Direction::Up) ? 1.0f : 0.0f);
		if (pressed && step != 0.0f)
		{
			object.facing = direction;
			object.hasAim = false;
		}
		else if (!pressed && (across == 0.0f) != (upDown == 0.0f))
		{
			// One of two let go: the way still held, straight.
			object.facing = across < 0.0f ? Direction::Left : across > 0.0f ? Direction::Right : upDown < 0.0f ? Direction::Up : Direction::Down;
			object.hasAim = false;
		}
		// Two held across each other: an object with a <facing> faces the
		// diagonal between them, and its <fire>s leave along it. Let go of
		// both, it keeps facing the way it last went.
		if (object.hasFacing && across != 0.0f && upDown != 0.0f)
		{
			object.aim = Vector2f{ across, upDown } * (1.0f / std::sqrt(2.0f));
			object.hasAim = true;
		}
	}
}
