// qt_window.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include "window/window.h"

#include <QImage>
#include <QPainter>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace xge
{
	class GameView;

	// The Window backend for the Qt application: draws every object into the
	// GameView's picture with QPainter, and reads its keys from the view. It is
	// the one backend that does not open a window of its own - the view is
	// already part of the application - so it is built in xgegui, not in
	// xgelib, and the engine library needs no Qt. It needs no other library
	// either, so it is also what the application falls back to when the one
	// chosen will not start.
	class QtWindow : public Window
	{
	public:
		explicit QtWindow(GameView& view);

		bool isOpen() const override;
		void close() override;
		void init(std::vector<Object>& objects) override;
		std::vector<std::pair<KeyCode, bool>> pollEvents() override;
		Controls controls() override;
		void clear(const std::string& colorName) override;
		void draw(Object& object) override;
		void display() override;
		float fit() const override;

	private:
		GameView& view;

		bool open{ true };

		// What an object shows: the image of its picture, which objects
		// showing the same picture share, asked again when its visualDirty
		// flag says it changed or the pictures' scale did; and that image
		// stretched to the screen pixels it covers, kept while they stay.
		struct Visual
		{
			QImage image;
			float drawnAt{ 0.0f };   // the pictureScale it was drawn at
			QImage sized;
			qint64 sizedFrom{ 0 };   // the image's cacheKey it was stretched from
		};
		Textures<QImage> images{ *this };
		std::unordered_map<Name, Visual> visuals;  // by object name

		// A frame: its background, then each picture at the frame's pixels.
		// draw() only notes what to draw; display() paints it, unless it is
		// the frame before over again (a board waiting for a move).
		struct Placed
		{
			QImage image;
			QRect where;
			bool operator==(const Placed& other) const { return where == other.where && image.cacheKey() == other.image.cacheKey(); }
		};
		struct Frame
		{
			QColor background;
			double scale{ 0.0 };
			std::vector<Placed> placed;
			bool operator==(const Frame& other) const { return background == other.background && scale == other.scale && placed == other.placed; }
		};
		Frame frame;
		Frame painted;

		void build(Object& object, Visual& visual);
	};
}
