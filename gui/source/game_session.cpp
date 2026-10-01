// game_session.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "game_session.h"

#include "game_view.h"
#include "qt_window.h"

#include <algorithm>
#include <exception>

namespace xge
{
	GameSession::GameSession(GameView& view, QObject* parent) :
		QObject(parent),
		view(view)
	{
		timer.setTimerType(Qt::PreciseTimer);
		connect(&timer, &QTimer::timeout, this, &GameSession::tick);
	}

	GameSession::~GameSession()
	{
		timer.stop();

		// The engine holds a reference to the game: it goes first.
		engine.reset();
		game.reset();
	}

	bool GameSession::load(const QString& file)
	{
		pause();

		emit aboutToUnload();
		engine.reset();
		game.reset();
		frameCount = 0;
		lastError.clear();

		try
		{
			game = std::make_unique<Game>(file.toStdString());

			const WindowDesc& desc = game->getWindowDesc();
			view.setGameSize(static_cast<int>(desc.width), static_cast<int>(desc.height));

			engine = std::make_unique<Engine>(*game, std::make_unique<QtWindow>(view));

			// The frame pace: the game moves a fixed amount a frame, so this is
			// also its speed.
			timer.setInterval(desc.framerate > 0 ? std::max(1, 1000 / desc.framerate) : 16);

			engine->render();
		}
		catch (const std::exception& e)
		{
			lastError = QString::fromUtf8(e.what());
			engine.reset();
			game.reset();
			emit loaded();
			return false;
		}

		emit loaded();
		play();
		return true;
	}

	void GameSession::play()
	{
		if (!engine || playing)
		{
			return;
		}

		playing = true;
		timer.start();
		emit playingChanged(true);
	}

	void GameSession::pause()
	{
		if (!playing)
		{
			return;
		}

		playing = false;
		timer.stop();
		emit playingChanged(false);
	}

	void GameSession::togglePlay()
	{
		if (playing)
		{
			pause();
		}
		else
		{
			play();
		}
	}

	void GameSession::step()
	{
		tick();
	}

	void GameSession::reset()
	{
		if (!engine)
		{
			return;
		}

		try
		{
			game->resetAll();
			frameCount = 0;
			engine->render();
		}
		catch (const std::exception& e)
		{
			fail(QString::fromUtf8(e.what()));
			return;
		}

		emit frameAdvanced();
	}

	void GameSession::redraw()
	{
		if (!engine)
		{
			return;
		}

		try
		{
			engine->render();
		}
		catch (const std::exception& e)
		{
			fail(QString::fromUtf8(e.what()));
		}
	}

	void GameSession::tick()
	{
		if (!engine)
		{
			return;
		}

		try
		{
			engine->step();
			engine->render();
			++frameCount;
		}
		catch (const std::exception& e)
		{
			fail(QString::fromUtf8(e.what()));
			return;
		}

		emit frameAdvanced();
	}

	void GameSession::fail(const QString& message)
	{
		pause();
		lastError = message;
		emit failed(message);
	}
}
