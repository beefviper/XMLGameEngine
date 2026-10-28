// object.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "game/object.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <functional>
#include <memory>
#include <stdexcept>

namespace xge
{
	std::string showPiece(RawTextPiece::Kind kind, float value, const std::vector<std::string>& choices)
	{
		if (kind == RawTextPiece::Kind::Choice)
		{
			if (choices.empty()) { return {}; }
			const long last = static_cast<long>(choices.size()) - 1;
			return choices[static_cast<std::size_t>(std::clamp(std::lround(std::floor(value)), 0L, last))];
		}
		if (kind != RawTextPiece::Kind::Letter)
		{
			return formatDisplayNumber(value);
		}
		const long whole = std::lround(std::floor(value));
		const long letter = ((whole % 26) + 26) % 26;
		return std::string(1, static_cast<char>('A' + letter));
	}

	std::string joinText(const std::vector<TextPiece>& pieces)
	{
		std::string text;
		for (std::size_t i = 0; i < pieces.size(); ++i)
		{
			const bool word = i > 0 && !(pieces[i].kind == RawTextPiece::Kind::Letter && pieces[i - 1].kind == RawTextPiece::Kind::Letter);
			if (word) { text += ' '; }
			text += pieces[i].shown;
		}
		return text;
	}

	const std::string& Object::lookName() const
	{
		static const std::string none;
		return looks.empty() ? none : looks[look].name;
	}

	void Object::showLook(std::size_t index)
	{
		if (index >= looks.size())
		{
			return;
		}

		look = index;
		spriteParams = looks[index].spriteParams;
		shapeKind = looks[index].shapeKind;
		bitmap = facingPicture(looks[index].bitmap);
		animationBitmaps = looks[index].frames;
		animationFrames = looks[index].frameTicks;
		animationTick = 0;
		animationIndex = 0;
		visualDirty = true;
	}

	std::shared_ptr<const Bitmap> Object::facingPicture(const std::shared_ptr<const Bitmap>& picture) const
	{
		if (!mirrored || !flips || !picture) { return picture; }
		const auto found = flips->find(picture.get());
		return found == flips->end() ? picture : found->second;
	}

	void Object::faceMirror()
	{
		if (!mirrors) { return; }
		const bool want = goingLeft == (mirrorWay == Direction::Left);
		if (want == mirrored) { return; }

		// Flipping twice is no flip, so what shows now flips back the same way.
		mirrored = want;
		if (bitmap && flips)
		{
			const auto found = flips->find(bitmap.get());
			if (found != flips->end()) { bitmap = found->second; }
		}
		visualDirty = true;
	}

	std::vector<std::pair<std::size_t, std::size_t>> tilesOf(const RawMap& map, char character)
	{
		std::vector<std::pair<std::size_t, std::size_t>> tiles;
		for (std::size_t row = 0; row < map.rows.size(); ++row)
		{
			for (std::size_t column = 0; column < map.rows[row].size(); ++column)
			{
				if (map.rows[row][column] == character) { tiles.emplace_back(column, row); }
			}
		}
		return tiles;
	}

	void placeOnMap(RawObject& object, const RawMap& map, std::size_t column, std::size_t row)
	{
		object.rawPosition = map.position;
		object.cell.map = map.name;
		object.cell.mapColumn = static_cast<int>(column) + 1;
		object.cell.mapRow = static_cast<int>(row) + 1;
		object.cell.mapColumns = map.rows.empty() ? 0 : static_cast<int>(map.rows.front().size());
		object.cell.mapRows = static_cast<int>(map.rows.size());
		object.cell.tileWidth = map.tileWidth;
		object.cell.tileHeight = map.tileHeight;
		if (std::find(object.outerGroups.begin(), object.outerGroups.end(), map.name) == object.outerGroups.end())
		{
			object.outerGroups.insert(object.outerGroups.begin(), map.name);
		}
	}

	void applyStyles(std::vector<RawObject>& objects, const std::vector<RawStyle>& styles)
	{
		for (RawObject& object : objects)
		{
			const auto apply = [&](RawSprite& sprite)
			{
				if (sprite.kind == "bitmap")
				{
					if (sprite.style.empty()) { return; }
					const auto style = std::find_if(styles.begin(), styles.end(), [&](const RawStyle& each) { return each.name == sprite.style; });
					if (style == styles.end())
					{
						throw std::runtime_error("object '" + object.name + "': style=\"" + sprite.style + "\" names no <style> of the game's <styles>");
					}
					// Its own scale and colors first; the style's for the rest.
					if (!sprite.hasScale && style->hasScale)
					{
						sprite.hasScale = true;
						sprite.scale = style->scale;
					}
					if (sprite.color.empty()) { sprite.color = style->color; }
					for (const auto& [character, color] : style->bitmapColors)
					{
						const bool own = std::any_of(sprite.bitmapColors.begin(), sprite.bitmapColors.end(), [&](const auto& each) { return each.first == character; });
						if (!own) { sprite.bitmapColors.emplace_back(character, color); }
					}
					return;
				}
				if (sprite.kind != "text") { return; }
				const bool noSize = sprite.size.kind == RawValue::Kind::Expression && sprite.size.text.empty();
				if (!sprite.style.empty())
				{
					const auto style = std::find_if(styles.begin(), styles.end(), [&](const RawStyle& each) { return each.name == sprite.style; });
					if (style == styles.end())
					{
						throw std::runtime_error("object '" + object.name + "': style=\"" + sprite.style + "\" names no <style> of the game's <styles>");
					}
					if (noSize) { sprite.size = style->size; }
					if (sprite.color.empty()) { sprite.color = style->color; }
				}
				if (sprite.size.kind == RawValue::Kind::Expression && sprite.size.text.empty())
				{
					throw std::runtime_error("object '" + object.name + "': a <text> needs a <size>, or a style= whose <style> has one");
				}
			};
			apply(object.sprite);
			for (RawSprite& look : object.looks)
			{
				apply(look);
				for (RawSprite& frame : look.frames) { apply(frame); }
			}
			for (RawSprite& frame : object.animation.frames) { apply(frame); }
			apply(object.cell.slot);
		}
	}

	void markSolid(std::vector<RawObject>& objects)
	{
		for (RawObject& object : objects)
		{
			const std::string& solid = object.rawCollisionData.solid;
			if (solid.empty()) { continue; }
			object.sprite.solid = solid;
			for (RawSprite& look : object.looks)
			{
				look.solid = solid;
				for (RawSprite& frame : look.frames) { frame.solid = solid; }
			}
			for (RawSprite& frame : object.animation.frames) { frame.solid = solid; }
		}
	}

	void lookWithAnimations(RawObject& object, const std::vector<RawSprite>& sprites, const std::vector<RawFrames>& animations, const std::string& where)
	{
		std::vector<RawSprite> looks;
		for (const RawFrames& animation : animations)
		{
			if (animation.frames.size() < 2) { throw std::runtime_error(where + ": <animation name=\"" + animation.name + "\"> needs at least two <frame>s"); }

			RawSprite look;
			for (const std::string& name : animation.frames)
			{
				const auto found = std::find_if(sprites.begin(), sprites.end(), [&](const RawSprite& sprite) { return sprite.name == name; });
				if (found == sprites.end())
				{
					throw std::runtime_error(where + ": <animation name=\"" + animation.name + "\"> > <frame sprite=\"" + name + "\">: no <sprite name=\"" + name + "\"> to show");
				}
				look.frames.push_back(*found);
			}
			// Its first picture, in the look's own name.
			std::vector<RawSprite> frames = std::move(look.frames);
			look = frames.front();
			look.name = animation.name;
			look.frames = std::move(frames);
			look.interval = animation.interval;
			looks.push_back(std::move(look));
		}
		for (const RawSprite& sprite : sprites)
		{
			if (sprite.name.empty()) { throw std::runtime_error(where + ": has <animation name=\"...\">s, so its sprites are looks too, and every one needs a name"); }
			looks.push_back(sprite);
		}
		for (std::size_t i = 0; i < looks.size(); ++i)
		{
			for (std::size_t j = i + 1; j < looks.size(); ++j)
			{
				if (looks[i].name == looks[j].name) { throw std::runtime_error(where + ": two looks are called \"" + looks[i].name + "\" (a <sprite> and an <animation>, or two of one)"); }
			}
		}

		object.hasAnimation = false;
		object.animation = {};
		object.sprite = looks.front();
		object.looks = std::move(looks);
	}

	const std::vector<std::string>& liveNumberParts()
	{
		static const std::vector<std::string> parts{ "x", "y", "velocity.x", "velocity.y", "heading", "altitude", "rise", "light", "blink" };
		return parts;
	}

	std::string liveNumberPart(const std::string& name)
	{
		// The longest first: ball.velocity.x is velocity.x, not x.
		for (const char* part : { "velocity.x", "velocity.y", "heading", "altitude", "rise", "light", "blink", "x", "y" })
		{
			const std::string tail = std::string(".") + part;
			if (name.size() > tail.size() && name.compare(name.size() - tail.size(), tail.size(), tail) == 0) { return part; }
		}
		return {};
	}

	std::optional<float> numberOf(const Object& object, const std::string& name)
	{
		if (const auto own = object.variable.find(name); own != object.variable.end()) { return own->second; }
		if (name == "x") { return object.position.x; }
		if (name == "y") { return object.position.y; }
		if (name == "velocity.x") { return object.velocity.x; }
		if (name == "velocity.y") { return object.velocity.y; }
		if (name == "heading" && object.hasHeading) { return object.heading; }
		if (name == "altitude") { return object.altitude; }
		if (name == "rise") { return object.rise; }
		if (name == "light") { return object.light; }
		if (name == "blink") { return object.blink; }
		return std::nullopt;
	}

	const std::vector<std::string>& touchNumberNames()
	{
		static const std::vector<std::string> names{ "touch.x", "touch.y", "touch.along", "touch.depth", "touch.normal.x", "touch.normal.y" };
		return names;
	}

	std::string renameStart(const std::string& text, const std::string& from, const std::string& to)
	{
		const auto word = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '.'; };
		const std::string start = from + ".";
		std::string out;
		std::size_t i = 0;
		while (i < text.size())
		{
			if (!word(text[i])) { out += text[i++]; continue; }
			std::size_t end = i;
			while (end < text.size() && word(text[end])) { ++end; }
			const std::string token = text.substr(i, end - i);
			out += token.compare(0, start.size(), start) == 0 ? to + "." + token.substr(start.size()) : token;
			i = end;
		}
		return out;
	}

	void nameSelf(std::vector<RawObject>& objects)
	{
		for (RawObject& object : objects)
		{
			const std::function<void(RawValue&)> value = [&](RawValue& each)
			{
				each.text = renameStart(each.text, "self", object.name);
				each.min = renameStart(each.min, "self", object.name);
				each.max = renameStart(each.max, "self", object.name);
				if (each.operations)
				{
					auto operations = std::make_shared<std::vector<RawOperation>>(*each.operations);
					for (RawOperation& operation : *operations)
					{
						for (RawOperand& operand : operation.operands) { value(operand.value); }
					}
					each.operations = std::move(operations);
				}
			};
			const auto commands = [&](std::vector<RawCommand>& list)
			{
				for (RawCommand& command : list)
				{
					if (command.verb == "trigger" && command.object.empty()) { command.object = object.name; }
					command.variable = renameStart(command.variable, "self", object.name);
					command.into = renameStart(command.into, "self", object.name);
					for (RawValue* each : { &command.amount, &command.seconds, &command.distance, &command.ahead,
						&command.positionX, &command.positionY, &command.velocityX, &command.velocityY })
					{
						value(*each);
					}
				}
			};
			const std::function<void(RawCondition&)> check = [&](RawCondition& condition)
			{
				for (RawCondition& part : condition.parts) { check(part); }
				if (condition.join != RawCondition::Join::None) { return; }

				// self, as a name or in front of a variable (self.worth), is
				// the object running the rule, as it is in its commands.
				const std::string own = "self.";
				for (std::string* name : { &condition.filterObject, &condition.distance })
				{
					if (*name == "self") { *name = object.name; }
				}
				if (condition.variableName.compare(0, own.size(), own) == 0 && condition.filterClass.empty()
					&& (condition.filterObject.empty() || condition.filterObject == object.name))
				{
					condition.variableName.erase(0, own.size());
				}
				value(condition.threshold);
				if (condition.filterClass.empty() && condition.filterObject.empty())
				{
					condition.filterObject = object.name;
				}
			};

			RawCollisionData& collisions = object.rawCollisionData;
			for (auto* edge : { &collisions.top, &collisions.bottom, &collisions.left, &collisions.right }) { commands(*edge); }
			for (RawCollisionRule& rule : collisions.others)
			{
				commands(rule.commands);
				if (rule.check.join != RawCondition::Join::None) { check(rule.check); }
			}
			for (RawTimer& timer : object.timers) { commands(timer.commands); }
			for (auto& [name, list] : object.action) { commands(list); }
		}
	}

	void renameOwn(RawObject& object, const std::string& from, const std::string& to)
	{
		const std::function<void(RawValue&)> value = [&](RawValue& each)
		{
			each.text = renameStart(each.text, from, to);
			each.min = renameStart(each.min, from, to);
			each.max = renameStart(each.max, from, to);
			if (each.operations)
			{
				auto operations = std::make_shared<std::vector<RawOperation>>(*each.operations);
				for (RawOperation& operation : *operations)
				{
					for (RawOperand& operand : operation.operands) { value(operand.value); }
				}
				each.operations = std::move(operations);
			}
		};
		const auto commands = [&](std::vector<RawCommand>& list)
		{
			for (RawCommand& command : list)
			{
				if (command.verb == "trigger" && command.object == from) { command.object = to; }
				command.variable = renameStart(command.variable, from, to);
				command.into = renameStart(command.into, from, to);
				for (RawValue* each : { &command.amount, &command.seconds, &command.distance, &command.ahead,
					&command.positionX, &command.positionY, &command.velocityX, &command.velocityY })
				{
					value(*each);
				}
			}
		};
		const std::function<void(RawCondition&)> check = [&](RawCondition& condition)
		{
			for (RawCondition& part : condition.parts) { check(part); }
			if (condition.filterObject == from) { condition.filterObject = to; }
		};

		object.name = to;
		for (RawValue* each : { &object.rawPosition.x, &object.rawPosition.y, &object.rawVelocity.x, &object.rawVelocity.y,
			&object.rawAcceleration.x, &object.rawAcceleration.y, &object.rawHeading, &object.rawDrag, &object.rawMass, &object.rawSeek, &object.rawLight,
			&object.rawAltitude, &object.rawRise, &object.rawFall, &object.rawAltitudeBounce })
		{
			value(*each);
		}
		for (auto& [name, each] : object.rawPulls) { value(each); }
		for (auto& [name, each] : object.variable) { value(each); }
		for (RawTextPiece& piece : object.sprite.text) { value(piece.value); }

		RawCollisionData& collisions = object.rawCollisionData;
		for (auto* edge : { &collisions.top, &collisions.bottom, &collisions.left, &collisions.right }) { commands(*edge); }
		for (RawCollisionRule& rule : collisions.others)
		{
			commands(rule.commands);
			if (rule.check.join != RawCondition::Join::None) { check(rule.check); }
			if (rule.slower) { value(*rule.slower); }
			if (rule.faster) { value(*rule.faster); }
		}
		for (RawTimer& timer : object.timers)
		{
			value(timer.interval);
			commands(timer.commands);
		}
		for (auto& [name, list] : object.action) { commands(list); }
	}

	void centerOnOwnSize(std::vector<RawObject>& objects)
	{
		for (RawObject& object : objects)
		{
			const auto center = [&](RawValue& value, const char* windowMiddle, const char* size)
			{
				if (value.kind != RawValue::Kind::Centered) { return; }
				const std::string middle = value.text.empty() ? windowMiddle : "(" + value.text + ")";
				value = RawValue::expression(middle + " - " + object.name + "." + size + " / 2");
			};
			center(object.rawPosition.x, "window.width.center", "width");
			center(object.rawPosition.y, "window.height.center", "height");
		}
	}

	RawSprite nothingSprite()
	{
		RawSprite sprite;
		sprite.kind = "rectangle";
		sprite.width = RawValue::expression("0");
		sprite.height = RawValue::expression("0");
		sprite.color = "color.black";
		return sprite;
	}

	bool showLookNamed(Object& object, const std::string& name)
	{
		for (std::size_t i = 0; i < object.looks.size(); ++i)
		{
			if (object.looks[i].name == name)
			{
				if (object.look != i) { object.showLook(i); }
				// A timer that follows this look starts over, even when the
				// object showed it already: blue again is six seconds again.
				for (auto& timer : object.timers)
				{
					if (timer.sprite == name)
					{
						timer.framesLeft = -1;
						timer.done = false;
					}
				}
				return true;
			}
		}
		return false;
	}

	void Object::showHeading()
	{
		if (turnables.empty()) { return; }

		// An animated object that turns has a turnable for each of its
		// pictures; the one that is showing is the one to turn.
		const std::size_t frame = turnables.size() > 1 && animationIndex < turnables.size() ? animationIndex : 0;

		const int degrees = wholeDegrees(heading);

		if (bitmap && turnedDegrees == degrees && turnedFrame == frame) { return; }

		bitmap = std::make_shared<const Bitmap>(turnables[frame]->at(static_cast<float>(degrees)));
		turnedDegrees = degrees;
		turnedFrame = frame;
		visualDirty = true;
		// The turned picture is the size it is now, not at the next draw: what
		// the edges and the touches of this frame look at.
		if (sizeKnown)
		{
			size = Vector2f{ static_cast<float>(std::max(bitmap->width, 1)), static_cast<float>(std::max(bitmap->height, 1)) };
		}
	}

	void Object::advanceAnimation()
	{
		if (animationBitmaps.size() < 2 || animationFrames < 1) { return; }

		if (++animationTick < animationFrames) { return; }

		animationTick = 0;
		animationIndex = (animationIndex + 1) % animationBitmaps.size();
		showAnimationFrame();
	}

	void Object::restartAnimation()
	{
		animationTick = 0;

		if (animationIndex == 0 || animationBitmaps.empty()) { return; }

		animationIndex = 0;
		showAnimationFrame();
	}

	void Object::startOver()
	{
		restartAnimation();
		for (auto& timer : timers)
		{
			timer.framesLeft = -1;
			timer.done = false;
		}
	}

	void Object::showAnimationFrame()
	{
		if (!turnables.empty()) { showHeading(); }
		else { bitmap = facingPicture(animationBitmaps[animationIndex]); }
		visualDirty = true;
	}

	std::ostream& operator<<(std::ostream& o, const WindowDesc& f)
	{
		o << "window: ";
		o << "title=" << f.name;
		o << ", width=" << f.width;
		o << ", height=" << f.height << '\n';

		return o;
	}

	std::ostream& operator<<(std::ostream& o, const RawObject& f) {
		o << "rawObject: " << "name=" << f.name << (f.groupName.empty() ? "" : ", group=" + f.groupName) << ", sprite=" << f.sprite.kind
			<< (f.cell.column > 0 ? " (cell " + std::to_string(f.cell.column) + "." + std::to_string(f.cell.row) + ")" : "")
			<< (f.hasAnimation ? " (animated, " + std::to_string(f.animation.frames.size()) + " frames)" : "") << '\n'
			<< "\tpos.x=" << f.rawPosition.x << ", pos.y=" << f.rawPosition.y << '\n'
			<< "\tvel.x=" << f.rawVelocity.x << ", vel.y=" << f.rawVelocity.y << '\n'

			// TODO: replace with operator<< for rawCollisionData
			<< "\tcollision=" << (f.rawCollisionData.enabled ? "true" : "false")
			<< (f.rawCollisionData.type == CollisionType::Pixel ? ", type=pixel" : "");

		if (!f.rawCollisionData.top.empty()) { o << ", top=" << f.rawCollisionData.top; }
		if (!f.rawCollisionData.bottom.empty()) { o << ", bottom=" << f.rawCollisionData.bottom; }
		if (!f.rawCollisionData.left.empty()) { o << ", left=" << f.rawCollisionData.left; }
		if (!f.rawCollisionData.right.empty()) { o << ", right=" << f.rawCollisionData.right; }

		for (auto& rawRule : f.rawCollisionData.others)
		{
			o << ", others=" << rawRule.commands;
			if (!rawRule.filterClass.empty()) { o << " (class=" << rawRule.filterClass << ")"; }
			if (!rawRule.filterObject.empty()) { o << " (object=" << rawRule.filterObject << ")"; }
			if (!rawRule.unlessClass.empty()) { o << " (unless=" << rawRule.unlessClass << ")"; }
			if (rawRule.slower) { o << " (slower=" << *rawRule.slower << ")"; }
			if (rawRule.faster) { o << " (faster=" << *rawRule.faster << ")"; }
		}
		o << '\n';

		for (auto& action : f.action)
		{
			o << "\taction=" << action.first << ", value=" << action.second << '\n';
		}

		for (auto& varible : f.variable)
		{
			o << "\tvariable=" << varible.first << ", value=" << varible.second << '\n';
		}

		return o;
	}

	std::ostream& operator<<(std::ostream& o, const Object& f) {
		o << "Object: " << "name=" << f.name << ", sprite=";
		for (std::size_t i = 0; i < f.spriteParams.size(); ++i)
		{
			o << (i ? "," : "") << f.spriteParams[i];
		}
		o << '\n';

		if (f.positionResolved)
		{
			o << "\tpos.x=" << f.position.x << ", pos.y=" << f.position.y << '\n';
		}
		else
		{
			o << "\tpos= ( Unknown, depends on size; not yet initialized by Engine )\n";
		}

		o << "\tvel.x=" << f.velocity.x << ", vel.y=" << f.velocity.y << '\n';

		if (f.acceleration.x != 0 || f.acceleration.y != 0)
		{
			o << "\tacc.x=" << f.acceleration.x << ", acc.y=" << f.acceleration.y << '\n';
		}

		if (f.hasHeading)
		{
			o << "\theading=" << f.heading << '\n';
		}

		if (f.animationBitmaps.size() > 1)
		{
			o << "\tanimation=" << f.animationBitmaps.size() << " frames, " << f.animationFrames << " game frames each\n";
		}

		if (f.drag != 0)
		{
			o << "\tdrag=" << f.drag << '\n';
		}

		if (f.mass != 0)
		{
			o << "\tmass=" << f.mass << '\n';
		}

		for (const auto& [name, strength] : f.pulls)
		{
			o << "\tpull=" << name << " " << strength << '\n';
		}

		if (f.light > 0.0f) { o << "\tlight=" << f.light << '\n'; }
		if (f.dark != Object::Darkness::None) { o << "\tdark=" << (f.dark == Object::Darkness::Unlit ? "unlit" : "unseen") << '\n'; }
		if (f.hasAltitude) { o << "\taltitude=" << f.altitude << " rise " << f.rise << " fall " << f.fall << " bounce " << f.altitudeBounce << '\n'; }
		if (!f.shadowOf.empty()) { o << "\tshadow of " << f.shadowOf << '\n'; }
		if (!f.seekTarget.empty())
		{
			o << "\tseek=" << f.seekTarget << " " << f.seekRate << '\n';
		}

		if (f.sizeKnown)
		{
			o << "\tsize.x=" << f.size.x << ", size.y=" << f.size.y << '\n';
		}
		else
		{
			o << "\tsize= ( Unknown, not yet initialized by Engine )\n";
		}

		o << "\tcollision=" << (f.collisionData.enabled ? "true" : "false")
			<< (f.collisionData.type == CollisionType::Pixel ? ", type=pixel" : "");


		// TODO: replace with operator<< for CollisionData
		auto printCollisionData = [&](const std::vector<Command>& commands, std::string edge)
		{
			if (commands.size() > 0)
			{
				o << ", " << edge << "=";
				for (auto& command : commands)
				{
					o << command << (&command != &commands.back() ? ";" : "");
				}
			}
		};

		if (f.collisionData.lockstep > 0)
		{
			o << ", lockstep=" << f.collisionData.lockstep;
		}

		printCollisionData(f.collisionData.top, "top");
		printCollisionData(f.collisionData.bottom, "bottom");
		printCollisionData(f.collisionData.left, "left");
		printCollisionData(f.collisionData.right, "right");

		for (auto& rule : f.collisionData.others)
		{
			o << ", others=";
			for (auto& command : rule.commands)
			{
				o << command << (&command != &rule.commands.back() ? ";" : "");
			}
			if (!rule.filterClass.empty()) { o << " (class=" << rule.filterClass << ")"; }
			if (!rule.filterObject.empty()) { o << " (object=" << rule.filterObject << ")"; }
			if (!rule.unlessClass.empty()) { o << " (unless=" << rule.unlessClass << ")"; }
			if (rule.slower) { o << " (slower=" << *rule.slower << ")"; }
			if (rule.faster) { o << " (faster=" << *rule.faster << ")"; }
		}

		o << '\n';

		for (auto& action : f.action)
		{
			o << "\taction=" << action.first << ", value=";
			for (auto& command : action.second)
			{
				o << command << (&command != &action.second.back() ? ";" : "");
			}
			o << '\n';
		}

		return o;
	}
}
