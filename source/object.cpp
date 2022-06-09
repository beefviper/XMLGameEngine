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
			<< "\tcollision=" << (f.rawCollisionData.enabled ? "true" : "false")
			<< (f.rawCollisionData.top.length() ? ", top=" + f.rawCollisionData.top : "")
			<< (f.rawCollisionData.bottom.length() ? ", bottom=" + f.rawCollisionData.bottom : "")
			<< (f.rawCollisionData.left.length() ? ", left=" + f.rawCollisionData.left : "")
			<< (f.rawCollisionData.right.length() ? ", right=" + f.rawCollisionData.right : "")
			<< (f.rawCollisionData.basic.length() ? ", basic=" + f.rawCollisionData.basic : "") << '\n';

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
		o << "Object: " << "name=" << f.name << ", src=" << f.src << '\n'
			<< "\tpos.x=" << f.position.x << ", pos.y=" << f.position.y << '\n'
			<< "\tvel.x=" << f.velocity.x << ", vel.y=" << f.velocity.y << '\n'
			<< "\tcollision=" << (f.collisionData.enabled ? "true" : "false");


		auto printCollisionData = [&](const std::vector<std::string>& params, std::string edge)
		{
			if (params.size() > 0)
			{
				o << ", " << edge << "=";
				for (auto &param : params)
				{
					o << param << (&param != &params.back() ? ";" : "");
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
		printCollisionData(f.collisionData.basic, "basic");

		o << '\n';

		for (auto& action : f.action)
		{
			o << "\taction=" << action.first << ", value=" << action.second.at(0) << ( action.second.size() > 1 ? action.second.at(1) : "") << '\n';
		}

		return o;
	}
}
