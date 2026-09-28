// window.cpp
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#include "window.h"
#include "window_sfml.h"

namespace xge
{
	std::unique_ptr<Window> WindowFactory::create(const WindowDesc& windowDesc)
	{
		// The only backend today. A future backend (SDL2 was the other
		// candidate discussed for the engine rewrite) would only need a
		// branch here - e.g. on a WindowDesc/XML-level "backend" setting -
		// not any change to Engine or anything else calling this.
		return std::make_unique<SFMLWindow>(windowDesc);
	}
}
