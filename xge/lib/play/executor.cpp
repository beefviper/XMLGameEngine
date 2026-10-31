// executor.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "play/executor.h"

#include "game/game.h"
#include "play/geometry.h"
#include "play/maze.h"
#include "play/verbs.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <utility>

namespace xge
{
	const Command& CommandExecutor::now(const Command& command, Command& made)
	{
		if (const auto* live = std::get_if<CmdLive>(&command))
		{
			made = game.makeNow(*live);
			return made;
		}
		return command;
	}

	bool CommandExecutor::executeScreenEdgeCollision(const Command& command, Object& object, Edge edge)
	{
		Doer doer;
		doer.self = &object;
		doer.screenEdge = edge;
		return run(command, doer);
	}

	bool CommandExecutor::executeObjectCollision(const Command& command, Object& object, Object& other, Edge edge)
	{
		Touch touch;
		touch.edge = edge;
		touch.normal = outOf(opposite(edge));
		touch.along = 0.5f;
		return executeObjectCollision(command, object, other, touch);
	}

	bool CommandExecutor::executeObjectCollision(const Command& command, Object& object, Object& other, const Touch& touch)
	{
		Doer doer;
		doer.self = &object;
		doer.other = &other;
		doer.touch = &touch;
		return run(command, doer);
	}

	bool CommandExecutor::executeInput(const Command& command, bool keyPressed)
	{
		Doer doer;
		doer.pressed = keyPressed;
		doer.byRule = false;
		return run(command, doer);
	}

	bool CommandExecutor::executeCondition(const Command& command)
	{
		return run(command, Doer{});
	}

	bool CommandExecutor::executeCheck(const Command& command, const Object::CellMatch& match)
	{
		Doer doer;
		doer.match = &match;
		return run(command, doer);
	}

	void CommandExecutor::executeAxis(const Command& command, Axis axis, float fraction)
	{
		const auto* trigger = std::get_if<CmdTriggerAction>(&shapeOf(command));
		if (!trigger)
		{
			return;
		}

		const bool horizontal = axis == Axis::MouseX || axis == Axis::Pad1X || axis == Axis::Pad2X;
		const WindowDesc& window = game.getWindowDesc();
		Object& object = game.getObject(trigger->object);
		for (const auto& action : object.action[trigger->action])
		{
			const auto* along = std::get_if<CmdTrack>(&shapeOf(action));
			if (!along) { continue; }
			if (along->path.empty())
			{
				track(object, horizontal, fraction, horizontal ? window.width : window.height);
			}
			else if (const auto path = game.getPaths().find(along->path); path != game.getPaths().end())
			{
				trackPath(object, path->second, fraction);
			}
		}
	}

	bool CommandExecutor::executeTimer(const Command& command, Object* owner)
	{
		Doer doer;
		doer.self = owner;
		return run(command, doer);
	}

	namespace
	{
		// What a let-go key still does: a held verb stops (a move, a climb,
		// a thrust, a turn, a sound that loops), and an action it runs is
		// let go too. Anything else belongs to the press alone.
		bool actsOnLettingGo(const Command& command) noexcept
		{
			return std::holds_alternative<CmdMove>(command) || std::holds_alternative<CmdClimb>(command)
				|| std::holds_alternative<CmdAccelerate>(command) || std::holds_alternative<CmdTurn>(command)
				|| std::holds_alternative<CmdThrust>(command) || std::holds_alternative<CmdPlay>(command)
				|| std::holds_alternative<CmdTriggerAction>(command);
		}
	}

	bool CommandExecutor::run(const Command& command, const Doer& doer)
	{
		if (!doer.pressed && !actsOnLettingGo(shapeOf(command))) { return true; }

		Object* const self = doer.self;
		const bool pressed = doer.pressed;
		Command made;
		bool done = true;
		std::visit(overload{
			// About the object running it; a state's lists have none.
			[&](const CmdReset&) { if (self) { restart(*self); } else { game.resetAll(); } },
			[&](const CmdDie&) { if (self) { xge::die(*self); } },
			[&](const CmdStop&) { if (self) { xge::stop(*self); } },
			[&](const CmdDrop&) { if (self) { xge::drop(*self); } },
			[&](const CmdReverse&)
			{
				// On a map of cells, a <go> turns round.
				if (self && self->onCells && self->cellGoing)
				{
					const Direction way = self->cellGoWay;
					self->cellGoWay = way == Direction::Up ? Direction::Down : way == Direction::Down ? Direction::Up : way == Direction::Left ? Direction::Right : Direction::Left;
					self->facing = self->cellGoWay;
				}
				else if (self) { reverse(*self); }
			},
			[&](const CmdRelease& r) { if (self) { release(*self, r.target, r.count); } },
			[&](const CmdFire& f) { if (self) { done = spawnProjectile(*self, f.projectileName); } },
			[&](const CmdChase& c) { if (self) { chase(*self, c); } },
			[&](const CmdAim& a) { if (self) { done = aim(*self, a); } },
			[&](const CmdHunt& h) { if (self) { startHunt(*self, h); } },
			[&](const CmdHuntHop& h) { if (self) { done = huntByHop(*self, h); } },
			[&](const CmdWander& w) { if (self) { startWander(*self, w); } },
			// In an action a move is held, like a key; anywhere else it is
			// one step, the whole block in lockstep with it.
			[&](const CmdMove& m)
			{
				if (!self) { return; }
				if (doer.action && self->onCells) { game.holdCellMove(*self, m, pressed); }
				else if (doer.action) { holdMove(*self, m.direction, m.step, pressed); }
				else { moveByStep(*self, m.direction, m.step); }
			},
			// A bounce off what it touched, or off the screen's edge.
			[&](const CmdBounce& b)
			{
				if (self && doer.touch) { xge::bounce(*self, *doer.touch, b.share); }
				else if (self && doer.screenEdge) { bounceScreenEdge(*self, *doer.screenEdge, b.share); }
			},
			// The screen edge's own.
			[&](const CmdStick&)
			{
				if (self && doer.screenEdge) { xge::stick(*self, game.getWindowDesc(), *doer.screenEdge); }
				else if (self && doer.touch) { stickAgainst(*self, doer.touch->edge); }
			},
			[&](const CmdWrap&) { if (self && doer.screenEdge) { xge::wrap(*self, game.getWindowDesc(), *doer.screenEdge); } },
			// What it touched.
			[&](const CmdDeflect& d) { if (self && doer.touch) { xge::deflect(*self, *doer.other, doer.touch->edge, d.maxAngle); } },
			[&](const CmdKnock& k) { if (self && doer.touch) { knock(*self, *doer.other, *doer.touch, k.bounce); } },
			[&](const CmdHole& h) { if (self && doer.touch) { cutHole(*self, *doer.other, h.radius, h.rough); } },
			[&](const CmdRide&) { if (self && doer.touch) { xge::ride(*self, *doer.other); } },
			[&](const CmdCarry&) { if (self && doer.touch) { carry(*self, *doer.other); } },
			[&](const CmdLand&) { if (self && doer.touch) { land(*self, *doer.other, doer.touch->edge); } },
			// The controls an action holds: a press, or on while it is down.
			[&](const CmdHop& h) { if (self) { done = hop(*self, h); } },
			[&](const CmdJump& j) { if (self && doer.action) { done = startJump(*self, j); } },
			[&](const CmdGo& g)
			{
				if (self && self->onCells) { done = game.goCells(*self, g.direction, g.speed); }
				else if (self && doer.action) { startGo(*self, g); }
			},
			[&](const CmdLeap& l) { if (self && doer.action) { done = leap(*self, l.height); } },
			[&](const CmdClimb& c)
			{
				if (!self || !doer.action) { return; }
				self->activeClimb[c.direction == Direction::Up ? 0 : 1] = pressed ? c.step : 0.0f;
				self->climbClass = c.ladderClass;
				self->climbOnce = pressed && doer.byRule;
			},
			[&](const CmdAccelerate& a) { if (self && doer.action) { applyActionThrust(*self, a.direction, pressed ? a.amount : 0.0f, a.burn); } },
			[&](const CmdTurn& t)
			{
				if (!self || !doer.action) { return; }
				// A shape on a cell map: a quarter turn on the press.
				if (self->isShape()) { if (pressed) { done = game.turnCells(*self, t.direction == Direction::Right); } }
				else { applyActionTurn(*self, t.direction, pressed ? t.rate : 0.0f); }
			},
			[&](const CmdThrust& t) { if (self && doer.action) { applyActionThrustAhead(*self, *doer.action, pressed ? t.amount : 0.0f, t.burn); } },
			// The game's own: a state's list, and the rest.
			[&](const CmdPushState& c) { game.pushState(c.name); },
			[&](const CmdPopState& c) { game.popState(c.name); },
			[&](const CmdTurns& t) { if (!self) { game.startTurns(t.players); } },
			[&](const CmdPass&) { if (!self) { game.passTurn(false); } },
			[&](const CmdOut&) { if (!self) { game.passTurn(true); } },
			[&](const CmdDemo&) { if (!self) { game.requestDemo(); } },
			[&](const CmdIncrement& i) { game.addTo(i.target, i.amount); },
			[&](const CmdDecrement& d) { game.addTo(d.target, -d.amount); },
			[&](const CmdCopy& c) { game.copyVariable(c.from, c.into); },
			[&](const CmdRank& c) { game.rank(c.entry, c.table); },
			[&](const CmdSet& c) { game.setNumber(c.target, c.value); },
			[&](const CmdPlay& p) { playHeld(p.sound, pressed); },
			[&](const CmdSilence& c) { game.silenceSound(c.sound); },
			[&](const CmdResetObject& r) { game.resetObject(r.target); },
			[&](const CmdBecome& b) { become(self, b); },
			[&](const CmdReveal& r) { game.reveal(r.target, r.count); },
			[&](const CmdShift& m) { game.shift(m.target, m.by); },
			[&](const CmdFollow& f) { follow(self, f); },
			[&](const CmdEnterScene& e) { game.enter(e); },
			[&](const CmdResetScene& r) { game.reset(r); },
			[&](const CmdSpawn& c) { done = spawn(c, self); },
			[&](const CmdSend& s)
			{
				const auto cell = [](const std::optional<float>& given) { return given ? std::optional<int>(static_cast<int>(std::lround(*given))) : std::nullopt; };
				done = game.sendCells(s.target, s.from, s.into, cell(s.column), cell(s.row));
			},
			[&](const CmdDespawn& d) { despawn(self, d); },
			// A cell map's.
			[&](const CmdSettle&) { if (self) { game.settle(*self); } },
			[&](const CmdSlide& m) { if (self && self->onCells) { game.slideCells(*self, m.direction); } },
			[&](const CmdClear&) { if (doer.match) { game.clearCells(*doer.match); } },
			[&](const CmdCollapse&) { if (doer.match) { game.collapseCells(*doer.match); } },
			[&](const CmdFall& fall) { if (doer.match) { game.fallCells(doer.match->map, fall.objClass); } },
			[&](const CmdCycle&) { if (self) { game.cycleCells(*self); } },
			[&](const CmdGrow&) { if (self && !self->chain.empty()) { ++self->chainGrow; } },
			[&](const CmdSplit&) { if (self) { game.splitChain(*self); } },
			[&](const CmdThrough&) { if (self && doer.touch) { game.stepThrough(); } },
			// Another action, of this object or another, as part of this.
			[&](const CmdTriggerAction& a) { triggerObjectAction(a.object, a.action, pressed, doer.byRule, a.from); },
			// <track /> runs on a pointing control (executeAxis), <hold> in a
			// <computer>, and a CmdLive is made into its command first.
			[&](const CmdTrack&) {},
			[&](const CmdHold&) {},
			[&](const CmdLive&) {},
		}, pressed ? now(command, made) : shapeOf(command));
		return done;
	}

	void CommandExecutor::become(Object* self, const CmdBecome& command)
	{
		if (!command.target.empty())
		{
			game.become(command.target, command.sprite, command.from, command.style);
		}
		else if (self && (command.from.empty() || self->lookName() == command.from))
		{
			if (!command.sprite.empty()) { showLookNamed(*self, command.sprite); }
			if (!command.style.empty()) { wearPaletteNamed(*self, command.style); }
		}
	}

	void CommandExecutor::follow(Object* self, const CmdFollow& command)
	{
		if (!command.target.empty())
		{
			game.follow(command.target, command.path, command.stagger, command.nearest, self);
		}
		else if (self)
		{
			game.follow(*self, command.path);
		}
	}

	void CommandExecutor::reverse(Object& object)
	{
		object.velocity = object.velocity * -1.0f;
	}

	// Taken from whatever had it: one thing is carried by one object at a time.
	void CommandExecutor::carry(Object& object, const Object& item)
	{
		if (item.name == object.carrying || item.name == object.dropped)
		{
			return;
		}

		for (auto& other : game.getCurrentObjects())
		{
			if (&other != &object && other.carrying == item.name)
			{
				other.carrying.clear();
			}
		}

		xge::carry(object, item);
	}

	void CommandExecutor::playHeld(const std::string& sound, bool keyPressed)
	{
		if (keyPressed)
		{
			game.requestSound(sound);
		}
		else if (game.soundLoops(sound))
		{
			game.silenceSound(sound);
		}
	}

	bool CommandExecutor::executeHeldInput(const Command& held)
	{
		Command made;
		const Command& command = now(held, made);

		// A looping sound right on a key goes on again, as a move does.
		if (const auto* play = std::get_if<CmdPlay>(&command))
		{
			if (!game.soundLoops(play->sound))
			{
				return false;
			}
			game.requestSound(play->sound);
			return true;
		}

		const auto* trigger = std::get_if<CmdTriggerAction>(&command);
		if (!trigger)
		{
			return false;
		}

		Object& object = game.getObject(trigger->object);
		for (const auto& written : object.action[trigger->action])
		{
			Command madeNow;
			const Command& actionCommand = now(written, madeNow);
			if (const auto* move = std::get_if<CmdMove>(&actionCommand))
			{
				holdStep(object, move->direction, move->step);
			}
			else if (const auto* climb = std::get_if<CmdClimb>(&actionCommand))
			{
				object.activeClimb[climb->direction == Direction::Up ? 0 : 1] = climb->step;
				object.climbClass = climb->ladderClass;
			}
			else if (const auto* thrust = std::get_if<CmdAccelerate>(&actionCommand))
			{
				applyActionThrust(object, thrust->direction, thrust->amount, thrust->burn);
			}
			else if (const auto* turn = std::get_if<CmdTurn>(&actionCommand))
			{
				applyActionTurn(object, turn->direction, turn->rate);
			}
			else if (const auto* ahead = std::get_if<CmdThrust>(&actionCommand))
			{
				applyActionThrustAhead(object, trigger->action, ahead->amount, ahead->burn);
			}
			else if (const auto* play = std::get_if<CmdPlay>(&actionCommand))
			{
				if (game.soundLoops(play->sound)) { game.requestSound(play->sound); }
			}
		}

		return true;
	}

	// Back where it started, facing the way it started and going the way it
	// started, with any <random> in that start drawn again (a new serve).
	// While a held key is moving it, that key still decides its velocity.
	void CommandExecutor::restart(Object& object)
	{
		game.drawStartAgain(object);
		xge::restart(object);
	}

	// Puts `count` of the objects called `target` that are out of play back in,
	// each centred on `from` and at its own starting velocity. A pool of them is
	// a <group>: the first ones found that are not in play are the ones used, and
	// when there are fewer than asked for the rest are simply not released.
	void CommandExecutor::release(const Object& from, const std::string& target, int count)
	{
		const Vector2f middle = from.position + sizeOf(from) * 0.5f;
		int released = 0;

		// Of an object that does not exist at the start, new copies.
		if (game.isDescription(target))
		{
			for (; released < count; ++released)
			{
				Object* made = game.spawn(target);
				if (!made) { break; }
				made->position = middle - sizeOf(*made) * 0.5f;
				made->velocity = made->start.velocity;
			}
			return;
		}

		for (auto& candidate : game.getCurrentObjects())
		{
			if (released >= count) { break; }
			if (candidate.isVisible || !candidate.exists || &candidate == &from || !candidate.named(target) || game.isAway(candidate)) { continue; }

			// Back in play is a new start: a <random> in it is drawn again,
			// and its timers and animation start over.
			game.drawStartAgain(candidate);
			candidate.startOver();
			candidate.position = middle - sizeOf(candidate) * 0.5f;
			candidate.velocity = candidate.start.velocity;
			candidate.riding = {};
			candidate.isVisible = true;
			candidate.collisionsOn = true;
			++released;
		}
	}

	void CommandExecutor::bounceScreenEdge(Object& object, Edge edge, float share)
	{
		// An object in lockstep (e.g. the invader block in spaceinvaders) bounces as a
		// whole block sideways instead of just moving this one object.
		// circleRectangleCollision never had lockstep behaviour, so that path
		// (executeObjectCollision) always goes straight to bounceOffEdge.
		if (object.collisionData.lockstep && (edge == Edge::Left || edge == Edge::Right))
		{
			game.updateLockstepObjects(object);
			return;
		}

		xge::bounceOffEdge(object, edge, share);
	}

	void CommandExecutor::moveByStep(Object& object, Direction direction, float step)
	{
		const auto apply = [&](Object& target)
		{
			switch (direction)
			{
			case Direction::Up:    target.position.y -= step; break;
			case Direction::Down:  target.position.y += step; break;
			case Direction::Left:  target.position.x -= step; break;
			case Direction::Right: target.position.x += step; break;
			}
		};

		if (object.collisionData.lockstep > 0)
		{
			for (auto& obj : game.getCurrentObjects())
			{
				if (obj.collisionData.lockstep == object.collisionData.lockstep)
				{
					apply(obj);
				}
			}
		}
		else
		{
			apply(object);
		}
	}

	// A jump asked for by a hop.*() action. It is only queued here:
	// playOneFrame makes it at the start of the frame's move, where it can be
	// refused if it would leave the window. Asking twice in one frame keeps
	// the later one, so a hop is always a single step in one direction.
	bool CommandExecutor::startJump(Object& object, const CmdJump& jump)
	{
		if (!startFlight(object, stepToward(jump.direction, jump.distance), jump.seconds, jump.arc))
		{
			return false;
		}

		object.facing = jump.direction;
		if (jump.direction == Direction::Left || jump.direction == Direction::Right) { object.goingLeft = jump.direction == Direction::Left; }
		object.hasAim = false;
		return true;
	}

	bool CommandExecutor::startFlight(Object& object, Vector2f whole, float seconds, float arc)
	{
		if (object.isAirborne())
		{
			return false;
		}

		// A jump that would land off the screen is not made at all (up from the
		// top row stays where it is), rather than going part of the way.
		const WindowDesc& window = game.getWindowDesc();
		const Box landing{ object.position + whole, sizeOf(object) };
		if (!insideWindow(landing, { window.width, window.height }))
		{
			return false;
		}

		const int frames = game.framesFor(seconds);
		object.jumpStep = whole * (1.0f / static_cast<float>(frames));
		object.jumpFramesLeft = frames;
		object.jumpFrames = frames;
		object.jumpArc = arc;
		return true;
	}

	bool CommandExecutor::hop(Object& object, const CmdHop& command)
	{
		// On a cell map: whole cells (one, with no distance), refused (and
		// touching) when it cannot go.
		if (object.onCells)
		{
			const auto cells = [](float pixels, float distance)
			{
				if (pixels == 0.0f && distance == 0.0f) { return 0; }
				const int count = std::max(1, static_cast<int>(std::lround(std::fabs(distance > 0.0f ? distance : pixels))));
				return pixels < 0.0f ? -count : count;
			};
			Vector2f way = command.step;
			if (way.x == 0.0f && way.y == 0.0f) { way = stepToward(command.direction, 1.0f); }
			const int across = way.x == 0.0f ? 0 : cells(way.x, command.distance);
			const int down = way.y == 0.0f ? 0 : cells(way.y, command.distance);
			if (!game.stepCells(object, across, down)) { return false; }
			object.facing = command.direction;
			return true;
		}

		// One in the air waits for the landing.
		if (object.isAirborne()) { return false; }

		if (command.seconds > 0.0f)
		{
			if (!startFlight(object, command.step, command.seconds, command.arc)) { return false; }
		}
		else
		{
			// playOneFrame refuses it again where the object is then; asked
			// here, so the action knows at once.
			const WindowDesc& window = game.getWindowDesc();
			if (!insideWindow({ object.position + command.step, sizeOf(object) }, { window.width, window.height })) { return false; }
			queueHop(object, command.step);
		}
		object.facing = command.direction;
		if (command.step.x != 0.0f) { object.goingLeft = command.step.x < 0.0f; }
		object.hasAim = false;
		return true;
	}

	bool CommandExecutor::huntByHop(Object& object, const CmdHuntHop& hunt)
	{
		if (object.isAirborne() || !game.isShown(object)) { return false; }

		const Object* quarry = game.nearestInPlay(object, hunt.target);
		if (!quarry) { return false; }

		// The four hops it could make, in the order a tie goes: up before
		// down, then left before right.
		const std::array<std::pair<Direction, Vector2f>, 4> hops = hunt.diagonal
			? std::array<std::pair<Direction, Vector2f>, 4>{ { { Direction::Up, { -hunt.across, -hunt.down } }, { Direction::Up, { hunt.across, -hunt.down } },
				{ Direction::Down, { -hunt.across, hunt.down } }, { Direction::Down, { hunt.across, hunt.down } } } }
			: std::array<std::pair<Direction, Vector2f>, 4>{ { { Direction::Up, { 0.0f, -hunt.down } }, { Direction::Left, { -hunt.across, 0.0f } },
				{ Direction::Down, { 0.0f, hunt.down } }, { Direction::Right, { hunt.across, 0.0f } } } };

		const WindowDesc& window = game.getWindowDesc();
		const Vector2f there = quarry->position + sizeOf(*quarry) * 0.5f;
		const Vector2f middle = object.position + sizeOf(object) * 0.5f;
		std::optional<std::pair<Direction, Vector2f>> best;
		float bestDistance = 0.0f;
		for (const auto& [way, step] : hops)
		{
			if (!insideWindow({ object.position + step, sizeOf(object) }, { window.width, window.height })) { continue; }
			const Vector2f off = middle + step - there;
			const float distance = off.x * off.x + off.y * off.y;
			if (!best || distance < bestDistance)
			{
				best = std::make_pair(way, step);
				bestDistance = distance;
			}
		}
		if (!best) { return false; }

		return hop(object, CmdHop{ best->first, 0.0f, best->second, hunt.seconds, hunt.arc });
	}

	void CommandExecutor::queueHop(Object& object, Vector2f step)
	{
		object.hopPending = step;
	}

	void CommandExecutor::triggerObjectAction(const std::string& objectName, const std::string& actionName, bool keyPressed, bool byRule, const std::string& from)
	{
		const auto showing = [&](const Object& object) { return from.empty() || object.lookName() == from; };
		Object& first = game.getObject(objectName);
		if (first.name == objectName || !first.inGroup(objectName))
		{
			if (showing(first)) { runAction(first, actionName, keyPressed, byRule); }
			return;
		}

		// A group's name: every member, each its own press (by place, since an
		// action may spawn and so grow the list). Those it reaches are found
		// first: a piece one of them sends onto its map (the next shape of
		// Tetris) does not answer the same press.
		std::vector<Object>& objects = game.getCurrentObjects();
		std::vector<std::size_t> reached;
		for (std::size_t i = 0; i < objects.size(); ++i)
		{
			if (objects[i].inGroup(objectName) && showing(objects[i]) && !objects[i].awayFromHome()) { reached.push_back(i); }
		}
		for (const std::size_t i : reached) { runAction(objects[i], actionName, keyPressed, byRule); }
	}

	void CommandExecutor::runAction(Object& object, const std::string& actionName, bool keyPressed, bool byRule)
	{
		// An object out of play does not act on a key: a ship that is not
		// there does not fire, and a cannon not yet brought in does nothing.
		// Letting go still goes through, so nothing stays held.
		if (keyPressed && !object.isVisible)
		{
			return;
		}
		// A piece sent away from its own map (a next-piece box) is only shown.
		if (object.awayFromHome())
		{
			return;
		}

		Doer doer;
		doer.self = &object;
		doer.pressed = keyPressed;
		doer.byRule = byRule;
		doer.action = &actionName;
		for (const auto& written : object.action[actionName])
		{
			// Refused: what comes after it answers a press that did nothing.
			if (!run(written, doer)) { return; }
		}
	}

	void CommandExecutor::knock(Object& object, Object& other, const Touch& touch, float bounce)
	{
		if (other.mass > 0.0f)
		{
			xge::knock(object, other, touch, bounce);
			return;
		}

		// Through a thing that does not move: the far side of it is the side
		// the object's own touched edge faces, and what rests there is
		// knocked as though the object, as heavy as can be, had hit it.
		const Edge farSide = touch.edge;
		const Box base{ other.position, sizeOf(other) };
		const Vector2f way = touch.normal * -1.0f;
		for (Object& thing : game.getCurrentObjects())
		{
			if (&thing == &object || &thing == &other || !(thing.mass > 0.0f) || !thing.collisionsOn || thing.isAirborne() || !game.isShown(thing))
			{
				continue;
			}

			const Box at{ thing.position, sizeOf(thing) };
			if (!restsAgainst(at, base, farSide))
			{
				continue;
			}

			const std::optional<Knocked> after = knockedApart(object.velocity, 0.0f, thing.velocity, thing.mass, way, bounce);
			if (!after)
			{
				continue;
			}
			thing.velocity = after->second;

			Touch felt;
			felt.edge = opposite(farSide);
			felt.normal = way;
			felt.point = { std::clamp(touch.point.x, at.left(), at.right()), std::clamp(touch.point.y, at.top(), at.bottom()) };
			felt.along = 0.5f;
			felt.time = touch.time;
			felt.through = &other.objClass;
			game.reactTo(thing, object, felt);
		}
	}

	// Coming down onto the top of `other` (its own bottom edge touched, not
	// going up): it stands there. Put on the top, its fall stopped, on the
	// ground until the next frame's landing is worked out; a leap ends, and
	// the keys held now decide the way across. Any other touch does nothing,
	// so a platform can be jumped up through and walked past, and nothing
	// lands while it climbs.
	void CommandExecutor::land(Object& object, const Object& other, Edge edge)
	{
		if (!landOn(object.position, object.velocity, sizeOf(object), { other.position, sizeOf(other) }, edge, object.climbing))
		{
			return;
		}

		object.grounded = true;

		if (object.leaping)
		{
			object.leaping = false;
			// One with a <grip> below 1 slides on from the speed it landed at.
			if (object.grip >= 1.0f)
			{
				object.velocity.x = object.activeMoveStep[static_cast<std::size_t>(Direction::Right)]
					- object.activeMoveStep[static_cast<std::size_t>(Direction::Left)];
			}
		}
	}

	// Heads for the nearest of `chase.target` in play: straight at its middle
	// at `chase.speed`, or, within `chase.stopWithin` of it, stops. It faces the
	// way it goes, and a heading turns to it. With none in play it carries on
	// as it was.
	void CommandExecutor::chase(Object& object, const CmdChase& command)
	{
		const Object* target = game.nearestInPlay(object, command.target);
		if (!target) { return; }

		if (const auto way = xge::chase(middleOf(object), middleOf(*target), command.speed, command.stopWithin, object.velocity))
		{
			object.facing = directionOf(*way);
			if (object.hasHeading)
			{
				object.heading = headingOf(*way);
				object.showHeading();
			}
		}
	}

	// Turns to the nearest of `target` in play: the next shots go straight at
	// where its middle is now, and the object faces that way (to the nearest
	// of the four, or exactly, if it has a heading). Lined, only along one of
	// the eight ways, and refused (false) when none is near enough.
	bool CommandExecutor::aim(Object& object, const CmdAim& command)
	{
		const Object* other = game.nearestInPlay(object, command.target);
		if (!other) { return !command.lined; }

		auto way = wayTo(middleOf(object), middleOf(*other));
		if (!way) { return !command.lined; }
		if (command.lined)
		{
			way = linedUp(middleOf(*other) - middleOf(object), command.within);
			if (!way) { return false; }
		}

		object.aim = *way;
		object.hasAim = true;
		object.facing = directionOf(object.aim);
		if (object.hasHeading)
		{
			object.heading = headingOf(object.aim);
			object.showHeading();
		}
		return true;
	}

	// Up off the ground at the speed that, under the object's own pull, rises
	// `height` pixels: v * v = 2 * pull * height. Only from the ground and not
	// on a ladder; nothing without a pull down (the loader refuses that).
	bool CommandExecutor::leap(Object& object, float height)
	{
		if (!object.grounded || object.climbing || object.acceleration.y <= 0.0f)
		{
			return false;
		}

		object.velocity.y = -leapSpeed(object.acceleration.y, height);
		object.grounded = false;
		object.leaping = true;
		return true;
	}

	// Records that `direction` is now being pushed by `amount` a frame (0 =
	// just released), and the variable it burns; playOneFrame
	// applies every direction being held once a frame. Unlike a move's velocity
	// there is nothing to recombine: each direction adds its own amount.
	void CommandExecutor::applyActionThrust(Object& object, Direction direction, float amount, const std::string& burn)
	{
		const auto index = static_cast<std::size_t>(direction);
		object.activeThrust[index] = amount;
		object.activeThrustBurn[index] = amount == 0.0f ? std::string{} : burn;
	}

	// Records that a turn is now held (rate 0 = just released);
	// playOneFrame turns the object once a frame while it is.
	void CommandExecutor::applyActionTurn(Object& object, Direction direction, float rate)
	{
		object.activeTurn[static_cast<std::size_t>(direction)] = rate;
	}

	// Records that thrust along the heading is now held (amount 0 = just
	// released) by that action, and the variable it burns. Each action's
	// thrust is its own, so two held at once add up.
	void CommandExecutor::applyActionThrustAhead(Object& object, const std::string& action, float amount, const std::string& burn)
	{
		if (amount == 0.0f)
		{
			object.activeThrustAheadBy.erase(action);
			return;
		}
		object.activeThrustAheadBy[action] = { amount, burn };
	}

	// Launches a projectile that is not already in flight. The name can be a
	// pool of them (a <group>): the first one not in flight is used, and with
	// all of them in flight nothing happens. From a shooter with a heading the
	// shot leaves its nose the way it faces, at the projectile's own speed;
	// from any other it leaves the top centre, at the projectile's own
	// velocity.
	bool CommandExecutor::spawnProjectile(Object& shooter, const std::string& projectileName)
	{
		Object* projectile = nullptr;
		if (game.isDescription(projectileName))
		{
			// A new one, as many as its <most> allows.
			projectile = game.spawn(projectileName);
		}
		else
		{
			for (auto& candidate : game.getCurrentObjects())
			{
				if (candidate.named(projectileName) && candidate.exists && !candidate.collisionsOn && !game.isAway(candidate))
				{
					projectile = &candidate;
					break;
				}
			}
			if (projectile)
			{
				// Fired is a new start: a <random> in the projectile's is drawn
				// again, and its timers and animation start over.
				game.drawStartAgain(*projectile);
				projectile->startOver();
			}
		}
		if (!projectile) { return false; }

		const Box from{ shooter.position, sizeOf(shooter) };
		const Vector2f shotSize = sizeOf(*projectile);
		const float speed = std::hypot(projectile->start.velocity.x, projectile->start.velocity.y);
		std::optional<Shot> shot;
		if (shooter.hasAim)
		{
			// Straight along the aim, from the middle of the shooter, just clear
			// of it, at the projectile's own speed.
			shot = shotAlong(from, shotSize, shooter.aim, speed);
		}
		else if (shooter.hasHeading)
		{
			shot = shotAhead(from, shotSize, shooter.heading, speed);
		}
		else if (shooter.hasFacing)
		{
			// From the middle of the side it faces, just clear of it, moving that
			// way at the projectile's own speed.
			shot = shotFacing(from, shotSize, shooter.facing, speed);
		}
		else
		{
			// From the shooter's top-centre: the projectile's own middle over
			// the shooter's, so a thin shot leaves the middle of the gun.
			shot = Shot{ shotFromTop(from, shotSize), projectile->start.velocity };
		}

		projectile->position = shot->position;
		projectile->velocity = shot->velocity;
		projectile->isVisible = true;
		projectile->collisionsOn = true;
		return true;
	}

	// A new object, a copy of `spawn.target` (Game::spawn), put and sent
	// where the command says; what it leaves out is as the file says it
	// starts. False when no more of it may exist.
	bool CommandExecutor::spawn(const CmdSpawn& command, const Object* self)
	{
		Object* made = game.spawn(command.target, command.random);
		if (!made) { return false; }

		// On a cell map: in a cell, a shape's top left one (counting from 1).
		if (made->onCells)
		{
			// Onto another map: its first cell unless told; on its own, its own.
			const bool elsewhere = !command.into.empty() && command.into != made->mapName;
			// Left out: the spawner's own cell, when it is on that map.
			const bool besideSelf = self && self->onCells && self->mapName == (command.into.empty() ? made->mapName : command.into);
			const auto cell = [&](const std::optional<float>& given, int own, float selfCell)
			{
				if (given) { return static_cast<int>(std::lround(*given)); }
				if (besideSelf) { return static_cast<int>(std::lround(selfCell)); }
				return elsewhere ? 1 : own + 1;
			};
			if (command.free)
			{
				if (!game.putInFreeCell(*made, command.into, command.freeBelow)) { return false; }
			}
			else if (!game.putOnCells(*made, cell(command.column, made->mapColumn, self ? self->cellColumn : 0.0f), cell(command.row, made->mapRow, self ? self->cellRow : 0.0f), command.into)) { return false; }
			if (command.velocityX) { made->velocity.x = *command.velocityX; }
			if (command.velocityY) { made->velocity.y = *command.velocityY; }
			return true;
		}

		const Vector2f size = sizeOf(*made);
		if (command.x) { made->position.x = command.centerX ? *command.x - size.x * 0.5f : *command.x; }
		if (command.y) { made->position.y = command.centerY ? *command.y - size.y * 0.5f : *command.y; }
		if (command.velocityX) { made->velocity.x = *command.velocityX; }
		if (command.velocityY) { made->velocity.y = *command.velocityY; }
		return true;
	}

	void CommandExecutor::despawn(Object* self, const CmdDespawn& command)
	{
		if (!command.target.empty()) { game.despawn(command.target); }
		else if (self) { game.despawn(*self); }
	}
}
