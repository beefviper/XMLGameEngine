// object.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "game/object.h"
#include "game/parts.h"

#include "picture/color.h"
#include "picture/font.h"
#include "picture/image.h"
#include "picture/shapes.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

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

	std::vector<std::string> Object::hitboxNames() const
	{
		std::vector<std::string> names;
		const auto add = [&](const std::vector<Hitbox>& boxes)
		{
			for (const Hitbox& hitbox : boxes)
			{
				if (!hitbox.name.empty() && std::find(names.begin(), names.end(), hitbox.name) == names.end()) { names.push_back(hitbox.name); }
			}
		};
		add(hitboxes());
		for (const std::vector<Hitbox>& frame : animation.frames.shows().hitboxes) { add(frame); }
		for (const Look& each : looks.all())
		{
			for (const std::vector<Hitbox>& frame : each.frames.shows().hitboxes) { add(frame); }
		}
		return names;
	}

	const std::string& Object::lookName() const
	{
		static const std::string none;
		return looks.all().empty() ? none : looks.all()[looks.shown].name;
	}

	void Object::showLook(std::size_t index)
	{
		if (index >= looks.all().size())
		{
			return;
		}

		// The looks.shown to go back to when a looks.shown that plays once ends.
		if (index != looks.shown && looks.shown < looks.all().size() && !looks.all()[looks.shown].frames.shows().once) { animation.lookBefore = looks.shown; }
		looks.shown = index;
		drawn.picture = looks.all()[index].picture;
		drawn.bitmap = facingPicture(looks.all()[index].bitmap);
		animation.frames = looks.all()[index].frames;
		animation.done = false;
		animation.elapsed = 0.0;
		animation.index = 0;
		// Turned to the way it faces: its own pictures to turn from now on.
		if (!looks.all()[index].turnables.empty())
		{
			drawn.turnables = looks.all()[index].turnables;
			drawn.turnedDegrees = -1;
			showHeading();
		}
		drawn.visualDirty = true;
		// The new look is the size it is now, not at the next draw: what the
		// edges, touches and card layout of this frame look at.
		size = pictureSize();
	}

	void Object::wearPalette(std::size_t index)
	{
		if (index >= looks.set->palettes.size() || index == looks.worn)
		{
			return;
		}

		looks.worn = index;
		const Palette& worn = looks.set->palettes[index];
		const double elapsed = animation.elapsed;
		const std::size_t at = animation.index;
		if (looks.all().empty())
		{
			animation.frames.edit().bitmaps = worn.animation;
			drawn.bitmap = facingPicture(worn.bitmap);
		}
		else
		{
			showLook(looks.shown);
		}
		if (!animation.frames.shows().bitmaps.empty())
		{
			animation.elapsed = elapsed;
			animation.index = at % animation.frames.shows().bitmaps.size();
			showAnimationFrame();
		}
		drawn.visualDirty = true;
	}

	PictureId Object::facingPicture(PictureId shown) const
	{
		if (!mirror.flipped || !looks.set->flips || !shown) { return shown; }
		const auto found = looks.set->flips->find(shown);
		return found == looks.set->flips->end() ? shown : found->second;
	}

	void Object::faceMirror()
	{
		if (!mirror.on) { return; }
		const bool want = mirror.goingLeft == (mirror.way == Direction::Left);
		if (want == mirror.flipped) { return; }

		// Flipping twice is no flip, so what shows now flips back the same way.
		mirror.flipped = want;
		if (drawn.bitmap && looks.set->flips)
		{
			const auto found = looks.set->flips->find(drawn.bitmap);
			if (found != looks.set->flips->end()) { drawn.bitmap = found->second; }
		}
		drawn.visualDirty = true;
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

	bool startOnMap(RawObject& object, const RawMap& map, char character)
	{
		const auto tiles = tilesOf(map, character);
		if (tiles.size() == 1)
		{
			placeOnMap(object, map, tiles.front().first, tiles.front().second, character);
			return true;
		}
		const bool sideBySide = tiles.size() == 2 && tiles[0].second == tiles[1].second && tiles[1].first == tiles[0].first + 1;
		if (sideBySide && !map.cells)
		{
			placeOnMap(object, map, tiles[0].first, tiles[0].second, character);
			object.cell.between = true;
			return true;
		}
		return false;
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
					if (!sprite.scale && style->scale) { sprite.scale = style->scale; }
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
			look.frameHitboxes = animation.frameHitboxes;
			look.frameCommands = animation.frameCommands;
			look.once = animation.once;
			look.stepped = animation.stepped;
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

	namespace
	{
		// A holder's live number; for an object that is no holder, 0 (and
		// what is set on it is let go).
		float& holderNumber(Parts& parts, Object& object, float Holder::* number)
		{
			if (Holder* holder = parts.holders.find(object.id)) { return holder->*number; }
			thread_local float none;
			none = 0.0f;
			return none;
		}

		// Its cell on a map of cells; for an object with no cell part, 0
		// (and what is set on it is let go, as it is on no map of cells).
		float& placeNumber(Parts& parts, Object& object, float Object::Cells::Place::* number)
		{
			if (Object::Cells* cells = parts.cells.find(object.id)) { return cells->place.*number; }
			thread_local float none;
			none = 0.0f;
			return none;
		}
		bool hasPlace(const Parts& parts, const Object& object) { return parts.cells.has(object.id); }
		bool isHolder(const Parts& parts, const Object& object) { return parts.holders.has(object.id); }
		bool hasMoves(const Parts& parts, const Object& object) { return parts.moves.has(object.id); }
		float& movesNumber(Parts& parts, Object& object)
		{
			if (Object::Moves* moves = parts.moves.find(object.id)) { return moves->count; }
			thread_local float none;
			none = 0.0f;
			return none;
		}
	}

	const std::vector<LivePart>& liveParts()
	{
		static const std::vector<LivePart> parts{
			{ "x", [](Parts&, Object& o) -> float& { return o.position.x; }, false, nullptr },
			{ "y", [](Parts&, Object& o) -> float& { return o.position.y; }, false, nullptr },
			{ "velocity.x", [](Parts&, Object& o) -> float& { return o.velocity.x; }, false, nullptr },
			{ "velocity.y", [](Parts&, Object& o) -> float& { return o.velocity.y; }, false, nullptr },
			{ "heading", [](Parts&, Object& o) -> float& { return o.heading; }, false, nullptr },
			{ "altitude", [](Parts&, Object& o) -> float& { return o.altitude; }, true, nullptr },
			{ "rise", [](Parts&, Object& o) -> float& { return o.rise; }, false, nullptr },
			{ "light", [](Parts&, Object& o) -> float& { return o.light; }, true, nullptr },
			{ "blink", [](Parts&, Object& o) -> float& { return o.blink; }, true, nullptr },
			{ "speed", [](Parts&, Object& o) -> float& { if (!o.motion.speedHeld) { o.motion.speed = std::hypot(o.velocity.x, o.velocity.y); } return o.motion.speed; }, false, nullptr },
			{ "column", [](Parts& p, Object& o) -> float& { return placeNumber(p, o, &Object::Cells::Place::column); }, false, hasPlace },
			{ "row", [](Parts& p, Object& o) -> float& { return placeNumber(p, o, &Object::Cells::Place::row); }, false, hasPlace },
			{ "moves", [](Parts& p, Object& o) -> float& { return movesNumber(p, o); }, true, hasMoves },
			{ "cards", [](Parts& p, Object& o) -> float& { return holderNumber(p, o, &Holder::itemCount); }, true, isHolder },
			{ "held", [](Parts& p, Object& o) -> float& { return holderNumber(p, o, &Holder::itemCount); }, true, isHolder },
			{ "calling", [](Parts& p, Object& o) -> float& { return holderNumber(p, o, &Holder::calling); }, true, isHolder },
			{ "called", [](Parts& p, Object& o) -> float& { return holderNumber(p, o, &Holder::called); }, true, isHolder },
			{ "points", [](Parts& p, Object& o) -> float& { return holderNumber(p, o, &Holder::points); }, true, isHolder },
			{ "pattern", [](Parts& p, Object& o) -> float& { return holderNumber(p, o, &Holder::pattern); }, true, isHolder },
			{ "strength", [](Parts& p, Object& o) -> float& { return holderNumber(p, o, &Holder::strength); }, true, isHolder },
			{ "odds", [](Parts& p, Object& o) -> float& { return holderNumber(p, o, &Holder::odds); }, true, isHolder },
			{ "lead", [](Parts& p, Object& o) -> float& { return holderNumber(p, o, &Holder::lead); }, true, isHolder },
			{ "winner", [](Parts& p, Object& o) -> float& { return holderNumber(p, o, &Holder::winner); }, true, isHolder },
			{ "tricks", [](Parts& p, Object& o) -> float& { return holderNumber(p, o, &Holder::tricks); }, true, isHolder },
			{ "deadwood", [](Parts& p, Object& o) -> float& { return holderNumber(p, o, &Holder::deadwood); }, true, isHolder },
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

	std::optional<float> numberOf(const Parts& parts, const Object& object, const std::string& name)
	{
		if (const auto own = object.variable.find(name); own != object.variable.end()) { return own->second; }
		const LivePart* part = livePartNamed(name);
		if (!part || (name == "heading" && !object.turning.hasHeading)) { return std::nullopt; }
		return part->of(const_cast<Parts&>(parts), const_cast<Object&>(object)); // only read
	}

	bool Object::Follows::has(const std::string& part) const
	{
		return std::any_of(each.begin(), each.end(), [&](const Follow& one)
		{
			switch (one.part)
			{
			case Follow::Part::X: return part == "x";
			case Follow::Part::Y: return part == "y";
			case Follow::Part::Heading: return part == "heading";
			case Follow::Part::Variable: return part == one.variable;
			case Follow::Part::Seconds: return false;
			}
			return false;
		});
	}

	bool renameStart(std::string& text, const std::string& from, const std::string& to)
	{
		const auto word = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '.'; };
		// A name that starts with `from` and a dot, at the start of a word.
		const auto startsHere = [&](std::size_t i)
		{
			return text.compare(i, from.size(), from) == 0 && i + from.size() < text.size() && text[i + from.size()] == '.';
		};
		std::size_t found = text.find(from);
		while (found != std::string::npos && (!startsHere(found) || (found > 0 && word(text[found - 1])))) { found = text.find(from, found + 1); }
		if (found == std::string::npos) { return false; }

		std::string out;
		out.reserve(text.size() + to.size());
		std::size_t i = 0;
		while (i < text.size())
		{
			if (!word(text[i])) { out += text[i++]; continue; }
			std::size_t end = i;
			while (end < text.size() && word(text[end])) { ++end; }
			if (end - i > from.size() && startsHere(i)) { out.append(to).append(text, i + from.size(), end - i - from.size()); }
			else { out.append(text, i, end - i); }
			i = end;
		}
		if (out == text) { return false; }
		text = std::move(out);
		return true;
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
			changed = renameStart(*text, from, to) || changed;
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
		for (RawCommand& command : list) { changed = nameSelfIn(command, name) || changed; }
		return changed;
	}

	void nameSelfInTests(std::vector<RawCommand>& list)
	{
		for (RawCommand& command : list)
		{
			if (command.test)
			{
				auto test = std::make_shared<RawCondition>(*command.test);
				nameSelfInCheck(*test, "self");
				nameSelfInTests(test->commands);
				command.test = std::move(test);
			}
		}
	}

	bool nameSelfIn(RawCommand& command, const std::string& name)
	{
		bool changed = false;
		{
			if (command.verb == "trigger" && command.object.empty()) { command.object = name; changed = true; }
			for (std::string* text : { &command.variable, &command.into })
			{
				changed = renameStart(*text, "self", name) || changed;
			}
			for (RawValue* each : { &command.amount, &command.seconds, &command.distance, &command.ahead,
				&command.positionX, &command.positionY, &command.velocityX, &command.velocityY, &command.accelerationX, &command.accelerationY, &command.column, &command.row, &command.member })
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

	void nameSelfInTest(RawCondition& condition, const std::string& name)
	{
		nameSelfInCheck(condition, name);
	}

	bool namesSelf(const RawCommand& command)
	{
		RawCommand probe = command;
		return nameSelfIn(probe, "self_");
	}

	namespace
	{
		// `from.` at the start of a name in a value made `to.`; whether any was.
		bool renameObjectInValue(RawValue& each, const std::string& from, const std::string& to)
		{
			bool changed = false;
			for (std::string* text : { &each.text, &each.offset, &each.min, &each.max })
			{
				changed = renameStart(*text, from, to) || changed;
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
			changed = renameStart(condition.variableName, from, to) || changed;
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
				changed = renameStart(*text, from, to) || changed;
			}
			for (RawValue* each : { &command.amount, &command.seconds, &command.arc, &command.distance, &command.ahead, &command.positionX, &command.positionY,
				&command.velocityX, &command.velocityY, &command.accelerationX, &command.accelerationY, &command.column, &command.row, &command.below, &command.member })
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

	bool namesOther(const RawCommand& command)
	{
		std::vector<RawCommand> probe{ command };
		return renameObjectIn(probe, "other", "other_");
	}

	bool namesOther(const RawCondition& condition)
	{
		RawCondition probe = condition;
		return renameObjectInCheck(probe, "other", "other_");
	}

	void nameOtherInTest(RawCondition& condition, const std::string& name)
	{
		renameObjectInCheck(condition, "other", name);
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

				// What it can touch: of its class=, named by its object=, in
				// the same state (or one of them in none).
				for (const RawObject& touched : objects)
				{
					if (&touched == &object) { continue; }
					if (!rule.filterClass.empty() && touched.objClass != rule.filterClass) { continue; }
					if (!rule.filterObject.empty() && !touched.named(rule.filterObject)) { continue; }
					if (!touched.ownerState.empty() && !object.ownerState.empty() && touched.ownerState != object.ownerState) { continue; }
					rule.touchable.push_back(touched.name);
				}
				if (rule.touchable.empty())
				{
					const std::string about = !rule.filterObject.empty() ? " named \"" + rule.filterObject + "\"" : !rule.filterClass.empty() ? " of class \"" + rule.filterClass + "\"" : std::string();
					throw std::runtime_error("object '" + object.name + "' > <collision>: names other, the object it touched, but there is nothing" + about + " for it to touch");
				}
				rules.push_back(std::move(rule));
			}
			object.rawCollisionData.others = std::move(rules);
		}
	}

	void nameSelf(std::vector<RawObject>& objects)
	{
		for (RawObject& object : objects)
		{
			const std::function<void(RawValue&)> value = [&](RawValue& each) { nameSelfInValue(each, object.name); };
			// The commands of its rules, timers and actions keep self, found
			// as each runs (Game::makeNow); only their tests are made its name.
			const auto own = [&](std::vector<RawCommand>& list) { nameSelfInTests(list); };
			const auto commands = [&](std::vector<RawCommand>& list) { nameSelfIn(list, object.name); };
			// A rule's test says self too, found as it is tested (Game::holds).
			const auto check = [&](RawCondition& condition) { nameSelfInCheck(condition, "self"); };

			RawCollisionData& collisions = object.rawCollisionData;
			for (auto& edge : collisions.edges)
			{
				own(edge.commands);
				for (EdgeGuard& guard : edge.guards) { if (guard.rawCheck.join != RawCondition::Join::None) { check(guard.rawCheck); } }
			}
			for (RawCollisionRule& rule : collisions.others)
			{
				own(rule.commands);
				if (rule.check.join != RawCondition::Join::None) { check(rule.check); }
			}
			for (RawTimer& timer : object.timers) { own(timer.commands); }
			for (auto& [name, list] : object.action) { own(list); }
			// Its animations: their live seconds, what a picture does as it
			// comes up and what an animation that ends does then.
			const auto animation = [&](RawValue& interval, std::vector<RawValue>& times, std::vector<std::vector<RawCommand>>& frames, std::vector<RawCommand>& then)
			{
				value(interval);
				for (RawValue& time : times) { value(time); }
				for (std::vector<RawCommand>& frame : frames) { commands(frame); }
				commands(then);
			};
			animation(object.animation.interval, object.animation.times, object.animation.frameCommands, object.animation.thenCommands);
			for (RawSprite& look : object.looks) { animation(look.interval, look.times, look.frameCommands, look.thenCommands); }

			// Its own numbers, worked out once or live (update="live"):
			// self in them is this object too.
			for (auto& each : object.variable) { value(each.second); }
			for (RawValue* half : { &object.rawPosition.x, &object.rawPosition.y }) { value(*half); }
			if (object.rawHeading) { value(*object.rawHeading); }
			// The numbers its texts show (<number>self.value</number>).
			for (RawTextPiece& piece : object.sprite.text) { value(piece.value); }
			for (RawSprite& look : object.looks)
			{
				for (RawTextPiece& piece : look.text) { value(piece.value); }
			}
		}
	}

	Renamed renameOwn(RawObject& object, const std::string& from, const std::string& to)
	{
		bool said = false;
		const auto value = [&](RawValue& each) { said = renameInValue(each, from, to) || said; };
		const auto exact = [&](std::string& each) { if (each == from) { each = to; said = true; } };
		const auto start = [&](std::string& each)
		{
			said = renameStart(each, from, to) || said;
		};
		const std::function<void(RawCondition&)> check = [&](RawCondition& condition)
		{
			for (RawCondition& part : condition.parts) { check(part); }
			exact(condition.filterObject);
			exact(condition.distance);
			exact(condition.sees);
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
				if (command.verb == "trigger") { exact(command.object); }
				start(command.variable);
				start(command.into);
				for (RawValue* each : { &command.amount, &command.seconds, &command.distance, &command.ahead,
					&command.positionX, &command.positionY, &command.velocityX, &command.velocityY, &command.accelerationX, &command.accelerationY, &command.column, &command.row, &command.member })
				{
					value(*each);
				}
			}
		};

		object.name = to;
		for (RawValue* each : { &object.rawPosition.x, &object.rawPosition.y, &object.rawVelocity.x, &object.rawVelocity.y, &object.rawSeek,
			&object.rawAltitude, &object.rawRise, &object.rawFall, &object.rawAltitudeBounce })
		{
			value(*each);
		}
		for (std::optional<RawVector2>* each : { &object.rawAcceleration, &object.rawPivot })
		{
			if (*each) { value((*each)->x); value((*each)->y); }
		}
		for (std::optional<RawValue>* each : { &object.rawHeading, &object.rawDrag, &object.rawFriction, &object.rawMass, &object.rawSurfaceDrag, &object.rawSurfaceGrip, &object.rawLight })
		{
			if (*each) { value(**each); }
		}
		for (auto& [name, each] : object.rawPulls) { value(each); }
		for (auto& [name, each] : object.variable) { value(each); }
		for (RawTextPiece& piece : object.sprite.text) { value(piece.value); }

		// Whether what its kind holds (its rules and actions) named it.
		said = false;
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
		for (auto& [actionName, list] : object.action) { commands(list); }
		for (RawTimer& timer : object.timers)
		{
			value(timer.interval);
			commands(timer.commands);
		}
		const bool kindNamedIt = said;
		said = false;
		const auto animation = [&](RawValue& interval, std::vector<RawValue>& times, std::vector<std::vector<RawCommand>>& frames, std::vector<RawCommand>& then)
		{
			value(interval);
			for (RawValue& time : times) { value(time); }
			for (std::vector<RawCommand>& frame : frames) { commands(frame); }
			commands(then);
		};
		animation(object.animation.interval, object.animation.times, object.animation.frameCommands, object.animation.thenCommands);
		for (RawSprite& look : object.looks) { animation(look.interval, look.times, look.frameCommands, look.thenCommands); }
		return { kindNamedIt, said };
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
		for (std::size_t i = 0; i < object.looks.set->palettes.size(); ++i)
		{
			if (object.looks.set->palettes[i].name == name)
			{
				object.wearPalette(i);
				return true;
			}
		}
		return false;
	}

	bool showLookNamed(Parts& parts, Object& object, const std::string& name)
	{
		for (std::size_t i = 0; i < object.looks.all().size(); ++i)
		{
			if (object.looks.all()[i].name == name)
			{
				// A look that plays once starts again from its first frame,
				// even when it is showing already: a second blast is a whole one.
				if (object.looks.shown != i || object.looks.all()[i].frames.shows().once) { object.showLook(i); }
				// A timer that follows this look starts over, even when the
				// object showed it already: blue again is six seconds again.
				if (std::vector<Timer::Clock>* clocks = parts.clocks.find(object.id))
				{
					for (std::size_t t = 0; t < clocks->size() && t < object.kind->timers.size(); ++t)
					{
						if (object.kind->timers[t].sprite == name) { (*clocks)[t] = {}; }
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
		const Vector2f shown = drawn.bitmap ? Vector2f{ static_cast<float>(drawn.bitmap->width), static_cast<float>(drawn.bitmap->height) } : size;
		const Vector2f middle = position + shown * 0.5f;
		return turning.hasPivot ? middle + turnedBy(turning.pivotFromMiddle, static_cast<float>(pivotDegrees)) : middle;
	}

	Vector2f Object::velocityAt(const Vector2f& at) const
	{
		if (turning.spin == 0.0f) { return velocity; }
		const Vector2f arm = at - pivotOnScreen();
		// Its turn over the step, as a speed of turning, degrees a second.
		const float radians = (turning.spinOver > 0.0f ? turning.spin / turning.spinOver : 0.0f) * 3.14159265358979323846f / 180.0f;
		return velocity + Vector2f{ -arm.y * radians, arm.x * radians };
	}

	void Object::showHeading()
	{
		if (drawn.turnables.empty()) { return; }

		// An animated object that turns has a turnable for each of its
		// pictures; the one that is showing is the one to turn.
		const std::size_t frame = drawn.turnables.size() > 1 && animation.index < drawn.turnables.size() ? animation.index : 0;

		const int degrees = wholeDegrees(heading);

		// Turning about a pivot: the picture is turned about its middle, so the
		// middle goes round the pivot, which stays where it is.
		if (turning.hasPivot && degrees != pivotDegrees)
		{
			position += turnedBy(turning.pivotFromMiddle, static_cast<float>(pivotDegrees)) - turnedBy(turning.pivotFromMiddle, static_cast<float>(degrees));
			pivotDegrees = degrees;
		}

		if (drawn.bitmap && drawn.turnedDegrees == degrees && drawn.turnedFrame == frame) { return; }

		drawn.bitmap = drawn.turnables[frame]->turned(degrees);
		drawn.turnedDegrees = degrees;
		drawn.turnedFrame = frame;
		drawn.visualDirty = true;
		// The turned picture is the size it is now, not at the next draw: what
		// the edges and the touches of this frame look at.
		size = pictureSize();
	}

	Object::AnimationStep Object::advanceAnimation(float& owed)
	{
		// One picture moves on only to end, when it plays once.
		if (animation.frames.shows().bitmaps.size() < (animation.frames.shows().once ? 1u : 2u) || animation.done)
		{
			owed = 0.0f;
			return AnimationStep::Stayed;
		}

		const double due = static_cast<double>(animation.frames.shows().times.empty() ? animation.frames.shows().seconds : animation.frames.shows().times[animation.index]);
		animation.elapsed += static_cast<double>(owed);
		if (animation.elapsed < due - static_cast<double>(kTimeSlack))
		{
			owed = 0.0f;
			return AnimationStep::Stayed;
		}

		// Its time is up: what the step had beyond it goes to the next. A
		// time of 0 or less (a live one reading 0) is a picture a step.
		owed = due > 0.0 ? static_cast<float>(std::max(0.0, animation.elapsed - due)) : 0.0f;
		animation.elapsed = 0.0;
		if (animation.frames.shows().once && animation.index + 1 == animation.frames.shows().bitmaps.size())
		{
			// Played through: a look goes back to the one before, an
			// object's one animation stays on its last picture.
			const std::size_t next = looks.all().empty() ? 0 : looks.all()[looks.shown].thenLook.value_or(animation.lookBefore);
			if (!looks.all().empty() && next != looks.shown && next < looks.all().size()) { showLook(next); }
			else { animation.done = true; }
			return AnimationStep::Ended;
		}
		animation.index = (animation.index + 1) % animation.frames.shows().bitmaps.size();
		showAnimationFrame();
		return AnimationStep::Moved;
	}

	void Object::timeAnimation(std::size_t look, std::size_t frame, float seconds)
	{
		const auto time = [&](float& every, std::vector<float>& times)
		{
			if (frame == std::string::npos) { every = seconds; }
			else if (frame < times.size()) { times[frame] = seconds; }
		};
		if (look == std::string::npos)
		{
			Frames& own = animation.frames.edit();
			time(own.seconds, own.times);
			return;
		}
		if (look >= looks.all().size()) { return; }
		Look& changed = looks.change()[look];
		// Showing it, the animation holds the same frames: let go of them
		// first, so they are changed in place, not copied each time.
		if (looks.shown == look) { animation.frames = {}; }
		Frames& timed = changed.frames.edit();
		time(timed.seconds, timed.times);
		if (looks.shown == look) { animation.frames = changed.frames; }
	}

	void Object::restartAnimation()
	{
		animation.elapsed = 0.0;
		animation.since = 0.0f;
		animation.share = 0.0f;
		animation.shareBefore = 0.0f;
		animation.done = false;

		if (animation.index == 0 || animation.frames.shows().bitmaps.empty()) { return; }

		animation.index = 0;
		showAnimationFrame();
	}

	void Object::showAnimationFrame()
	{
		if (!drawn.turnables.empty()) { showHeading(); }
		else { drawn.bitmap = facingPicture(animation.frames.shows().bitmaps[animation.index]); }
		drawn.visualDirty = true;
		size = pictureSize();
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
		if (drawn.bitmap) { return { static_cast<float>(std::max(drawn.bitmap->width, 1)), static_cast<float>(std::max(drawn.bitmap->height, 1)) }; }
		// A circle or a rectangle is drawn on whole pixels, rounded up.
		const Vector2f exact = measureShapeSize(drawn.picture);
		if (drawn.picture->kind == ShapeKind::Circle || drawn.picture->kind == ShapeKind::Rectangle) { return { std::ceil(exact.x), std::ceil(exact.y) }; }
		return exact;
	}

	bool Object::drawnAtScale() const noexcept
	{
		return drawn.picture->kind == ShapeKind::Text || drawn.picture->kind == ShapeKind::Circle || drawn.picture->kind == ShapeKind::Rectangle;
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
			// Measured by drawing it, so kept: a window asks every frame. One
			// cache a thread (a computer thinks on its own), emptied when full.
			thread_local std::unordered_map<std::string, Vector2f> measured;
			const std::string key = picture.text + '\n' + picture.font + '\n' + std::to_string(picture.textSize);
			if (const auto found = measured.find(key); found != measured.end()) { return found->second; }
			const Bitmap drawn = drawText(picture.text, picture.textSize, colorFromName(picture.color), textFontFile(picture.font));
			if (measured.size() >= 512) { measured.clear(); }
			return measured.emplace(key, Vector2f{ static_cast<float>(drawn.width), static_cast<float>(drawn.height) }).first->second;
		}
		case ShapeKind::Image:
			if (!picture.image) { return {}; }
			return { static_cast<float>(picture.image->width), static_cast<float>(picture.image->height) };
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
