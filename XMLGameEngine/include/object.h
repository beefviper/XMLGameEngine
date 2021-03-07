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
#include <ostream>

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

		friend std::ostream& operator<<(std::ostream& o, WindowDesc const& f);
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
		std::unique_ptr<sf::RenderTexture> renderTexture;
		std::unique_ptr<sf::Sprite> sprite;

		friend std::ostream& operator<<(std::ostream& o, Object const& f);
	};
}
