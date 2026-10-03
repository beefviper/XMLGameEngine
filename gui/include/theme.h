// theme.h
// XML Game Engine
// author: beefviper
// date: Oct 3, 2026

#pragma once

class QApplication;

namespace xge
{
	// Gives every control of XGEGUI more to see than a flat, one-colored
	// default: the Fusion style (the same raised, shaded controls on every
	// platform, with sunken text and number boxes), a faint tint on every
	// other row of a list or tree, and buttons, column headings, the splitter
	// grip and the selection drawn with shading and borders. Every color is
	// worked out from the application's own palette, so it follows a light or a
	// dark theme. Call once, after the QApplication is made and before any
	// window is.
	void applyTheme(QApplication& app);
}
