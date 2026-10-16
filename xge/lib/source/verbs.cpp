// verbs.cpp
// XML Game Engine
// author: beefviper
// date: Oct 12, 2026

#include "verbs.h"

#include "command.h"
#include "geometry.h"

#include <algorithm>
#include <utility>

namespace xge
{
	Object makeObject(std::string name, std::vector<std::string> sprite, bool collides)
	{
		Object object{};
		object.name = std::move(name);
		object.shapeKind = shapeKindFromTag(sprite.empty() ? std::string{} : sprite.front());
		object.spriteParams = std::move(sprite);
		object.isVisible = true;
		object.isVisibleOriginal = true;
		object.collisionData.enabled = collides;
		object.collisionEnabledOriginal = collides;
		return object;
	}

	void startAt(Object& object, Vector2f position, Vector2f velocity)
	{
		object.position = position;
		object.positionOriginal = position;
		object.velocity = velocity;
		object.velocityOriginal = velocity;
	}

	Vector2f sizeOf(const Object& object)
	{
		if (object.size.x != 0.0f || object.size.y != 0.0f) { return object.size; }
		return measureShapeSize(object.spriteParams, object.shapeKind);
	}

	Vector2f motionOf(const Object& object) noexcept
	{
		return object.velocity + object.riding;
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

		if (direction == Direction::Left || direction == Direction::Right)
		{
			if (!object.leaping)
			{
				object.velocity.x = object.activeMoveStep[static_cast<std::size_t>(Direction::Right)]
					- object.activeMoveStep[static_cast<std::size_t>(Direction::Left)];
			}
		}
		else
		{
			object.velocity.y = object.activeMoveStep[static_cast<std::size_t>(Direction::Down)]
				- object.activeMoveStep[static_cast<std::size_t>(Direction::Up)];
		}
	}

	void holdMove(Object& object, Direction direction, float step, bool pressed)
	{
		holdStep(object, direction, pressed ? step : 0.0f);
		if (pressed && step != 0.0f)
		{
			object.facing = direction;
			object.hasAim = false;
		}
	}
}
