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
		void init(std::vector<Object>& objects);
		void updateTextIncrementValue(Object& object);

		// Sets a text object's displayed number to an explicit value and
		// re-renders it - the general form updateTextIncrementValue's "+1" is
		// built on top of, and what refreshes a HUD text object bound to
		// another object's <variable> (see Game::incrementText).
		void setDisplayedNumber(Object& object, float value);

	private:
		void createCircle(Object& object);
		void createRectangle(Object& object);
		void createText(Object& object);
		void createImage(Object& object);

		GridData setGridData(Object& object);
	};
}