// game_sfml.h
// XML Game Engine
// author: beefviper
// date: Feb 23, 2021

#pragma once

#include "utils.h"
#include "object.h"

#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>

#include <iostream>
#include <memory>
#include <cmath>
#include <map>
#include <string>

namespace xge
{
	class game_sfml
	{
	public:
		void init(std::vector<Object>& preObjects, std::vector<Object>& objects);
		void updateTextIncrementValue(Object& object);

	private:
		void createCircle(Object& object);
		void createRectangle(Object& object);
		void createText(Object& object);
		void createImage(Object& object);
	};
}