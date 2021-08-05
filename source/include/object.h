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

	struct RawCollisionData
	{
		bool enabled{ false };
		std::string top;
		std::string bottom;
		std::string left;
		std::string right;
		std::string basic;
	};

	struct CollisionData
	{
		bool enabled{ false };
		std::vector<std::string> top;
		std::vector<std::string> bottom;
		std::vector<std::string> left;
		std::vector<std::string> right;
		std::vector<std::string> basic;
	};

	struct Vector2str
	{
		std::string x;
		std::string y;
	};

	struct Vector2i
	{
		int x = 0;
		int y = 0;
	};

	struct GridData
	{
		Vector2i max{ 1,1 };
		Vector2i padding{ 0,0 };
		Vector2i obj{ 0,0 };
	};

	struct RawObject
	{
		std::string name;
		std::string src;
		bool isVisible{ true };
		Vector2str rawPosition;
		Vector2str rawVelocity;
		RawCollisionData rawCollisionData;
		std::map<std::string, std::string> action;

		friend std::ostream& operator<<(std::ostream& o, RawObject const& f);
	};

	struct Object
	{
		std::string name;
		std::string src;
		bool isVisible{ true };
		sf::Vector2f position;
		sf::Vector2f positionOriginal;
		sf::Vector2f velocity;
		sf::Vector2f velocityOriginal;
		CollisionData collisionData;
		std::vector<std::string> spriteParams;
		std::map<std::string, std::string> action;
		std::unique_ptr<sf::RenderTexture> renderTexture = nullptr;
		std::unique_ptr<sf::Sprite> sprite = nullptr;

		friend std::ostream& operator<<(std::ostream& o, Object const& f);
	};
}
