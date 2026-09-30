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
		o << "rawObject: " << "name=" << f.name << ", src=" << f.src << '\n'
			<< "\tpos.x=" << f.rawPosition.x << ", pos.y=" << f.rawPosition.y << '\n'
			<< "\tvel.x=" << f.rawVelocity.x << ", vel.y=" << f.rawVelocity.y << '\n'

			// TODO: replace with operator<< for rawCollisionData
			<< "\tcollision=" << (f.rawCollisionData.enabled ? "true" : "false")
			<< (f.rawCollisionData.top.length() ? ", top=" + f.rawCollisionData.top : "")
			<< (f.rawCollisionData.bottom.length() ? ", bottom=" + f.rawCollisionData.bottom : "")
			<< (f.rawCollisionData.left.length() ? ", left=" + f.rawCollisionData.left : "")
			<< (f.rawCollisionData.right.length() ? ", right=" + f.rawCollisionData.right : "");

		for (auto& rawRule : f.rawCollisionData.basic)
		{
			o << ", basic=" << rawRule.action;
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
			o << "\taction=" << varible.first << ", value=" << varible.second << '\n';
		}

		return o;
	}

	std::ostream& operator<<(std::ostream& o, const Object& f) {
		o << "Object: " << "name=" << f.name << ", src=" << f.src << '\n';

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

		if (f.collisionData.group > 0)
		{
			o << ", group=" << f.collisionData.group;
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
