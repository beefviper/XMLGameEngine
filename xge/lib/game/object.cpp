// object.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "game/object.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>

namespace xge
{
	std::string showPiece(RawTextPiece::Kind kind, float value)
	{
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
		bitmap = looks[index].bitmap;
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
			for (RawSprite& look : object.looks) { apply(look); }
			for (RawSprite& frame : object.animation.frames) { apply(frame); }
			apply(object.cell.slot);
		}
	}

	void nameSelf(std::vector<RawObject>& objects)
	{
		for (RawObject& object : objects)
		{
			const auto commands = [&](std::vector<RawCommand>& list)
			{
				for (RawCommand& command : list)
				{
					if (command.verb == "trigger" && command.object.empty()) { command.object = object.name; }
				}
			};
			const std::function<void(RawCondition&)> check = [&](RawCondition& condition)
			{
				for (RawCondition& part : condition.parts) { check(part); }
				if (condition.join == RawCondition::Join::None && condition.filterClass.empty() && condition.filterObject.empty())
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

	void Object::showAnimationFrame()
	{
		if (!turnables.empty()) { showHeading(); }
		else { bitmap = animationBitmaps[animationIndex]; }
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
