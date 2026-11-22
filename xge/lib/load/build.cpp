// build.cpp
// XML Game Engine
// author: beefviper
// date: Nov 15, 2026
//
// One object made from what the file says of it (Evaluator::makeObject),
// in named steps: its looks and picture, names, numbers, ways of moving,
// timers, board-game moves, holder, place and collisions, then actions.

#include "load/expr.h"

#include "load/spelling.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

namespace xge
{
	Object Evaluator::makeObject(WrittenObject& writtenObject, const FirstPass& first, Vector2f cellOffset, int thisLockstep)
	{
		const std::string where = "object '" + writtenObject.name + "'";
		struct Self
		{
			std::string& name;
			std::string was;
			~Self() { name = std::move(was); }
		} self{ selfName, std::exchange(selfName, writtenObject.name) };

		// Only a text showing a number needs working out again here, for the
		// value it has by now (see above). The object and its first look
		// hold this one picture.
		const HeldPicture picture = (writtenObject.sprite.kind == "text" && showsNumbers(writtenObject.sprite))
			? buildPicture(writtenObject.sprite, where, nullptr, nullptr, &writtenObject)
			: first.picture;

		// A position that uses the size of a text (its own, or another
		// object's) is worked out again when that text's words change - see
		// Object::Layout.
		const std::vector<std::string> positionSizeDependencies = sizeDependenciesOf(writtenObject);

		// How long each picture of an animation lasts.
		const float animationSeconds = writtenObject.hasAnimation ? animationSecondsOf(writtenObject.animation.interval, where + " > <animation>") : 0.0f;

		// The object's looks, if it has several sprites and no animation.
		const std::vector<Object::Look> looks = buildLooks(writtenObject, first, picture, where);

		Object object{};
		object.kind->written = writtenObject.kind;
		object.id = parts->newId();
		const std::uint64_t order = parts->orderOf(object.id);
		buildSprite(writtenObject, first, picture, animationSeconds, object, where);
		buildNames(writtenObject, object);
		buildNumbers(writtenObject, object, cellOffset, positionSizeDependencies, where);
		buildMotion(writtenObject, object, where);
		buildTimers(writtenObject, object, where);
		buildLookParts(writtenObject, object, looks, where);
		buildMoves(writtenObject, object, looks, where);
		buildHolder(writtenObject, object, order, where);
		buildPlace(writtenObject, object, looks, where);
		buildCollisions(writtenObject, object, looks, thisLockstep, where);
		buildActions(writtenObject, object, where);

		// How it started is how it was before its first frame.
		object.before = startFor(object);

		// object's visual is built later by whichever Window backend is
		// running, once one exists (see Window::init() in window/window.h) -
		// visualDirty starts true (Object's own default), so nothing
		// needs to happen here.
		return object;
	}

	// makeObject's looks: each named <sprite> or <animation> made, the first the
	// very picture it starts with, and where each once look goes when it ends.
	std::vector<Object::Look> Evaluator::buildLooks(WrittenObject& writtenObject, const FirstPass& first, const HeldPicture& picture, const std::string& where)
	{
		std::vector<Object::Look> looks;
		if (first.looks)
		{
			looks = *first.looks;
		}
		else if (!writtenObject.looks.empty())
		{
			for (const WrittenSprite& writtenLook : writtenObject.looks)
			{
				Object::Look look;
				look.name = writtenLook.name;
				std::shared_ptr<const Turnable> turned;
				look.picture = buildPicture(writtenLook, where + " > <sprite name=\"" + writtenLook.name + "\">", &look.bitmap,
					writtenObject.writtenHeading ? &turned : nullptr);
				if (turned) { look.turnables = { turned }; }
				if (!writtenLook.frames.empty())
				{
					// A look that moves: its pictures, all one size, and how long each shows.
					const std::string here = where + " > <animation name=\"" + writtenLook.name + "\">";
					look.turnables.clear();
					Object::Frames& moving = look.frames.edit();
					moving.bitmaps = buildAnimationBitmaps(writtenLook.frames, here, writtenObject.writtenHeading ? &look.turnables : nullptr);
					moving.seconds = animationSecondsOf(writtenLook.interval, here);
					moving.times = animationTimesOf(writtenLook.times, moving.seconds, here);
					moving.once = writtenLook.once;
					moving.stepped = writtenLook.stepped;
					moving.then = processCommands(writtenLook.thenCommands, here + " > <then>");
					moving.commands = frameCommandsOf(writtenLook.frameCommands, here);
				}
				looks.push_back(std::move(look));
			}

			// The first look is the sprite the object starts with; it must be
			// the very same picture.
			looks.front().picture = picture;
			looks.front().bitmap = first.bitmap;
			if (!looks.front().frames.shows().bitmaps.empty()) { looks.front().frames.edit().bitmaps.front() = first.bitmap; }
			if (first.turned && looks.front().frames.shows().bitmaps.empty()) { looks.front().turnables = { first.turned }; }

			// The look each once look goes to when it ends, by its name.
			for (std::size_t i = 0; i < writtenObject.looks.size(); ++i)
			{
				const std::string& then = writtenObject.looks[i].thenSprite;
				if (then.empty()) { continue; }
				const auto found = std::find_if(looks.begin(), looks.end(), [&](const Object::Look& each) { return each.name == then; });
				if (found == looks.end())
				{
					std::vector<std::string> names;
					for (const Object::Look& each : looks) { names.push_back(each.name); }
					throw std::runtime_error(where + " > <animation name=\"" + writtenObject.looks[i].name + "\"> > <then sprite=\"" + then + "\">: the object has no look of that name"
						+ didYouMean(then, names) + ". Its looks are: " + listOf(names));
				}
				looks[i].thenLook = static_cast<std::size_t>(found - looks.begin());
			}
		}
		return looks;
	}

	// makeObject's picture: the sprite it starts as, its animation, and a
	// text's live pieces.
	void Evaluator::buildSprite(WrittenObject& writtenObject, const FirstPass& first, const HeldPicture& picture, float animationSeconds, Object& object, const std::string& where)
	{
		object.drawn.picture = picture;
		object.drawn.bitmap = first.bitmap;
		if (first.turned) { object.drawn.turnables = { first.turned }; }

		if (writtenObject.hasAnimation)
		{
			Object::Frames& own = object.animation.frames.edit();
			own.bitmaps = first.animation;
			if (!first.animationTurned.empty()) { object.drawn.turnables = first.animationTurned; }
			own.seconds = animationSeconds;
			own.times = animationTimesOf(writtenObject.animation.times, animationSeconds, where + " > <animation>");
			own.once = writtenObject.animation.once;
			own.stepped = writtenObject.animation.stepped;
			own.then = processCommands(writtenObject.animation.thenCommands, where + " > <animation> > <then>");
			own.commands = frameCommandsOf(writtenObject.animation.frameCommands, where + " > <animation>");
			object.drawn.bitmap = own.bitmaps.front();
		}

		const auto showsLive = [](const std::vector<TextPiece>& pieces) { return std::any_of(pieces.begin(), pieces.end(), [](const TextPiece& piece) { return !piece.owner.empty(); }); };
		if (writtenObject.looks.empty())
		{
			if (object.drawn.picture->kind == ShapeKind::Text && showsNumbers(writtenObject.sprite))
			{
				std::vector<TextPiece> pieces = textPieces(writtenObject.sprite, where, &writtenObject);
				if (showsLive(pieces)) { parts->boundTexts.add(object.id, parts->orderOf(object.id), { std::move(pieces), {}, 0 }); }
			}
			return;
		}
		// Each look that is a text showing a number keeps its own pieces.
		std::vector<std::vector<TextPiece>> looks;
		bool live = false;
		for (const WrittenSprite& look : writtenObject.looks)
		{
			looks.push_back(look.kind == "text" && showsNumbers(look) ? textPieces(look, where + " > <sprite name=\"" + look.name + "\">", &writtenObject) : std::vector<TextPiece>{});
			live = live || showsLive(looks.back());
		}
		if (live)
		{
			std::vector<TextPiece> shown = looks.front();
			parts->boundTexts.add(object.id, parts->orderOf(object.id), { std::move(shown), std::move(looks), 0 });
		}
	}

	// makeObject's names: its group, class and owners, and whether it is
	// drawn, seen and there.
	void Evaluator::buildNames(WrittenObject& writtenObject, Object& object)
	{
		object.groupName = Name::of(writtenObject.groupName);
		object.outerGroups = NameStore::program().listOf(writtenObject.outerGroups);
		object.name = Name::of(writtenObject.name);
		object.line = writtenObject.line;
		object.objClass = Name::of(writtenObject.objClass);
		object.isDrawn = writtenObject.isDrawn;
		object.ownerState = Name::of(writtenObject.ownerState);
		object.ownerScene = Name::of(writtenObject.ownerScene);
		startFor(object).ownerScene = object.ownerScene;

		if (writtenObject.objClass == "projectile")
		{
			writtenObject.isVisible = false;
		}

		object.isVisible = writtenObject.isVisible;
		object.exists = writtenObject.exists;
		startFor(object).exists = writtenObject.exists;
		// One that does not exist is in nothing that looks for what is out
		// there, as one taken out of play is not.
		if (!object.exists) { object.isVisible = false; }
		object.most = writtenObject.most;
	}

	// makeObject's numbers: its variables, then where it starts, what it
	// follows, its velocity, acceleration and heading.
	void Evaluator::buildNumbers(WrittenObject& writtenObject, Object& object, Vector2f cellOffset, const std::vector<std::string>& positionSizeDependencies, const std::string& where)
	{
		// Its variables first, so its position and velocity can use them
		// (a serve drawn as an angle: its speed times the cosine of its angle).
		for (auto& writtenVariable : writtenObject.variable)
		{
			const float value = evaluateStart(writtenVariable.second, where);
			object.variable[Name::of(writtenVariable.first)] = value;
			startFor(object).variable[Name::of(writtenVariable.first)] = value;

			// Keep the cross-object symbol table entry (registered above,
			// before any expression compiled) up to date with the real
			// value now that it's known.
			objectVariables[writtenObject.name + "." + writtenVariable.first] = value;
		}

		object.startDrawsRandom = writtenObject.writtenPosition.x.drawsRandom() || writtenObject.writtenPosition.y.drawsRandom()
			|| writtenObject.writtenVelocity.x.drawsRandom() || writtenObject.writtenVelocity.y.drawsRandom()
			|| std::any_of(writtenObject.variable.begin(), writtenObject.variable.end(), [](const auto& variable) { return variable.second.drawsRandom(); });

		startFor(object).position.x = evaluateStart(writtenObject.writtenPosition.x, where);
		startFor(object).position.y = evaluateStart(writtenObject.writtenPosition.y, where);

		// A cell is its group's position and its place in the
		// group; start.position keeps both, so a reset puts each
		// cell (each brick) back in its own place.
		object.layout.cellOffset = cellOffset;
		object.position = startFor(object).position + object.layout.cellOffset;
		startFor(object).position = object.position;

		if (!positionSizeDependencies.empty()) { parts->sizeUses.add(object.id, parts->orderOf(object.id), { NameStore::program().of(positionSizeDependencies), {} }); }

		// What it follows, worked out again every frame (Game::follow).
		using Follow = Object::Follow;
		if (writtenObject.writtenPosition.x.live) { parts->give(parts->follows, object.id).each.push_back({ Follow::Part::X, "", writtenObject.writtenPosition.x }); }
		if (writtenObject.writtenPosition.y.live) { parts->give(parts->follows, object.id).each.push_back({ Follow::Part::Y, "", writtenObject.writtenPosition.y }); }
		if (writtenObject.writtenHeading && writtenObject.writtenHeading->live) { parts->give(parts->follows, object.id).each.push_back({ Follow::Part::Heading, "", *writtenObject.writtenHeading }); }
		for (const auto& [name, value] : writtenObject.variable)
		{
			if (value.live) { parts->give(parts->follows, object.id).each.push_back({ Follow::Part::Variable, name, value }); }
		}

		object.velocity.x = evaluate(writtenObject.writtenVelocity.x, where);
		object.velocity.y = evaluate(writtenObject.writtenVelocity.y, where);

		startFor(object).velocity = object.velocity;

		if (writtenObject.writtenAcceleration)
		{
			object.acceleration.x = evaluate(writtenObject.writtenAcceleration->x, where);
			object.acceleration.y = evaluate(writtenObject.writtenAcceleration->y, where);
		}
		startFor(object).acceleration = object.acceleration;

		if (writtenObject.writtenHeading)
		{
			object.turning.hasHeading = true;
			float degrees = std::fmod(evaluateStart(*writtenObject.writtenHeading, where), 360.0f);
			if (degrees < 0.0f) { degrees += 360.0f; }
			object.heading = degrees;
		}
		startFor(object).heading = object.heading;
	}

	// A <drag> or <grip>, written as a half-life (the seconds to lose half
	// the speed; 0: at once), as the rate its speed is lost at each second:
	// kept over t is e to the -rate t; infinite, all of it at once.
	float Evaluator::lossRate(float written, const std::string& what) const
	{
		if (!(written >= 0.0f)) { throw std::runtime_error(what + " is " + formatDisplayNumber(written) + "; expected a half-life in seconds, 0 or more (0: at once)"); }
		return written == 0.0f ? std::numeric_limits<float>::infinity() : std::log(2.0f) / written;
	}

	// makeObject's ways of moving: drag, surface, friction, mass, pulls,
	// seeking, light and dark, altitude, a tether and which way it faces.
	void Evaluator::buildMotion(WrittenObject& writtenObject, Object& object, const std::string& where)
	{
		if (writtenObject.writtenDrag)
		{
			object.motion.drag = lossRate(evaluate(*writtenObject.writtenDrag, where), where + " > <drag>");
		}

		if (writtenObject.hasSurface)
		{
			Object::Surface surface;
			if (writtenObject.writtenSurfaceDrag)
			{
				surface.drag = lossRate(evaluate(*writtenObject.writtenSurfaceDrag, where + " > <surface> > <drag>"), where + " > <surface> > <drag>");
			}
			if (writtenObject.writtenSurfaceGrip)
			{
				surface.grip = lossRate(evaluate(*writtenObject.writtenSurfaceGrip, where + " > <surface> > <grip>"), where + " > <surface> > <grip>");
			}
			parts->surfaces.add(object.id, parts->orderOf(object.id), surface);
		}

		if (writtenObject.writtenFriction)
		{
			parts->rests.add(object.id, parts->orderOf(object.id), {});
			object.motion.friction = evaluate(*writtenObject.writtenFriction, where);
			if (object.motion.friction < 0.0f)
			{
				throw std::runtime_error(where + ": <friction> is " + std::to_string(object.motion.friction) + "; expected 0 or more");
			}
		}

		if (writtenObject.writtenMass)
		{
			object.motion.mass = evaluate(*writtenObject.writtenMass, where);
			if (!(object.motion.mass > 0.0f))
			{
				throw std::runtime_error(where + ": <mass> is " + formatDisplayNumber(object.motion.mass) + "; expected more than 0");
			}
		}

		if (!writtenObject.writtenPulls.empty() || !writtenObject.seekTarget.empty())
		{
			Object::Pull& pull = parts->pulls.add(object.id, parts->orderOf(object.id), {});
			for (const auto& [name, strength] : writtenObject.writtenPulls)
			{
				pull.toward.emplace_back(name, evaluate(strength, where + " > <pull object=\"" + name + "\">"));
			}
			if (!writtenObject.seekTarget.empty())
			{
				pull.seek = writtenObject.seekTarget;
				pull.seekRate = evaluate(writtenObject.writtenSeek, where + " > <seek>");
				if (pull.seekRate < 0.0f) { throw std::runtime_error(where + ": <seek> turns " + formatDisplayNumber(pull.seekRate) + " degrees a second; expected 0 or more"); }
			}
		}

		if (writtenObject.writtenLight)
		{
			object.light = evaluate(*writtenObject.writtenLight, where + " > <light>");
			if (object.light < 0.0f) { throw std::runtime_error(where + ": <light> reaches " + formatDisplayNumber(object.light) + " pixels; expected 0 or more"); }
			startFor(object).light = object.light;
			if (!writtenObject.lightPast.empty()) { parts->lightPasts.add(object.id, parts->orderOf(object.id), { NameStore::program().of(writtenObject.lightPast) }); }
		}
		if (writtenObject.dark == "unlit") { object.dark.kind = Object::Darkness::Unlit; }
		else if (writtenObject.dark == "unseen") { object.dark.kind = Object::Darkness::Unseen; }
		object.dark.hidden = object.dark.kind != Object::Darkness::None;

		Object::Aloft* aloft = writtenObject.hasAltitude || !writtenObject.shadowOf.empty() ? &parts->alofts.add(object.id, parts->orderOf(object.id), {}) : nullptr;
		if (writtenObject.hasAltitude)
		{
			const std::string here = where + " > <altitude>";
			const auto given = [&](const WrittenValue& value) { return !(value.kind == WrittenValue::Kind::Expression && value.text.empty()); };
			aloft->has = true;
			object.altitude = given(writtenObject.writtenAltitude) ? evaluate(writtenObject.writtenAltitude, here) : 0.0f;
			object.rise = given(writtenObject.writtenRise) ? evaluate(writtenObject.writtenRise, here) : 0.0f;
			aloft->fall = given(writtenObject.writtenFall) ? evaluate(writtenObject.writtenFall, here) : 0.0f;
			aloft->bounce = given(writtenObject.writtenAltitudeBounce) ? evaluate(writtenObject.writtenAltitudeBounce, here) : 0.0f;
			if (object.altitude < 0.0f) { throw std::runtime_error(here + ": <start> is " + formatDisplayNumber(object.altitude) + "; it cannot start below the ground"); }
			if (aloft->fall < 0.0f) { throw std::runtime_error(here + ": <fall> is " + formatDisplayNumber(aloft->fall) + "; expected 0 or more"); }
			if (!(aloft->bounce >= 0.0f && aloft->bounce <= 1.0f)) { throw std::runtime_error(here + ": <bounce> is " + formatDisplayNumber(aloft->bounce) + "; expected 0 up to 1"); }
			startFor(object).altitude = object.altitude;
			startFor(object).rise = object.rise;
		}
		if (aloft) { aloft->shadowOf = writtenObject.shadowOf; }

		if (writtenObject.sprite.kind == "tether")
		{
			if (writtenObject.writtenHeading || !writtenObject.looks.empty() || writtenObject.hasAnimation)
			{
				throw std::runtime_error(where + ": a <tether> is drawn where its ends are; it has no <heading>, other looks or <animation>");
			}
			Object::Tether& tether = parts->tethers.add(object.id, parts->orderOf(object.id), {});
			tether.from = writtenObject.sprite.tetherFrom;
			tether.to = writtenObject.sprite.tetherTo;
			if (!writtenObject.sprite.thickness.text.empty() || writtenObject.sprite.thickness.kind != WrittenValue::Kind::Expression)
			{
				tether.thickness = static_cast<int>(std::lround(evaluate(writtenObject.sprite.thickness, where)));
				if (tether.thickness < 1) { throw std::runtime_error(where + ": a <tether> has a <thickness> under 1"); }
			}
			tether.color = colorFromName(writtenObject.sprite.color.empty() ? "color.white" : writtenObject.sprite.color);
		}

		if (!writtenObject.facing.empty())
		{
			object.hasFacing = true;
			object.facing = writtenObject.facing == "down" ? Direction::Down
				: writtenObject.facing == "left" ? Direction::Left
				: writtenObject.facing == "right" ? Direction::Right
				: Direction::Up;
		}
		startFor(object).facing = object.facing;
	}

	// makeObject's timers, a named one's time left a number of its own.
	void Evaluator::buildTimers(WrittenObject& writtenObject, Object& object, const std::string& where)
	{
		object.kind->timers = processTimers(writtenObject.timers, where);
		if (!object.kind->timers.empty()) { parts->clocks.add(object.id, parts->orderOf(object.id), std::vector<Timer::Clock>(object.kind->timers.size())); }
		for (const Timer& timer : object.kind->timers)
		{
			if (timer.name.empty()) { continue; }
			// A named timer's time left (see init): where it starts.
			const float left = std::ceil(timer.seconds);
			object.variable[timer.name] = left;
			startFor(object).variable[timer.name] = left;
			objectVariables[writtenObject.name + "." + timer.name] = left;
		}
	}

	// makeObject's looks given it, the first one's animation, the look for
	// each way it moves (<poses>) and for where what it is joined to lies.
	void Evaluator::buildLookParts(WrittenObject& writtenObject, Object& object, const std::vector<Object::Look>& looks, const std::string& where)
	{
		object.looks.set->all = looks;

		// Animations timed live (<seconds update="live">), followed after the
		// object's other live numbers: the animation's own seconds, and each
		// picture's, its own or, with none, the animation's.
		const auto followSeconds = [&](const WrittenValue& interval, const std::vector<WrittenValue>& times, std::size_t look)
		{
			using Follow = Object::Follow;
			if (interval.live) { parts->give(parts->follows, object.id).each.push_back({ Follow::Part::Seconds, "", interval, look, std::string::npos }); }
			for (std::size_t i = 0; i < times.size(); ++i)
			{
				const WrittenValue& time = frameTimeGiven(times[i]) ? times[i] : interval;
				if (time.live) { parts->give(parts->follows, object.id).each.push_back({ Follow::Part::Seconds, "", time, look, i }); }
			}
		};
		if (writtenObject.hasAnimation) { followSeconds(writtenObject.animation.interval, writtenObject.animation.times, std::string::npos); }
		for (std::size_t i = 0; i < writtenObject.looks.size() && i < looks.size(); ++i)
		{
			if (!writtenObject.looks[i].frames.empty()) { followSeconds(writtenObject.looks[i].interval, writtenObject.looks[i].times, i); }
		}
		if (!looks.empty() && !looks.front().frames.shows().bitmaps.empty())
		{
			// It starts as a look that moves.
			object.animation.frames = looks.front().frames;
		}

		// The look for each way it moves; one left out is another's.
		if (!writtenObject.poses.empty())
		{
			const auto lookOf = [&](const std::string& pose) -> std::optional<std::size_t>
			{
				const auto named = writtenObject.poses.find(pose);
				if (named == writtenObject.poses.end()) { return std::nullopt; }
				for (std::size_t i = 0; i < looks.size(); ++i)
				{
					if (looks[i].name == named->second) { return i; }
				}
				std::vector<std::string> names;
				for (const Object::Look& look : looks) { names.push_back(look.name); }
				throw std::runtime_error(where + " > <poses> > <" + pose + " sprite=\"" + named->second + "\">: the object has no look of that name"
					+ (names.empty() ? std::string(" (it has no looks)") : didYouMean(named->second, names) + ". Its looks are: " + listOf(names)));
			};
			const std::size_t standing = *lookOf("standing");
			const std::size_t walking = lookOf("walking").value_or(standing);
			const std::size_t falling = lookOf("falling").value_or(standing);
			object.looks.set->poses[static_cast<std::size_t>(Object::Pose::Standing)] = standing;
			object.looks.set->poses[static_cast<std::size_t>(Object::Pose::Walking)] = walking;
			object.looks.set->poses[static_cast<std::size_t>(Object::Pose::Skidding)] = lookOf("skidding").value_or(walking);
			object.looks.set->poses[static_cast<std::size_t>(Object::Pose::Falling)] = falling;
			object.looks.set->poses[static_cast<std::size_t>(Object::Pose::Rising)] = lookOf("rising").value_or(falling);
			object.looks.set->poses[static_cast<std::size_t>(Object::Pose::Thrusting)] = lookOf("thrusting").value_or(standing);
			object.looks.set->hasPoses = true;
			object.looks.set->first = standing;
			if (standing != 0) { object.showLook(standing); }
		}

		// The look for where the piece it is joined to lies.
		if (!writtenObject.joins.empty())
		{
			std::size_t at = 0;
			for (const char* join : { "alone", "left", "right", "up", "down" })
			{
				const std::string& name = writtenObject.joins.at(join);
				const auto look = std::find_if(looks.begin(), looks.end(), [&](const Object::Look& each) { return each.name == name; });
				if (look == looks.end())
				{
					std::vector<std::string> names;
					for (const Object::Look& each : looks) { names.push_back(each.name); }
					throw std::runtime_error(where + " > <joins> > <" + join + " sprite=\"" + name + "\">: the object has no look of that name"
						+ (names.empty() ? std::string(" (it has no looks)") : didYouMean(name, names) + ". Its looks are: " + listOf(names)));
				}
				object.looks.set->joins[at++] = static_cast<std::size_t>(look - looks.begin());
			}
			object.looks.set->hasJoins = true;
		}
	}

	// makeObject's board-game moves and marks, each set's look found.
	void Evaluator::buildMoves(WrittenObject& writtenObject, Object& object, const std::vector<Object::Look>& looks, const std::string& where)
	{
		// A board game piece's moves, each set's look found.
		for (const WrittenObject::WrittenMoveSet& writtenSet : writtenObject.moveSets)
		{
			const auto lookNamed = [&](const std::string& name, const std::string& at) -> std::size_t
			{
				const auto look = std::find_if(looks.begin(), looks.end(), [&](const Object::Look& each) { return each.name == name; });
				if (look == looks.end())
				{
					std::vector<std::string> names;
					for (const Object::Look& each : looks) { names.push_back(each.name); }
					throw std::runtime_error(where + " > " + at + " sprite=\"" + name + "\">: the object has no look of that name"
						+ (names.empty() ? std::string(" (it has no looks)") : didYouMean(name, names) + ". Its looks are: " + listOf(names)));
				}
				return static_cast<std::size_t>(look - looks.begin());
			};
			Object::MoveSet set;
			if (!writtenSet.sprite.empty()) { set.look = lookNamed(writtenSet.sprite, "<moves"); }
			set.forced = writtenSet.forced;
			for (const WrittenObject::WrittenPieceMove& move : writtenSet.moves) { set.moves.push_back({ move.jump, move.objClass, move.x, move.y }); }
			if (!writtenSet.promote.empty()) { set.promoteLook = lookNamed(writtenSet.promote, "<moves> > <promote"); }
			set.promoteRow = writtenSet.promoteRow;
			for (const WrittenObject::WrittenMoveSet::WrittenFlank& flank : writtenSet.flanks) { set.flanks.push_back({ flank.objClass, flank.straight }); }
			set.fall = writtenSet.fall;
			set.on = NameStore::program().of(writtenSet.on);
			set.closeClass = writtenSet.closeClass;
			set.closeObject = writtenSet.closeObject;
			for (const auto& [column, row] : writtenSet.route)
			{
				if (column > writtenObject.cell.mapColumns || row > writtenObject.cell.mapRows)
				{
					throw std::runtime_error(where + " > <moves> > <route>: column " + std::to_string(column) + ", row " + std::to_string(row) + " is off its map, which is "
						+ std::to_string(writtenObject.cell.mapColumns) + " cells across and " + std::to_string(writtenObject.cell.mapRows) + " down");
				}
				set.route.cells.push_back({ column, row });
			}
			set.route.start = writtenSet.routeStart;
			set.route.end = writtenSet.routeEnd;
			set.route.home = writtenSet.home;
			set.route.steps = writtenSet.steps;
			set.route.into = writtenSet.stepsInto;
			set.route.hits = NameStore::program().of(writtenSet.hits);
			if (!set.route.cells.empty() && set.route.steps.empty()) { throw std::runtime_error(where + " > <moves>: a <route> needs <steps holder=\"...\" />, what says how far each move goes"); }
			parts->give(parts->moves, object.id).sets.push_back(std::move(set));
		}
		for (const WrittenObject::WrittenMarkSet& writtenSet : writtenObject.markSets)
		{
			Object::MarkSet set;
			if (!writtenSet.sprite.empty())
			{
				const auto look = std::find_if(looks.begin(), looks.end(), [&](const Object::Look& each) { return each.name == writtenSet.sprite; });
				if (look == looks.end())
				{
					std::vector<std::string> names;
					for (const Object::Look& each : looks) { names.push_back(each.name); }
					throw std::runtime_error(where + " > <marks sprite=\"" + writtenSet.sprite + "\">: the object has no look of that name"
						+ (names.empty() ? std::string(" (it has no looks)") : didYouMean(writtenSet.sprite, names) + ". Its looks are: " + listOf(names)));
				}
				set.look = static_cast<std::size_t>(look - looks.begin());
			}
			for (const WrittenObject::WrittenMark& written : writtenSet.marks)
			{
				Object::Mark mark;
				mark.kind = written.kind == "held" ? Object::Mark::Kind::Held : written.kind == "moves" ? Object::Mark::Kind::Moves
					: written.kind == "movable" ? Object::Mark::Kind::Movable : written.kind == "put" ? Object::Mark::Kind::Put : Object::Mark::Kind::Last;
				mark.object = written.object;
				mark.objClass = written.objClass;
				mark.variable = written.variable;
				set.marks.push_back(std::move(mark));
			}
			parts->give(parts->pickers, object.id).marks.sets.push_back(std::move(set));
		}
	}

	// makeObject's holder (<holds>), hand, held item and turns.
	void Evaluator::buildHolder(WrittenObject& writtenObject, Object& object, std::uint64_t order, const std::string& where)
	{
		// Holders: a holder's rules, a hand, a held member.
		if (writtenObject.holder.is)
		{
			const WrittenObject::WrittenHolder& written = writtenObject.holder;
			const auto number = [&](const WrittenValue& value) { return value.kind == WrittenValue::Kind::Expression && value.text.empty() ? 0.0f : evaluate(value, where); };
			Holder& holder = parts->holders.add(object.id, order, {});
			holder.holdsClass = written.holdsClass;
			holder.holdsSprite = written.holdsSprite;
			holder.topSprite = written.topSprite;
			holder.spread = { number(written.spreadX), number(written.spreadY) };
			holder.lookSpread = { number(written.lookSpreadX), number(written.lookSpreadY) };
			holder.spreadSprite = written.spreadSprite;
			holder.startCount = static_cast<int>(std::lround(number(written.startCount)));
			holder.startMore = static_cast<int>(std::lround(number(written.startMore)));
			holder.startShuffled = written.startShuffled;
			holder.pickedInto = written.pickedInto;
			holder.pickedSends = !written.pickedSends ? 0 : std::max(1, static_cast<int>(std::lround(number(*written.pickedSends))));
			holder.emptyFrom = written.emptyFrom;
			holder.emptyRandom = written.emptyRandom;
			holder.pickedSound = written.pickedSound;
			holder.emptyLeave = std::max(0, static_cast<int>(std::lround(number(written.emptyLeave))));
			holder.spreadWidth = number(written.spreadWidth);
			holder.carry = written.carry;
			holder.orderBy = NameStore::program().of(written.orderBy);
			holder.counts = written.counts;
			holder.lift = number(written.lift);
			holder.pointsMost = written.hasPoints ? number(written.pointsMost) : 0.0f;
			holder.score = written.score;
			holder.scoreWith = written.scoreWith;
			holder.scoreFull = written.scoreFull;
			holder.scoreBest = written.scoreBest;
			holder.melds = written.melds;
			holder.meldsBest = written.meldsBest;
			holder.takes = written.takes;
		}
		if (writtenObject.hand)
		{
			Hand& hand = parts->hands.add(object.id, order, {});
			hand.own = writtenObject.handHolder;
			hand.seat = writtenObject.handSeat;
			hand.team = writtenObject.handTeam;
			hand.holds = writtenObject.handHolds;
		}
		// A card's two looks are its face and its back; with any other number
		// of looks (a die's six faces, a token's one) every look is a face.
		if (writtenObject.held)
		{
			const bool facesOnly = writtenObject.looks.size() != 2;
			parts->items.add(object.id, order, { true, writtenObject.item.startHolder, writtenObject.item.worths, 0, 0, facesOnly });
		}
		if (writtenObject.takesTurns) { parts->turns.add(object.id, order, { true, writtenObject.turnsSprite, writtenObject.ownsClass }); }
	}

	// makeObject's mirror, grip, timers' looks, its tile on a map, a
	// maze's walls and its pivot.
	void Evaluator::buildPlace(WrittenObject& writtenObject, Object& object, const std::vector<Object::Look>& looks, const std::string& where)
	{
		if (!writtenObject.mirror.empty())
		{
			if (writtenObject.writtenHeading) { throw std::runtime_error(where + ": <mirror> flips a picture that does not turn; it has a <heading>"); }
			object.mirror.on = true;
			object.mirror.way = writtenObject.mirror == "right" ? Direction::Right : Direction::Left;
			object.mirror.goingLeft = object.facing == Direction::Left;
			object.looks.set->flips = flipsOf(object);
			object.faceMirror();
		}

		if (writtenObject.writtenGrip)
		{
			object.motion.grip = lossRate(evaluate(*writtenObject.writtenGrip, where + " > <grip>"), where + " > <grip>");
		}
		for (const Timer& timer : object.kind->timers)
		{
			if (!timer.sprite.empty() && std::none_of(looks.begin(), looks.end(), [&](const Object::Look& look) { return look.name == timer.sprite; }))
			{
				std::vector<std::string> names;
				for (const Object::Look& look : looks) { names.push_back(look.name); }
				throw std::runtime_error(where + " > <timer sprite=\"" + timer.sprite + "\">: the object has no look of that name"
					+ (names.empty() ? std::string(" (it has no looks: several named <sprite>s and no <animation>)") : didYouMean(timer.sprite, names) + ". Its looks are: " + listOf(names)));
			}
		}

		// On a map: which tile, and the map itself, for what walks it.
		if (writtenObject.cell.mapColumn > 0)
		{
			// A piece of a cell map, a chain or a shape has a cell part.
			Object::Cells* cell = writtenObject.cell.cells || !writtenObject.chain.empty() || !writtenObject.shapeRows.empty()
				? &parts->cells.add(object.id, parts->orderOf(object.id), {}) : nullptr;
			Object::MapPlace& mapPlace = parts->mapPlaces.add(object.id, parts->orderOf(object.id), {});
			mapPlace.name = Name::of(writtenObject.cell.map);
			mapPlace.home = mapPlace.name;
			if (cell) { cell->chain = writtenObject.chain; }
			mapPlace.column = writtenObject.cell.mapColumn - 1;
			mapPlace.row = writtenObject.cell.mapRow - 1;
			mapPlace.columns = writtenObject.cell.mapColumns;
			mapPlace.rows = writtenObject.cell.mapRows;
			mapPlace.tile = { evaluate(writtenObject.cell.tileWidth, where), evaluate(writtenObject.cell.tileHeight, where) };
			mapPlace.stack = writtenObject.cell.stack;
			mapPlace.stackAcross = writtenObject.cell.stackAcross;
			mapPlace.origin = startFor(object).position - object.layout.cellOffset;

			// On a cell map: the cell it is in, its map's checks, and a
			// shape's cells, each the object its character picks.
			object.onCells = writtenObject.cell.cells;
			if (object.onCells)
			{
				cell->place.column = cell->start.column = static_cast<float>(writtenObject.cell.mapColumn);
				cell->place.row = cell->start.row = static_cast<float>(writtenObject.cell.mapRow);
				for (const WrittenCheck& check : writtenObject.cell.checks)
				{
					cell->checks.push_back({ check.kind, check.objClass, processCommands(check.commands, "map '" + writtenObject.cell.map + "' > <checks> > <" + check.kind + ">"), check.count, check.straight, check.alike });
				}
			}
			for (std::size_t row = 0; row < writtenObject.shapeRows.size(); ++row)
			{
				const std::string& cells = writtenObject.shapeRows[row];
				cell->shapeSize = std::max({ cell->shapeSize, static_cast<int>(cells.size()), static_cast<int>(writtenObject.shapeRows.size()) });
				for (std::size_t column = 0; column < cells.size(); ++column)
				{
					const auto piece = writtenObject.shapePieces.find(cells[column]);
					if (piece == writtenObject.shapePieces.end()) { continue; }
					cell->shape.push_back({ static_cast<int>(column), static_cast<int>(row), piece->second });
				}
			}
			if (cell && cell->shapeSize > 0 && cell->shape.empty())
			{
				throw std::runtime_error(where + " > <shape>: none of its characters picks an object of map '" + writtenObject.cell.map + "'");
			}
			if (cell) { cell->shapeStart = cell->shape; }
		}
		if (!writtenObject.maze.map.empty())
		{
			Maze& maze = parts->mazes.add(object.id, parts->orderOf(object.id), {});
			maze.map = writtenObject.maze.map;
			maze.walls = writtenObject.maze.walls;
			maze.slows = writtenObject.maze.slows;
		}
		for (const MazeWall& wall : writtenObject.maze.walls)
		{
			if (!wall.sprite.empty() && std::none_of(looks.begin(), looks.end(), [&](const Object::Look& look) { return look.name == wall.sprite; }))
			{
				throw std::runtime_error(where + " > <maze> > <wall class=\"" + wall.objClass + "\" sprite=\"" + wall.sprite + "\">: the object has no look of that name");
			}
		}

		object.showHeading();

		// <pivot>: the point it turns about, from the middle of the picture as
		// drawn; its <position> is where that point is on the screen, so the
		// square of the turned picture goes where puts it there.
		if (writtenObject.writtenPivot)
		{
			if (object.drawn.turnables.empty() || !object.drawn.bitmap)
			{
				throw std::runtime_error(where + ": <pivot> is the point a <heading> turns a picture of <line>s or a <bitmap> about; it has no such picture that turns");
			}
			const Turnable& drawn = *object.drawn.turnables.front();
			Vector2f middle;
			if (!drawn.lines.empty())
			{
				float left = drawn.lines.front().x1, right = left, top = drawn.lines.front().y1, bottom = top;
				for (const LineSegment& line : drawn.lines)
				{
					for (const float x : { line.x1, line.x2 }) { left = std::min(left, x); right = std::max(right, x); }
					for (const float y : { line.y1, line.y2 }) { top = std::min(top, y); bottom = std::max(bottom, y); }
				}
				middle = { (left + right) / 2.0f, (top + bottom) / 2.0f };
			}
			else
			{
				middle = { static_cast<float>(drawn.picture->width) / 2.0f, static_cast<float>(drawn.picture->height) / 2.0f };
			}
			const Vector2f pivot{ evaluate(writtenObject.writtenPivot->x, where + " > <pivot>"), evaluate(writtenObject.writtenPivot->y, where + " > <pivot>") };
			object.turning.hasPivot = true;
			object.turning.pivotFromMiddle = pivot - middle;
			object.pivotDegrees = wholeDegrees(object.heading);
			const Vector2f square{ static_cast<float>(object.drawn.bitmap->width), static_cast<float>(object.drawn.bitmap->height) };
			object.position = object.position - square * 0.5f - turnedBy(object.turning.pivotFromMiddle, static_cast<float>(object.pivotDegrees));
			startFor(object).position = object.position;
			startFor(object).pivotDegrees = object.pivotDegrees;
		}
	}

	// makeObject's collisions: on or off, the kind, solid pixels, hitboxes
	// (its own and each frame's), its edge rules and its rules with others.
	void Evaluator::buildCollisions(WrittenObject& writtenObject, Object& object, const std::vector<Object::Look>& looks, int thisLockstep, const std::string& where)
	{
		object.collisionsOn = writtenObject.writtenCollisionData.enabled;
		startFor(object).isVisible = object.isVisible;
		startFor(object).collisionsOn = object.collisionsOn;
		object.kind->collisionData.lockstep = thisLockstep;
		object.kind->collisionData.type = writtenObject.writtenCollisionData.type;

		// A pixel collision tests the pixels that are drawn; a line
		// drawing has them from the start, and a circle or rectangle
		// is solid all over, but what a text or an image looks like
		// is only known once a window backend has drawn it.
		if (object.kind->collisionData.type == CollisionType::Pixel
			&& (object.drawn.picture->kind == ShapeKind::Text || object.drawn.picture->kind == ShapeKind::Image))
		{
			throw std::runtime_error(where + ": <type>pixel</type> needs a sprite of lines, a circle or a rectangle; the pixels of text and images are only known to a window backend");
		}

		// Which of its pixels touch: only for a picture of rows, tested
		// pixel by pixel.
		if (!writtenObject.writtenCollisionData.solid.empty())
		{
			if (object.kind->collisionData.type != CollisionType::Pixel)
			{
				throw std::runtime_error(where + ": a <solid char> says which pixels touch, so it needs <type>pixel</type>");
			}
			const auto rows = [](const WrittenSprite& sprite) { return sprite.kind == "bitmap"; };
			if (!rows(writtenObject.sprite) || !std::all_of(writtenObject.looks.begin(), writtenObject.looks.end(), rows)
				|| !std::all_of(writtenObject.animation.frames.begin(), writtenObject.animation.frames.end(), rows))
			{
				throw std::runtime_error(where + ": a <solid char> names characters of a <bitmap>, and every sprite of the object must be one");
			}
		}

		// What it touches with, if not its picture.
		object.kind->collisionData.hitboxes.clear();
		for (const WrittenHitbox& written : writtenObject.writtenCollisionData.hitboxes)
		{
			object.kind->collisionData.hitboxes.push_back(makeHitbox(written, object, where + " > <hitbox>"));
		}
		// And each animation frame's own.
		const auto frameHitboxes = [&](const std::vector<std::vector<WrittenHitbox>>& written, const std::string& here)
		{
			std::vector<std::vector<Hitbox>> made;
			for (const std::vector<WrittenHitbox>& frame : written)
			{
				made.emplace_back();
				for (const WrittenHitbox& hitbox : frame) { made.back().push_back(makeHitbox(hitbox, object, here + " > <frame> > <hitbox>")); }
			}
			return made;
		};
		if (auto own = frameHitboxes(writtenObject.animation.frameHitboxes, where + " > <animation>"); !own.empty()) { object.animation.frames.edit().hitboxes = std::move(own); }
		for (std::size_t i = 0; i < object.looks.all().size() && i < writtenObject.looks.size(); ++i)
		{
			auto made = frameHitboxes(writtenObject.looks[i].frameHitboxes, where + " > <animation name=\"" + writtenObject.looks[i].name + "\">");
			if (!made.empty()) { object.looks.set->all[i].frames.edit().hitboxes = std::move(made); }
		}
		if (object.looks.shown < object.looks.all().size() && !object.looks.all()[object.looks.shown].frames.shows().bitmaps.empty()) { object.animation.frames = object.looks.all()[object.looks.shown].frames; }

		for (std::size_t edge = 0; edge < object.kind->collisionData.edges.size(); ++edge)
		{
			const EdgeRules<WrittenCommand>& written = writtenObject.writtenCollisionData.edges[edge];
			object.kind->collisionData.edges[edge] = { processCommands(written.commands, where), written.guards };
			for (const EdgeGuard& edgeGuard : written.guards)
			{
				if (!edgeGuard.sprite.empty() && std::none_of(looks.begin(), looks.end(), [&](const Object::Look& look) { return look.name == edgeGuard.sprite; }))
				{
					throw std::runtime_error(where + ": a <collision edge sprite=\"" + edgeGuard.sprite + "\"> names no look of the object (a look is one of several named <sprite>s)");
				}
			}
		}

		for (auto& writtenRule : writtenObject.writtenCollisionData.others)
		{
			CollisionRule rule;
			rule.line = writtenRule.line;
			rule.filterClass = writtenRule.filterClass;
			rule.filterObject = writtenRule.filterObject;
			rule.unlessClass = writtenRule.unlessClass;
			rule.throughClass = writtenRule.throughClass;
			rule.whileSprite = writtenRule.whileSprite;
			rule.hitbox = writtenRule.hitbox;
			rule.by = writtenRule.by;
			if (!rule.whileSprite.empty() && std::none_of(looks.begin(), looks.end(), [&](const Object::Look& look) { return look.name == rule.whileSprite; }))
			{
				throw std::runtime_error(where + ": a <collision sprite=\"" + rule.whileSprite + "\"> names no look of the object (a look is one of several named <sprite>s)");
			}
			const std::string& side = writtenRule.side;
			if (side == "top" || side == "vertical") { rule.sides.push_back(Edge::Top); }
			if (side == "bottom" || side == "vertical") { rule.sides.push_back(Edge::Bottom); }
			if (side == "left" || side == "horizontal") { rule.sides.push_back(Edge::Left); }
			if (side == "right" || side == "horizontal") { rule.sides.push_back(Edge::Right); }
			// One naming other is made, so checked, for each thing it can
			// touch, and kept as made for the first (other is found as it runs).
			for (std::size_t i = 1; loading && i < writtenRule.touchable.size(); ++i)
			{
				const std::string was = std::exchange(otherName, writtenRule.touchable[i]);
				rule.loadChecks.push_back(processCommands(writtenRule.commands, where + ", touching '" + otherName + "'"));
				otherName = was;
			}
			const std::string was = std::exchange(otherName, writtenRule.touchable.empty() ? std::string() : writtenRule.touchable.front());
			rule.commands = processCommands(writtenRule.commands, where);
			otherName = was;
			object.kind->collisionData.others.push_back(std::move(rule));
		}
	}

	// makeObject's actions.
	void Evaluator::buildActions(WrittenObject& writtenObject, Object& object, const std::string& where)
	{
		for (auto& writtenAction : writtenObject.action)
		{
			object.kind->action[writtenAction.first] = processCommands(writtenAction.second, where);
		}
	}
}
