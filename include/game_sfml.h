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

namespace xge
{
	class game_sfml
	{
	public:
		void init(std::vector<Object>& objects);
		void updateSFMLTextIncrementValue(Object& object);
	};
}