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

	std::ostream& operator<<(std::ostream& o, const Object& f) {
		o << "object: " << "name=" << f.name << ", src=" << f.src << '\n'
			<< "\tpos.x=" << f.position.x << ", pos.y=" << f.position.y << '\n'
			<< "\tvel.x=" << f.velocity.x << ", vel.y=" << f.velocity.y << '\n'
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
}
