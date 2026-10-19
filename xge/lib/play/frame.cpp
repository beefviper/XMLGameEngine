// frame.cpp
// XML Game Engine
// author: beefviper
// date: Oct 12, 2026

#include "play/frame.h"

#include "play/collision.h"
#include "play/geometry.h"
#include "play/maze.h"
#include "play/verbs.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>

namespace xge
{
	namespace
	{
		// Whether it is in motion at all this frame: moving, riding something,
		// or having just made a hop.
		bool isMoving(const Object& object) noexcept
		{
			const Vector2f motion = motionOf(object);
			return motion.x != 0 || motion.y != 0 || object.hopped;
		}

		// Whether a and b can touch at all, whatever their rules: both have
		// their collisions on, neither is in the middle of a jump (it is over
		// everything), and they are not one lockstep block (the invaders
		// never collide with each other). Then whether a rule answers is the
		// game's (FrameRules::canCollide).
		bool canTouch(const Object& a, const Object& b, FrameRules& rules)
		{
			if (!a.collisionData.enabled || !b.collisionData.enabled) { return false; }
			if (a.isAirborne() || b.isAirborne()) { return false; }
			if (a.collisionData.lockstep != 0 && a.collisionData.lockstep == b.collisionData.lockstep) { return false; }
			return rules.canCollide(a, b);
		}

		// Climbing (a <climb> in an action, held): an object at a ladder - an
		// object of its climbClass with the climber's middle over it and its
		// feet between the ladder's top and bottom - that is on the ground and
		// asked to go up or down it, and can, gets on it, lined up with its
		// middle. On it, it goes the step held each frame (none held: it stays
		// put), its feet kept between the two ends, with no pull and no
		// landing, and its own velocity at 0 so no key walks it off. Reaching
		// either end it gets off, standing there, and the keys held take it on
		// as before.
		void applyClimbing(std::vector<Object>& objects, FrameRules& rules)
		{
			for (auto& object : objects)
			{
				if (object.climbClass.empty() || !rules.inPlay(object)) { continue; }

				const Box climber{ object.position, object.size };
				const auto ladder = std::find_if(objects.begin(), objects.end(), [&](const Object& other)
					{
						return &other != &object && other.objClass == object.climbClass && rules.inPlay(other)
							&& atLadder(climber, { other.position, other.size });
					});

				if (ladder == objects.end())
				{
					object.climbing = false;
					continue;
				}

				const float step = object.activeClimb[1] - object.activeClimb[0];
				const bool wasClimbing = object.climbing;
				if (!climbLadder(object.position, object.size, step, object.climbing, object.grounded, { ladder->position, ladder->size }))
				{
					continue;
				}

				if (!wasClimbing) { object.leaping = false; }
				object.velocity = {};

				if (!object.climbing)
				{
					object.velocity.x = object.activeMoveStep[static_cast<std::size_t>(Direction::Right)]
						- object.activeMoveStep[static_cast<std::size_t>(Direction::Left)];
				}
			}
		}

		// A constant pull (<acceleration>, gravity being one with only a y)
		// and held thrust (<accelerate>) both change an object's velocity,
		// once a frame, before it is moved. Thrust that burns a variable takes
		// 1 off it every frame it is on, and does nothing while the variable
		// is at 0 or below: a ship out of fuel still falls.
		void applyAcceleration(std::vector<Object>& objects, FrameRules& rules)
		{
			for (auto& object : objects)
			{
				// On a ladder nothing pulls it (applyClimbing moves it).
				if (!rules.inPlay(object) || object.climbing) { continue; }

				Vector2f change = object.acceleration;

				// A thruster with no fuel does nothing.
				const auto burnFuel = [&](const std::string& burn, const char* verb)
				{
					return burn.empty() || rules.burn(object, burn, verb);
				};

				for (std::size_t i = 0; i < object.activeThrust.size(); ++i)
				{
					const float amount = object.activeThrust[i];
					if (amount == 0.0f) { continue; }

					if (!burnFuel(object.activeThrustBurn[i], "accelerate")) { continue; }

					switch (static_cast<Direction>(i))
					{
					case Direction::Up:    change.y -= amount; break;
					case Direction::Down:  change.y += amount; break;
					case Direction::Left:  change.x -= amount; break;
					case Direction::Right: change.x += amount; break;
					}
				}

				// Turning, and thrust along the way the object then faces. A
				// heading is degrees clockwise from straight up.
				if (object.hasHeading)
				{
					const float turn = object.activeTurn[static_cast<std::size_t>(Direction::Right)]
						- object.activeTurn[static_cast<std::size_t>(Direction::Left)];
					if (turn != 0.0f)
					{
						object.heading = std::fmod(object.heading + turn, 360.0f);
						if (object.heading < 0.0f) { object.heading += 360.0f; }
						object.showHeading();
					}

					for (const auto& [action, thrust] : object.activeThrustAheadBy)
					{
						if (burnFuel(thrust.second, "thrust"))
						{
							change += ahead(object.heading) * thrust.first;
						}
					}
				}

				object.velocity += change;

				// Drag: what is left of the speed after this frame.
				if (object.drag != 0.0f)
				{
					object.velocity *= 1.0f - object.drag;
				}
			}
		}

		// Makes the hops queued by hop.*() actions. A hop is a jump, not a
		// slide: the object is simply somewhere else, one step away, before
		// this frame's collisions are worked out, so it is judged by where it
		// lands and not by what it would have brushed past on the way. A hop
		// that would take it out of the window is refused. Whether or not it
		// went, an object that asked to hop counts as moving for the rest of
		// the frame, so the collisions at its new spot run even when nothing
		// else there is moving.
		void applyHops(std::vector<Object>& objects, const WindowDesc& window, FrameRules& rules)
		{
			const Vector2f windowSize{ window.width, window.height };

			for (auto& object : objects)
			{
				object.hopped = false;

				// Whatever last frame's collisions said, this frame's start
				// over: what it rides, and whether it stands on something.
				object.riding = {};
				object.grounded = false;

				// A jump under way goes on a step; the frame it lands, it
				// counts as having moved, so it meets whatever it landed on.
				if (object.isAirborne())
				{
					if (rules.inPlay(object))
					{
						const Vector2f target = object.position + object.jumpStep;
						if (insideWindow({ target, object.size }, windowSize)) { object.position = target; }
					}
					--object.jumpFramesLeft;
					object.hopped = true;
				}

				if (object.hopPending.x == 0 && object.hopPending.y == 0)
				{
					continue;
				}

				const Vector2f target = object.position + object.hopPending;
				object.hopPending = {};

				if (!rules.inPlay(object)) { continue; }

				if (insideWindow({ target, object.size }, windowSize))
				{
					object.position = target;
				}
				object.hopped = true;
			}
		}

		// The object of that name, or null.
		Object* named(std::vector<Object>& objects, const std::string& name)
		{
			const auto found = std::find_if(objects.begin(), objects.end(), [&](const Object& object) { return object.name == name; });
			return found == objects.end() ? nullptr : &*found;
		}

		// What is carried goes the way its carrier goes this frame, lent the
		// carrier's motion as a ride is, so it is swept and meets what is on
		// the way (a key reaching a gate). Either out of play, and it is put
		// down.
		void lendCarried(std::vector<Object>& objects, FrameRules& rules)
		{
			for (auto& carrier : objects)
			{
				if (carrier.carrying.empty()) { continue; }

				Object* item = named(objects, carrier.carrying);
				if (!item || !rules.inPlay(carrier) || !rules.inPlay(*item))
				{
					carrier.carrying.clear();
					continue;
				}

				item->riding = motionOf(carrier) - item->velocity;
			}
		}

		// After the move: what is carried is put exactly where it is held
		// (the carrier may have been stopped on the way, or hopped), and
		// what was put down can be picked up again once the two are apart.
		void holdCarried(std::vector<Object>& objects)
		{
			for (auto& carrier : objects)
			{
				if (!carrier.carrying.empty())
				{
					if (Object* item = named(objects, carrier.carrying))
					{
						item->position = carrier.position + carrier.carryOffset;
					}
				}

				if (!carrier.dropped.empty())
				{
					const Object* item = named(objects, carrier.dropped);
					if (!item || !boxesOverlap({ carrier.position, sizeOf(carrier) }, { item->position, sizeOf(*item) }))
					{
						carrier.dropped.clear();
					}
				}
			}
		}

		// The nearest of that name or group in play to `from`, leaving it out.
		const Object* nearestInPlay(const std::vector<Object>& objects, FrameRules& rules, const Object& from, const std::string& name)
		{
			const Vector2f middle = from.position + sizeOf(from) * 0.5f;
			const Object* best = nullptr;
			float bestApart = 0.0f;
			for (const auto& other : objects)
			{
				if (&other == &from || (other.name != name && !other.inGroup(name)) || !rules.inPlay(other)) { continue; }
				const Vector2f apart = other.position + sizeOf(other) * 0.5f - middle;
				const float distance = apart.x * apart.x + apart.y * apart.y;
				if (!best || distance < bestApart) { best = &other; bestApart = distance; }
			}
			return best;
		}

		// <pull>: toward the middle of the nearest of each, that much speed a
		// frame. <seek>: the way it goes turned toward the nearest of its
		// target, and its heading with it.
		void pullAndSeek(std::vector<Object>& objects, FrameRules& rules)
		{
			for (auto& object : objects)
			{
				if (!rules.inPlay(object) || (object.pulls.empty() && object.seekTarget.empty())) { continue; }

				const Vector2f middle = object.position + sizeOf(object) * 0.5f;
				for (const auto& [name, strength] : object.pulls)
				{
					const Object* toward = nearestInPlay(objects, rules, object, name);
					if (!toward) { continue; }
					if (const auto way = wayTo(middle, toward->position + sizeOf(*toward) * 0.5f))
					{
						object.velocity = object.velocity + *way * strength;
					}
				}

				if (object.seekTarget.empty()) { continue; }
				const Object* target = nearestInPlay(objects, rules, object, object.seekTarget);
				if (!target) { continue; }
				const Vector2f before = object.velocity;
				object.velocity = turnedToward(object.velocity, target->position + sizeOf(*target) * 0.5f - middle, object.seekRate);
				if (object.hasHeading && (object.velocity.x != before.x || object.velocity.y != before.y))
				{
					object.heading = headingOf(object.velocity);
					object.showHeading();
				}
			}
		}

		// <altitude>: what is in the air rises by its rise, which falls by its
		// fall each frame; at the ground it stops there, sent back up with
		// its bounce's share of how fast it came down (none, once that is
		// less than a frame's fall, so it comes to rest).
		void riseAndFall(std::vector<Object>& objects, FrameRules& rules)
		{
			for (auto& object : objects)
			{
				if (!object.hasAltitude || !rules.inPlay(object)) { continue; }
				object.rise -= object.fall;
				object.altitude += object.rise;
				if (object.altitude > 0.0f) { continue; }
				object.altitude = 0.0f;
				if (object.rise < 0.0f)
				{
					const float up = -object.rise * object.altitudeBounce;
					object.rise = up > object.fall ? up : 0.0f;
				}
			}
		}

		// A <shadow> is kept under what casts it, on the ground, and shown
		// while that is.
		void castShadows(std::vector<Object>& objects)
		{
			for (auto& shadow : objects)
			{
				if (shadow.shadowOf.empty()) { continue; }
				const Object* caster = named(objects, shadow.shadowOf);
				if (!caster) { continue; }
				shadow.position = caster->position + sizeOf(*caster) * 0.5f - sizeOf(shadow) * 0.5f;
				shadow.isVisible = caster->isVisible;
			}
		}

		// What is <dark> is drawn only where a <light> of something in play
		// reaches some part of it; an unseen one, from the first time it does.
		void lightAndDark(std::vector<Object>& objects, FrameRules& rules)
		{
			struct Light
			{
				Vector2f middle;
				float reach;
			};
			std::vector<Light> lights;
			bool anyDark = false;
			for (const auto& object : objects)
			{
				anyDark = anyDark || object.dark != Object::Darkness::None;
				if (object.light > 0.0f && rules.inPlay(object))
				{
					lights.push_back({ object.position + sizeOf(object) * 0.5f, object.light });
				}
			}
			if (!anyDark) { return; }

			for (auto& object : objects)
			{
				if (object.dark == Object::Darkness::None) { continue; }
				const Vector2f size = sizeOf(object);
				const bool lit = std::any_of(lights.begin(), lights.end(), [&](const Light& light)
					{
						const float x = std::clamp(light.middle.x, object.position.x, object.position.x + size.x);
						const float y = std::clamp(light.middle.y, object.position.y, object.position.y + size.y);
						return (x - light.middle.x) * (x - light.middle.x) + (y - light.middle.y) * (y - light.middle.y) <= light.reach * light.reach;
					});
				if (object.dark == Object::Darkness::Unseen)
				{
					object.seen = object.seen || lit;
					object.inDark = !object.seen;
				}
				else
				{
					object.inDark = !lit;
				}
			}
		}

		// A <tether> is a line from one object's middle to another's, drawn
		// again after the move wherever the two have got to; its own position
		// and size follow it, so what it touches is where it is now. It is in
		// play while both ends are.
		void drawTethers(std::vector<Object>& objects)
		{
			for (auto& tether : objects)
			{
				if (tether.tetherFrom.empty()) { continue; }

				const Object* from = named(objects, tether.tetherFrom);
				const Object* to = named(objects, tether.tetherTo);
				if (!from || !to) { continue; }
				tether.isVisible = from->isVisible && to->isVisible;
				if (!tether.isVisible) { continue; }

				const Vector2f a = from->position + sizeOf(*from) * 0.5f;
				const Vector2f b = to->position + sizeOf(*to) * 0.5f;
				const float half = static_cast<float>(tether.tetherThickness) * 0.5f;
				const Vector2f corner{ std::floor(std::min(a.x, b.x) - half), std::floor(std::min(a.y, b.y) - half) };

				LineSegment line;
				line.x1 = a.x - corner.x - half;
				line.y1 = a.y - corner.y - half;
				line.x2 = b.x - corner.x - half;
				line.y2 = b.y - corner.y - half;
				line.color = tether.tetherColor;
				line.thickness = tether.tetherThickness;

				auto drawn = std::make_shared<const Bitmap>(rasterizeLines({ line }));
				tether.position = corner;
				tether.size = { static_cast<float>(drawn->width), static_cast<float>(drawn->height) };
				tether.spriteParams = { "line", std::to_string(drawn->width), std::to_string(drawn->height) };
				tether.bitmap = std::move(drawn);
				tether.visualDirty = true;
			}
		}

		// Plays one frame of movement. Every object in play moves by its
		// velocity (and by whatever it is riding), but not in one jump: each
		// pair of objects that can touch is swept along its own path, the
		// earliest touch anywhere is found, everything is moved up to that
		// moment, that pair's rules run, and the rest of the frame carries on
		// from there with whatever velocities the rules left behind
		// (xge::playFrame) - so a ball that hits a brick a third of the way
		// through its move spends the other two thirds heading back the way it
		// came. Each object is swept by itself; a lockstep set has no bounding
		// box of its own, so when most of the invaders are gone only the ones
		// left are tested.
		void moveObjects(std::vector<Object>& all, const WindowDesc& window, FrameRules& rules)
		{
			applyHops(all, window, rules);
			lendCarried(all, rules);

			struct Pair
			{
				Object* first;
				Object* second;
			};

			// Whether each is in play, asked once: a game with many objects out
			// of play (the other rooms of a state) would otherwise ask it for
			// every pair.
			std::vector<char> playing(all.size());
			for (std::size_t i = 0; i < all.size(); ++i)
			{
				playing[i] = rules.inPlay(all[i]) ? 1 : 0;
			}

			std::vector<Pair> pairs;
			for (std::size_t i = 0; i < all.size(); ++i)
			{
				if (!playing[i])
				{
					continue;
				}

				for (std::size_t j = i + 1; j < all.size(); ++j)
				{
					if (playing[j] && canTouch(all[i], all[j], rules))
					{
						pairs.push_back({ &all[i], &all[j] });
					}
				}
			}

			// Where each object is worked out from where it last was set (the start
			// of the frame, or the moment a reaction changed it) and how much of the
			// frame it has been going since, not by adding up the pieces: so an
			// object no reaction touched ends the frame exactly one whole step on,
			// however many reactions the others had.
			std::vector<Vector2f> origin(all.size());
			std::vector<float> since(all.size(), 0.0f);
			for (std::size_t i = 0; i < all.size(); ++i)
			{
				origin[i] = all[i].position;
			}
			float played = 0.0f;

			const auto advanceTo = [&](float total)
			{
				played = total;
				for (std::size_t i = 0; i < all.size(); ++i)
				{
					if (rules.inPlay(all[i])) // TODO: at some point you might wanna collide with invisible objects
					{
						all[i].position = origin[i] + motionOf(all[i]) * (total - since[i]);
					}
				}
			};

			// Anything taken out of play by an earlier reaction this frame (a
			// bullet that just died), or that is not moving, sits out.
			const auto sweep = [&](const Pair& pair, float left) -> std::optional<SweepHit>
			{
				Object& a = *pair.first;
				Object& b = *pair.second;
				if (!rules.inPlay(a) || !rules.inPlay(b) || !canTouch(a, b, rules) || (!isMoving(a) && !isMoving(b)))
				{
					return std::nullopt;
				}
				return CollisionDetector::sweep(a, motionOf(a) * left, b, motionOf(b) * left);
			};

			// A reaction that moved, turned, showed or hid anything starts that
			// object again from where it is now.
			struct Before
			{
				Vector2f position;
				Vector2f motion;
				bool inPlay;
			};
			std::vector<Before> before(all.size());

			playFrame(pairs, sweep, advanceTo, [&](const Pair& pair, Edge edgeOfSecond, float at)
				{
					// What the touch was, found once, before anything answers it.
					const Contact contact = CollisionDetector::contact(*pair.first, *pair.second, edgeOfSecond, at);

					for (std::size_t i = 0; i < all.size(); ++i)
					{
						before[i] = { all[i].position, motionOf(all[i]), rules.inPlay(all[i]) };
					}

					rules.touched(*pair.first, *pair.second, contact);

					for (std::size_t i = 0; i < all.size(); ++i)
					{
						if (all[i].position != before[i].position || motionOf(all[i]) != before[i].motion || rules.inPlay(all[i]) != before[i].inPlay)
						{
							origin[i] = all[i].position;
							since[i] = played;
						}
					}
				});

			for (auto& object : all)
			{
				object.hopped = false;
			}
		}
	}

	void playOneFrame(std::vector<Object>& objects, const WindowDesc& window, FrameRules& rules)
	{
		// A picture that changes with time (an <animation>) counts this frame
		// whether or not the object is moving; only what is in play does, so a
		// pause or a menu holds it where it was.
		for (auto& object : objects)
		{
			if (rules.inPlay(object)) { object.advanceAnimation(); }
		}

		rules.timers();

		rules.followPaths();

		// Who walks a maze chooses its way, now that a path may have let it go.
		walkMazes(objects, rules);

		applyClimbing(objects, rules);

		applyAcceleration(objects, rules);

		pullAndSeek(objects, rules);

		riseAndFall(objects, rules);

		// Screen-edge checks: independent per object, order doesn't matter.
		for (auto& object : objects)
		{
			if (rules.inPlay(object) && object.collisionData.enabled && isMoving(object))
			{
				for (const Edge edge : { Edge::Top, Edge::Bottom, Edge::Left, Edge::Right })
				{
					if (CollisionDetector::touchesScreenEdge(object, window, edge))
					{
						rules.atEdge(object, edge);
					}
				}
			}
		}

		// Movement and object-against-object collisions are one job, so that
		// a fast or small object cannot jump over a thin one between frames.
		moveObjects(objects, window, rules);

		// The edge checks above run before the move, and only for an object
		// that is moving, so on their own they let a stuck object end the
		// frame poking out past the wall (and stay there once it stopped).
		for (auto& object : objects)
		{
			if (rules.inPlay(object) && object.collisionData.enabled)
			{
				for (const Edge edge : { Edge::Top, Edge::Bottom, Edge::Left, Edge::Right })
				{
					if (rules.sticksAt(object, edge) && CollisionDetector::touchesScreenEdge(object, window, edge))
					{
						stick(object, window, edge);
					}
				}
			}
		}

		holdCarried(objects);

		drawTethers(objects);

		castShadows(objects);

		lightAndDark(objects, rules);

		rules.conditions();
	}
}
