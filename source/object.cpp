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

		return o;
	}

	std::ostream& operator<<(std::ostream& o, const Object& f) {
		o << "Object: " << "name=" << f.name << ", src=" << f.src << '\n'
			<< "\tpos.x=" << f.position.x << ", pos.y=" << f.position.y << '\n'
			<< "\tvel.x=" << f.velocity.x << ", vel.y=" << f.velocity.y << '\n'
			<< "\tcollision=" << (f.collisionData.enabled ? "true" : "false");

		if (f.collisionData.top.size() > 0)
		{
			o << ", top=";
			for (auto param : f.collisionData.top)
			{
				o << param << (&param != &f.collisionData.top.back() ? ";" : "");
			}
		}

		if (f.collisionData.bottom.size() > 0)
		{
			o << ", top=";
			for (auto param : f.collisionData.bottom)
			{
				o << param << (&param != &f.collisionData.bottom.back() ? ";" : "");
			}
		}

		if (f.collisionData.left.size() > 0)
		{
			o << ", top=";
			for (auto param : f.collisionData.left)
			{
				o << param << (&param != &f.collisionData.left.back() ? ";" : "");
			}
		}

		if (f.collisionData.right.size() > 0)
		{
			o << ", top=";
			for (auto param : f.collisionData.right)
			{
				o << param << (&param != &f.collisionData.right.back() ? ";" : "");
			}
		}

		if (f.collisionData.basic.size() > 0)
		{
			o << ", top=";
			for (auto param : f.collisionData.basic)
			{
				o << param << (&param != &f.collisionData.basic.back() ? ";" : "");
			}
		}

		o << '\n';

		for (auto& action : f.action)
		{
			o << "\taction=" << action.first << ", value=" << action.second << '\n';
		}

		return o;
	}
}
