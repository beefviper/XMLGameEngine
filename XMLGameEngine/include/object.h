// object.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include <SFML/Graphics.hpp>

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace xge
{
	struct WindowDesc
	{
		std::string name;
		float width{};
		float height{};
		std::string background;
		std::string fullscreen;
		int framerate{};
	};

	struct CollisionData
	{
		std::string top;
		std::string bottom;
		std::string left;
		std::string right;
		std::string basic;
	};

	struct Vector2str
	{
		std::string x;
		std::string y;
	};

	struct Object
	{
		void init(const std::string& name, const std::string& src, const std::map<std::string, std::string>& action);

		std::string name;
		std::string src;
		std::map<std::string, std::string> action;

		Vector2str sposition;
		Vector2str svelocity;
		sf::Vector2f position;
		sf::Vector2f velocity;
		bool collision{ false };
		CollisionData collisionData;
		std::vector<float> params;
		std::vector<std::string> sparams;
		std::shared_ptr<sf::RenderTexture> renderTexture;
		std::shared_ptr<sf::Sprite> sprite;

		friend std::ostream& operator<<(std::ostream& o, Object const& f) {
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
	};
}
