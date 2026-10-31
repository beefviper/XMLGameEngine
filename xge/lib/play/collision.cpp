// collision.cpp
// XML Game Engine
// author: beefviper
// date: Sept 30, 2026

#include "play/collision.h"

#include <algorithm>
#include <cmath>

namespace xge
{
	namespace
	{
		// What the object touches other objects with: its <hitbox>, or else
		// its picture's box.
		Box boxOf(const Object& object) noexcept
		{
			if (object.collisionData.hasHitbox)
			{
				return { object.position + object.collisionData.hitboxOffset, object.collisionData.hitboxSize };
			}
			return { object.position, object.size };
		}

		bool isRound(const Object& object)
		{
			if (object.collisionData.hasHitbox) { return object.collisionData.hitboxRound; }
			return object.picture.kind == ShapeKind::Circle;
		}

		// Whether the point `local` (from the top left of boxOf) is touched
		// with by `object`: its picture's pixels when its collisions are of
		// type pixel (those its <solid char>s name), otherwise its shape.
		bool solidAt(const Object& object, const Vector2f& local)
		{
			if (object.collisionData.type == CollisionType::Pixel && object.bitmap)
			{
				return object.bitmap->touchesAt(static_cast<int>(std::floor(local.x)), static_cast<int>(std::floor(local.y)));
			}

			return solidShapeAt(object.size, isRound(object), local);
		}

		bool pixelsOverlapAt(const Object& a, const Object& b)
		{
			return xge::pixelsOverlapAt(boxOf(a), [&](const Vector2f& local) { return solidAt(a, local); },
				boxOf(b), [&](const Vector2f& local) { return solidAt(b, local); });
		}
	}

	bool touchesScreenEdge(const Object& object, const WindowDesc& windowDesc, Edge edge)
	{
		// The window's edges are met by the picture, hitbox or not.
		return pastWindowEdge({ object.position, object.size }, { windowDesc.width, windowDesc.height }, edge);
	}

	std::optional<Edge> objectsOverlap(const Object& a, const Object& b)
	{
		const std::optional<Edge> edge = shapesOverlap(boxOf(a), isRound(a), boxOf(b), isRound(b));

		// Touching boxes are only a first look when pixels are what counts.
		if (edge && usesPixels(a, b) && !pixelsOverlap(a, b))
		{
			return std::nullopt;
		}

		return edge;
	}

	bool usesPixels(const Object& a, const Object& b) noexcept
	{
		return a.collisionData.type == CollisionType::Pixel || b.collisionData.type == CollisionType::Pixel;
	}

	bool pixelsOverlap(const Object& a, const Object& b)
	{
		return pixelsOverlapAt(a, b);
	}

	Contact contactBetween(const Object& a, const Object& b, Edge edgeOfB, float time)
	{
		Contact contact = contactOf(boxOf(a), isRound(a), boxOf(b), isRound(b), edgeOfB, time);

		// Tested by pixels: where the pixels meet, and the way what is drawn
		// faces there (a slope of a <line>, the round of a ball).
		if (usesPixels(a, b))
		{
			const auto solidA = [&](const Vector2f& local) { return solidAt(a, local); };
			const auto solidB = [&](const Vector2f& local) { return solidAt(b, local); };
			pixelContactAt(boxOf(a), solidA, boxOf(b), solidB, contact.point, contact.normal);

			// How deep is how far the first must go along the way apart for
			// no drawn pixels to meet, not how far their boxes overlap (a
			// flipper's square is far bigger than its bar).
			contact.depth = 0.0f;
			const Box first = boxOf(a);
			const Box second = boxOf(b);
			const auto clearAt = [&](float apart)
			{
				return !xge::pixelsOverlapAt(Box{ first.position + contact.normal * apart, first.size }, solidA, second, solidB);
			};
			for (float apart = 0.0f; apart <= first.size.x + first.size.y; apart += 0.5f)
			{
				if (clearAt(apart))
				{
					// Narrowed down within the last half pixel.
					float low = std::max(0.0f, apart - 0.5f);
					float high = apart;
					for (int refine = 0; refine < 6 && apart > 0.0f; ++refine)
					{
						const float middle = (low + high) / 2;
						if (clearAt(middle)) { high = middle; } else { low = middle; }
					}
					contact.depth = high;
					break;
				}
			}
		}
		return contact;
	}

	std::optional<SweepHit> sweepObjects(const Object& a, const Vector2f& moveA, const Object& b, const Vector2f& moveB)
	{
		return sweepThings(boxOf(a), isRound(a), [&](const Vector2f& local) { return solidAt(a, local); }, moveA,
			boxOf(b), isRound(b), [&](const Vector2f& local) { return solidAt(b, local); }, moveB, usesPixels(a, b));
	}

	namespace
	{
		// How far from its pivot any of a turning object reaches: the pivot's
		// way from the middle, and half the square's diagonal past that.
		float turningReach(const Object& object) noexcept
		{
			const Vector2f arm = object.hasPivot ? object.pivotFromMiddle : Vector2f{};
			return std::hypot(arm.x, arm.y) + std::hypot(object.size.x, object.size.y) / 2.0f;
		}

		// Where one of a pair is `time` through a sweep, moving by `move` and
		// turning by `turn` degrees over it: its box, and whether a point of
		// it (from that box's top left) touches with. One turning is turned
		// about its pivot, which goes with the move, and asked at its exact
		// heading then, not the nearest whole degree.
		struct Posed
		{
			Box box;
			const Object* object{};
			const Turnable* drawing{};
			float degrees{};
			bool round{};

			bool solidAt(const Vector2f& local) const
			{
				if (!drawing) { return xge::solidAt(*object, local); }
				if (object->collisionData.type == CollisionType::Pixel)
				{
					return drawing->touchesAt(degrees, box.size.x, local.x, local.y);
				}
				return solidShapeAt(box.size, round, local);
			}
		};
	}

	std::optional<SweepHit> sweepTurning(const Object& a, const Vector2f& moveA, float turnA, const Object& b, const Vector2f& moveB, float turnB)
	{
		const auto posed = [](const Object& object, const Vector2f& move, float turn, float time)
		{
			Posed at;
			at.object = &object;
			at.round = isRound(object);
			if (turn == 0.0f || object.turnables.empty())
			{
				at.box = boxOf(object);
				at.box.position = at.box.position + move * time;
				return at;
			}

			const std::size_t frame = object.turnables.size() > 1 && object.animationIndex < object.turnables.size() ? object.animationIndex : 0;
			at.drawing = object.turnables[frame].get();
			at.degrees = object.heading + turn * time;
			const Vector2f arm = object.hasPivot ? object.pivotFromMiddle : Vector2f{};
			at.box = { object.position + move * time + turnedBy(arm, static_cast<float>(object.pivotDegrees)) - turnedBy(arm, at.degrees), object.size };
			return at;
		};

		const auto touchesAt = [&](float time)
		{
			const Posed first = posed(a, moveA, turnA, time);
			const Posed second = posed(b, moveB, turnB, time);
			if (!boxesMeet(first.box, second.box)) { return false; }
			return pixelsOverlapAt(first.box, [&](const Vector2f& local) { return first.solidAt(local); },
				second.box, [&](const Vector2f& local) { return second.solidAt(local); });
		};

		// The way the first goes against the second, where they are: each
		// one's move, and the speed its turn gives the point at the other's
		// middle.
		const auto middleOf = [](const Object& object) { const Box box = boxOf(object); return box.position + box.size * 0.5f; };
		const auto turnedAt = [](const Object& object, float turn, const Vector2f& point)
		{
			const Vector2f arm = point - object.pivotOnScreen();
			const float radians = turn * 3.14159265358979323846f / 180.0f;
			return Vector2f{ -arm.y * radians, arm.x * radians };
		};
		const Vector2f relative = moveA + turnedAt(a, turnA, middleOf(b)) - moveB - turnedAt(b, turnB, middleOf(a));

		// Half a pixel at a time of whatever goes furthest: the two moves
		// against each other, and each turn's outermost arc.
		const float pi = 3.14159265358979323846f;
		const float arcs = std::abs(turnA) * pi / 180.0f * turningReach(a) + std::abs(turnB) * pi / 180.0f * turningReach(b);
		const float reach = std::max(std::abs(moveA.x - moveB.x), std::abs(moveA.y - moveB.y)) + arcs;
		const int steps = std::clamp(static_cast<int>(std::ceil(reach / 0.5f)), 1, 4000);

		if (touchesAt(0.0f))
		{
			return SweepHit{ 0.0f, edgeFromMotion(relative) };
		}

		float previous = 0.0f;
		for (int i = 1; i <= steps; ++i)
		{
			const float time = static_cast<float>(i) / static_cast<float>(steps);
			if (touchesAt(time))
			{
				float low = previous;
				float high = time;
				for (int refine = 0; refine < 8; ++refine)
				{
					const float middle = (low + high) / 2;
					if (touchesAt(middle)) { high = middle; } else { low = middle; }
				}
				return SweepHit{ high, edgeFromMotion(relative) };
			}
			previous = time;
		}
		return std::nullopt;
	}

	Box reachOver(const Object& object, const Vector2f& move) noexcept
	{
		// One turning reaches anywhere round its pivot it can swing to.
		if (object.spin != 0.0f && !object.turnables.empty())
		{
			const float radius = turningReach(object);
			const Vector2f pivot = object.pivotOnScreen();
			return sweptBox({ pivot - Vector2f{ radius, radius }, Vector2f{ radius * 2.0f, radius * 2.0f } }, move);
		}
		return sweptBox(boxOf(object), move);
	}
}
