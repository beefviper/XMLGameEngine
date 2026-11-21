// shapes.h
// XML Game Engine
// author: beefviper
// date: Nov 14, 2026

#pragma once

#include "picture/bitmap.h"
#include "picture/color.h"

namespace xge
{
	// A circle's and a rectangle's pictures, drawn by the engine so that
	// every window shows them alike and a window holds no drawing code of its
	// own. `scale` is how many pixels of the bitmap go to one of the game's:
	// a window drawn bigger than the game asks for its own scale, so the
	// edges stay sharp (see Window::pictureScale).
	//
	// A circle's bitmap is its diameter times the scale, rounded up, with the
	// edge smoothed; a rectangle's is its size times the scale, rounded up,
	// filled to the edges.
	Bitmap drawCircle(float radius, const Color& color, float scale = 1.0f);
	Bitmap drawRectangle(float width, float height, const Color& color, float scale = 1.0f);
}
