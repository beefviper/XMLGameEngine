// object.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "object.h"

namespace xge
{
	std::ostream& operator<<(std::ostream& o, WindowDesc const& f)
	{
		o << "window: ";
		o << "title=" << f.name;
		o << ", width=" << f.width;
		o << ", height=" << f.height << '\n';

		return o;
	}

	std::ostream& operator<<(std::ostream& o, Object const& f) {
		o << "object: " << "name=" << f.name << ", src=" << f.src << '\n'
			<< "\tpos.x=" << f.position.x << ", pos.y=" << f.position.y << '\n'
			<< "\tvel.x=" << f.velocity.x << ", vel.y=" << f.velocity.y << '\n'
			<< "\tcollision=" << (f.collision ? "true" : "false")
			<< (f.collisionData.top.length() ? ", top=" + f.collisionData.top : "")
			<< (f.collisionData.bottom.length() ? ", bottom=" + f.collisionData.bottom : "")
			<< (f.collisionData.left.length() ? ", left=" + f.collisionData.left : "")
			<< (f.collisionData.right.length() ? ", right=" + f.collisionData.right : "")
			<< (f.collisionData.basic.length() ? ", basic=" + f.collisionData.basic : "") << '\n';

		for (auto& action : f.action)
		{
			o << "\taction=" << action.first << ", value=" << action.second << '\n';
		}

		return o;
	}
}
