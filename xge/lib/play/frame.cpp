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
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace xge
{
	namespace
	{
		// What an object's held keys drive, still for one no key drives.
		const Drive& driveOf(FrameRules& rules, const Object& object)
		{
			static const Drive still{};
			const Drive* drive = rules.parts().drives.find(object.id);
			return drive ? *drive : still;
		}

		// Each object of `objects` that has a part in `store`, with it, in
		// object order: a pass over the few that have it, not over all.
		template <typename Part, typename Work>
		void eachWithPart(std::vector<Object>& objects, FrameRules& rules, Store<Part>& store, Work&& work)
		{
			store.each([&](ObjectId id, Part& part)
			{
				const std::size_t at = rules.parts().at(id);
				if (at < objects.size() && objects[at].id == id) { work(objects[at], part); }
			});
		}

		// What a rate of loss (a <drag>, a <grip>) leaves of a speed over a
		// step: e to the -rate t (infinite: nothing). Worked out once for
		// each rate a step, not for each object.
		class KeptOver
		{
		public:
			explicit KeptOver(float stepT) : t(stepT) {}
			float operator()(float rate)
			{
				if (rate != lastRate)
				{
					lastRate = rate;
					lastKept = std::isinf(rate) ? 0.0f : std::exp(-rate * t);
				}
				return lastKept;
			}

		private:
			float t;
			float lastRate{ -1.0f };
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
		// everything), and they are not one lockstep block (a marching
		// fleet never collides with itself). Whether a rule answers is the
		// game's (FrameRules::canCollide), asked once (FrameRules::partners).
		bool canTouch(const Object& a, const Object& b) noexcept
		{
			if (!a.collisionsOn || !b.collisionsOn) { return false; }
			if (a.isAirborne() || b.isAirborne()) { return false; }
			return a.kind->collisionData.lockstep == 0 || a.kind->collisionData.lockstep != b.kind->collisionData.lockstep;
		}

		// Whether something the climber lands on (a class or object one of
		// its <land /> rules names) is under it with its top at `feet`, give
		// or take a couple of pixels (a sloped girder). Under any of it: a
		// ladder can come up through a hole between two pieces of ground.
		bool floorAt(const std::vector<Object>& objects, const Object& climber, float feet, FrameRules& rules)
		{
			for (const auto& rule : climber.kind->collisionData.others)
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

		// A <climb>: one on the ground at a ladder (its climbClass under its
		// middle), asked to go up or down, gets on, lined up; on it, it goes
		// the step held, with no pull, and gets off at either end (climbLadder).
		// Pieces stacked one on another, as wide and lined up, are one ladder.
		void applyClimbing(std::vector<Object>& objects, FrameRules& rules, float t)
		{
			eachWithPart(objects, rules, rules.parts().climbs, [&](Object& object, Object::Climb& climb)
			{
				if (climb.climbClass.empty() || !rules.inPlay(object)) { return; }

				const auto isLadder = [&](const Object& other)
					{
						return &other != &object && other.objClass == climb.climbClass && rules.inPlay(other);
					};
				const Box climber{ object.position, object.size };
				const auto ladder = std::find_if(objects.begin(), objects.end(), [&](const Object& other)
					{
						return isLadder(other) && atLadder(climber, { other.position, other.size });
					});

				if (ladder == objects.end())
				{
					object.footing.climbing = false;
					return;
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
				const float step = climb.activeClimb[1] - climb.activeClimb[0];
				const bool holding = object.footing.climbing && !climb.climbOnce && step != 0.0f;
				const bool holdTop = holding && step < 0.0f && !floorAt(objects, object, whole.top(), rules);
				const bool holdBottom = holding && step > 0.0f && !floorAt(objects, object, whole.bottom(), rules);
				const bool wasClimbing = object.footing.climbing;
				if (!climbLadder(object.position, object.velocity, object.size, step, t, object.footing.climbing, object.footing.grounded, whole, holdTop, holdBottom))
				{
					return;
				}

				if (!wasClimbing) { object.footing.jumping = false; }
				if (wasClimbing && !object.footing.climbing && climb.climbOnce)
				{
					climb.activeClimb = {};
					climb.climbOnce = false;
				}

				if (!object.footing.climbing)
				{
					const Drive& drive = driveOf(rules, object);
					object.velocity.x = drive.moveStep[static_cast<std::size_t>(Direction::Right)]
						- drive.moveStep[static_cast<std::size_t>(Direction::Left)];
				}
			});
		}

		// The ground: a thing with a <grip> whose middle is over a <surface>
		// (the last in the file's order, of those in play) has that
		// surface's drag and grip this frame in place of its own.
		void findGround(std::vector<Object>& objects, FrameRules& rules)
		{
			const Store<Object::Surface>& kinds = rules.parts().surfaces;
			std::vector<std::pair<const Object*, const Object::Surface*>> surfaces;
			for (auto& object : objects)
			{
				object.ground = { object.motion.drag, object.motion.grip };
				if (kinds.size() == 0) { continue; }
				const Object::Surface* surface = kinds.find(object.id);
				if (surface && rules.inPlay(object)) { surfaces.emplace_back(&object, surface); }
			}
			if (surfaces.empty()) { return; }

			for (auto& object : objects)
			{
				if (std::isinf(object.motion.grip) || kinds.has(object.id) || !rules.inPlay(object)) { continue; }
				const Vector2f middle = middleOf(object);
				for (auto ground = surfaces.rbegin(); ground != surfaces.rend(); ++ground)
				{
					const Object& surface = *ground->first;
					const Vector2f size = sizeOf(surface);
					if (middle.x < surface.position.x || middle.y < surface.position.y
						|| middle.x >= surface.position.x + size.x || middle.y >= surface.position.y + size.y)
					{
						continue;
					}
					if (ground->second->drag) { object.ground.drag = *ground->second->drag; }
					if (ground->second->grip) { object.ground.grip = *ground->second->grip; }
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
		void applyGrip(std::vector<Object>& objects, FrameRules& rules, float seconds)
		{
			KeptOver kept(seconds);
			// Close enough to the speed asked for to be it: a twentieth of a
			// pixel a sixtieth of a second.
			const float snap = 0.05f / kSixtieth;
			for (auto& object : objects)
			{
				if (std::isinf(object.motion.grip) || object.footing.climbing || !rules.inPlay(object)) { continue; }

				if (object.turning.hasHeading)
				{
					const Vector2f way = ahead(object.heading);
					const float along = dotOf(object.velocity, way);
					const Vector2f across = object.velocity - way * along;
					object.velocity -= across * (1.0f - kept(object.ground.grip));
					continue;
				}

				const float keeps = kept(object.ground.grip);
				const auto ease = [&](float& velocity, float wanted)
				{
					velocity = wanted + (velocity - wanted) * keeps;
					if (std::abs(wanted - velocity) < snap) { velocity = wanted; }
				};
				const auto& step = driveOf(rules, object).moveStep;
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
		Object::Pose poseOf(const Object& object, const Drive& drive, const Object::Flight* flight)
		{
			if (!drive.thrustAhead.empty()
				|| std::any_of(drive.thrust.begin(), drive.thrust.end(), [](float amount) { return amount != 0.0f; }))
			{
				return Object::Pose::Thrusting;
			}
			if (flight)
			{
				return flight->flown * 2.0 < static_cast<double>(flight->seconds) ? Object::Pose::Rising : Object::Pose::Falling;
			}
			const bool pulled = object.acceleration.y != 0.0f;
			if (pulled && !object.footing.grounded)
			{
				return object.velocity.y < 0.0f ? Object::Pose::Rising : Object::Pose::Falling;
			}
			if (object.velocity.x == 0.0f && (pulled || object.velocity.y == 0.0f)) { return Object::Pose::Standing; }

			const auto& step = drive.moveStep;
			const float wanted = step[static_cast<std::size_t>(Direction::Right)] - step[static_cast<std::size_t>(Direction::Left)];
			const auto sign = [](float value) { return (value > 0.0f) - (value < 0.0f); };
			if (!std::isinf(object.motion.grip) && object.velocity.x != 0.0f && sign(wanted) != sign(object.velocity.x)) { return Object::Pose::Skidding; }
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
				if (!object.mirror.on && !object.looks.set->hasPoses) { continue; }
				if (!rules.inPlay(object)) { continue; }

				if (object.mirror.on)
				{
					const auto& step = driveOf(rules, object).moveStep;
					const float wanted = step[static_cast<std::size_t>(Direction::Right)] - step[static_cast<std::size_t>(Direction::Left)];
					if (wanted != 0.0f) { object.mirror.goingLeft = wanted < 0.0f; }
					else if (const Object::Flight* flight = object.airborne ? rules.parts().flights.find(object.id) : nullptr; flight && flight->whole.x != 0.0f) { object.mirror.goingLeft = flight->whole.x < 0.0f; }
					else if (object.velocity.x != 0.0f) { object.mirror.goingLeft = object.velocity.x < 0.0f; }
					object.faceMirror();
				}

				if (!object.looks.set->hasPoses) { continue; }
				const auto& poses = object.looks.set->poses;
				if (std::find(poses.begin(), poses.end(), object.looks.shown) == poses.end()) { continue; }

				const std::size_t look = poses[static_cast<std::size_t>(poseOf(object, driveOf(rules, object), rules.parts().flights.find(object.id)))];
				if (look != object.looks.shown) { object.showLook(look); }
			}
		}

		// A constant pull (<acceleration>, gravity being one with only a y)
		// and held thrust (<thrust direction>) both change an object's velocity,
		// once a frame, before it is moved. Thrust that burns a variable takes
		// 1 off it every frame it is on, and does nothing while the variable
		// is at 0 or below: a ship out of fuel still falls.
		void applyAcceleration(std::vector<Object>& objects, FrameRules& rules, float seconds)
		{
			const float t = seconds;
			KeptOver kept(t);
			for (auto& object : objects)
			{
				object.motion.gained = {};

				// Nothing to do for most things (a wall): looked at before
				// asking whether it is in play.
				const Drive& drive = driveOf(rules, object);
				const bool thrusting = drive.thrust[0] != 0.0f || drive.thrust[1] != 0.0f
					|| drive.thrust[2] != 0.0f || drive.thrust[3] != 0.0f;
				if (!thrusting && !object.turning.hasHeading && object.ground.drag == 0.0f && object.acceleration.x == 0.0f && object.acceleration.y == 0.0f)
				{
					continue;
				}

				// On a ladder nothing pulls it (applyClimbing moves it).
				if (!rules.inPlay(object) || object.footing.climbing) { continue; }

				// At rest (Object::Rest): nothing pulls it while it lies
				// where it came to rest, still, on something still there as
				// it was. Anything else wakes it.
				if (Object::Rest* rest = rules.parts().rests.find(object.id); rest && object.motion.friction > 0.0f)
				{
					if (rest->settledAge < 1000.0f) { rest->settledAge += seconds; }
					if (rest->resting)
					{
						rest->resting = object.velocity.x == 0.0f && object.velocity.y == 0.0f && object.position == rest->at
							&& std::all_of(rest->on.begin(), rest->on.end(), [&](const Object::Support& support)
								{
									const auto under = std::find_if(objects.begin(), objects.end(), [&](const Object& other) { return other.name == support.name; });
									return under != objects.end() && rules.inPlay(*under) && under->position == support.at
										&& under->heading == support.heading && under->turning.spin == 0.0f;
								});
					}
					if (rest->resting) { continue; }
				}

				const Vector2f was = object.velocity;
				Vector2f change = object.acceleration;

				// A thruster with no fuel does nothing.
				const auto burnFuel = [&](const Name burn)
				{
					return burn.empty() || rules.burn(object, burn);
				};

				for (std::size_t i = 0; i < drive.thrust.size(); ++i)
				{
					const float amount = drive.thrust[i];
					if (amount == 0.0f) { continue; }

					if (!burnFuel(drive.thrustBurn[i])) { continue; }

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
					const float turn = drive.turn[static_cast<std::size_t>(Direction::Right)]
						- drive.turn[static_cast<std::size_t>(Direction::Left)];
					if (turn != 0.0f)
					{
						object.heading = std::fmod(object.heading + turn * t, 360.0f);
						if (object.heading < 0.0f) { object.heading += 360.0f; }
						object.showHeading();
					}

					// A swing: held, on through its stroke; let go, back. The
					// stroke is half a cosine, still at both ends and fastest half
					// way. The turn is its spin this frame, swept by moveObjects.
					object.turning.spin = 0.0f;
					if (Object::Swing* swing = rules.parts().swings.find(object.id); swing && swing->seconds > 0.0f)
					{
						const float stroke = seconds / swing->seconds;
						const float done = std::clamp(swing->done + (swing->held ? stroke : -stroke), 0.0f, 1.0f);
						if (done != swing->done)
						{
							swing->done = done;
							const float pi = 3.14159265358979323846f;
							const float eased = (1.0f - std::cos(pi * done)) / 2.0f;
							float now = std::fmod(startOf(rules.parts(), object).heading + swing->by * eased, 360.0f);
							if (now < 0.0f) { now += 360.0f; }
							object.turning.spin = turnBetween(object.heading, now);
							object.turning.spinOver = t;
							object.heading = now;
							object.showHeading();
						}
					}

					for (const auto& [action, thrust] : drive.thrustAhead)
					{
						if (burnFuel(thrust.second))
						{
							change += ahead(object.heading) * thrust.first;
						}
					}
				}

				// Drag: what is left of the speed after this step, losing its
				// share each unit of time while the push goes on (the sums are
				// exact for a steady push: v' = a - k v). `gained` is what the
				// sweep takes off the step's end speed to go the step's true
				// way: twice the end speed less the step's mean.
				const float drag = object.ground.drag;
				if (drag == 0.0f)
				{
					object.velocity += change * t;
					object.motion.gained = object.velocity - was;
				}
				else if (std::isinf(drag) || !(t > 0.0f))
				{
					object.velocity = std::isinf(drag) ? Vector2f{} : was;
					object.motion.gained = object.velocity * 2.0f;
				}
				else
				{
					const float left = kept(drag);
					const float k = drag;
					const float lost = (1.0f - left) / k;
					object.velocity = was * left + change * lost;
					const Vector2f mean = (was * lost + change * ((t - lost) / k)) * (1.0f / t);
					object.motion.gained = (object.velocity - mean) * 2.0f;
				}
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
				if (Object::Flight* flight = object.airborne ? rules.parts().flights.find(object.id) : nullptr)
				{
					const double whole = static_cast<double>(flight->seconds);
					double flown = flight->flown + static_cast<double>(seconds);
					if (flown >= whole - static_cast<double>(kTimeSlack)) { flown = whole; }
					if (rules.inPlay(object))
					{
						const float share = static_cast<float>((flown - flight->flown) / whole);
						const Vector2f target = object.position + flight->whole * share;
						if (insideWindow({ target, object.size }, windowSize)) { object.position = target; }
					}
					flight->flown = flown;
					const bool landed = flown >= whole;

					// An arc: drawn up off its way and back down, highest
					// halfway, on the ground again as it lands.
					if (flight->arc > 0.0f)
					{
						const float t = static_cast<float>(flown / whole);
						object.altitude = landed ? 0.0f : 4.0f * flight->arc * t * (1.0f - t);
					}
					if (landed)
					{
						rules.parts().flights.remove(object.id);
						object.airborne = false;
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
			if (rules.parts().carries.size() == 0) { return; }
			for (auto& carrier : objects)
			{
				Object::Carry* carry = rules.parts().carries.find(carrier.id);
				if (!carry || carry->what.empty()) { continue; }

				Object* item = named(objects, carry->what);
				if (!item || !rules.inPlay(carrier) || !rules.inPlay(*item))
				{
					carry->what.clear();
					continue;
				}

				item->riding = motionOf(carrier, 1.0f) - item->velocity;
			}
		}

		// After the move: what is carried is put exactly where it is held
		// (the carrier may have been stopped on the way, or hopped), and
		// what was put down can be picked up again once the two are apart.
		void holdCarried(std::vector<Object>& objects, FrameRules& rules)
		{
			if (rules.parts().carries.size() == 0) { return; }
			for (auto& carrier : objects)
			{
				Object::Carry* carry = rules.parts().carries.find(carrier.id);
				if (!carry) { continue; }
				if (!carry->what.empty())
				{
					if (Object* item = named(objects, carry->what))
					{
						item->position = carrier.position + carry->offset;
					}
				}

				if (!carry->dropped.empty())
				{
					const Object* item = named(objects, carry->dropped);
					if (!item || !boxesOverlap({ carrier.position, sizeOf(carrier) }, { item->position, sizeOf(*item) }))
					{
						carry->dropped.clear();
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
			eachWithPart(objects, rules, rules.parts().pulls, [&](Object& object, const Object::Pull& part)
			{
				const Object::Pull* pull = &part;
				if (!rules.inPlay(object)) { return; }

				const Vector2f middle = middleOf(object);
				for (const auto& [name, strength] : pull->toward)
				{
					const Object* toward = nearestInPlay(objects, object, name, inPlay);
					if (!toward) { continue; }
					if (const auto way = wayTo(middle, middleOf(*toward)))
					{
						object.velocity = object.velocity + *way * (strength * t);
						object.motion.gained += *way * (strength * t);
					}
				}

				if (pull->seek.empty()) { return; }
				const Object* target = nearestInPlay(objects, object, pull->seek, inPlay);
				if (!target) { return; }
				const Vector2f before = object.velocity;
				object.velocity = turnedToward(object.velocity, middleOf(*target) - middle, pull->seekRate * t);
				if (object.turning.hasHeading && (object.velocity.x != before.x || object.velocity.y != before.y))
				{
					object.heading = headingOf(object.velocity);
					object.showHeading();
				}
			});
		}

		// <altitude>: what is in the air rises by its rise, which falls by its
		// fall each frame; at the ground it stops there, sent back up with
		// its bounce's share of how fast it came down (none, once that is
		// less than a frame's fall, so it comes to rest).
		void riseAndFall(std::vector<Object>& objects, FrameRules& rules, float seconds)
		{
			const float t = seconds;
			eachWithPart(objects, rules, rules.parts().alofts, [&](Object& object, const Object::Aloft& part)
			{
				const Object::Aloft* aloft = &part;
				if (!aloft->has || !rules.inPlay(object)) { return; }
				// A constant fall: the altitude goes by the mean of the rise
				// before and after.
				const float fall = aloft->fall * t;
				object.altitude += (object.rise - fall * 0.5f) * t;
				object.rise -= fall;
				if (object.altitude > 0.0f) { return; }
				object.altitude = 0.0f;
				if (object.rise < 0.0f)
				{
					const float up = -object.rise * aloft->bounce;
					object.rise = up > aloft->fall * kSixtieth ? up : 0.0f;
				}
			});
		}

		// A <shadow> is kept under what casts it, on the ground, and shown
		// while that is.
		void castShadows(std::vector<Object>& objects, FrameRules& rules)
		{
			if (rules.parts().alofts.size() == 0) { return; }
			for (auto& shadow : objects)
			{
				const Object::Aloft* aloft = rules.parts().alofts.find(shadow.id);
				if (!aloft || aloft->shadowOf.empty()) { continue; }
				const Object* caster = named(objects, aloft->shadowOf);
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
				// The classes it does not shine through (null: none).
				const Object::LightPast* past;
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
					lights.push_back({ &object, object.position + sizeOf(object) * 0.5f, object.light, rules.parts().lightPasts.find(object.id), {} });
				}
			}
			if (!anyDark) { return; }

			// Each light's blockers, gathered once a frame: those of its
			// <past> classes in play within its reach.
			for (Light& light : lights)
			{
				if (!light.past) { continue; }
				const Box reach{ light.middle - Vector2f{ light.reach, light.reach }, Vector2f{ light.reach, light.reach } * 2.0f };
				for (const Object& other : objects)
				{
					if (&other == light.object) { continue; }
					const Box box{ other.position, sizeOf(other) };
					if (!boxesMeet(reach, box) || !rules.inPlay(other)) { continue; }
					if (std::find(light.past->classes.begin(), light.past->classes.end(), other.objClass) == light.past->classes.end()) { continue; }
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
						if (!light.past) { return true; }
						// Its nearest part, a pixel in toward its middle, so a
						// neighbour it only touches does not stand in the way.
						const Vector2f middle = object.position + size * 0.5f;
						const Vector2f in = middle - Vector2f{ x, y };
						const float far = lengthOf(in);
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
		void drawTethers(std::vector<Object>& objects, FrameRules& rules)
		{
			if (rules.parts().tethers.size() == 0) { return; }
			for (auto& tether : objects)
			{
				Object::Tether* ends = rules.parts().tethers.find(tether.id);
				if (!ends || ends->from.empty()) { continue; }

				const Object* from = named(objects, ends->from);
				const Object* to = named(objects, ends->to);
				tether.isVisible = from && to && from->isVisible && to->isVisible;
				if (!tether.isVisible) { continue; }

				const Vector2f a = from->position + sizeOf(*from) * 0.5f;
				const Vector2f b = to->position + sizeOf(*to) * 0.5f;
				const float half = static_cast<float>(ends->thickness) * 0.5f;
				const Vector2f corner{ std::floor(std::min(a.x, b.x) - half), std::floor(std::min(a.y, b.y) - half) };

				LineSegment line;
				line.x1 = a.x - corner.x - half;
				line.y1 = a.y - corner.y - half;
				line.x2 = b.x - corner.x - half;
				line.y2 = b.y - corner.y - half;
				line.color = ends->color;
				line.thickness = ends->thickness;

				Bitmap drawn = rasterizeLines({ line });
				tether.position = corner;
				tether.size = { static_cast<float>(drawn.width), static_cast<float>(drawn.height) };
				tether.drawn.picture.edit().kind = ShapeKind::Line;
				tether.drawn.picture.edit().size = tether.size;
				if (ends->drawn) { rules.resources().replace(ends->drawn, std::move(drawn)); }
				else { ends->drawn = rules.resources().own(std::move(drawn)); }
				tether.drawn.bitmap = ends->drawn;
				tether.drawn.visualDirty = true;
			}
		}

		// A pair of objects in play that a rule answers for and that can touch,
		// or an object and a side of the window it has rules for (`second`
		// none, `j` past the objects: their count plus the edge's index).
		struct Pair
		{
			Object* first;
			Object* second;
			std::size_t i;
			std::size_t j;
			bool byPicture;
			Edge screenEdge = Edge::Top;
		};

		// The window's sides are met in the sweep, at the moment of crossing,
		// by an object with rules for them. A <stepped /> one jumps, so has no
		// moment of crossing: it is asked before the move (playOneFrame), and
		// so is a piece of a cell map, which meets its map's edges by its moves.
		bool sweptAtEdges(const Object& object) noexcept
		{
			return !object.animation.frames.shows().stepped && !object.onCells;
		}

		// What moveObjects keeps about each object over one frame's sweep.
		class FrameSweep
		{
		public:
			FrameSweep(std::vector<Object>& objects, const WindowDesc& windowDesc, FrameRules& frameRules, float stepT)
				: all(objects), window(windowDesc), rules(frameRules), t(stepT), playing(objects.size()), origin(objects.size()), since(objects.size(), 0.0f),
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
				for (std::size_t i = 0; i < all.size(); ++i)
				{
					if (!playing[i] || !sweptAtEdges(all[i])) { continue; }
					for (const Edge edge : { Edge::Top, Edge::Bottom, Edge::Left, Edge::Right })
					{
						if (!all[i].kind->collisionData.edges[edgeIndex(edge)].commands.empty())
						{
							found.push_back({ &all[i], nullptr, i, all.size() + edgeIndex(edge), false, edge });
						}
					}
				}
				return found;
			}

			// The things the pairs name: the objects, then the four sides.
			std::size_t things() const { return all.size() + 4; }

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
				if (!pair.second) { return sweepEdge(pair, left); }
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
				for (std::size_t i = 0; i < all.size(); ++i)
				{
					before[i] = { all[i].position, sweptMotion(all[i], t), all[i].size, all[i].heading };
				}

				if (pair.second)
				{
					// What the touch was, found once, before anything answers it.
					const Contact contact = contactBetween(*pair.first, *pair.second, edgeOfSecond, at, pair.byPicture);
					rules.touched(*pair.first, *pair.second, contact);
				}
				else
				{
					rules.atEdge(*pair.first, pair.screenEdge);
				}

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

			// An object and a side of the window: already past it (a hit at
			// once), or when through what is left of the step its move takes
			// it past; far when it will not get there unless a reaction
			// changes it. One that is not moving sits out, as it did before
			// the sides were swept.
			PairSweep sweepEdge(const Pair& pair, float left)
			{
				const Object& object = *pair.first;
				if (!playing[pair.i] || !object.collisionsOn || !isMoving(object)) { return { std::nullopt, true }; }
				const Box box{ object.position, object.size };
				if (pastWindowEdge(box, { window.width, window.height }, pair.screenEdge)) { return { SweepHit{ 0.0f, pair.screenEdge }, false }; }
				const std::optional<float> when = timeToWindowEdge(box, sweptMotion(object, t) * left, { window.width, window.height }, pair.screenEdge);
				if (!when) { return { std::nullopt, true }; }
				return { SweepHit{ *when, pair.screenEdge }, false };
			}

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
			const WindowDesc& window;
			FrameRules& rules;
			// The step's length in seconds.
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

		// Plays one frame of movement, from touch to touch (xge::playFrame).
		// Each object is swept by itself; a lockstep set has no box of its
		// own, so only the ones left are tested.
		void moveObjects(std::vector<Object>& all, const WindowDesc& window, FrameRules& rules, float seconds)
		{
			applyHops(all, window, rules, seconds);
			lendCarried(all, rules);

			FrameSweep frame(all, window, rules, seconds);
			playFrame(frame.pairs(), frame.things(),
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

			// Objects answered alike are asked about once a pair of groups,
			// the answer kept in a table (while it stays small).
			const std::vector<std::size_t> alike = answerGroups(objects);
			std::vector<std::uint32_t> group(objects.size());
			std::size_t groups = 0;
			{
				std::unordered_map<std::size_t, std::uint32_t> numbered;
				for (std::size_t i = 0; i < alike.size(); ++i)
				{
					group[i] = numbered.emplace(alike[i], static_cast<std::uint32_t>(numbered.size())).first->second;
				}
				groups = numbered.size();
			}
			const bool table = !alike.empty() && groups <= 2048;
			std::vector<signed char> answered(table ? groups * groups : 0, -1);
			const auto canMeet = [&](std::size_t i, std::size_t j)
			{
				if (!table) { return canCollide(objects[i], objects[j]); }
				signed char& answer = answered[group[i] * groups + group[j]];
				if (answer < 0)
				{
					answer = canCollide(objects[i], objects[j]) ? 1 : 0;
					answered[group[j] * groups + group[i]] = answer;
				}
				return answer == 1;
			};

			for (std::size_t i = 0; i < objects.size(); ++i)
			{
				const auto ask = [&](std::size_t j)
				{
					if (canMeet(i, j))
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
			const Object::MapPlace& place = mapOf(parts(), object);
			if (place.name != map) { continue; }
			if (grid.at.empty())
			{
				grid.origin = place.origin;
				grid.tile = place.tile;
				grid.columns = place.columns;
				grid.rows = place.rows;
				grid.at.resize(static_cast<std::size_t>(grid.columns) * static_cast<std::size_t>(grid.rows));
			}
			if (grid.onMap(place.column, place.row))
			{
				grid.at[static_cast<std::size_t>(place.row) * static_cast<std::size_t>(grid.columns) + static_cast<std::size_t>(place.column)].push_back(i);
			}
		}
		return mazeGrids.emplace(map, std::move(grid)).first->second;
	}

	// A picture that changes with time (an <animation>) counts this step
	// whether or not the object is moving; only what is in play does, so a
	// pause or a menu holds it where it was.
	static void advanceAnimations(std::vector<Object>& objects, FrameRules& rules, float seconds)
	{
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
			for (float owed = seconds; owed > kTimeSlack && !object.animation.frames.shows().bitmaps.empty() && rules.inPlay(object);)
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
			if (object.animation.frames.shows().stepped && rules.inPlay(object))
			{
				object.animation.since += seconds;
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
	}

	// The window's sides, for what jumps rather than moves (the rest meet
	// them in the sweep): independent per object, order doesn't matter.
	static void meetEdgesWithoutSweep(std::vector<Object>& objects, const WindowDesc& window, FrameRules& rules)
	{
		for (auto& object : objects)
		{
			// A piece of a cell map meets its map's edges by its moves.
			if (object.collisionsOn && !object.onCells && !sweptAtEdges(object) && isMoving(object) && rules.inPlay(object))
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
	}

	// The edge rules run only for an object that is moving, so on their own
	// they let a stuck object end the frame poking out past the wall (and
	// stay there once it stopped).
	static void stickAtEdges(std::vector<Object>& objects, const WindowDesc& window, FrameRules& rules)
	{
		for (auto& object : objects)
		{
			if (object.collisionsOn && rules.inPlay(object))
			{
				for (const Edge edge : { Edge::Top, Edge::Bottom, Edge::Left, Edge::Right })
				{
					if (touchesScreenEdge(object, window, edge) && rules.sticksAt(object, edge))
					{
						stickInside(object.position, object.velocity, object.size, { window.width, window.height }, edge);
					}
				}
			}
	}
	}

	// How everything ended up, to tell next frame what has moved.
	static void rememberWhereAll(std::vector<Object>& objects)
	{
		for (auto& object : objects) { object.remember(); }
	}

	void playOneFrame(std::vector<Object>& objects, const WindowDesc& window, FrameRules& rules, float seconds)
	{
		advanceAnimations(objects, rules, seconds);

		rules.timers(seconds);

		rules.followPaths(seconds);

		rules.cellMoves(seconds);

		// Who walks a maze chooses its way, now that a path may have let it go.
		walkMazes(objects, rules, seconds);

		applyClimbing(objects, rules, seconds);

		findGround(objects, rules);

		applyGrip(objects, rules, seconds);

		applyAcceleration(objects, rules, seconds);

		pullAndSeek(objects, rules, seconds);

		riseAndFall(objects, rules, seconds);

		meetEdgesWithoutSweep(objects, window, rules);

		// Movement and object-against-object collisions are one job, so that
		// a fast or small object cannot jump over a thin one between frames.
		moveObjects(objects, window, rules, seconds);

		stickAtEdges(objects, window, rules);

		holdCarried(objects, rules);

		rules.follow();

		drawTethers(objects, rules);

		castShadows(objects, rules);

		lightAndDark(objects, rules);

		takePoses(objects, rules);

		// A condition that puts something elsewhere has moved it.
		rememberWhereAll(objects);

		rules.conditions();
	}
}
