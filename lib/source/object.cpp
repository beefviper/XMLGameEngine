// object.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "object.h"

#include <cmath>

namespace xge
{
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

	bool showLookNamed(Object& object, const std::string& name)
	{
		for (std::size_t i = 0; i < object.looks.size(); ++i)
		{
			if (object.looks[i].name == name)
			{
				if (object.look != i) { object.showLook(i); }
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

		long degrees = std::lround(heading) % 360;
		if (degrees < 0) { degrees += 360; }

		if (bitmap && turnedDegrees == degrees && turnedFrame == frame) { return; }

		bitmap = std::make_shared<const Bitmap>(turnables[frame]->at(static_cast<float>(degrees)));
		turnedDegrees = static_cast<int>(degrees);
		turnedFrame = frame;
		visualDirty = true;
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
			<< (f.sprite.isGrid ? " (grid)" : "")
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

		for (auto& rawRule : f.rawCollisionData.basic)
		{
			o << ", basic=" << rawRule.commands;
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

		for (auto& rule : f.collisionData.basic)
		{
			o << ", basic=";
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
