// null.cpp
// XML Game Engine
// author: beefviper
// date: Nov 10, 2026

#include "window/null.h"

namespace xge
{
	NullWindow::NullWindow(const WindowDesc& windowDesc)
	{
		(void)windowDesc; // its size is the game's, and nothing is drawn at it
	}

	void NullWindow::init(std::vector<Object>& objects)
	{
		for (Object& object : objects) { prepare(object); }
	}
}
