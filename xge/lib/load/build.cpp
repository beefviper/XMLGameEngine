// build.cpp
// XML Game Engine
// author: beefviper
// date: Nov 15, 2026
//
// One object made from what the file says of it (game_expr::makeObject),
// in named steps: its looks and picture, names, numbers, ways of moving,
// timers, board-game moves, holder, place and collisions, then actions.

#include "load/expr.h"

#include "load/spelling.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>

namespace xge
{
	Object game_expr::makeObject(RawObject& rawObject, const FirstPass& first, const WindowDesc& windowDesc, Vector2f cellOffset, int thisLockstep)
	{
		const std::string where = "object '" + rawObject.name + "'";

		// Only a text showing a number needs working out again here, for the
		// value it has by now (see above).
		const Picture picture = (rawObject.sprite.kind == "text" && showsNumbers(rawObject.sprite))
			? buildPicture(rawObject.sprite, where, nullptr, nullptr, &rawObject)
			: first.picture;

		// A position that uses the size of a text (its own, or another
		// object's) is worked out again when that text's words change - see
		// Object::positionUsesSize.
		const std::vector<std::string> positionSizeDependencies = sizeDependenciesOf(rawObject);

		// How long each picture of an animation lasts.
		const int animationFrames = rawObject.hasAnimation ? animationFramesOf(rawObject.animation.interval, windowDesc, where + " > <animation>") : 0;

		// The object's looks, if it has several sprites and no animation.
		const std::vector<Object::Look> looks = buildLooks(rawObject, first, windowDesc, picture, where);

		Object object{};
		buildSprite(rawObject, first, windowDesc, picture, animationFrames, object, where);
		buildNames(rawObject, object);
		buildNumbers(rawObject, object, cellOffset, positionSizeDependencies, where);
		buildMotion(rawObject, object, where);
		buildTimers(rawObject, object, where);
		buildLookParts(rawObject, object, looks, where);
		buildMoves(rawObject, object, looks, where);
		buildHolder(rawObject, object, where);
		buildPlace(rawObject, object, looks, where);
		buildCollisions(rawObject, object, looks, thisLockstep, where);
		buildActions(rawObject, object, where);

		// How it started is how it was before its first frame.
		object.before = object.start;

		// object's visual is built later by whichever Window backend is
		// running, once one exists (see Window::init() in window/window.h) -
		// visualDirty starts true (Object's own default), so nothing
		// needs to happen here.
		return object;
	}

	// makeObject's looks: each named <sprite> or <animation> made, the first the
	// very picture it starts with, and where each once look goes when it ends.
	std::vector<Object::Look> game_expr::buildLooks(RawObject& rawObject, const FirstPass& first, const WindowDesc& windowDesc, const Picture& picture, const std::string& where)
	{
		std::vector<Object::Look> looks;
		if (first.looks)
		{
			looks = *first.looks;
		}
		else if (!rawObject.looks.empty())
		{
			for (const RawSprite& rawLook : rawObject.looks)
			{
				Object::Look look;
				look.name = rawLook.name;
				std::shared_ptr<const Turnable> turned;
				look.picture = buildPicture(rawLook, where + " > <sprite name=\"" + rawLook.name + "\">", &look.bitmap,
					rawObject.hasHeading ? &turned : nullptr);
				if (turned) { look.turnables = { turned }; }
				if (!rawLook.frames.empty())
				{
					// A look that moves: its pictures, all one size, and how long each shows.
					const std::string here = where + " > <animation name=\"" + rawLook.name + "\">";
					look.turnables.clear();
					look.frames = buildAnimationBitmaps(rawLook.frames, here, rawObject.hasHeading ? &look.turnables : nullptr);
					look.frameTicks = animationFramesOf(rawLook.interval, windowDesc, here);
					look.frameTimes = animationTimesOf(rawLook.times, look.frameTicks, windowDesc, here);
					look.once = rawLook.once;
					look.then = processCommands(rawLook.thenCommands, here + " > <then>");
				}
				if (look.picture.kind == ShapeKind::Text && showsNumbers(rawLook))
				{
					throw std::runtime_error(where + ": a look cannot be a text showing a <number> or a <letter>");
				}
				looks.push_back(std::move(look));
			}

			// The first look is the sprite the object starts with; it must be
			// the very same picture.
			looks.front().picture = picture;
			looks.front().bitmap = first.bitmap;
			if (!looks.front().frames.empty()) { looks.front().frames.front() = first.bitmap; }
			if (first.turned && looks.front().frames.empty()) { looks.front().turnables = { first.turned }; }

			// The look each once look goes to when it ends, by its name.
			for (std::size_t i = 0; i < rawObject.looks.size(); ++i)
			{
				const std::string& then = rawObject.looks[i].thenSprite;
				if (then.empty()) { continue; }
				const auto found = std::find_if(looks.begin(), looks.end(), [&](const Object::Look& each) { return each.name == then; });
				if (found == looks.end())
				{
					std::vector<std::string> names;
					for (const Object::Look& each : looks) { names.push_back(each.name); }
					throw std::runtime_error(where + " > <animation name=\"" + rawObject.looks[i].name + "\"> > <then sprite=\"" + then + "\">: the object has no look of that name"
						+ didYouMean(then, names) + ". Its looks are: " + listOf(names));
				}
				looks[i].thenLook = static_cast<std::size_t>(found - looks.begin());
			}
		}
		return looks;
	}

	// makeObject's picture: the sprite it starts as, its animation, and a
	// text's live pieces.
	void game_expr::buildSprite(RawObject& rawObject, const FirstPass& first, const WindowDesc& windowDesc, const Picture& picture, int animationFrames, Object& object, const std::string& where)
	{
		object.drawn.picture = picture;
		object.drawn.bitmap = first.bitmap;
		if (first.turned) { object.drawn.turnables = { first.turned }; }

		if (rawObject.hasAnimation)
		{
			object.animation.bitmaps = first.animation;
			if (!first.animationTurned.empty()) { object.drawn.turnables = first.animationTurned; }
			object.animation.frames = animationFrames;
			object.animation.times = animationTimesOf(rawObject.animation.times, animationFrames, windowDesc, where + " > <animation>");
			object.animation.once = rawObject.animation.once;
			object.animation.then = processCommands(rawObject.animation.thenCommands, where + " > <animation> > <then>");
			object.drawn.bitmap = object.animation.bitmaps.front();
		}

		if (object.drawn.picture.kind == ShapeKind::Text && showsNumbers(rawObject.sprite))
		{
			std::vector<TextPiece> pieces = textPieces(rawObject.sprite, where, &rawObject);
			if (std::any_of(pieces.begin(), pieces.end(), [](const TextPiece& piece) { return !piece.owner.empty(); }))
			{
				object.drawn.textPieces = std::move(pieces);
			}
		}
	}

	// makeObject's names: its group, class and owners, and whether it is
	// drawn, seen and there.
	void game_expr::buildNames(RawObject& rawObject, Object& object)
	{
		object.groupName = rawObject.groupName;
		object.outerGroups = rawObject.outerGroups;
		object.name = rawObject.name;
		object.line = rawObject.line;
		object.objClass = rawObject.objClass;
		object.isDrawn = rawObject.isDrawn;
		object.ownerState = rawObject.ownerState;
		object.ownerScene = rawObject.ownerScene;
		object.start.ownerScene = rawObject.ownerScene;

		if (rawObject.objClass == "projectile")
		{
			rawObject.isVisible = false;
		}

		object.isVisible = rawObject.isVisible;
		object.exists = rawObject.exists;
		object.start.exists = rawObject.exists;
		// One that does not exist is in nothing that looks for what is out
		// there, as one taken out of play is not.
		if (!object.exists) { object.isVisible = false; }
		object.most = rawObject.most;
	}

	// makeObject's numbers: its variables, then where it starts, what it
	// follows, its velocity, acceleration and heading.
	void game_expr::buildNumbers(RawObject& rawObject, Object& object, Vector2f cellOffset, const std::vector<std::string>& positionSizeDependencies, const std::string& where)
	{
		// Its variables first, so its position and velocity can use them
		// (a serve drawn as an angle: its speed times the cosine of its angle).
		for (auto& rawVariable : rawObject.variable)
		{
			const float value = evaluateStart(rawVariable.second, where);
			object.variable[rawVariable.first] = value;
			object.start.variable[rawVariable.first] = value;

			// Keep the cross-object symbol table entry (registered above,
			// before any expression compiled) up to date with the real
			// value now that it's known.
			objectVariables[rawObject.name + "." + rawVariable.first] = value;
		}

		object.startDrawsRandom = rawObject.rawPosition.x.drawsRandom() || rawObject.rawPosition.y.drawsRandom()
			|| rawObject.rawVelocity.x.drawsRandom() || rawObject.rawVelocity.y.drawsRandom()
			|| std::any_of(rawObject.variable.begin(), rawObject.variable.end(), [](const auto& variable) { return variable.second.drawsRandom(); });

		object.start.position.x = evaluateStart(rawObject.rawPosition.x, where);
		object.start.position.y = evaluateStart(rawObject.rawPosition.y, where);

		// A cell is its group's position and its place in the
		// group; start.position keeps both, so a reset puts each
		// cell (each brick) back in its own place.
		object.cellOffset = cellOffset;
		object.position = object.start.position + object.cellOffset;
		object.start.position = object.position;

		object.sizeDependencies = positionSizeDependencies;
		object.positionUsesSize = !positionSizeDependencies.empty();

		// What it follows, worked out again every frame (Game::follow).
		using Follow = Object::Follow;
		if (rawObject.rawPosition.x.live) { object.follows.push_back({ Follow::Part::X, "", rawObject.rawPosition.x }); }
		if (rawObject.rawPosition.y.live) { object.follows.push_back({ Follow::Part::Y, "", rawObject.rawPosition.y }); }
		if (rawObject.hasHeading && rawObject.rawHeading.live) { object.follows.push_back({ Follow::Part::Heading, "", rawObject.rawHeading }); }
		for (const auto& [name, value] : rawObject.variable)
		{
			if (value.live) { object.follows.push_back({ Follow::Part::Variable, name, value }); }
		}

		object.velocity.x = evaluate(rawObject.rawVelocity.x, where);
		object.velocity.y = evaluate(rawObject.rawVelocity.y, where);

		object.start.velocity = object.velocity;

		if (rawObject.hasAcceleration)
		{
			object.acceleration.x = evaluate(rawObject.rawAcceleration.x, where);
			object.acceleration.y = evaluate(rawObject.rawAcceleration.y, where);
		}
		object.start.acceleration = object.acceleration;

		if (rawObject.hasHeading)
		{
			object.hasHeading = true;
			float degrees = std::fmod(evaluateStart(rawObject.rawHeading, where), 360.0f);
			if (degrees < 0.0f) { degrees += 360.0f; }
			object.heading = degrees;
		}
		object.start.heading = object.heading;
	}

	// makeObject's ways of moving: drag, surface, friction, mass, pulls,
	// seeking, light and dark, altitude, a tether and which way it faces.
	void game_expr::buildMotion(RawObject& rawObject, Object& object, const std::string& where)
	{
		if (rawObject.hasDrag)
		{
			object.motion.drag = evaluate(rawObject.rawDrag, where);
			if (object.motion.drag < 0.0f || object.motion.drag >= 1.0f)
			{
				throw std::runtime_error(where + ": <drag> is " + std::to_string(object.motion.drag) + "; expected 0 up to (not including) 1");
			}
		}

		if (rawObject.hasSurface)
		{
			object.surface.is = true;
			if (rawObject.hasSurfaceDrag)
			{
				object.surface.drag = evaluate(rawObject.rawSurfaceDrag, where + " > <surface> > <drag>");
				if (*object.surface.drag < 0.0f || *object.surface.drag >= 1.0f)
				{
					throw std::runtime_error(where + " > <surface> > <drag> is " + formatDisplayNumber(*object.surface.drag) + "; expected 0 up to (not including) 1");
				}
			}
			if (rawObject.hasSurfaceGrip)
			{
				object.surface.grip = evaluate(rawObject.rawSurfaceGrip, where + " > <surface> > <grip>");
				if (!(*object.surface.grip > 0.0f && *object.surface.grip <= 1.0f))
				{
					throw std::runtime_error(where + " > <surface> > <grip> is " + formatDisplayNumber(*object.surface.grip) + "; expected above 0, up to 1");
				}
			}
		}

		if (rawObject.hasFriction)
		{
			object.motion.friction = evaluate(rawObject.rawFriction, where);
			if (object.motion.friction < 0.0f)
			{
				throw std::runtime_error(where + ": <friction> is " + std::to_string(object.motion.friction) + "; expected 0 or more");
			}
		}

		if (rawObject.hasMass)
		{
			object.motion.mass = evaluate(rawObject.rawMass, where);
			if (!(object.motion.mass > 0.0f))
			{
				throw std::runtime_error(where + ": <mass> is " + formatDisplayNumber(object.motion.mass) + "; expected more than 0");
			}
		}

		for (const auto& [name, strength] : rawObject.rawPulls)
		{
			object.pulls.emplace_back(name, evaluate(strength, where + " > <pull object=\"" + name + "\">"));
		}
		if (!rawObject.seekTarget.empty())
		{
			object.seekTarget = rawObject.seekTarget;
			object.seekRate = evaluate(rawObject.rawSeek, where + " > <seek>");
			if (object.seekRate < 0.0f) { throw std::runtime_error(where + ": <seek> turns " + formatDisplayNumber(object.seekRate) + " degrees a frame; expected 0 or more"); }
		}

		if (rawObject.hasLight)
		{
			object.light = evaluate(rawObject.rawLight, where + " > <light>");
			if (object.light < 0.0f) { throw std::runtime_error(where + ": <light> reaches " + formatDisplayNumber(object.light) + " pixels; expected 0 or more"); }
			object.start.light = object.light;
			object.lightPast = rawObject.lightPast;
		}
		if (rawObject.dark == "unlit") { object.dark.kind = Object::Darkness::Unlit; }
		else if (rawObject.dark == "unseen") { object.dark.kind = Object::Darkness::Unseen; }
		object.dark.hidden = object.dark.kind != Object::Darkness::None;

		if (rawObject.hasAltitude)
		{
			const std::string here = where + " > <altitude>";
			const auto given = [&](const RawValue& value) { return !(value.kind == RawValue::Kind::Expression && value.text.empty()); };
			object.aloft.has = true;
			object.altitude = given(rawObject.rawAltitude) ? evaluate(rawObject.rawAltitude, here) : 0.0f;
			object.rise = given(rawObject.rawRise) ? evaluate(rawObject.rawRise, here) : 0.0f;
			object.aloft.fall = given(rawObject.rawFall) ? evaluate(rawObject.rawFall, here) : 0.0f;
			object.aloft.bounce = given(rawObject.rawAltitudeBounce) ? evaluate(rawObject.rawAltitudeBounce, here) : 0.0f;
			if (object.altitude < 0.0f) { throw std::runtime_error(here + ": <start> is " + formatDisplayNumber(object.altitude) + "; it cannot start below the ground"); }
			if (object.aloft.fall < 0.0f) { throw std::runtime_error(here + ": <fall> is " + formatDisplayNumber(object.aloft.fall) + "; expected 0 or more"); }
			if (!(object.aloft.bounce >= 0.0f && object.aloft.bounce <= 1.0f)) { throw std::runtime_error(here + ": <bounce> is " + formatDisplayNumber(object.aloft.bounce) + "; expected 0 up to 1"); }
			object.start.altitude = object.altitude;
			object.start.rise = object.rise;
		}
		object.aloft.shadowOf = rawObject.shadowOf;

		if (rawObject.sprite.kind == "tether")
		{
			if (rawObject.hasHeading || !rawObject.looks.empty() || rawObject.hasAnimation)
			{
				throw std::runtime_error(where + ": a <tether> is drawn where its ends are; it has no <heading>, other looks or <animation>");
			}
			object.tether.from = rawObject.sprite.tetherFrom;
			object.tether.to = rawObject.sprite.tetherTo;
			if (!rawObject.sprite.thickness.text.empty() || rawObject.sprite.thickness.kind != RawValue::Kind::Expression)
			{
				object.tether.thickness = static_cast<int>(std::lround(evaluate(rawObject.sprite.thickness, where)));
				if (object.tether.thickness < 1) { throw std::runtime_error(where + ": a <tether> has a <thickness> under 1"); }
			}
			object.tether.color = colorFromName(rawObject.sprite.color.empty() ? "color.white" : rawObject.sprite.color);
		}

		if (!rawObject.facing.empty())
		{
			object.hasFacing = true;
			object.facing = rawObject.facing == "down" ? Direction::Down
				: rawObject.facing == "left" ? Direction::Left
				: rawObject.facing == "right" ? Direction::Right
				: Direction::Up;
		}
		object.start.facing = object.facing;
	}

	// makeObject's timers, a named one's time left a number of its own.
	void game_expr::buildTimers(RawObject& rawObject, Object& object, const std::string& where)
	{
		object.timers = processTimers(rawObject.timers, where);
		for (const Timer& timer : object.timers)
		{
			if (timer.name.empty()) { continue; }
			// A named timer's time left (see init): where it starts.
			const float left = std::ceil(timer.seconds);
			object.variable[timer.name] = left;
			object.start.variable[timer.name] = left;
			objectVariables[rawObject.name + "." + timer.name] = left;
		}
	}

	// makeObject's looks given it, the first one's animation, the look for
	// each way it moves (<poses>) and for where what it is joined to lies.
	void game_expr::buildLookParts(RawObject& rawObject, Object& object, const std::vector<Object::Look>& looks, const std::string& where)
	{
		object.looks.all = looks;
		if (!looks.empty() && !looks.front().frames.empty())
		{
			// It starts as a look that moves.
			object.animation.bitmaps = looks.front().frames;
			object.animation.frames = looks.front().frameTicks;
			object.animation.times = looks.front().frameTimes;
			object.animation.once = looks.front().once;
		}

		// The look for each way it moves; one left out is another's.
		if (!rawObject.poses.empty())
		{
			const auto lookOf = [&](const std::string& pose) -> std::optional<std::size_t>
			{
				const auto named = rawObject.poses.find(pose);
				if (named == rawObject.poses.end()) { return std::nullopt; }
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
			object.looks.poses[static_cast<std::size_t>(Object::Pose::Standing)] = standing;
			object.looks.poses[static_cast<std::size_t>(Object::Pose::Walking)] = walking;
			object.looks.poses[static_cast<std::size_t>(Object::Pose::Skidding)] = lookOf("skidding").value_or(walking);
			object.looks.poses[static_cast<std::size_t>(Object::Pose::Falling)] = falling;
			object.looks.poses[static_cast<std::size_t>(Object::Pose::Rising)] = lookOf("rising").value_or(falling);
			object.looks.poses[static_cast<std::size_t>(Object::Pose::Thrusting)] = lookOf("thrusting").value_or(standing);
			object.looks.hasPoses = true;
			object.looks.first = standing;
			if (standing != 0) { object.showLook(standing); }
		}

		// The look for where the piece it is joined to lies.
		if (!rawObject.joins.empty())
		{
			std::size_t at = 0;
			for (const char* join : { "alone", "left", "right", "up", "down" })
			{
				const std::string& name = rawObject.joins.at(join);
				const auto look = std::find_if(looks.begin(), looks.end(), [&](const Object::Look& each) { return each.name == name; });
				if (look == looks.end())
				{
					std::vector<std::string> names;
					for (const Object::Look& each : looks) { names.push_back(each.name); }
					throw std::runtime_error(where + " > <joins> > <" + join + " sprite=\"" + name + "\">: the object has no look of that name"
						+ (names.empty() ? std::string(" (it has no looks)") : didYouMean(name, names) + ". Its looks are: " + listOf(names)));
				}
				object.looks.joins[at++] = static_cast<std::size_t>(look - looks.begin());
			}
			object.looks.hasJoins = true;
		}
	}

	// makeObject's board-game moves and marks, each set's look found.
	void game_expr::buildMoves(RawObject& rawObject, Object& object, const std::vector<Object::Look>& looks, const std::string& where)
	{
		// A board game piece's moves, each set's look found.
		for (const RawObject::RawMoveSet& rawSet : rawObject.moveSets)
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
			if (!rawSet.sprite.empty()) { set.look = lookNamed(rawSet.sprite, "<moves"); }
			set.forced = rawSet.forced;
			for (const RawObject::RawPieceMove& move : rawSet.moves) { set.moves.push_back({ move.jump, move.objClass, move.x, move.y }); }
			if (!rawSet.promote.empty()) { set.promoteLook = lookNamed(rawSet.promote, "<moves> > <promote"); }
			set.promoteRow = rawSet.promoteRow;
			for (const RawObject::RawMoveSet::RawFlank& flank : rawSet.flanks) { set.flanks.push_back({ flank.objClass, flank.straight }); }
			set.fall = rawSet.fall;
			set.on = rawSet.on;
			set.closeClass = rawSet.closeClass;
			set.closeObject = rawSet.closeObject;
			object.moves.sets.push_back(std::move(set));
		}
		for (const RawObject::RawMarkSet& rawSet : rawObject.markSets)
		{
			Object::MarkSet set;
			if (!rawSet.sprite.empty())
			{
				const auto look = std::find_if(looks.begin(), looks.end(), [&](const Object::Look& each) { return each.name == rawSet.sprite; });
				if (look == looks.end())
				{
					std::vector<std::string> names;
					for (const Object::Look& each : looks) { names.push_back(each.name); }
					throw std::runtime_error(where + " > <marks sprite=\"" + rawSet.sprite + "\">: the object has no look of that name"
						+ (names.empty() ? std::string(" (it has no looks)") : didYouMean(rawSet.sprite, names) + ". Its looks are: " + listOf(names)));
				}
				set.look = static_cast<std::size_t>(look - looks.begin());
			}
			for (const RawObject::RawMark& raw : rawSet.marks)
			{
				Object::Mark mark;
				mark.kind = raw.kind == "held" ? Object::Mark::Kind::Held : raw.kind == "moves" ? Object::Mark::Kind::Moves
					: raw.kind == "movable" ? Object::Mark::Kind::Movable : raw.kind == "put" ? Object::Mark::Kind::Put : Object::Mark::Kind::Last;
				mark.object = raw.object;
				mark.objClass = raw.objClass;
				mark.variable = raw.variable;
				set.marks.push_back(std::move(mark));
			}
			object.marks.sets.push_back(std::move(set));
		}
	}

	// makeObject's holder (<holds>), hand, held item and turns.
	void game_expr::buildHolder(RawObject& rawObject, Object& object, const std::string& where)
	{
		// Holders: a holder's rules, a hand, a held member.
		if (rawObject.holder.is)
		{
			const RawObject::RawHolder& raw = rawObject.holder;
			const auto number = [&](const RawValue& value) { return value.kind == RawValue::Kind::Expression && value.text.empty() ? 0.0f : evaluate(value, where); };
			Object::Holder& holder = object.holder;
			holder.is = true;
			holder.holdsClass = raw.holdsClass;
			holder.holdsSprite = raw.holdsSprite;
			holder.topSprite = raw.topSprite;
			holder.spread = { number(raw.spreadX), number(raw.spreadY) };
			holder.lookSpread = { number(raw.lookSpreadX), number(raw.lookSpreadY) };
			holder.spreadSprite = raw.spreadSprite;
			holder.startCount = static_cast<int>(std::lround(number(raw.startCount)));
			holder.startMore = static_cast<int>(std::lround(number(raw.startMore)));
			holder.pickedInto = raw.pickedInto;
			holder.pickedSends = !raw.hasPickedSend ? 0 : std::max(1, static_cast<int>(std::lround(number(raw.pickedSends))));
			holder.emptyFrom = raw.emptyFrom;
			holder.emptyRandom = raw.emptyRandom;
			holder.pickedSound = raw.pickedSound;
			holder.emptyLeave = std::max(0, static_cast<int>(std::lround(number(raw.emptyLeave))));
			holder.spreadWidth = number(raw.spreadWidth);
			holder.carry = raw.carry;
			holder.orderBy = raw.orderBy;
			holder.counts = raw.counts;
			holder.lift = number(raw.lift);
			holder.pointsMost = raw.hasPoints ? number(raw.pointsMost) : 0.0f;
			holder.score = raw.score;
			holder.scoreWith = raw.scoreWith;
			holder.scoreFull = raw.scoreFull;
			holder.scoreBest = raw.scoreBest;
			holder.melds = raw.melds;
			holder.meldsBest = raw.meldsBest;
			holder.takes = raw.takes;
		}
		object.hand.is = rawObject.hand;
		object.hand.own = rawObject.handHolder;
		object.hand.seat = rawObject.handSeat;
		object.hand.team = rawObject.handTeam;
		object.hand.holds = rawObject.handHolds;
		object.item.is = rawObject.held;
		object.item.startHolder = rawObject.item.startHolder;
		object.item.worths = rawObject.item.worths;
		object.takesTurns = rawObject.takesTurns;
		object.turnsSprite = rawObject.turnsSprite;
		object.ownsClass = rawObject.ownsClass;
	}

	// makeObject's mirror, grip, timers' looks, its tile on a map, a
	// maze's walls and its pivot.
	void game_expr::buildPlace(RawObject& rawObject, Object& object, const std::vector<Object::Look>& looks, const std::string& where)
	{
		if (!rawObject.mirror.empty())
		{
			if (rawObject.hasHeading) { throw std::runtime_error(where + ": <mirror> flips a picture that does not turn; it has a <heading>"); }
			object.mirror.on = true;
			object.mirror.way = rawObject.mirror == "right" ? Direction::Right : Direction::Left;
			object.mirror.goingLeft = object.facing == Direction::Left;
			object.mirror.flips = flipsOf(object);
			object.faceMirror();
		}

		if (rawObject.hasGrip)
		{
			object.motion.grip = evaluate(rawObject.rawGrip, where + " > <grip>");
			if (!(object.motion.grip > 0.0f && object.motion.grip <= 1.0f))
			{
				throw std::runtime_error(where + " > <grip> is " + formatDisplayNumber(object.motion.grip) + "; expected above 0, up to 1 (1: it goes at once at the speed its moves ask for)");
			}
		}
		for (const Timer& timer : object.timers)
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
		if (rawObject.cell.mapColumn > 0)
		{
			object.map.name = rawObject.cell.map;
			object.map.home = rawObject.cell.map;
			object.cell.chain = rawObject.chain;
			object.map.column = rawObject.cell.mapColumn - 1;
			object.map.row = rawObject.cell.mapRow - 1;
			object.map.columns = rawObject.cell.mapColumns;
			object.map.rows = rawObject.cell.mapRows;
			object.map.tile = { evaluate(rawObject.cell.tileWidth, where), evaluate(rawObject.cell.tileHeight, where) };
			object.map.origin = object.start.position - object.cellOffset;

			// On a cell map: the cell it is in, its map's checks, and a
			// shape's cells, each the object its character picks.
			object.map.cells = rawObject.cell.cells;
			if (object.map.cells)
			{
				object.cellColumn = object.start.cellColumn = static_cast<float>(rawObject.cell.mapColumn);
				object.cellRow = object.start.cellRow = static_cast<float>(rawObject.cell.mapRow);
				for (const RawCheck& check : rawObject.cell.checks)
				{
					object.cell.checks.push_back({ check.kind, check.objClass, processCommands(check.commands, "map '" + rawObject.cell.map + "' > <checks> > <" + check.kind + ">"), check.count, check.straight, check.alike });
				}
			}
			for (std::size_t row = 0; row < rawObject.shapeRows.size(); ++row)
			{
				const std::string& cells = rawObject.shapeRows[row];
				object.cell.shapeSize = std::max({ object.cell.shapeSize, static_cast<int>(cells.size()), static_cast<int>(rawObject.shapeRows.size()) });
				for (std::size_t column = 0; column < cells.size(); ++column)
				{
					const auto piece = rawObject.shapePieces.find(cells[column]);
					if (piece == rawObject.shapePieces.end()) { continue; }
					object.cell.shape.push_back({ static_cast<int>(column), static_cast<int>(row), piece->second });
				}
			}
			if (object.isShape() && object.cell.shape.empty())
			{
				throw std::runtime_error(where + " > <shape>: none of its characters picks an object of map '" + rawObject.cell.map + "'");
			}
			object.cell.shapeStart = object.cell.shape;
		}
		object.maze.map = rawObject.maze.map;
		object.maze.walls = rawObject.maze.walls;
		object.maze.slows = rawObject.maze.slows;
		for (const MazeWall& wall : object.maze.walls)
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
		if (rawObject.hasPivot)
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
			const Vector2f pivot{ evaluate(rawObject.rawPivot.x, where + " > <pivot>"), evaluate(rawObject.rawPivot.y, where + " > <pivot>") };
			object.hasPivot = true;
			object.pivotFromMiddle = pivot - middle;
			object.pivotDegrees = wholeDegrees(object.heading);
			const Vector2f square{ static_cast<float>(object.drawn.bitmap->width), static_cast<float>(object.drawn.bitmap->height) };
			object.position = object.position - square * 0.5f - turnedBy(object.pivotFromMiddle, static_cast<float>(object.pivotDegrees));
			object.start.position = object.position;
			object.start.pivotDegrees = object.pivotDegrees;
		}
	}

	// makeObject's collisions: on or off, the kind, solid pixels, hitboxes
	// (its own and each frame's), its edge rules and its rules with others.
	void game_expr::buildCollisions(RawObject& rawObject, Object& object, const std::vector<Object::Look>& looks, int thisLockstep, const std::string& where)
	{
		object.collisionsOn = rawObject.rawCollisionData.enabled;
		object.start.isVisible = object.isVisible;
		object.start.collisionsOn = object.collisionsOn;
		object.collisionData.lockstep = thisLockstep;
		object.collisionData.type = rawObject.rawCollisionData.type;

		// A pixel collision tests the pixels that are drawn; a line
		// drawing has them from the start, and a circle or rectangle
		// is solid all over, but what a text or an image looks like
		// is only known once a window backend has drawn it.
		if (object.collisionData.type == CollisionType::Pixel
			&& (object.drawn.picture.kind == ShapeKind::Text || object.drawn.picture.kind == ShapeKind::Image))
		{
			throw std::runtime_error(where + ": <type>pixel</type> needs a sprite of lines, a circle or a rectangle; the pixels of text and images are only known to a window backend");
		}

		// Which of its pixels touch: only for a picture of rows, tested
		// pixel by pixel.
		if (!rawObject.rawCollisionData.solid.empty())
		{
			if (object.collisionData.type != CollisionType::Pixel)
			{
				throw std::runtime_error(where + ": a <solid char> says which pixels touch, so it needs <type>pixel</type>");
			}
			const auto rows = [](const RawSprite& sprite) { return sprite.kind == "bitmap"; };
			if (!rows(rawObject.sprite) || !std::all_of(rawObject.looks.begin(), rawObject.looks.end(), rows)
				|| !std::all_of(rawObject.animation.frames.begin(), rawObject.animation.frames.end(), rows))
			{
				throw std::runtime_error(where + ": a <solid char> names characters of a <bitmap>, and every sprite of the object must be one");
			}
		}

		// What it touches with, if not its picture.
		object.collisionData.hitboxes.clear();
		for (const RawHitbox& raw : rawObject.rawCollisionData.hitboxes)
		{
			object.collisionData.hitboxes.push_back(makeHitbox(raw, object, where + " > <hitbox>"));
		}
		// And each animation frame's own.
		const auto frameHitboxes = [&](const std::vector<std::vector<RawHitbox>>& written, const std::string& here)
		{
			std::vector<std::vector<Hitbox>> made;
			for (const std::vector<RawHitbox>& frame : written)
			{
				made.emplace_back();
				for (const RawHitbox& raw : frame) { made.back().push_back(makeHitbox(raw, object, here + " > <frame> > <hitbox>")); }
			}
			return made;
		};
		object.animation.hitboxes = frameHitboxes(rawObject.animation.frameHitboxes, where + " > <animation>");
		for (std::size_t i = 0; i < object.looks.all.size() && i < rawObject.looks.size(); ++i)
		{
			object.looks.all[i].frameHitboxes = frameHitboxes(rawObject.looks[i].frameHitboxes, where + " > <animation name=\"" + rawObject.looks[i].name + "\">");
		}
		if (object.looks.shown < object.looks.all.size() && !object.looks.all[object.looks.shown].frames.empty()) { object.animation.hitboxes = object.looks.all[object.looks.shown].frameHitboxes; }

		for (std::size_t edge = 0; edge < object.collisionData.edges.size(); ++edge)
		{
			const EdgeRules<RawCommand>& raw = rawObject.rawCollisionData.edges[edge];
			object.collisionData.edges[edge] = { processCommands(raw.commands, where), raw.guards };
			for (const EdgeGuard& edgeGuard : raw.guards)
			{
				if (!edgeGuard.sprite.empty() && std::none_of(looks.begin(), looks.end(), [&](const Object::Look& look) { return look.name == edgeGuard.sprite; }))
				{
					throw std::runtime_error(where + ": a <collision edge sprite=\"" + edgeGuard.sprite + "\"> names no look of the object (a look is one of several named <sprite>s)");
				}
			}
		}

		for (auto& rawRule : rawObject.rawCollisionData.others)
		{
			CollisionRule rule;
			rule.line = rawRule.line;
			rule.filterClass = rawRule.filterClass;
			rule.filterObject = rawRule.filterObject;
			rule.unlessClass = rawRule.unlessClass;
			rule.throughClass = rawRule.throughClass;
			rule.whileSprite = rawRule.whileSprite;
			rule.hitbox = rawRule.hitbox;
			rule.by = rawRule.by;
			if (!rule.whileSprite.empty() && std::none_of(looks.begin(), looks.end(), [&](const Object::Look& look) { return look.name == rule.whileSprite; }))
			{
				throw std::runtime_error(where + ": a <collision sprite=\"" + rule.whileSprite + "\"> names no look of the object (a look is one of several named <sprite>s)");
			}
			const std::string& side = rawRule.side;
			if (side == "top" || side == "vertical") { rule.sides.push_back(Edge::Top); }
			if (side == "bottom" || side == "vertical") { rule.sides.push_back(Edge::Bottom); }
			if (side == "left" || side == "horizontal") { rule.sides.push_back(Edge::Left); }
			if (side == "right" || side == "horizontal") { rule.sides.push_back(Edge::Right); }
			rule.commands = processCommands(rawRule.commands, where);
			if (!rawRule.otherIs.empty())
			{
				rule.otherIs = rawRule.otherIs;
				rule.written = std::make_shared<const std::vector<RawCommand>>(rawRule.commands);
				rule.where = where;
			}
			object.collisionData.others.push_back(std::move(rule));
		}
	}

	// makeObject's actions.
	void game_expr::buildActions(RawObject& rawObject, Object& object, const std::string& where)
	{
		for (auto& rawAction : rawObject.action)
		{
			object.action[rawAction.first] = processCommands(rawAction.second, where);
		}
	}
}
