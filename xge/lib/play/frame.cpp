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
		// Whether it has moved, so its touches are looked for: it is going
		// somewhere this frame (a velocity, or riding something), or it is
		// not where the last frame left it (anything that put it somewhere:
		// a hop, a <set>, a teleport, a spawn). Two that have not moved
		// cannot have come to touch, and are not looked at.
		bool isMoving(const Object& object) noexcept
		{
			const Vector2f motion = motionOf(object);
			return motion.x != 0 || motion.y != 0 || object.spin != 0 || object.position != object.before.position;
		}

		// Whether a and b can touch at all, whatever their rules: both have
		// their collisions on, neither is in the middle of a jump (it is over
		// everything), and they are not one lockstep block (the invaders
		// never collide with each other). Whether a rule answers is the
		// game's (FrameRules::canCollide), asked once (FrameRules::partners).
		bool canTouch(const Object& a, const Object& b) noexcept
		{
			if (!a.collisionsOn || !b.collisionsOn) { return false; }
			if (a.isAirborne() || b.isAirborne()) { return false; }
			return a.collisionData.lockstep == 0 || a.collisionData.lockstep != b.collisionData.lockstep;
		}

		// Climbing (a <climb> in an action, held): an object at a ladder - an
		// object of its climbClass with the climber's middle over it and its
		// feet between the ladder's top and bottom - that is on the ground and
		// asked to go up or down it, and can, gets on it, lined up with its
		// middle. On it, it goes the step held each frame (none held: it stays
		// put), its feet kept between the two ends, with no pull and no
		// landing. The step is its velocity (none across, so no key walks it
		// off), so it touches what it passes as anything moving does. Reaching
		// either end it gets off, standing there, and the keys held take it on
		// as before; a climb a rule or timer started lets go there. A ladder
		// is also pieces stacked one on another, as wide and lined up: the
		// whole stack is one ladder.
		// Whether something the climber lands on (a class or object one of
		// its <land /> rules names) is under it with its top at `feet`, give
		// or take a couple of pixels (a sloped girder). Under any of it: a
		// ladder can come up through a hole between two pieces of ground.
		bool floorAt(const std::vector<Object>& objects, const Object& climber, float feet, FrameRules& rules)
		{
			for (const auto& rule : climber.collisionData.others)
			{
				if (std::none_of(rule.commands.begin(), rule.commands.end(), [](const Command& command) { return std::holds_alternative<CmdLand>(shapeOf(command)); }))
				{
					continue;
				}
				for (const Object& other : objects)
				{
					if (&other == &climber || !rules.inPlay(other)) { continue; }
					if (!rule.filterClass.empty() && other.objClass != rule.filterClass) { continue; }
					if (!rule.filterObject.empty() && !other.named(rule.filterObject)) { continue; }
					const bool under = climber.position.x < other.position.x + other.size.x && other.position.x < climber.position.x + climber.size.x;
					if (under && std::abs(other.position.y - feet) <= 2.0f)
					{
						return true;
					}
				}
			}
			return false;
		}

		void applyClimbing(std::vector<Object>& objects, FrameRules& rules)
		{
			for (auto& object : objects)
			{
				if (object.climbClass.empty() || !rules.inPlay(object)) { continue; }

				const auto isLadder = [&](const Object& other)
					{
						return &other != &object && other.objClass == object.climbClass && rules.inPlay(other);
					};
				const Box climber{ object.position, object.size };
				const auto ladder = std::find_if(objects.begin(), objects.end(), [&](const Object& other)
					{
						return isLadder(other) && atLadder(climber, { other.position, other.size });
					});

				if (ladder == objects.end())
				{
					object.climbing = false;
					continue;
				}

				Box whole{ ladder->position, ladder->size };
				for (bool grew = true; grew;)
				{
					grew = false;
					for (const Object& other : objects)
					{
						if (!isLadder(other) || !(other.size.y > 0.0f) || std::abs(other.position.x - whole.position.x) > 0.5f || std::abs(other.size.x - whole.size.x) > 0.5f) { continue; }
						const Box piece{ other.position, other.size };
						if (std::abs(piece.top() - whole.bottom()) <= 0.5f)
						{
							whole.size.y += piece.size.y;
							grew = true;
						}
						else if (std::abs(piece.bottom() - whole.top()) <= 0.5f)
						{
							whole.position.y -= piece.size.y;
							whole.size.y += piece.size.y;
							grew = true;
						}
					}
				}

				// A player's climb holds at an end with nothing there to
				// stand on (the top of a broken ladder) rather than stepping
				// off into the air; one a rule started lets go anyway.
				const float step = object.activeClimb[1] - object.activeClimb[0];
				const bool holding = object.climbing && !object.climbOnce && step != 0.0f;
				const bool holdTop = holding && step < 0.0f && !floorAt(objects, object, whole.top(), rules);
				const bool holdBottom = holding && step > 0.0f && !floorAt(objects, object, whole.bottom(), rules);
				const bool wasClimbing = object.climbing;
				if (!climbLadder(object.position, object.velocity, object.size, step, object.climbing, object.grounded, whole, holdTop, holdBottom))
				{
					continue;
				}

				if (!wasClimbing) { object.leaping = false; }
				if (wasClimbing && !object.climbing && object.climbOnce)
				{
					object.activeClimb = {};
					object.climbOnce = false;
				}

				if (!object.climbing)
				{
					object.velocity.x = object.activeMoveStep[static_cast<std::size_t>(Direction::Right)]
						- object.activeMoveStep[static_cast<std::size_t>(Direction::Left)];
				}
			}
		}

		// <grip>: what an object's held moves ask for, it goes a part of the way
		// to each frame, so it slides into a walk and out of one. Across,
		// unless a leap holds it; up and down too, when nothing pulls it.
		void applyGrip(std::vector<Object>& objects, FrameRules& rules)
		{
			for (auto& object : objects)
			{
				if (object.grip >= 1.0f || object.climbing || !rules.inPlay(object)) { continue; }

				const auto ease = [&](float& velocity, float wanted)
				{
					velocity += (wanted - velocity) * object.grip;
					if (std::abs(wanted - velocity) < 0.05f) { velocity = wanted; }
				};
				const auto& step = object.activeMoveStep;
				if (!object.leaping)
				{
					ease(object.velocity.x, step[static_cast<std::size_t>(Direction::Right)] - step[static_cast<std::size_t>(Direction::Left)]);
				}
				if (object.acceleration.y == 0.0f)
				{
					ease(object.velocity.y, step[static_cast<std::size_t>(Direction::Down)] - step[static_cast<std::size_t>(Direction::Up)]);
				}
			}
		}

		// How an object is moving, for its <poses>: in the air (a jump under
		// way, or nothing under it while something pulls it) rising or
		// falling; on the ground standing, walking, or, with a <grip> below 1,
		// skidding while it slides another way to the one its keys ask for.
		// Thrusting, before any of those, while a held <thrust> or
		// <accelerate> is on.
		Object::Pose poseOf(const Object& object)
		{
			if (!object.activeThrustAheadBy.empty()
				|| std::any_of(object.activeThrust.begin(), object.activeThrust.end(), [](float amount) { return amount != 0.0f; }))
			{
				return Object::Pose::Thrusting;
			}
			if (object.isAirborne())
			{
				return object.jumpFramesLeft * 2 > object.jumpFrames ? Object::Pose::Rising : Object::Pose::Falling;
			}
			const bool pulled = object.acceleration.y != 0.0f;
			if (pulled && !object.grounded)
			{
				return object.velocity.y < 0.0f ? Object::Pose::Rising : Object::Pose::Falling;
			}
			if (object.velocity.x == 0.0f && (pulled || object.velocity.y == 0.0f)) { return Object::Pose::Standing; }

			const auto& step = object.activeMoveStep;
			const float wanted = step[static_cast<std::size_t>(Direction::Right)] - step[static_cast<std::size_t>(Direction::Left)];
			const auto sign = [](float value) { return (value > 0.0f) - (value < 0.0f); };
			if (object.grip < 1.0f && object.velocity.x != 0.0f && sign(wanted) != sign(object.velocity.x)) { return Object::Pose::Skidding; }
			return Object::Pose::Walking;
		}

		// After the frame's move: which way each object with a <mirror> is
		// going, and the look each with <poses> takes for how it moves, while
		// it shows one of them (a <become> to another look takes it out of
		// its poses until one brings it back).
		void takePoses(std::vector<Object>& objects, FrameRules& rules)
		{
			for (auto& object : objects)
			{
				if (!object.mirrors && !object.hasPoses) { continue; }
				if (!rules.inPlay(object)) { continue; }

				if (object.mirrors)
				{
					const auto& step = object.activeMoveStep;
					const float wanted = step[static_cast<std::size_t>(Direction::Right)] - step[static_cast<std::size_t>(Direction::Left)];
					if (wanted != 0.0f) { object.goingLeft = wanted < 0.0f; }
					else if (object.isAirborne() && object.jumpStep.x != 0.0f) { object.goingLeft = object.jumpStep.x < 0.0f; }
					else if (object.velocity.x != 0.0f) { object.goingLeft = object.velocity.x < 0.0f; }
					object.faceMirror();
				}

				if (!object.hasPoses) { continue; }
				const auto& poses = object.poseLooks;
				if (std::find(poses.begin(), poses.end(), object.look) == poses.end()) { continue; }

				const std::size_t look = poses[static_cast<std::size_t>(poseOf(object))];
				if (look != object.look) { object.showLook(look); }
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
				// Nothing to do for most things (a wall): looked at before
				// asking whether it is in play.
				const bool thrusting = object.activeThrust[0] != 0.0f || object.activeThrust[1] != 0.0f
					|| object.activeThrust[2] != 0.0f || object.activeThrust[3] != 0.0f;
				if (!thrusting && !object.hasHeading && object.drag == 0.0f && object.acceleration.x == 0.0f && object.acceleration.y == 0.0f)
				{
					continue;
				}

				// On a ladder nothing pulls it (applyClimbing moves it).
				if (!rules.inPlay(object) || object.climbing) { continue; }

				Vector2f change = object.acceleration;

				// A thruster with no fuel does nothing.
				const auto burnFuel = [&](const std::string& burn)
				{
					return burn.empty() || rules.burn(object, burn);
				};

				for (std::size_t i = 0; i < object.activeThrust.size(); ++i)
				{
					const float amount = object.activeThrust[i];
					if (amount == 0.0f) { continue; }

					if (!burnFuel(object.activeThrustBurn[i])) { continue; }

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

					// A swing: held, toward where it swings to; let go, back
					// to where it started; never past either, by the shorter way.
					// The turn is its spin this frame: the move sweeps it from
					// where it was to where it is now (moveObjects), and what
					// it strikes on the way takes its speed there (velocityAt).
					object.spin = 0.0f;
					if (object.swingRate > 0.0f)
					{
						const float target = object.swingHeld ? object.swingTo : object.start.heading;
						const float apart = turnBetween(object.heading, target);
						if (apart != 0.0f)
						{
							const float step = std::clamp(apart, -object.swingRate, object.swingRate);
							object.heading = std::fmod(object.heading + step + 360.0f, 360.0f);
							object.showHeading();
							object.spin = step;
						}
					}

					for (const auto& [action, thrust] : object.activeThrustAheadBy)
					{
						if (burnFuel(thrust.second))
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
		// lands and not by what it would have brushed past on the way (being
		// somewhere new, it counts as having moved). A hop that would take it
		// out of the window is refused.
		void applyHops(std::vector<Object>& objects, const WindowDesc& window, FrameRules& rules)
		{
			const Vector2f windowSize{ window.width, window.height };

			for (auto& object : objects)
			{
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

					// An arc: drawn up off its way and back down, highest
					// halfway, on the ground again as it lands.
					if (object.jumpArc > 0.0f)
					{
						const float t = 1.0f - static_cast<float>(object.jumpFramesLeft) / static_cast<float>(object.jumpFrames);
						object.altitude = object.jumpFramesLeft > 0 ? 4.0f * object.jumpArc * t * (1.0f - t) : 0.0f;
					}
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

		// <pull>: toward the middle of the nearest of each, that much speed a
		// frame. <seek>: the way it goes turned toward the nearest of its
		// target, and its heading with it.
		void pullAndSeek(std::vector<Object>& objects, FrameRules& rules)
		{
			const auto inPlay = [&](const Object& other) { return rules.inPlay(other); };
			for (auto& object : objects)
			{
				if ((object.pulls.empty() && object.seekTarget.empty()) || !rules.inPlay(object)) { continue; }

				const Vector2f middle = middleOf(object);
				for (const auto& [name, strength] : object.pulls)
				{
					const Object* toward = nearestInPlay(objects, object, name, inPlay);
					if (!toward) { continue; }
					if (const auto way = wayTo(middle, middleOf(*toward)))
					{
						object.velocity = object.velocity + *way * strength;
					}
				}

				if (object.seekTarget.empty()) { continue; }
				const Object* target = nearestInPlay(objects, object, object.seekTarget, inPlay);
				if (!target) { continue; }
				const Vector2f before = object.velocity;
				object.velocity = turnedToward(object.velocity, middleOf(*target) - middle, object.seekRate);
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
		// play while both ends are (an end may be a copy not made yet).
		void drawTethers(std::vector<Object>& objects)
		{
			for (auto& tether : objects)
			{
				if (tether.tetherFrom.empty()) { continue; }

				const Object* from = named(objects, tether.tetherFrom);
				const Object* to = named(objects, tether.tetherTo);
				tether.isVisible = from && to && from->isVisible && to->isVisible;
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
				tether.picture.kind = ShapeKind::Line;
				tether.picture.size = tether.size;
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
				std::size_t i;
				std::size_t j;
			};

			// Whether each is in play, asked once and again only after a
			// reaction (which may hide something, or change the state): asking
			// for every pair, every time the frame is swept on, was most of a
			// busy frame.
			std::vector<char> playing(all.size());
			for (std::size_t i = 0; i < all.size(); ++i)
			{
				playing[i] = rules.inPlay(all[i]) ? 1 : 0;
			}

			// Only the pairs a rule answers for (partners), and of those the
			// ones that can touch now.
			const std::vector<std::vector<std::size_t>>& partners = rules.partners(all);
			std::vector<Pair> pairs;
			for (std::size_t i = 0; i < all.size(); ++i)
			{
				if (!playing[i])
				{
					continue;
				}

				for (const std::size_t j : partners[i])
				{
					if (playing[j] && canTouch(all[i], all[j]))
					{
						pairs.push_back({ &all[i], &all[j], i, j });
					}
				}
			}

			// Where each object is worked out from where it last was set (the start
			// of the frame, or the moment a reaction changed it) and how much of the
			// frame it has been going since, not by adding up the pieces: so an
			// object no reaction touched ends the frame exactly one whole step on,
			// however many reactions the others had.
			// One that turns this frame (a swing: its spin) goes back to the
			// way it faced at the start, and turns through the frame as it
			// moves: at any moment of it, it faces turnFrom plus that share of
			// its spin, about its pivot, which stays put (or goes with it).
			std::vector<Vector2f> origin(all.size());
			std::vector<float> since(all.size(), 0.0f);
			std::vector<float> turnFrom(all.size(), 0.0f);
			std::vector<int> originDegrees(all.size(), 0);
			const auto spinning = [&](std::size_t i) { return all[i].spin != 0.0f && all[i].hasHeading; };
			const auto facingAt = [&](std::size_t i, float total)
			{
				return std::fmod(turnFrom[i] + all[i].spin * total + 720.0f, 360.0f);
			};
			for (std::size_t i = 0; i < all.size(); ++i)
			{
				if (spinning(i))
				{
					turnFrom[i] = std::fmod(all[i].heading - all[i].spin + 720.0f, 360.0f);
					all[i].heading = turnFrom[i];
					all[i].showHeading();
				}
				origin[i] = all[i].position;
				originDegrees[i] = all[i].pivotDegrees;
			}
			float played = 0.0f;

			const auto advanceTo = [&](float total)
			{
				played = total;
				for (std::size_t i = 0; i < all.size(); ++i)
				{
					if (playing[i]) // TODO: at some point you might wanna collide with invisible objects
					{
						all[i].position = origin[i] + motionOf(all[i]) * (total - since[i]);
						if (spinning(i))
						{
							all[i].pivotDegrees = originDegrees[i];
							all[i].heading = facingAt(i, total);
							all[i].showHeading();
						}
					}
				}
			};

			// The box each covers over the rest of the frame, found once: until
			// a reaction changes it, what it covers over what is left of the
			// frame later is inside it. A changed one's is found again when
			// next asked (`fresh`).
			std::vector<Box> reach(all.size());
			std::vector<char> fresh(all.size(), 0);
			const auto reachOf = [&](std::size_t i, const Vector2f& move) -> const Box&
			{
				if (!fresh[i])
				{
					reach[i] = reachOver(all[i], move);
					fresh[i] = 1;
				}
				return reach[i];
			};

			// Anything taken out of play by an earlier reaction this frame (a
			// bullet that just died), or that is not moving, sits out. Two too
			// far apart to meet over the rest of the frame are far (playFrame
			// leaves them be until a reaction changes one of them).
			const auto sweep = [&](const Pair& pair, float left) -> PairSweep
			{
				Object& a = *pair.first;
				Object& b = *pair.second;
				if (!playing[pair.i] || !playing[pair.j] || !canTouch(a, b) || (!isMoving(a) && !isMoving(b)))
				{
					return {};
				}

				const Vector2f moveA = motionOf(a) * left;
				const Vector2f moveB = motionOf(b) * left;
				if (!boxesMeet(reachOf(pair.i, moveA), reachOf(pair.j, moveB)))
				{
					return { std::nullopt, true };
				}
				if (spinning(pair.i) || spinning(pair.j))
				{
					return { sweepTurning(a, moveA, a.spin * left, b, moveB, b.spin * left), false };
				}
				return { sweepObjects(a, moveA, b, moveB), false };
			};

			// A reaction that moved, turned, showed or hid anything starts that
			// object again from where it is now.
			struct Before
			{
				Vector2f position;
				Vector2f motion;
				Vector2f size;
				float heading;
			};
			std::vector<Before> before(all.size());

			playFrame(pairs, all.size(), sweep, advanceTo, [&](const Pair& pair, Edge edgeOfSecond, float at, std::vector<std::size_t>& changed)
				{
					// What the touch was, found once, before anything answers it.
					const Contact contact = contactBetween(*pair.first, *pair.second, edgeOfSecond, at);

					for (std::size_t i = 0; i < all.size(); ++i)
					{
						before[i] = { all[i].position, motionOf(all[i]), all[i].size, all[i].heading };
					}

					rules.touched(*pair.first, *pair.second, contact);

					for (std::size_t i = 0; i < all.size(); ++i)
					{
						// Turned some other way by a reaction: it stops turning there.
						const bool turned = spinning(i) && all[i].heading != before[i].heading;
						if (turned) { all[i].spin = 0.0f; }

						const char nowPlaying = rules.inPlay(all[i]) ? 1 : 0;
						if (turned || all[i].position != before[i].position || motionOf(all[i]) != before[i].motion || nowPlaying != playing[i])
						{
							origin[i] = all[i].position;
							originDegrees[i] = all[i].pivotDegrees;
							since[i] = played;
							fresh[i] = 0;
							changed.push_back(i);
						}
						else if (all[i].size != before[i].size)
						{
							fresh[i] = 0;
							changed.push_back(i);
						}
						playing[i] = nowPlaying;
					}
				});
		}
	}

	void FrameRules::keepFor(const std::vector<Object>& objects)
	{
		if (keptFor == objects.data() && keptCount == objects.size()) { return; }
		keptFor = objects.data();
		keptCount = objects.size();
		partnersFound = false;
		partnerLists.clear();
		mazeGrids.clear();
	}

	const std::vector<std::vector<std::size_t>>& FrameRules::partners(const std::vector<Object>& objects)
	{
		keepFor(objects);
		if (!partnersFound)
		{
			// Only a pair with a rule between them is asked about: one with
			// rules against every later object, one without against the later
			// ones that have rules.
			std::vector<char> asks(objects.size());
			std::vector<std::size_t> askers;
			for (std::size_t i = 0; i < objects.size(); ++i)
			{
				asks[i] = hasRules(objects[i]) ? 1 : 0;
				if (asks[i]) { askers.push_back(i); }
			}

			partnerLists.assign(objects.size(), {});
			for (std::size_t i = 0; i < objects.size(); ++i)
			{
				const auto ask = [&](std::size_t j)
				{
					if (canCollide(objects[i], objects[j])) { partnerLists[i].push_back(j); }
				};
				if (asks[i])
				{
					for (std::size_t j = i + 1; j < objects.size(); ++j) { ask(j); }
				}
				else
				{
					for (auto later = std::upper_bound(askers.begin(), askers.end(), i); later != askers.end(); ++later) { ask(*later); }
				}
			}
			partnersFound = true;
		}
		return partnerLists;
	}

	const MazeGrid& FrameRules::mazeGrid(const std::vector<Object>& objects, const std::string& map)
	{
		keepFor(objects);
		auto found = mazeGrids.find(map);
		if (found != mazeGrids.end()) { return found->second; }

		MazeGrid grid;
		for (std::size_t i = 0; i < objects.size(); ++i)
		{
			const Object& object = objects[i];
			if (object.mapName != map) { continue; }
			if (grid.at.empty())
			{
				grid.origin = object.mapOrigin;
				grid.tile = object.mapTile;
				grid.columns = object.mapColumns;
				grid.rows = object.mapRows;
				grid.at.resize(static_cast<std::size_t>(grid.columns) * static_cast<std::size_t>(grid.rows));
			}
			if (grid.onMap(object.mapColumn, object.mapRow))
			{
				grid.at[static_cast<std::size_t>(object.mapRow) * static_cast<std::size_t>(grid.columns) + static_cast<std::size_t>(object.mapColumn)].push_back(i);
			}
		}
		return mazeGrids.emplace(map, std::move(grid)).first->second;
	}

	void playOneFrame(std::vector<Object>& objects, const WindowDesc& window, FrameRules& rules)
	{
		// A picture that changes with time (an <animation>) counts this frame
		// whether or not the object is moving; only what is in play does, so a
		// pause or a menu holds it where it was.
		const float second = 1.0f / static_cast<float>(window.framerate > 0 ? window.framerate : 60);
		for (auto& object : objects)
		{
			// Just loaded or reset: where it is now is its place, not a move.
			if (object.placed) { object.remember(); }

			if (object.animationBitmaps.size() > 1 && rules.inPlay(object)) { object.advanceAnimation(); }
			if (object.blink > 0.0f && rules.inPlay(object)) { object.blink = std::max(0.0f, object.blink - second); }
		}

		rules.timers();

		rules.followPaths();

		rules.cellMoves();

		// Who walks a maze chooses its way, now that a path may have let it go.
		walkMazes(objects, rules);

		applyClimbing(objects, rules);

		applyGrip(objects, rules);

		applyAcceleration(objects, rules);

		pullAndSeek(objects, rules);

		riseAndFall(objects, rules);

		// Screen-edge checks: independent per object, order doesn't matter.
		for (auto& object : objects)
		{
			// A piece of a cell map meets its map's edges by its moves.
			if (object.collisionsOn && !object.onCells && isMoving(object) && rules.inPlay(object))
			{
				for (const Edge edge : { Edge::Top, Edge::Bottom, Edge::Left, Edge::Right })
				{
					if (touchesScreenEdge(object, window, edge))
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
			if (object.collisionsOn && rules.inPlay(object))
			{
				for (const Edge edge : { Edge::Top, Edge::Bottom, Edge::Left, Edge::Right })
				{
					if (touchesScreenEdge(object, window, edge) && rules.sticksAt(object, edge))
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

		takePoses(objects, rules);

		// How everything ended up, to tell next frame what has moved. A
		// condition that puts something elsewhere has moved it.
		for (auto& object : objects) { object.remember(); }

		rules.conditions();
	}
}
