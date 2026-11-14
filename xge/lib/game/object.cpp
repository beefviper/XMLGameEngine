// object.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "game/object.h"

#include "picture/color.h"
#include "picture/font.h"
#include "picture/image.h"

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

		// The look to go back to when a look that plays once ends.
		if (index != look && look < looks.size() && !looks[look].once) { lookBefore = look; }
		look = index;
		picture = looks[index].picture;
		bitmap = facingPicture(looks[index].bitmap);
		animationBitmaps = looks[index].frames;
		animationFrames = looks[index].frameTicks;
		animationTimes = looks[index].frameTimes;
		animationOnce = looks[index].once;
		animationDone = false;
		animationTick = 0;
		animationIndex = 0;
		// Turned to the way it faces: its own pictures to turn from now on.
		if (!looks[index].turnables.empty())
		{
			turnables = looks[index].turnables;
			turnedDegrees = -1;
			showHeading();
		}
		visualDirty = true;
		// The new look is the size it is now, not at the next draw: what the
		// edges, touches and card layout of this frame look at.
		size = pictureSize();
	}

	void Object::wearPalette(std::size_t index)
	{
		if (index >= palettes.size() || index == palette)
		{
			return;
		}

		palette = index;
		const Palette& worn = palettes[index];
		const int tick = animationTick;
		const std::size_t at = animationIndex;
		if (looks.empty())
		{
			animationBitmaps = worn.animation;
			bitmap = facingPicture(worn.bitmap);
		}
		else
		{
			looks = worn.looks;
			showLook(look);
		}
		if (!animationBitmaps.empty())
		{
			animationTick = tick;
			animationIndex = at % animationBitmaps.size();
			showAnimationFrame();
		}
		visualDirty = true;
	}

	std::shared_ptr<const Bitmap> Object::facingPicture(const std::shared_ptr<const Bitmap>& shown) const
	{
		if (!mirrored || !flips || !shown) { return shown; }
		const auto found = flips->find(shown.get());
		return found == flips->end() ? shown : found->second;
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

	bool addEdgeRule(RawCollisionData& collisions, const std::string& edges, const std::vector<RawCommand>& commands,
		const std::string& sprite, const std::string& unlessClass, const RawCondition& check)
	{
		const bool all = edges == "all";
		const bool vertical = all || edges == "vertical";
		const bool horizontal = all || edges == "horizontal";
		bool any = false;
		for (const auto& [edge, named] : { std::pair{ Edge::Top, vertical || edges == "top" }, std::pair{ Edge::Bottom, vertical || edges == "bottom" },
			std::pair{ Edge::Left, horizontal || edges == "left" }, std::pair{ Edge::Right, horizontal || edges == "right" } })
		{
			if (!named) { continue; }
			any = true;
			EdgeRules<RawCommand>& rules = collisions.edges[edgeIndex(edge)];
			if (!commands.empty())
			{
				rules.guards.push_back(EdgeGuard{ rules.commands.size(), commands.size(), sprite, unlessClass, check, std::nullopt });
			}
			rules.commands.insert(rules.commands.end(), commands.begin(), commands.end());
		}
		return any;
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

	void placeOnMap(RawObject& object, const RawMap& map, std::size_t column, std::size_t row, char character)
	{
		object.cell.character = character;
		object.rawPosition = map.position;
		object.cell.map = map.name;
		object.cell.mapColumn = static_cast<int>(column) + 1;
		object.cell.mapRow = static_cast<int>(row) + 1;
		object.cell.mapColumns = map.across();
		object.cell.mapRows = map.down();
		object.cell.cells = map.cells;
		object.cell.checks = map.checks;
		object.cell.tileWidth = map.tileWidth;
		object.cell.tileHeight = map.tileHeight;
		if (std::find(object.outerGroups.begin(), object.outerGroups.end(), map.name) == object.outerGroups.end())
		{
			object.outerGroups.insert(object.outerGroups.begin(), map.name);
		}
	}

	void waitOnMap(RawObject& object, const RawMap& map, char character)
	{
		placeOnMap(object, map, 0, 0, character);
		object.exists = false;
	}

	RawObject mapHolder(const RawMap& map)
	{
		RawObject holder;
		holder.name = map.name + ".cells";
		holder.sprite = nothingSprite();
		holder.isDrawn = false;
		holder.rawVelocity = { RawValue::expression("0"), RawValue::expression("0") };
		waitOnMap(holder, map);
		return holder;
	}

	void pickShapePieces(std::vector<RawObject>& objects, std::size_t first)
	{
		for (std::size_t i = first; i < objects.size(); ++i)
		{
			RawObject& shape = objects[i];
			for (const std::string& row : shape.shapeRows)
			{
				for (const char character : row)
				{
					if (shape.shapePieces.count(character)) { continue; }
					std::vector<const RawObject*> found;
					for (std::size_t j = first; j < objects.size(); ++j)
					{
						const RawObject& piece = objects[j];
						if (piece.cell.character == character && piece.cell.map == shape.cell.map) { found.push_back(&piece); }
					}
					if (found.empty()) { continue; }
					const RawObject& piece = *found.front();
					if (found.size() == 1 || piece.exists)
					{
						shape.shapePieces[character] = !piece.exists || piece.groupName.empty() ? piece.name : piece.groupName;
						continue;
					}
					// Several waiting pieces (a group's inner groups, each a
					// jewel): the innermost group around them all, which a
					// spawn takes one member of at random.
					std::vector<std::string> shared = piece.outerGroups;
					for (const RawObject* other : found)
					{
						std::size_t same = 0;
						while (same < shared.size() && same < other->outerGroups.size() && shared[same] == other->outerGroups[same]) { ++same; }
						shared.resize(same);
					}
					if (!shared.empty()) { shape.shapePieces[character] = shared.back(); }
				}
			}
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
					if (sprite.color.empty() && !style->color.empty())
					{
						sprite.color = style->color;
						sprite.styleColor = true;
					}
					for (const auto& [character, color] : style->bitmapColors)
					{
						const bool own = std::any_of(sprite.bitmapColors.begin(), sprite.bitmapColors.end(), [&](const auto& each) { return each.first == character; });
						if (!own)
						{
							sprite.bitmapColors.emplace_back(character, color);
							sprite.styleChars += character;
						}
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
					if (sprite.font.empty()) { sprite.font = style->font; }
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
		// The animations that go round first, then the sprites, then those
		// played once: an object starts as the first, and never as a look
		// that plays once.
		std::vector<RawSprite> looks;
		std::vector<RawSprite> played;
		for (const RawFrames& animation : animations)
		{
			if (animation.frames.size() < (animation.once ? 1u : 2u)) { throw std::runtime_error(where + ": <animation name=\"" + animation.name + "\"> needs at least two <frame>s (one, with <once />)"); }

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
			look.times = animation.times;
			look.once = animation.once;
			look.thenSprite = animation.thenSprite;
			look.thenCommands = animation.thenCommands;
			(animation.once ? played : looks).push_back(std::move(look));
		}
		for (const RawSprite& sprite : sprites)
		{
			if (sprite.name.empty()) { throw std::runtime_error(where + ": has <animation name=\"...\">s, so its sprites are looks too, and every one needs a name"); }
			looks.push_back(sprite);
		}
		for (RawSprite& look : played) { looks.push_back(std::move(look)); }
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

	const std::vector<LivePart>& liveParts()
	{
		static const std::vector<LivePart> parts{
			{ "x", [](Object& o) -> float& { return o.position.x; }, false },
			{ "y", [](Object& o) -> float& { return o.position.y; }, false },
			{ "velocity.x", [](Object& o) -> float& { return o.velocity.x; }, false },
			{ "velocity.y", [](Object& o) -> float& { return o.velocity.y; }, false },
			{ "heading", [](Object& o) -> float& { return o.heading; }, false },
			{ "altitude", [](Object& o) -> float& { return o.altitude; }, true },
			{ "rise", [](Object& o) -> float& { return o.rise; }, false },
			{ "light", [](Object& o) -> float& { return o.light; }, true },
			{ "blink", [](Object& o) -> float& { return o.blink; }, true },
			{ "speed", [](Object& o) -> float& { if (!o.speedHeld) { o.speed = std::hypot(o.velocity.x, o.velocity.y); } return o.speed; }, false },
			{ "column", [](Object& o) -> float& { return o.cellColumn; }, false },
			{ "row", [](Object& o) -> float& { return o.cellRow; }, false },
			{ "moves", [](Object& o) -> float& { return o.moveCount; }, true },
			{ "cards", [](Object& o) -> float& { return o.cardCount; }, true },
			{ "held", [](Object& o) -> float& { return o.cardCount; }, true },
			{ "calling", [](Object& o) -> float& { return o.calling; }, true },
			{ "called", [](Object& o) -> float& { return o.called; }, true },
			{ "points", [](Object& o) -> float& { return o.points; }, true },
			{ "pattern", [](Object& o) -> float& { return o.pattern; }, true },
			{ "strength", [](Object& o) -> float& { return o.strength; }, true },
			{ "odds", [](Object& o) -> float& { return o.odds; }, true },
			{ "lead", [](Object& o) -> float& { return o.lead; }, true },
			{ "winner", [](Object& o) -> float& { return o.winner; }, true },
			{ "tricks", [](Object& o) -> float& { return o.tricks; }, true },
			{ "deadwood", [](Object& o) -> float& { return o.deadwood; }, true },
		};
		return parts;
	}

	const LivePart* livePartNamed(const std::string& part)
	{
		for (const LivePart& each : liveParts())
		{
			if (part == each.name) { return &each; }
		}
		return nullptr;
	}

	const std::vector<std::string>& liveNumberParts()
	{
		static const std::vector<std::string> names = []
		{
			std::vector<std::string> all;
			for (const LivePart& part : liveParts()) { all.push_back(part.name); }
			return all;
		}();
		return names;
	}

	std::string liveNumberPart(const std::string& name)
	{
		// The longest that fits: ball.velocity.x is velocity.x, not x.
		std::string longest;
		for (const LivePart& part : liveParts())
		{
			const std::string tail = std::string(".") + part.name;
			if (name.size() > tail.size() && name.compare(name.size() - tail.size(), tail.size(), tail) == 0 && tail.size() > longest.size() + 1)
			{
				longest = part.name;
			}
		}
		return longest;
	}

	std::optional<float> numberOf(const Object& object, const std::string& name)
	{
		if (const auto own = object.variable.find(name); own != object.variable.end()) { return own->second; }
		const LivePart* part = livePartNamed(name);
		if (!part || (name == "heading" && !object.hasHeading)) { return std::nullopt; }
		return part->of(const_cast<Object&>(object)); // only read
	}

	bool Object::followsPart(const std::string& part) const
	{
		return std::any_of(follows.begin(), follows.end(), [&](const Follow& each)
		{
			switch (each.part)
			{
			case Follow::Part::X: return part == "x";
			case Follow::Part::Y: return part == "y";
			case Follow::Part::Heading: return part == "heading";
			case Follow::Part::Variable: return part == each.variable;
			}
			return false;
		});
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

	namespace
	{
		// self. at the start of a name in a value, made `name`; whether any was.
		bool nameSelfInValue(RawValue& each, const std::string& name);

		// A rule's or an action's test: self, as a name or in front of a
		// variable (self.worth), is the object running it, as it is in its
		// commands; a test naming nothing is about that object.
		void nameSelfInCheck(RawCondition& condition, const std::string& name)
		{
			for (RawCondition& part : condition.parts) { nameSelfInCheck(part, name); }
			if (condition.join != RawCondition::Join::None) { return; }

			const std::string own = "self.";
			for (std::string* each : { &condition.filterObject, &condition.distance, &condition.sees })
			{
				if (*each == "self") { *each = name; }
			}
			if (condition.variableName.compare(0, own.size(), own) == 0 && condition.filterClass.empty()
				&& (condition.filterObject.empty() || condition.filterObject == name))
			{
				condition.variableName.erase(0, own.size());
			}
			nameSelfInValue(condition.threshold, name);
			if (condition.filterClass.empty() && condition.filterObject.empty())
			{
				condition.filterObject = name;
			}
		}

		bool nameSelfInValue(RawValue& each, const std::string& name)
		{
			return renameInValue(each, "self", name);
		}
	}

	bool renameInValue(RawValue& each, const std::string& from, const std::string& to)
	{
		bool changed = false;
		for (std::string* text : { &each.text, &each.min, &each.max, &each.offset })
		{
			std::string renamed = renameStart(*text, from, to);
			if (renamed != *text) { *text = std::move(renamed); changed = true; }
		}
		if (each.operations)
		{
			auto operations = std::make_shared<std::vector<RawOperation>>(*each.operations);
			for (RawOperation& operation : *operations)
			{
				for (RawOperand& operand : operation.operands) { changed = renameInValue(operand.value, from, to) || changed; }
			}
			each.operations = std::move(operations);
		}
		return changed;
	}

	bool nameSelfIn(std::vector<RawCommand>& list, const std::string& name)
	{
		bool changed = false;
		for (RawCommand& command : list)
		{
			if (command.verb == "trigger" && command.object.empty()) { command.object = name; changed = true; }
			for (std::string* text : { &command.variable, &command.into })
			{
				std::string renamed = renameStart(*text, "self", name);
				if (renamed != *text) { *text = std::move(renamed); changed = true; }
			}
			for (RawValue* each : { &command.amount, &command.seconds, &command.distance, &command.ahead,
				&command.positionX, &command.positionY, &command.velocityX, &command.velocityY, &command.accelerationX, &command.accelerationY, &command.column, &command.row })
			{
				changed = nameSelfInValue(*each, name) || changed;
			}
			// A condition in an action: its test and its commands.
			if (command.test)
			{
				command.test = std::make_shared<RawCondition>(*command.test);
				nameSelfInCheck(*command.test, name);
				nameSelfIn(command.test->commands, name);
				changed = true;
			}
		}
		return changed;
	}

	namespace
	{
		// `from.` at the start of a name in a value made `to.`; whether any was.
		bool renameObjectInValue(RawValue& each, const std::string& from, const std::string& to)
		{
			bool changed = false;
			for (std::string* text : { &each.text, &each.offset, &each.min, &each.max })
			{
				std::string renamed = renameStart(*text, from, to);
				if (renamed != *text) { *text = std::move(renamed); changed = true; }
			}
			if (each.operations)
			{
				auto operations = std::make_shared<std::vector<RawOperation>>(*each.operations);
				for (RawOperation& operation : *operations)
				{
					for (RawOperand& operand : operation.operands) { changed = renameObjectInValue(operand.value, from, to) || changed; }
				}
				each.operations = std::move(operations);
			}
			return changed;
		}

		// The same in a test: the object it is about or measures to, its
		// variable, its number, and what it holds.
		bool renameObjectInCheck(RawCondition& condition, const std::string& from, const std::string& to)
		{
			bool changed = false;
			for (std::string* each : { &condition.filterObject, &condition.distance, &condition.sees })
			{
				if (*each == from) { *each = to; changed = true; }
			}
			std::string renamed = renameStart(condition.variableName, from, to);
			if (renamed != condition.variableName) { condition.variableName = std::move(renamed); changed = true; }
			changed = renameObjectInValue(condition.threshold, from, to) || changed;
			for (std::vector<RawCondition>* list : { &condition.parts, &condition.inner })
			{
				for (RawCondition& part : *list) { changed = renameObjectInCheck(part, from, to) || changed; }
			}
			changed = renameObjectIn(condition.first, from, to) || changed;
			changed = renameObjectIn(condition.commands, from, to) || changed;
			return changed;
		}
	}

	bool renameObjectIn(std::vector<RawCommand>& list, const std::string& from, const std::string& to)
	{
		bool changed = false;
		for (RawCommand& command : list)
		{
			for (std::string* each : { &command.object, &command.partner, &command.retreat })
			{
				if (*each == from) { *each = to; changed = true; }
			}
			for (std::string* text : { &command.variable, &command.into })
			{
				std::string renamed = renameStart(*text, from, to);
				if (renamed != *text) { *text = std::move(renamed); changed = true; }
			}
			for (RawValue* each : { &command.amount, &command.seconds, &command.arc, &command.distance, &command.ahead, &command.positionX, &command.positionY,
				&command.velocityX, &command.velocityY, &command.accelerationX, &command.accelerationY, &command.column, &command.row, &command.below })
			{
				changed = renameObjectInValue(*each, from, to) || changed;
			}
			if (command.test)
			{
				auto test = std::make_shared<RawCondition>(*command.test);
				if (renameObjectInCheck(*test, from, to)) { command.test = std::move(test); changed = true; }
			}
		}
		return changed;
	}

	void nameOther(std::vector<RawObject>& objects)
	{
		for (RawObject& object : objects)
		{
			std::vector<RawCollisionRule> rules;
			for (RawCollisionRule& rule : object.rawCollisionData.others)
			{
				RawCollisionRule probe = rule;
				const bool inCommands = renameObjectIn(probe.commands, "other", "other_");
				const bool inCheck = renameObjectInCheck(probe.check, "other", "other_");
				if (!inCommands && !inCheck)
				{
					rules.push_back(std::move(rule));
					continue;
				}

				// Once for each object it can touch: of its class=, named by
				// its object=, in the same state (or one of them in none).
				bool any = false;
				for (const RawObject& touched : objects)
				{
					if (&touched == &object) { continue; }
					if (!rule.filterClass.empty() && touched.objClass != rule.filterClass) { continue; }
					if (!rule.filterObject.empty() && !touched.named(rule.filterObject)) { continue; }
					if (!touched.ownerState.empty() && !object.ownerState.empty() && touched.ownerState != object.ownerState) { continue; }
					any = true;
					RawCollisionRule each = rule;
					each.filterObject = touched.name;
					each.otherIs = touched.name;
					renameObjectIn(each.commands, "other", touched.name);
					renameObjectInCheck(each.check, "other", touched.name);
					rules.push_back(std::move(each));
				}
				if (!any)
				{
					const std::string about = !rule.filterObject.empty() ? " named \"" + rule.filterObject + "\"" : !rule.filterClass.empty() ? " of class \"" + rule.filterClass + "\"" : std::string();
					throw std::runtime_error("object '" + object.name + "' > <collision>: names other, the object it touched, but there is nothing" + about + " for it to touch");
				}
			}
			object.rawCollisionData.others = std::move(rules);
		}
	}

	void nameSelf(std::vector<RawObject>& objects)
	{
		for (RawObject& object : objects)
		{
			const std::function<void(RawValue&)> value = [&](RawValue& each) { nameSelfInValue(each, object.name); };
			const auto commands = [&](std::vector<RawCommand>& list) { nameSelfIn(list, object.name); };
			const auto check = [&](RawCondition& condition) { nameSelfInCheck(condition, object.name); };

			RawCollisionData& collisions = object.rawCollisionData;
			for (auto& edge : collisions.edges)
			{
				commands(edge.commands);
				for (EdgeGuard& guard : edge.guards) { if (guard.rawCheck.join != RawCondition::Join::None) { check(guard.rawCheck); } }
			}
			for (RawCollisionRule& rule : collisions.others)
			{
				commands(rule.commands);
				if (rule.check.join != RawCondition::Join::None) { check(rule.check); }
			}
			for (RawTimer& timer : object.timers) { commands(timer.commands); }
			for (auto& [name, list] : object.action) { commands(list); }

			// Its own numbers, worked out once or live (update="live"):
			// self in them is this object too.
			for (auto& each : object.variable) { value(each.second); }
			for (RawValue* half : { &object.rawPosition.x, &object.rawPosition.y, &object.rawHeading }) { value(*half); }
			// The numbers its texts show (<number>self.value</number>).
			for (RawTextPiece& piece : object.sprite.text) { value(piece.value); }
		for (RawSprite& look : object.looks)
		{
			for (RawTextPiece& piece : look.text) { value(piece.value); }
		}
			for (RawSprite& look : object.looks)
			{
				for (RawTextPiece& piece : look.text) { value(piece.value); }
			}
		}
	}

	void renameOwn(RawObject& object, const std::string& from, const std::string& to)
	{
		const auto value = [&](RawValue& each) { renameInValue(each, from, to); };
		const std::function<void(RawCondition&)> check = [&](RawCondition& condition)
		{
			for (RawCondition& part : condition.parts) { check(part); }
			if (condition.filterObject == from) { condition.filterObject = to; }
			if (condition.distance == from) { condition.distance = to; }
			if (condition.sees == from) { condition.sees = to; }
			value(condition.threshold);
		};
		std::function<void(std::vector<RawCommand>&)> commands = [&](std::vector<RawCommand>& list)
		{
			for (RawCommand& command : list)
			{
				if (command.test)
				{
					command.test = std::make_shared<RawCondition>(*command.test);
					check(*command.test);
					commands(command.test->commands);
				}
				if (command.verb == "trigger" && command.object == from) { command.object = to; }
				command.variable = renameStart(command.variable, from, to);
				command.into = renameStart(command.into, from, to);
				for (RawValue* each : { &command.amount, &command.seconds, &command.distance, &command.ahead,
					&command.positionX, &command.positionY, &command.velocityX, &command.velocityY, &command.accelerationX, &command.accelerationY, &command.column, &command.row })
				{
					value(*each);
				}
			}
		};

		object.name = to;
		for (RawValue* each : { &object.rawPosition.x, &object.rawPosition.y, &object.rawVelocity.x, &object.rawVelocity.y,
			&object.rawAcceleration.x, &object.rawAcceleration.y, &object.rawHeading, &object.rawPivot.x, &object.rawPivot.y, &object.rawDrag, &object.rawFriction, &object.rawMass, &object.rawSurfaceDrag, &object.rawSurfaceGrip, &object.rawSeek, &object.rawLight,
			&object.rawAltitude, &object.rawRise, &object.rawFall, &object.rawAltitudeBounce })
		{
			value(*each);
		}
		for (auto& [name, each] : object.rawPulls) { value(each); }
		for (auto& [name, each] : object.variable) { value(each); }
		for (RawTextPiece& piece : object.sprite.text) { value(piece.value); }

		RawCollisionData& collisions = object.rawCollisionData;
		for (auto& edge : collisions.edges)
		{
			commands(edge.commands);
			for (EdgeGuard& guard : edge.guards) { if (guard.rawCheck.join != RawCondition::Join::None) { check(guard.rawCheck); } }
		}
		for (RawCollisionRule& rule : collisions.others)
		{
			commands(rule.commands);
			if (rule.check.join != RawCondition::Join::None) { check(rule.check); }
		}
		for (RawTimer& timer : object.timers)
		{
			value(timer.interval);
			commands(timer.commands);
		}
		for (auto& [name, list] : object.action) { commands(list); }
	}

	namespace
	{
		// An operation of the values given, in that order.
		RawValue operationOf(const char* op, std::vector<RawValue> values)
		{
			RawOperation operation;
			operation.op = op;
			for (RawValue& value : values) { operation.operands.push_back(RawOperand{ "value", std::move(value) }); }
			RawValue made;
			made.kind = RawValue::Kind::Formula;
			made.operations = std::make_shared<std::vector<RawOperation>>(1, std::move(operation));
			return made;
		}
	}

	RawValue centeredMiddle(const RawValue& value, const std::string& windowMiddle)
	{
		if (value.operations)
		{
			RawValue middle;
			middle.kind = RawValue::Kind::Formula;
			middle.operations = value.operations;
			return middle;
		}
		if (!value.offset.empty()) { return operationOf("add", { RawValue::expression(windowMiddle), RawValue::expression(value.offset) }); }
		return RawValue::expression(value.text.empty() ? windowMiddle : value.text);
	}

	void centerOnOwnSize(std::vector<RawObject>& objects)
	{
		for (RawObject& object : objects)
		{
			const auto center = [&](RawValue& value, const std::string& window, const char* size)
			{
				if (value.kind != RawValue::Kind::Centered) { return; }
				const RawValue own = RawValue::expression(object.name + "." + size);
				const bool live = value.live;
				if (value.edge == RawValue::Edge::Start)
				{
					// Its left or top on the window's, the offset in from it.
					value = RawValue::expression(value.offset.empty() ? std::string("0") : value.offset);
				}
				else if (value.edge == RawValue::Edge::End)
				{
					// Its right or bottom on the window's: the window's size less
					// its own, and the offset in from it.
					std::vector<RawValue> parts{ RawValue::expression(window), own };
					if (!value.offset.empty()) { parts.push_back(RawValue::expression(value.offset)); }
					value = operationOf("subtract", std::move(parts));
				}
				else
				{
					// Its middle where the middle goes: that less half its size.
					value = operationOf("subtract", { centeredMiddle(value, window + ".center"), operationOf("divide", { own, RawValue::expression("2") }) });
				}
				value.live = live;
			};
			center(object.rawPosition.x, "window.width", "width");
			center(object.rawPosition.y, "window.height", "height");
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

	bool wearPaletteNamed(Object& object, const std::string& name)
	{
		for (std::size_t i = 0; i < object.palettes.size(); ++i)
		{
			if (object.palettes[i].name == name)
			{
				object.wearPalette(i);
				return true;
			}
		}
		return false;
	}

	bool showLookNamed(Object& object, const std::string& name)
	{
		for (std::size_t i = 0; i < object.looks.size(); ++i)
		{
			if (object.looks[i].name == name)
			{
				// A look that plays once starts again from its first frame,
				// even when it is showing already: a second blast is a whole one.
				if (object.look != i || object.looks[i].once) { object.showLook(i); }
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

	Vector2f turnedBy(const Vector2f& way, float degrees) noexcept
	{
		const float angle = degrees * 3.14159265358979323846f / 180.0f;
		const float c = std::cos(angle);
		const float s = std::sin(angle);
		return { way.x * c - way.y * s, way.x * s + way.y * c };
	}

	Vector2f Object::pivotOnScreen() const
	{
		// The picture as it shows now, turned: the square every heading
		// comes out as.
		const Vector2f shown = bitmap ? Vector2f{ static_cast<float>(bitmap->width), static_cast<float>(bitmap->height) } : size;
		const Vector2f middle = position + shown * 0.5f;
		return hasPivot ? middle + turnedBy(pivotFromMiddle, static_cast<float>(pivotDegrees)) : middle;
	}

	Vector2f Object::velocityAt(const Vector2f& at) const
	{
		if (spin == 0.0f) { return velocity; }
		const Vector2f arm = at - pivotOnScreen();
		const float radians = spin * 3.14159265358979323846f / 180.0f;
		return velocity + Vector2f{ -arm.y * radians, arm.x * radians };
	}

	void Object::showHeading()
	{
		if (turnables.empty()) { return; }

		// An animated object that turns has a turnable for each of its
		// pictures; the one that is showing is the one to turn.
		const std::size_t frame = turnables.size() > 1 && animationIndex < turnables.size() ? animationIndex : 0;

		const int degrees = wholeDegrees(heading);

		// Turning about a pivot: the picture is turned about its middle, so the
		// middle goes round the pivot, which stays where it is.
		if (hasPivot && degrees != pivotDegrees)
		{
			position += turnedBy(pivotFromMiddle, static_cast<float>(pivotDegrees)) - turnedBy(pivotFromMiddle, static_cast<float>(degrees));
			pivotDegrees = degrees;
		}

		if (bitmap && turnedDegrees == degrees && turnedFrame == frame) { return; }

		bitmap = std::make_shared<const Bitmap>(turnables[frame]->at(static_cast<float>(degrees)));
		turnedDegrees = degrees;
		turnedFrame = frame;
		visualDirty = true;
		// The turned picture is the size it is now, not at the next draw: what
		// the edges and the touches of this frame look at.
		size = pictureSize();
	}

	bool Object::advanceAnimation()
	{
		// One picture moves on only to end, when it plays once.
		if (animationBitmaps.size() < (animationOnce ? 1u : 2u) || animationFrames < 1 || animationDone) { return false; }

		const int due = animationTimes.empty() ? animationFrames : animationTimes[animationIndex];
		if (++animationTick < due) { return false; }

		animationTick = 0;
		if (animationOnce && animationIndex + 1 == animationBitmaps.size())
		{
			// Played through: a look goes back to the one before, an
			// object's one animation stays on its last picture.
			const std::size_t next = looks.empty() ? 0 : looks[look].thenLook.value_or(lookBefore);
			if (!looks.empty() && next != look && next < looks.size()) { showLook(next); }
			else { animationDone = true; }
			return true;
		}
		animationIndex = (animationIndex + 1) % animationBitmaps.size();
		showAnimationFrame();
		return false;
	}

	void Object::restartAnimation()
	{
		animationTick = 0;
		animationDone = false;

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

	const Command& shapeOf(const Command& command) noexcept
	{
		if (const auto* live = std::get_if<CmdLive>(&command))
		{
			return live->loaded->command;
		}
		return command;
	}

	Vector2f Object::pictureSize() const
	{
		if (bitmap) { return { static_cast<float>(std::max(bitmap->width, 1)), static_cast<float>(std::max(bitmap->height, 1)) }; }
		// A circle or a rectangle is drawn on whole pixels, rounded up.
		const Vector2f exact = measureShapeSize(picture);
		if (picture.kind == ShapeKind::Circle || picture.kind == ShapeKind::Rectangle) { return { std::ceil(exact.x), std::ceil(exact.y) }; }
		return exact;
	}

	Vector2f measureShapeSize(const Picture& picture)
	{
		switch (picture.kind)
		{
		case ShapeKind::Circle:
			return { picture.radius * 2.0f, picture.radius * 2.0f };
		case ShapeKind::Rectangle:
		case ShapeKind::Line:
			return picture.size;
		case ShapeKind::Text:
		{
			const Bitmap drawn = drawText(picture.text, picture.textSize, colorFromName(picture.color), textFontFile(picture.font));
			return { static_cast<float>(drawn.width), static_cast<float>(drawn.height) };
		}
		case ShapeKind::Image:
		{
			const std::shared_ptr<const Bitmap> file = loadPicture(picture.file);
			return { static_cast<float>(file->width), static_cast<float>(file->height) };
		}
		case ShapeKind::Unknown:
		default:
			return {};
		}
	}

	std::string formatDisplayNumber(float value)
	{
		if (value == std::floor(value))
		{
			return std::to_string(static_cast<long long>(value));
		}
		return std::to_string(value);
	}
}
