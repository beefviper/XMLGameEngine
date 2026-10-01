// qt_window.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include "window.h"

#include <QImage>

#include <map>
#include <string>

namespace xge
{
	class GameView;

	// The Window backend for the Qt application: draws every object into the
	// GameView's picture with QPainter, and reads its keys from the view. It is
	// the one backend that does not open a window of its own - the view is
	// already part of the application - so it is built in XGEGUI, not in
	// XGELIB, and the engine library needs no Qt. The SFML, raylib and SDL2
	// backends each want a window to themselves, which is why this one draws
	// the picture itself instead of embedding them.
	class QtWindow : public Window
	{
	public:
		explicit QtWindow(GameView& view);

		bool isOpen() const override;
		void close() override;
		void init(std::vector<Object>& objects) override;
		std::vector<std::pair<KeyCode, bool>> pollEvents() override;
		void clear(const std::string& colorName) override;
		void draw(Object& object) override;
		void display() override;

	private:
		GameView& view;
		bool open{ true };

		// Each object's picture, built once and again whenever the object's
		// visualDirty flag says it changed (same pattern as the other backends).
		std::map<std::string, QImage> visuals;

		// The name of the font loaded from assets/tuffy.ttf, empty if that
		// failed (text is then drawn with the built-in 8x8 font).
		QString fontFamily;
		bool fontLoaded{ false };

		void build(Object& object, QImage& visual);
		QImage buildCircle(const Object& object);
		QImage buildRectangle(const Object& object);
		QImage buildText(const Object& object);
		QImage buildImage(const Object& object);
		static QImage fromBitmap(const Bitmap& bitmap);
		void loadFont();
	};
}
