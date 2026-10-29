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

	bool knock(Object& object, Object& other, const Touch& touch, float bounce)
	{
		// The way from the object into the other.
		const Vector2f way = touch.normal * -1.0f;
		const std::optional<Knocked> after = knockedApart(object.velocity, object.mass, other.velocity, other.mass, way, bounce);
		if (!after)
		{
			return false;
		}

		object.velocity = after->first;
		other.velocity = after->second;

		// Gone into each other: apart again, each by its share.
		if (touch.depth > 0.0f && (object.mass > 0.0f || other.mass > 0.0f))
		{
			const float objectShare = object.mass <= 0.0f ? 0.0f : other.mass <= 0.0f ? 1.0f : other.mass / (object.mass + other.mass);
			object.position -= way * (touch.depth * objectShare);
			other.position += way * (touch.depth * (1.0f - objectShare));
		}

		return true;
	}

	void bounce(Object& object, Edge edge)
	{
		object.velocity = bounced(object.velocity, edge);
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
		object.collisionData.enabled = false;
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
		object.position = object.positionOriginal;
		object.climbing = false;
		object.leaping = false;
		object.grounded = false;
		object.hasAim = false;
		const bool keyHeld = std::any_of(object.activeMoveStep.begin(), object.activeMoveStep.end(), [](float step) { return step != 0.0f; });
		if (!keyHeld)
		{
			object.velocity = object.velocityOriginal;
		}
		object.heading = object.headingOriginal;
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
