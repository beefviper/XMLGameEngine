// embedded_window.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "embedded_window.h"

#include "game_stage.h"

#include <QtGui/qtguiglobal.h>

#if QT_CONFIG(opengl)
#include <QOpenGLContext>
#include <QSurface>
#endif

namespace xge
{
	QtContextKeeper::QtContextKeeper()
	{
#if QT_CONFIG(opengl)
		context = QOpenGLContext::currentContext();
		surface = context ? context->surface() : nullptr;
#endif
	}

	QtContextKeeper::~QtContextKeeper()
	{
#if QT_CONFIG(opengl)
		if (context && surface)
		{
			context->makeCurrent(surface);
		}
#endif
	}

	EmbeddedWindow::EmbeddedWindow(std::unique_ptr<Window> inner, GameStage& stage) :
		inner(std::move(inner)),
		stage(stage)
	{
	}

	EmbeddedWindow::~EmbeddedWindow()
	{
		frameKeeper.reset();

		// The library frees its textures and closes its window in its own
		// context, not in whichever one Qt left current.
		QtContextKeeper keeper;
		inner->activate();
		inner.reset();
	}

	bool EmbeddedWindow::isOpen() const
	{
		return inner->isOpen();
	}

	void EmbeddedWindow::close()
	{
		QtContextKeeper keeper;
		inner->activate();
		inner->close();
	}

	void EmbeddedWindow::init(std::vector<Object>& objects)
	{
		QtContextKeeper keeper;
		inner->activate();
		inner->init(objects);
	}

	std::vector<std::pair<KeyCode, bool>> EmbeddedWindow::pollEvents()
	{
		{
			QtContextKeeper keeper;
			inner->activate();

			// The library still has to pump its own events (it never sees a key
			// here, but a close request would arrive that way); what it reports
			// is let go in favour of what Qt saw.
			static_cast<void>(inner->pollEvents());
		}

		return stage.takeKeyEvents();
	}

	void EmbeddedWindow::clear(const std::string& colorName)
	{
		// The frame starts: the library's context is current until display().
		frameKeeper.reset();
		frameKeeper = std::make_unique<QtContextKeeper>();

		try
		{
			inner->activate();
			inner->clear(colorName);
		}
		catch (...)
		{
			frameKeeper.reset();
			throw;
		}
	}

	void EmbeddedWindow::draw(Object& object)
	{
		// Normally inside a frame (after clear()); on its own, it keeps Qt's
		// context the same way the other calls do.
		std::unique_ptr<QtContextKeeper> keeper;
		if (!frameKeeper)
		{
			keeper = std::make_unique<QtContextKeeper>();
			inner->activate();
		}

		try
		{
			inner->draw(object);
		}
		catch (...)
		{
			frameKeeper.reset();
			throw;
		}
	}

	void EmbeddedWindow::display()
	{
		{
			// Takes over from the frame's keeper, if there was one, so Qt's
			// context comes back even if display() throws.
			std::unique_ptr<QtContextKeeper> keeper = std::move(frameKeeper);
			if (!keeper)
			{
				keeper = std::make_unique<QtContextKeeper>();
				inner->activate();
			}

			inner->display();
		}

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
