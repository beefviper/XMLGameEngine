// object.h
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

#include <SFML/Graphics.hpp>

#include "utils.h"

namespace xge
{
	struct CollisionData
	{
		std::string top;
		std::string bottom;
		std::string left;
		std::string right;
		std::string default;
	};

	struct Object
	{
		void init(std::string name, std::string src, std::map<std::string, std::string> action);

		std::string name;
		std::string src;
		std::map<std::string, std::string> action;

		sf::Vector2f position;
		sf::Vector2f velocity;
		bool collision{ false };
		std::string scollisionData;
		CollisionData collisionData;
		std::vector<float> params;
		std::vector<std::string> sparams;
		std::shared_ptr<sf::RenderTexture> renderTexture;
		std::shared_ptr<sf::Sprite> sprite;


		friend std::ostream& operator<<(std::ostream& o, xge::Object const& f) {
			o << "object: " << "name=" << f.name << ", src=" << f.src << '\n'
				<< "\tpos.x=" << f.position.x << ", pos.y=" << f.position.y << '\n'
				<< "\tvel.x=" << f.velocity.x << ", vel.y=" << f.velocity.y << '\n'
				<< "\tcollision=" << f.collision << '\n'
				<< "\tscollisionData=" << f.scollisionData << '\n';

			for (auto& action : f.action)
			{
				o << "\taction=" << action.first << ", value=" << action.second << '\n';
			}

			return o;
		}
	};

	struct SObject
	{
		std::string name;
		std::string ssrc;
		xge::Vector2str sposition;
		xge::Vector2str svelocity;
		std::string scollision;
		std::string scollisiondata;
	};
}
