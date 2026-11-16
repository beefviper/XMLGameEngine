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
#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace xge
{
	namespace
	{
		// What a share lost each unit of time (a <drag>, a <grip>) leaves
		// over a step: (1 - share) to the power t. Worked out once for each
		// share a step, not for each object.
		class KeptOver
		{
		public:
			explicit KeptOver(float stepT) : t(stepT) {}
			float operator()(float share)
			{
				if (share != lastShare)
				{
					lastShare = share;
					lastKept = share >= 1.0f ? 0.0f : std::pow(1.0f - share, t);
				}
				return lastKept;
			}

		private:
			float t;
			float lastShare{ -1.0f };
			float lastKept{ 1.0f };
		};

		// Whether it has moved or changed, so its touches are looked for: it
		// is going somewhere this frame (a velocity, or riding something), it
		// is not where the last frame left it (anything that put it
		// somewhere: a hop, a <set>, a teleport, a spawn), it shows another
		// picture (a look, an animation's frame, a size, mirrored), or it
		// came into play or had its collisions turned on. Two that have
		// neither moved nor changed cannot have come to touch, and are not
		// looked at.
		bool isMoving(const Object& object) noexcept
		{
			const Vector2f motion = motionOf(object, 1.0f);
			return motion.x != 0 || motion.y != 0 || object.turning.spin != 0 || object.position != object.before.position
				|| object.animation.shareBefore > 0.0f
				|| !(object.shown() == object.shownBefore)
				|| (object.collisionsOn && !object.before.collisionsOn) || (object.isVisible && !object.before.isVisible);
		}

		// Whether a and b can touch at all, whatever their rules: both have
		// their collisions on, neither is in the middle of a timed hop (it is over
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
				if (object.footing.climbClass.empty() || !rules.inPlay(object)) { continue; }

				const auto isLadder = [&](const Object& other)
					{
						return &other != &object && other.objClass == object.footing.climbClass && rules.inPlay(other);
					};
				const Box climber{ object.position, object.size };
				const auto ladder = std::find_if(objects.begin(), objects.end(), [&](const Object& other)
					{
						return isLadder(other) && atLadder(climber, { other.position, other.size });
					});

				if (ladder == objects.end())
				{
					object.footing.climbing = false;
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
				const float step = object.footing.activeClimb[1] - object.footing.activeClimb[0];
				const bool holding = object.footing.climbing && !object.footing.climbOnce && step != 0.0f;
				const bool holdTop = holding && step < 0.0f && !floorAt(objects, object, whole.top(), rules);
				const bool holdBottom = holding && step > 0.0f && !floorAt(objects, object, whole.bottom(), rules);
				const bool wasClimbing = object.footing.climbing;
				if (!climbLadder(object.position, object.velocity, object.size, step, object.footing.climbing, object.footing.grounded, whole, holdTop, holdBottom))
				{
					continue;
				}

				if (!wasClimbing) { object.footing.jumping = false; }
				if (wasClimbing && !object.footing.climbing && object.footing.climbOnce)
				{
					object.footing.activeClimb = {};
					object.footing.climbOnce = false;
				}

				if (!object.footing.climbing)
				{
					object.velocity.x = object.drive.moveStep[static_cast<std::size_t>(Direction::Right)]
						- object.drive.moveStep[static_cast<std::size_t>(Direction::Left)];
				}
			}
		}

		// The ground: a thing with a <grip> whose middle is over a <surface>
		// (the last in the file's order, of those in play) has that
		// surface's drag and grip this frame in place of its own.
		void findGround(std::vector<Object>& objects, FrameRules& rules)
		{
			std::vector<const Object*> surfaces;
			for (auto& object : objects)
			{
				object.surface.groundDrag = object.motion.drag;
				object.surface.groundGrip = object.motion.grip;
				if (object.surface.is && rules.inPlay(object)) { surfaces.push_back(&object); }
			}
			if (surfaces.empty()) { return; }

			for (auto& object : objects)
			{
				if (object.motion.grip >= 1.0f || object.surface.is || !rules.inPlay(object)) { continue; }
				const Vector2f middle = middleOf(object);
				for (auto ground = surfaces.rbegin(); ground != surfaces.rend(); ++ground)
				{
					const Object& surface = **ground;
					const Vector2f size = sizeOf(surface);
					if (middle.x < surface.position.x || middle.y < surface.position.y
						|| middle.x >= surface.position.x + size.x || middle.y >= surface.position.y + size.y)
					{
						continue;
					}
					if (surface.surface.drag) { object.surface.groundDrag = *surface.surface.drag; }
					if (surface.surface.grip) { object.surface.groundGrip = *surface.surface.grip; }
					break;
				}
			}
		}

		// <grip>: what an object's held moves ask for, it goes a part of the way
		// to each frame, so it slides into a walk and out of one. Across,
		// unless a jump holds it; up and down too, when nothing pulls it.
		// With a <heading> it is a wheel's grip instead: each frame its speed
		// across the way it faces loses that share, so it goes where it
		// points and slides when the grip is low (a car on oil).
		void applyGrip(std::vector<Object>& objects, FrameRules& rules, const StepTime& time)
		{
			KeptOver kept(time.t);
			// Close enough to the speed asked for to be it: a twentieth of a
			// pixel a sixtieth of a second.
			const float snap = 0.05f / time.sixtieth;
			for (auto& object : objects)
			{
				if (object.motion.grip >= 1.0f || object.footing.climbing || !rules.inPlay(object)) { continue; }

				if (object.turning.hasHeading)
				{
					const Vector2f way = ahead(object.heading);
					const float along = object.velocity.x * way.x + object.velocity.y * way.y;
					const Vector2f across = object.velocity - way * along;
					object.velocity -= across * (1.0f - kept(object.surface.groundGrip));
					continue;
				}

				const float keeps = kept(object.surface.groundGrip);
				const auto ease = [&](float& velocity, float wanted)
				{
					velocity = wanted + (velocity - wanted) * keeps;
					if (std::abs(wanted - velocity) < snap) { velocity = wanted; }
				};
				const auto& step = object.drive.moveStep;
				if (!object.footing.jumping)
				{
					ease(object.velocity.x, step[static_cast<std::size_t>(Direction::Right)] - step[static_cast<std::size_t>(Direction::Left)]);
				}
				if (object.acceleration.y == 0.0f)
				{
					ease(object.velocity.y, step[static_cast<std::size_t>(Direction::Down)] - step[static_cast<std::size_t>(Direction::Up)]);
				}
			}
		}

		// How an object is moving, for its <poses>: in the air (a timed hop under
		// way, or nothing under it while something pulls it) rising or
		// falling; on the ground standing, walking, or, with a <grip> below 1,
		// skidding while it slides another way to the one its keys ask for.
		// Thrusting, before any of those, while a held <thrust> or
		// <thrust direction> is on.
		Object::Pose poseOf(const Object& object)
		{
			if (!object.drive.thrustAhead.empty()
				|| std::any_of(object.drive.thrust.begin(), object.drive.thrust.end(), [](float amount) { return amount != 0.0f; }))
			{
				return Object::Pose::Thrusting;
			}
			if (object.isAirborne())
			{
				return object.flight.flown * 2.0 < static_cast<double>(object.flight.seconds) ? Object::Pose::Rising : Object::Pose::Falling;
			}
			const bool pulled = object.acceleration.y != 0.0f;
			if (pulled && !object.footing.grounded)
			{
				return object.velocity.y < 0.0f ? Object::Pose::Rising : Object::Pose::Falling;
			}
			if (object.velocity.x == 0.0f && (pulled || object.velocity.y == 0.0f)) { return Object::Pose::Standing; }

			const auto& step = object.drive.moveStep;
			const float wanted = step[static_cast<std::size_t>(Direction::Right)] - step[static_cast<std::size_t>(Direction::Left)];
			const auto sign = [](float value) { return (value > 0.0f) - (value < 0.0f); };
			if (object.motion.grip < 1.0f && object.velocity.x != 0.0f && sign(wanted) != sign(object.velocity.x)) { return Object::Pose::Skidding; }
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
				if (!object.mirror.on && !object.looks.hasPoses) { continue; }
				if (!rules.inPlay(object)) { continue; }

				if (object.mirror.on)
				{
					const auto& step = object.drive.moveStep;
					const float wanted = step[static_cast<std::size_t>(Direction::Right)] - step[static_cast<std::size_t>(Direction::Left)];
					if (wanted != 0.0f) { object.mirror.goingLeft = wanted < 0.0f; }
					else if (object.isAirborne() && object.flight.whole.x != 0.0f) { object.mirror.goingLeft = object.flight.whole.x < 0.0f; }
					else if (object.velocity.x != 0.0f) { object.mirror.goingLeft = object.velocity.x < 0.0f; }
					object.faceMirror();
				}

				if (!object.looks.hasPoses) { continue; }
				const auto& poses = object.looks.poses;
				if (std::find(poses.begin(), poses.end(), object.looks.shown) == poses.end()) { continue; }

				const std::size_t look = poses[static_cast<std::size_t>(poseOf(object))];
				if (look != object.looks.shown) { object.showLook(look); }
			}
		}

		// A constant pull (<acceleration>, gravity being one with only a y)
		// and held thrust (<thrust direction>) both change an object's velocity,
		// once a frame, before it is moved. Thrust that burns a variable takes
		// 1 off it every frame it is on, and does nothing while the variable
		// is at 0 or below: a ship out of fuel still falls.
		void applyAcceleration(std::vector<Object>& objects, FrameRules& rules, const StepTime& step)
		{
			const float t = step.t;
			const float seconds = step.seconds;
			KeptOver kept(t);
			for (auto& object : objects)
			{
				object.motion.gained = {};

				// Nothing to do for most things (a wall): looked at before
				// asking whether it is in play.
				const bool thrusting = object.drive.thrust[0] != 0.0f || object.drive.thrust[1] != 0.0f
					|| object.drive.thrust[2] != 0.0f || object.drive.thrust[3] != 0.0f;
				if (!thrusting && !object.turning.hasHeading && object.surface.groundDrag == 0.0f && object.acceleration.x == 0.0f && object.acceleration.y == 0.0f)
				{
					continue;
				}

				// On a ladder nothing pulls it (applyClimbing moves it).
				if (!rules.inPlay(object) || object.footing.climbing) { continue; }

				// At rest (Object::Rest): nothing pulls it while it lies
				// where it came to rest, still, on something still there as
				// it was. Anything else wakes it.
				if (object.motion.friction > 0.0f)
				{
					if (object.rest.settledAge < 1000.0f) { object.rest.settledAge += seconds; }
					if (object.rest.resting)
					{
						object.rest.resting = object.velocity.x == 0.0f && object.velocity.y == 0.0f && object.position == object.rest.at
							&& std::all_of(object.rest.on.begin(), object.rest.on.end(), [&](const Object::Support& support)
								{
									const auto under = std::find_if(objects.begin(), objects.end(), [&](const Object& other) { return other.name == support.name; });
									return under != objects.end() && rules.inPlay(*under) && under->position == support.at
										&& under->heading == support.heading && under->turning.spin == 0.0f;
								});
					}
					if (object.rest.resting) { continue; }
				}

				const Vector2f was = object.velocity;
				Vector2f change = object.acceleration;

				// A thruster with no fuel does nothing.
				const auto burnFuel = [&](const std::string& burn)
				{
					return burn.empty() || rules.burn(object, burn);
				};

				for (std::size_t i = 0; i < object.drive.thrust.size(); ++i)
				{
					const float amount = object.drive.thrust[i];
					if (amount == 0.0f) { continue; }

					if (!burnFuel(object.drive.thrustBurn[i])) { continue; }

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
				if (object.turning.hasHeading)
				{
					const float turn = object.drive.turn[static_cast<std::size_t>(Direction::Right)]
						- object.drive.turn[static_cast<std::size_t>(Direction::Left)];
					if (turn != 0.0f)
					{
						object.heading = std::fmod(object.heading + turn * t, 360.0f);
						if (object.heading < 0.0f) { object.heading += 360.0f; }
						object.showHeading();
					}

					// A swing: held, it goes on through its stroke toward where
					// it swings to; let go, back through it to where it started.
					// The stroke is eased, as a half wave: it starts from still,
					// is fastest half way and slows to a stop at the end, so
					// its heading, its turn a frame and how that changes all
					// come from one curve. The turn is its spin this frame: the
					// move sweeps it from where it was to where it is now
					// (moveObjects), and what it strikes on the way takes its
					// speed there (velocityAt).
					object.turning.spin = 0.0f;
					if (object.swing.seconds > 0.0f)
					{
						const float stroke = seconds / object.swing.seconds;
						const float done = std::clamp(object.swing.done + (object.swing.held ? stroke : -stroke), 0.0f, 1.0f);
						if (done != object.swing.done)
						{
							object.swing.done = done;
							const float pi = 3.14159265358979323846f;
							const float eased = (1.0f - std::cos(pi * done)) / 2.0f;
							float now = std::fmod(object.start.heading + object.swing.by * eased, 360.0f);
							if (now < 0.0f) { now += 360.0f; }
							object.turning.spin = turnBetween(object.heading, now);
							object.heading = now;
							object.showHeading();
						}
					}

					for (const auto& [action, thrust] : object.drive.thrustAhead)
					{
						if (burnFuel(thrust.second))
						{
							change += ahead(object.heading) * thrust.first;
						}
					}
				}

				object.velocity += change * t;

				// Drag: what is left of the speed after this step.
				if (object.surface.groundDrag != 0.0f)
				{
					object.velocity *= kept(object.surface.groundDrag);
				}
				object.motion.gained = object.velocity - was;
			}
		}

		// Makes the hops queued by hop.*() actions. A hop is a step, not a
		// slide: the object is simply somewhere else, one step away, before
		// this frame's collisions are worked out, so it is judged by where it
		// lands and not by what it would have brushed past on the way (being
		// somewhere new, it counts as having moved). A hop that would take it
		// out of the window is refused.
		void applyHops(std::vector<Object>& objects, const WindowDesc& window, FrameRules& rules, float seconds)
		{
			const Vector2f windowSize{ window.width, window.height };

			for (auto& object : objects)
			{
				// Whatever last frame's collisions said, this frame's start
				// over: what it rides, and whether it stands on something.
				object.riding = {};
				object.footing.grounded = false;

				// A jump under way goes on by this step's share of it, and a
				// step it ends inside lands it at its end; the step it lands,
				// it counts as having moved, so it meets whatever it landed on.
				if (object.isAirborne())
				{
					const double whole = static_cast<double>(object.flight.seconds);
					double flown = object.flight.flown + static_cast<double>(seconds);
					if (flown >= whole - static_cast<double>(kTimeSlack)) { flown = whole; }
					if (rules.inPlay(object))
					{
						const float share = static_cast<float>((flown - object.flight.flown) / whole);
						const Vector2f target = object.position + object.flight.whole * share;
						if (insideWindow({ target, object.size }, windowSize)) { object.position = target; }
					}
					object.flight.flown = flown;

					// An arc: drawn up off its way and back down, highest
					// halfway, on the ground again as it lands.
					if (object.flight.arc > 0.0f)
					{
						const float t = static_cast<float>(flown / whole);
						object.altitude = object.isAirborne() ? 4.0f * object.flight.arc * t * (1.0f - t) : 0.0f;
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
				if (carrier.carry.what.empty()) { continue; }

				Object* item = named(objects, carrier.carry.what);
				if (!item || !rules.inPlay(carrier) || !rules.inPlay(*item))
				{
					carrier.carry.what.clear();
					continue;
				}

				item->riding = motionOf(carrier, 1.0f) - item->velocity;
			}
		}

		// After the move: what is carried is put exactly where it is held
		// (the carrier may have been stopped on the way, or hopped), and
		// what was put down can be picked up again once the two are apart.
		void holdCarried(std::vector<Object>& objects)
		{
			for (auto& carrier : objects)
			{
				if (!carrier.carry.what.empty())
				{
					if (Object* item = named(objects, carrier.carry.what))
					{
						item->position = carrier.position + carrier.carry.offset;
					}
				}

				if (!carrier.carry.dropped.empty())
				{
					const Object* item = named(objects, carrier.carry.dropped);
					if (!item || !boxesOverlap({ carrier.position, sizeOf(carrier) }, { item->position, sizeOf(*item) }))
					{
						carrier.carry.dropped.clear();
					}
				}
			}
		}

		// <pull>: toward the middle of the nearest of each, that much speed a
		// frame. <seek>: the way it goes turned toward the nearest of its
		// target, and its heading with it.
		void pullAndSeek(std::vector<Object>& objects, FrameRules& rules, float t)
		{
			const auto inPlay = [&](const Object& other) { return rules.inPlay(other); };
			for (auto& object : objects)
			{
				if ((object.pull.toward.empty() && object.pull.seek.empty()) || !rules.inPlay(object)) { continue; }

				const Vector2f middle = middleOf(object);
				for (const auto& [name, strength] : object.pull.toward)
				{
					const Object* toward = nearestInPlay(objects, object, name, inPlay);
					if (!toward) { continue; }
					if (const auto way = wayTo(middle, middleOf(*toward)))
					{
						object.velocity = object.velocity + *way * (strength * t);
						object.motion.gained += *way * (strength * t);
					}
				}

				if (object.pull.seek.empty()) { continue; }
				const Object* target = nearestInPlay(objects, object, object.pull.seek, inPlay);
				if (!target) { continue; }
				const Vector2f before = object.velocity;
				object.velocity = turnedToward(object.velocity, middleOf(*target) - middle, object.pull.seekRate * t);
				if (object.turning.hasHeading && (object.velocity.x != before.x || object.velocity.y != before.y))
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
		void riseAndFall(std::vector<Object>& objects, FrameRules& rules, const StepTime& step)
		{
			const float t = step.t;
			for (auto& object : objects)
			{
				if (!object.aloft.has || !rules.inPlay(object)) { continue; }
				// A constant fall: the altitude goes by the mean of the rise
				// before and after.
				const float fall = object.aloft.fall * t;
				object.altitude += (object.rise - fall * 0.5f) * t;
				object.rise -= fall;
				if (object.altitude > 0.0f) { continue; }
				object.altitude = 0.0f;
				if (object.rise < 0.0f)
				{
					const float up = -object.rise * object.aloft.bounce;
					object.rise = up > object.aloft.fall * step.sixtieth ? up : 0.0f;
				}
			}
		}

		// A <shadow> is kept under what casts it, on the ground, and shown
		// while that is.
		void castShadows(std::vector<Object>& objects)
		{
			for (auto& shadow : objects)
			{
				if (shadow.aloft.shadowOf.empty()) { continue; }
				const Object* caster = named(objects, shadow.aloft.shadowOf);
				if (!caster) { continue; }
				shadow.position = caster->position + sizeOf(*caster) * 0.5f - sizeOf(shadow) * 0.5f;
				shadow.isVisible = caster->isVisible;
			}
		}

		// What is <dark> is drawn only where a <light> of something in play
		// reaches some part of it, and, for a light with <past>es, where the
		// line from the light to that part crosses nothing in play of those
		// classes; an unseen one, from the first time it is lit.
		void lightAndDark(std::vector<Object>& objects, FrameRules& rules)
		{
			struct Light
			{
				const Object* object;
				Vector2f middle;
				float reach;
				// What it does not shine through, in play and in reach, with boxes.
				std::vector<std::pair<const Object*, Box>> blockers;
			};
			std::vector<Light> lights;
			bool anyDark = false;
			for (const auto& object : objects)
			{
				anyDark = anyDark || object.dark.kind != Object::Darkness::None;
				if (object.light > 0.0f && rules.inPlay(object))
				{
					lights.push_back({ &object, object.position + sizeOf(object) * 0.5f, object.light, {} });
				}
			}
			if (!anyDark) { return; }

			// Each light's blockers, gathered once a frame: those of its
			// <past> classes in play within its reach.
			for (Light& light : lights)
			{
				if (light.object->lightPast.empty()) { continue; }
				const Box reach{ light.middle - Vector2f{ light.reach, light.reach }, Vector2f{ light.reach, light.reach } * 2.0f };
				for (const Object& other : objects)
				{
					if (&other == light.object) { continue; }
					const Box box{ other.position, sizeOf(other) };
					if (!boxesMeet(reach, box) || !rules.inPlay(other)) { continue; }
					if (std::find(light.object->lightPast.begin(), light.object->lightPast.end(), other.objClass) == light.object->lightPast.end()) { continue; }
					light.blockers.emplace_back(&other, box);
				}
			}

			// Whether a blocker of the light, other than the lit one, lies
			// across the line from the light to `point`.
			const auto blocked = [](const Light& light, const Object& lit, const Vector2f& point)
			{
				const Box line = sweptBox({ light.middle, {} }, point - light.middle);
				for (const auto& [other, box] : light.blockers)
				{
					if (other == &lit || !boxesMeet(line, box)) { continue; }
					if (lineCrosses(light.middle, point, *other)) { return true; }
				}
				return false;
			};

			for (auto& object : objects)
			{
				if (object.dark.kind == Object::Darkness::None) { continue; }
				const Vector2f size = sizeOf(object);
				const bool lit = std::any_of(lights.begin(), lights.end(), [&](const Light& light)
					{
						const float x = std::clamp(light.middle.x, object.position.x, object.position.x + size.x);
						const float y = std::clamp(light.middle.y, object.position.y, object.position.y + size.y);
						if ((x - light.middle.x) * (x - light.middle.x) + (y - light.middle.y) * (y - light.middle.y) > light.reach * light.reach) { return false; }
						if (light.object->lightPast.empty()) { return true; }
						// Its nearest part, a pixel in toward its middle, so a
						// neighbour it only touches does not stand in the way.
						const Vector2f middle = object.position + size * 0.5f;
						const Vector2f in = middle - Vector2f{ x, y };
						const float far = std::hypot(in.x, in.y);
						const Vector2f point = far > 1.0f ? Vector2f{ x, y } + in * (1.0f / far) : middle;
						return !blocked(light, object, point);
					});
				if (object.dark.kind == Object::Darkness::Unseen)
				{
					object.dark.seen = object.dark.seen || lit;
					object.dark.hidden = !object.dark.seen;
				}
				else
				{
					object.dark.hidden = !lit;
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
				if (tether.tether.from.empty()) { continue; }

				const Object* from = named(objects, tether.tether.from);
				const Object* to = named(objects, tether.tether.to);
				tether.isVisible = from && to && from->isVisible && to->isVisible;
				if (!tether.isVisible) { continue; }

				const Vector2f a = from->position + sizeOf(*from) * 0.5f;
				const Vector2f b = to->position + sizeOf(*to) * 0.5f;
				const float half = static_cast<float>(tether.tether.thickness) * 0.5f;
				const Vector2f corner{ std::floor(std::min(a.x, b.x) - half), std::floor(std::min(a.y, b.y) - half) };

				LineSegment line;
				line.x1 = a.x - corner.x - half;
				line.y1 = a.y - corner.y - half;
				line.x2 = b.x - corner.x - half;
				line.y2 = b.y - corner.y - half;
				line.color = tether.tether.color;
				line.thickness = tether.tether.thickness;

				auto drawn = std::make_shared<const Bitmap>(rasterizeLines({ line }));
				tether.position = corner;
				tether.size = { static_cast<float>(drawn->width), static_cast<float>(drawn->height) };
				tether.drawn.picture.kind = ShapeKind::Line;
				tether.drawn.picture.size = tether.size;
				tether.drawn.bitmap = std::move(drawn);
				tether.drawn.visualDirty = true;
			}
		}

		// A pair of objects in play that a rule answers for and that can touch.
		struct Pair
		{
			Object* first;
			Object* second;
			std::size_t i;
			std::size_t j;
			bool byPicture;
		};

		// What moveObjects keeps about each object over one frame's sweep.
		class FrameSweep
		{
		public:
			FrameSweep(std::vector<Object>& objects, FrameRules& frameRules, float stepT)
				: all(objects), rules(frameRules), t(stepT), playing(objects.size()), origin(objects.size()), since(objects.size(), 0.0f),
				turnFrom(objects.size(), 0.0f), originDegrees(objects.size(), 0), reach(objects.size()), fresh(objects.size(), 0), before(objects.size())
			{
				// Whether each is in play, asked once and again only after a
				// reaction (which may hide something, or change the state): asking
				// for every pair, every time the frame is swept on, was most of a
				// busy frame.
				for (std::size_t i = 0; i < all.size(); ++i)
				{
					playing[i] = rules.inPlay(all[i]) ? 1 : 0;
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
				for (std::size_t i = 0; i < all.size(); ++i)
				{
					if (spinning(i))
					{
						turnFrom[i] = std::fmod(all[i].heading - all[i].turning.spin + 720.0f, 360.0f);
						all[i].heading = turnFrom[i];
						all[i].showHeading();
					}
					origin[i] = all[i].position;
					originDegrees[i] = all[i].pivotDegrees;
				}
			}

			// Only the pairs a rule answers for (partners), and of those the
			// ones that can touch now.
			std::vector<Pair> pairs() const
			{
				const std::vector<std::vector<std::size_t>>& partners = rules.partners(all);
				const std::vector<std::vector<char>>& byPicture = rules.partnersByPicture();
				std::vector<Pair> found;
				for (std::size_t i = 0; i < all.size(); ++i)
				{
					if (!playing[i])
					{
						continue;
					}

					for (std::size_t k = 0; k < partners[i].size(); ++k)
					{
						const std::size_t j = partners[i][k];
						if (playing[j] && canTouch(all[i], all[j]))
						{
							found.push_back({ &all[i], &all[j], i, j, byPicture[i][k] != 0 });
						}
					}
				}
				return found;
			}

			void advanceTo(float total)
			{
				played = total;
				for (std::size_t i = 0; i < all.size(); ++i)
				{
					if (playing[i])
					{
						all[i].position = origin[i] + sweptMotion(all[i], t) * (total - since[i]);
						if (spinning(i))
						{
							all[i].pivotDegrees = originDegrees[i];
							all[i].heading = facingAt(i, total);
							all[i].showHeading();
						}
					}
				}
			}

			// Anything taken out of play by an earlier reaction this frame (a
			// bullet that just died), or that is not moving, sits out. Two too
			// far apart to meet over the rest of the frame are far (playFrame
			// leaves them be until a reaction changes one of them).
			PairSweep sweep(const Pair& pair, float left)
			{
				Object& a = *pair.first;
				Object& b = *pair.second;
				if (!playing[pair.i] || !playing[pair.j] || !canTouch(a, b) || (!isMoving(a) && !isMoving(b)))
				{
					return {};
				}

				const Vector2f moveA = sweptMotion(a, t) * left;
				const Vector2f moveB = sweptMotion(b, t) * left;
				if (!boxesMeet(reachOf(pair.i, moveA), reachOf(pair.j, moveB)))
				{
					return { std::nullopt, true };
				}
				if (spinning(pair.i) || spinning(pair.j))
				{
					return { sweepTurning(a, moveA, a.turning.spin * left, b, moveB, b.turning.spin * left), false };
				}
				return { sweepObjects(a, moveA, b, moveB, pair.byPicture), false };
			}

			// The pair's rules run. A reaction that moved, turned, showed or hid
			// anything starts that object again from where it is now.
			void react(const Pair& pair, Edge edgeOfSecond, float at, std::vector<std::size_t>& changed)
			{
				// What the touch was, found once, before anything answers it.
				const Contact contact = contactBetween(*pair.first, *pair.second, edgeOfSecond, at, pair.byPicture);

				for (std::size_t i = 0; i < all.size(); ++i)
				{
					before[i] = { all[i].position, sweptMotion(all[i], t), all[i].size, all[i].heading };
				}

				rules.touched(*pair.first, *pair.second, contact);

				for (std::size_t i = 0; i < all.size(); ++i)
				{
					// Turned some other way by a reaction: it stops turning there.
					const bool turned = spinning(i) && all[i].heading != before[i].heading;
					if (turned) { all[i].turning.spin = 0.0f; }

					const char nowPlaying = rules.inPlay(all[i]) ? 1 : 0;
					if (turned || all[i].position != before[i].position || sweptMotion(all[i], t) != before[i].motion || nowPlaying != playing[i])
					{
						// Moving some other way now: at its velocity, its pull's
						// share of the step spent.
						all[i].motion.gained = {};
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
			}

		private:
			struct Before
			{
				Vector2f position;
				Vector2f motion;
				Vector2f size;
				float heading;
			};

			bool spinning(std::size_t i) const { return all[i].turning.spin != 0.0f && all[i].turning.hasHeading; }

			float facingAt(std::size_t i, float total) const
			{
				return std::fmod(turnFrom[i] + all[i].turning.spin * total + 720.0f, 360.0f);
			}

			// The box each covers over the rest of the frame, found once: until
			// a reaction changes it, what it covers over what is left of the
			// frame later is inside it. A changed one's is found again when
			// next asked (`fresh`).
			const Box& reachOf(std::size_t i, const Vector2f& move)
			{
				if (!fresh[i])
				{
					reach[i] = reachOver(all[i], move);
					fresh[i] = 1;
				}
				return reach[i];
			}

			std::vector<Object>& all;
			FrameRules& rules;
			// The step's length in the game's units of time.
			float t;
			std::vector<char> playing;
			std::vector<Vector2f> origin;
			std::vector<float> since;
			std::vector<float> turnFrom;
			std::vector<int> originDegrees;
			std::vector<Box> reach;
			std::vector<char> fresh;
			std::vector<Before> before;
			float played = 0.0f;
		};

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
		void moveObjects(std::vector<Object>& all, const WindowDesc& window, FrameRules& rules, const StepTime& step)
		{
			applyHops(all, window, rules, step.seconds);
			lendCarried(all, rules);

			FrameSweep frame(all, rules, step.t);
			playFrame(frame.pairs(), all.size(),
				[&](const Pair& pair, float left) { return frame.sweep(pair, left); },
				[&](float total) { frame.advanceTo(total); },
				[&](const Pair& pair, Edge edgeOfSecond, float at, std::vector<std::size_t>& changed) { frame.react(pair, edgeOfSecond, at, changed); });
		}
	}

	void FrameRules::keepFor(const std::vector<Object>& objects)
	{
		if (keptFor == objects.data() && keptCount == objects.size()) { return; }
		keptFor = objects.data();
		keptCount = objects.size();
		partnersFound = false;
		partnerLists.clear();
		pictureLists.clear();
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
			pictureLists.assign(objects.size(), {});
			for (std::size_t i = 0; i < objects.size(); ++i)
			{
				const auto ask = [&](std::size_t j)
				{
					if (canCollide(objects[i], objects[j]))
					{
						partnerLists[i].push_back(j);
						pictureLists[i].push_back(touchesByPicture(objects[i], objects[j]) ? 1 : 0);
					}
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
			if (object.map.name != map) { continue; }
			if (grid.at.empty())
			{
				grid.origin = object.map.origin;
				grid.tile = object.map.tile;
				grid.columns = object.map.columns;
				grid.rows = object.map.rows;
				grid.at.resize(static_cast<std::size_t>(grid.columns) * static_cast<std::size_t>(grid.rows));
			}
			if (grid.onMap(object.map.column, object.map.row))
			{
				grid.at[static_cast<std::size_t>(object.map.row) * static_cast<std::size_t>(grid.columns) + static_cast<std::size_t>(object.map.column)].push_back(i);
			}
		}
		return mazeGrids.emplace(map, std::move(grid)).first->second;
	}

	void playOneFrame(std::vector<Object>& objects, const WindowDesc& window, FrameRules& rules, const StepTime& step)
	{
		const float seconds = step.seconds;
		// A picture that changes with time (an <animation>) counts this step
		// whether or not the object is moving; only what is in play does, so a
		// pause or a menu holds it where it was.
		// Those whose once animation ended, by index and the look it was:
		// their <then> may spawn, and the list of objects grow.
		std::vector<std::pair<std::size_t, std::size_t>> ended;
		// Those whose animation moved on to a picture with commands of its own:
		// by index, the look and the picture.
		std::vector<std::array<std::size_t, 3>> cameUp;
		for (std::size_t i = 0; i < objects.size(); ++i)
		{
			Object& object = objects[i];
			// Just loaded or reset: where it is now is its place, not a move.
			if (object.placed) { object.remember(); }

			// Each picture the step passes, its commands in turn; an
			// animation that ends stops there, its <then> run once.
			bool passed = false;
			for (float owed = seconds; owed > kTimeSlack && !object.animation.bitmaps.empty() && rules.inPlay(object);)
			{
				const std::size_t was = object.looks.shown;
				const Object::AnimationStep moved = object.advanceAnimation(owed);
				if (moved == Object::AnimationStep::Stayed) { break; }
				passed = true;
				if (moved == Object::AnimationStep::Ended) { ended.emplace_back(i, was); break; }
				if (object.hasFrameCommands(was, object.animation.index)) { cameUp.push_back({ i, was, object.animation.index }); }
			}
			// A stepped one moves this step only if its picture changed: by
			// what its velocity covered since it last did.
			if (object.animation.stepped && rules.inPlay(object))
			{
				object.animation.since += step.t;
				object.animation.shareBefore = object.animation.share;
				object.animation.share = 0.0f;
				if (passed)
				{
					object.animation.share = object.animation.since;
					object.animation.since = 0.0f;
				}
			}
			if (object.blink > 0.0f && rules.inPlay(object)) { object.blink = std::max(0.0f, object.blink - seconds); }
		}
		for (const auto& [index, look] : ended) { rules.animationEnded(index, look); }
		for (const auto& [index, look, picture] : cameUp) { rules.frameCameUp(index, look, picture); }

		rules.timers(seconds);

		rules.followPaths(seconds);

		rules.cellMoves(seconds);

		// Who walks a maze chooses its way, now that a path may have let it go.
		walkMazes(objects, rules);

		applyClimbing(objects, rules);

		findGround(objects, rules);

		applyGrip(objects, rules, step);

		applyAcceleration(objects, rules, step);

		pullAndSeek(objects, rules, step.t);

		riseAndFall(objects, rules, step);

		// Screen-edge checks: independent per object, order doesn't matter.
		for (auto& object : objects)
		{
			// A piece of a cell map meets its map's edges by its moves.
			if (object.collisionsOn && !object.map.cells && isMoving(object) && rules.inPlay(object))
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
		moveObjects(objects, window, rules, step);

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

		rules.follow();

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
