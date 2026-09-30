// object.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "object.h"

namespace xge
{
	std::ostream& operator<<(std::ostream& o, const WindowDesc& f)
	{
		o << "window: ";
		o << "title=" << f.name;
		o << ", width=" << f.width;
		o << ", height=" << f.height << '\n';

		return o;
	}

	std::ostream& operator<<(std::ostream& o, const RawObject& f) {
		o << "rawObject: " << "name=" << f.name << ", sprite=" << f.sprite.kind
			<< (f.sprite.isGrid ? " (grid)" : "") << '\n'
			<< "\tpos.x=" << f.rawPosition.x << ", pos.y=" << f.rawPosition.y << '\n'
			<< "\tvel.x=" << f.rawVelocity.x << ", vel.y=" << f.rawVelocity.y << '\n'

			// TODO: replace with operator<< for rawCollisionData
			<< "\tcollision=" << (f.rawCollisionData.enabled ? "true" : "false");

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

		if (f.sizeKnown)
		{
			o << "\tsize.x=" << f.size.x << ", size.y=" << f.size.y << '\n';
		}
		else
		{
			o << "\tsize= ( Unknown, not yet initialized by Engine )\n";
		}

		o << "\tcollision=" << (f.collisionData.enabled ? "true" : "false");


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
