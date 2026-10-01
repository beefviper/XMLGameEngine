// embedded_window.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include "window.h"

#include <memory>

namespace xge
{
	class GameStage;

	// A window library's Window made to work inside the Qt application: it does
	// what the library's Window does and changes only what is Qt's to decide.
	//
	//   keys     come from Qt (the GameStage's KeyQueue) rather than from the
	//            library, whose window never has the keyboard focus here
	//   frames   a window that drew to a back buffer has each finished frame
	//            handed to the GameStage's view to show
	//   context  the library's graphics context is made current again before
	//            each frame, because Qt's own OpenGL widgets share the thread
	//            and leave theirs current
	class EmbeddedWindow : public Window
	{
	public:
		EmbeddedWindow(std::unique_ptr<Window> inner, GameStage& stage);

		bool isOpen() const override;
		void close() override;
		void init(std::vector<Object>& objects) override;
		std::vector<std::pair<KeyCode, bool>> pollEvents() override;
		void clear(const std::string& colorName) override;
		void draw(Object& object) override;
		void display() override;
		const Bitmap* backBuffer() const override;
		void activate() override;

	private:
		std::unique_ptr<Window> inner;
		GameStage& stage;
	};
}
