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
		// What the object touches other objects with is one or more parts:
		// each of its <hitbox>es, or else (and always when byPicture) its
		// picture; and, but for byPicture, the hitboxes of the animation
		// frame showing. A part is a hitbox's number (the object's own
		// first, then the frame's), or `picture`.
		constexpr std::size_t picture = static_cast<std::size_t>(-1);

		std::size_t ownParts(const Object& object, bool byPicture) noexcept
		{
			return byPicture || object.hitboxes.empty() ? 1 : object.hitboxes.size();
		}

		std::size_t partsOf(const Object& object, bool byPicture) noexcept
		{
			return ownParts(object, byPicture) + (byPicture ? 0 : object.frameHitboxes().size());
		}

		std::size_t partOf(const Object& object, bool byPicture, std::size_t index) noexcept
		{
			const std::size_t own = ownParts(object, byPicture);
			if (index >= own) { return object.hitboxes.size() + (index - own); }
			return byPicture || object.hitboxes.empty() ? picture : index;
		}

		const Hitbox& hitboxAt(const Object& object, std::size_t part) noexcept
		{
			const std::size_t own = object.hitboxes.size();
			return part < own ? object.hitboxes[part] : object.frameHitboxes()[part - own];
		}

		// A part's box. An inset hitbox is the picture's box pulled in, left
		// and right swapped while the picture is drawn mirrored; a shape is
		// placed from the picture's right instead while it is.
		Box boxOf(const Object& object, std::size_t part = picture) noexcept
		{
			if (part == picture) { return { object.position, object.size }; }
			const Hitbox& hitbox = hitboxAt(object, part);
			if (hitbox.inset)
			{
				const float left = object.mirror.flipped ? hitbox.right : hitbox.left;
				const float right = object.mirror.flipped ? hitbox.left : hitbox.right;
				return { object.position + Vector2f{ left, hitbox.top },
					{ std::max(0.0f, object.size.x - left - right), std::max(0.0f, object.size.y - hitbox.top - hitbox.bottom) } };
			}
			const float x = object.mirror.flipped ? object.size.x - hitbox.offset.x - hitbox.size.x : hitbox.offset.x;
			return { object.position + Vector2f{ x, hitbox.offset.y }, hitbox.size };
		}

		bool isRound(const Object& object, std::size_t part = picture)
		{
			if (part != picture) { return hitboxAt(object, part).round; }
			return object.drawn.picture.kind == ShapeKind::Circle;
		}

		// Whether the point `local` (from the top left of the part's box) is
		// touched with by `object`: its picture's pixels when its collisions
		// are of type pixel (those its <solid char>s name), otherwise its shape.
		bool solidAt(const Object& object, const Vector2f& local, std::size_t part = picture)
		{
			if (object.kind->collisionData.type == CollisionType::Pixel && object.drawn.bitmap)
			{
				return object.drawn.bitmap->touchesAt(static_cast<int>(std::floor(local.x)), static_cast<int>(std::floor(local.y)));
			}

			return solidShapeAt(boxOf(object, part).size, isRound(object, part), local);
		}

		bool pixelsOverlapAt(const Object& a, std::size_t partA, const Object& b, std::size_t partB)
		{
			return xge::pixelsOverlapAt(boxOf(a, partA), [&](const Vector2f& local) { return solidAt(a, local, partA); },
				boxOf(b, partB), [&](const Vector2f& local) { return solidAt(b, local, partB); });
		}

		// Calls `each(partA, partB)` for every part of a against every part of
		// b, until it answers true.
		template <typename Each>
		void forParts(const Object& a, const Object& b, bool byPicture, Each each)
		{
			for (std::size_t i = 0; i < partsOf(a, byPicture); ++i)
			{
				for (std::size_t j = 0; j < partsOf(b, byPicture); ++j)
				{
					if (each(partOf(a, byPicture, i), partOf(b, byPicture, j))) { return; }
				}
			}
		}
	}

	bool touchesScreenEdge(const Object& object, const WindowDesc& windowDesc, Edge edge)
	{
		// The window's edges are met by the picture, hitbox or not.
		return pastWindowEdge({ object.position, object.size }, { windowDesc.width, windowDesc.height }, edge);
	}

	std::optional<Edge> objectsOverlap(const Object& a, const Object& b, bool byPicture)
	{
		std::optional<Edge> found;
		forParts(a, b, byPicture, [&](std::size_t partA, std::size_t partB)
		{
			const std::optional<Edge> edge = shapesOverlap(boxOf(a, partA), isRound(a, partA), boxOf(b, partB), isRound(b, partB));
			// Touching boxes are only a first look when pixels are what counts.
			if (edge && usesPixels(a, b) && !pixelsOverlapAt(a, partA, b, partB)) { return false; }
			found = edge;
			return found.has_value();
		});
		return found;
	}

	bool touchesWith(const Object& a, const std::string& hitboxA, const Object& b, const std::string& hitboxB)
	{
		// The parts it may be: those of that name, or with none every part.
		const auto parts = [](const Object& object, const std::string& name)
		{
			std::vector<std::size_t> found;
			for (std::size_t i = 0; i < partsOf(object, false); ++i)
			{
				const std::size_t part = partOf(object, false, i);
				if (name.empty() || (part != picture && hitboxAt(object, part).name == name)) { found.push_back(part); }
			}
			return found;
		};
		// Just met counts: each box grown by a hair, as a sweep leaves the
		// two touching, not overlapping.
		constexpr float hair = 0.01f;
		const auto grown = [&](Box box) { box.position -= Vector2f{ hair, hair }; box.size += Vector2f{ hair * 2.0f, hair * 2.0f }; return box; };
		for (const std::size_t partA : parts(a, hitboxA))
		{
			for (const std::size_t partB : parts(b, hitboxB))
			{
				if (shapesOverlap(grown(boxOf(a, partA)), isRound(a, partA), boxOf(b, partB), isRound(b, partB))) { return true; }
			}
		}
		return false;
	}

	bool usesPixels(const Object& a, const Object& b) noexcept
	{
		return a.kind->collisionData.type == CollisionType::Pixel || b.kind->collisionData.type == CollisionType::Pixel;
	}

	bool pixelsOverlap(const Object& a, const Object& b)
	{
		bool found = false;
		forParts(a, b, false, [&](std::size_t partA, std::size_t partB) { found = pixelsOverlapAt(a, partA, b, partB); return found; });
		return found;
	}

	Contact contactBetween(const Object& a, const Object& b, Edge edgeOfB, float time, bool byPicture)
	{
		// The two parts that touch: those nearest each other.
		std::size_t partA = picture;
		std::size_t partB = picture;
		float nearest = 0.0f;
		bool found = false;
		forParts(a, b, byPicture, [&](std::size_t i, std::size_t j)
		{
			const Box one = boxOf(a, i);
			const Box two = boxOf(b, j);
			const float gap = std::max(std::max(one.position.x - (two.position.x + two.size.x), two.position.x - (one.position.x + one.size.x)),
				std::max(one.position.y - (two.position.y + two.size.y), two.position.y - (one.position.y + one.size.y)));
			if (!found || gap < nearest) { nearest = gap; partA = i; partB = j; found = true; }
			return false;
		});

		Contact contact = contactOf(boxOf(a, partA), isRound(a, partA), boxOf(b, partB), isRound(b, partB), edgeOfB, time);

		// Tested by pixels: where the pixels meet, and the way what is drawn
		// faces there (a slope of a <line>, the round of a ball).
		if (usesPixels(a, b))
		{
			const auto solidA = [&](const Vector2f& local) { return solidAt(a, local, partA); };
			const auto solidB = [&](const Vector2f& local) { return solidAt(b, local, partB); };
			pixelContactAt(boxOf(a, partA), solidA, boxOf(b, partB), solidB, contact.point, contact.normal);

			// How deep is how far the first must go along the way apart for
			// no drawn pixels to meet, not how far their boxes overlap (a
			// flipper's square is far bigger than its bar).
			contact.depth = 0.0f;
			const Box first = boxOf(a, partA);
			const Box second = boxOf(b, partB);
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

	std::optional<SweepHit> sweepObjects(const Object& a, const Vector2f& moveA, const Object& b, const Vector2f& moveB, bool byPicture)
	{
		// The first moment any part of one meets any part of the other.
		std::optional<SweepHit> earliest;
		forParts(a, b, byPicture, [&](std::size_t partA, std::size_t partB)
		{
			const std::optional<SweepHit> hit = sweepThings(boxOf(a, partA), isRound(a, partA), [&](const Vector2f& local) { return solidAt(a, local, partA); }, moveA,
				boxOf(b, partB), isRound(b, partB), [&](const Vector2f& local) { return solidAt(b, local, partB); }, moveB, usesPixels(a, b));
			if (hit && (!earliest || hit->time < earliest->time)) { earliest = hit; }
			return false;
		});
		return earliest;
	}

	namespace
	{
		// How far from its pivot any of a turning object reaches: the pivot's
		// way from the middle, and half the square's diagonal past that.
		float turningReach(const Object& object) noexcept
		{
			const Vector2f arm = object.turning.hasPivot ? object.turning.pivotFromMiddle : Vector2f{};
			return std::hypot(arm.x, arm.y) + std::hypot(object.size.x, object.size.y) / 2.0f;
		}

		// Where one of a pair is `time` through a sweep, moving by `move` and
		// turning by `turn` degrees over it: its box, and whether a point of
		// it (from that box's top left) touches with. One turning is turned
		// about its pivot, which goes with the move, and is the picture
		// showHeading would show then (at the nearest whole degree, kept by
		// Turnable::turnedTo), so the sweep and the touch see one shape.
		struct Posed
		{
			Box box;
			const Object* object{};
			const Bitmap* drawing{};
			bool round{};
			std::size_t part{ picture };

			bool solidAt(const Vector2f& local) const
			{
				if (!drawing) { return xge::solidAt(*object, local, part); }
				if (object->kind->collisionData.type == CollisionType::Pixel)
				{
					return drawing->touchesAt(static_cast<int>(std::floor(local.x)), static_cast<int>(std::floor(local.y)));
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
			at.part = partOf(object, false, 0);
			at.round = isRound(object, at.part);
			if (turn == 0.0f || object.drawn.turnables.empty())
			{
				at.box = boxOf(object, at.part);
				at.box.position = at.box.position + move * time;
				return at;
			}

			// Drawn and placed as showHeading would at that moment: the
			// picture at the nearest whole degree, turned about its pivot.
			const std::size_t frame = object.drawn.turnables.size() > 1 && object.animation.index < object.drawn.turnables.size() ? object.animation.index : 0;
			const int degrees = wholeDegrees(object.heading + turn * time);
			at.drawing = &object.drawn.turnables[frame]->turnedTo(degrees);
			const Vector2f arm = object.turning.hasPivot ? object.turning.pivotFromMiddle : Vector2f{};
			at.box = { object.position + move * time + turnedBy(arm, static_cast<float>(object.pivotDegrees)) - turnedBy(arm, static_cast<float>(degrees)), object.size };
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

	bool lineCrosses(const Vector2f& from, const Vector2f& to, const Object& object)
	{
		const Box box = boxOf(object);
		const Vector2f along = to - from;
		// A point swept along the line against the shape: a hit within it.
		const std::optional<SweepHit> hit = sweepShapes(Box{ from, {} }, false, along, box, isRound(object), {});
		if (!hit) { return false; }
		if (object.kind->collisionData.type != CollisionType::Pixel || !object.drawn.bitmap) { return true; }

		// Its pixels: from where the line meets its box, half a pixel at a time.
		const float length = std::hypot(along.x, along.y);
		if (!(length > 0.0f)) { return solidAt(object, from - box.position); }
		const int steps = static_cast<int>(std::ceil(length / 0.5f));
		for (int i = static_cast<int>(std::floor(hit->time * static_cast<float>(steps))); i <= steps; ++i)
		{
			const Vector2f point = from + along * (static_cast<float>(i) / static_cast<float>(steps));
			const Vector2f local = point - box.position;
			if (local.x < 0.0f || local.y < 0.0f || local.x >= box.size.x || local.y >= box.size.y) { continue; }
			if (solidAt(object, local)) { return true; }
		}
		return false;
	}

	Box reachOver(const Object& object, const Vector2f& move) noexcept
	{
		// One turning reaches anywhere round its pivot it can swing to.
		if (object.turning.spin != 0.0f && !object.drawn.turnables.empty())
		{
			const float radius = turningReach(object);
			const Vector2f pivot = object.pivotOnScreen();
			return sweptBox({ pivot - Vector2f{ radius, radius }, Vector2f{ radius * 2.0f, radius * 2.0f } }, move);
		}
		// Its hitboxes and its picture, whichever a pair touches by: all.
		const Box all = boxOf(object);
		Vector2f low = all.position;
		Vector2f high = all.position + all.size;
		for (std::size_t i = 0; i < partsOf(object, false); ++i)
		{
			const std::size_t part = partOf(object, false, i);
			if (part == picture) { continue; }
			const Box hit = boxOf(object, part);
			low = { std::min(low.x, hit.position.x), std::min(low.y, hit.position.y) };
			high = { std::max(high.x, hit.position.x + hit.size.x), std::max(high.y, hit.position.y + hit.size.y) };
		}
		return sweptBox({ low, high - low }, move);
	}
}
