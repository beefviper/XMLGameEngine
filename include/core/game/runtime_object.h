// runtime_object.h
// XML Game Engine
// author: beefviper
// date: 20-Jul-26

#pragma once

#include "core/types.h"

#include <string>

namespace xge
{

struct Object
{
	std::string name;
	Vector2 position;
	Color color;
};

} // namespace xge
