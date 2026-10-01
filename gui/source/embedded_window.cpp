// embedded_window.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "embedded_window.h"

#include "game_stage.h"

namespace xge
{
	EmbeddedWindow::EmbeddedWindow(std::unique_ptr<Window> inner, GameStage& stage) :
		inner(std::move(inner)),
		stage(stage)
	{
	}

	bool EmbeddedWindow::isOpen() const
	{
		return inner->isOpen();
	}

	void EmbeddedWindow::close()
	{
		inner->close();
	}

	void EmbeddedWindow::init(std::vector<Object>& objects)
	{
		inner->activate();
		inner->init(objects);
	}

	std::vector<std::pair<KeyCode, bool>> EmbeddedWindow::pollEvents()
	{
		inner->activate();

		// The library still has to pump its own events (it never sees a key
		// here, but a close request would arrive that way); what it reports is
		// let go in favour of what Qt saw.
		static_cast<void>(inner->pollEvents());

		return stage.takeKeyEvents();
	}

	void EmbeddedWindow::clear(const std::string& colorName)
	{
		// Qt may have painted since the last frame.
		inner->activate();
		inner->clear(colorName);
	}

	void EmbeddedWindow::draw(Object& object)
	{
		inner->draw(object);
	}

	void EmbeddedWindow::display()
	{
		inner->display();

		if (const Bitmap* frame = inner->backBuffer())
		{
			stage.view().showFrame(*frame);
		}
	}

	const Bitmap* EmbeddedWindow::backBuffer() const
	{
		return inner->backBuffer();
	}

	void EmbeddedWindow::activate()
	{
		inner->activate();
	}
}
