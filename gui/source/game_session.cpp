// game_session.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "game_session.h"

#include "game_view.h"
#include "qt_window.h"

#include <exception>

namespace xge
{
	GameSession::GameSession(GameView& view, QObject* parent) :
		QObject(parent),
		view(view)
	{
		// The timer only wakes the session up often; advance() decides from the
		// clock whether a frame is due. A timer of the frame's own length
		// (1000 / 60 = 16 ms) drifts against the clock, and Windows only wakes
		// a timer on its own grid, so frames came early, late and in pairs.
		timer.setTimerType(Qt::PreciseTimer);
		timer.setInterval(2);
		connect(&timer, &QTimer::timeout, this, &GameSession::advance);
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
			framePeriodNs = 1'000'000'000 / (desc.framerate > 0 ? desc.framerate : 60);

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
		clock.start();
		lastNs = 0;
		owedNs = 0;
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

	void GameSession::advance()
	{
		const qint64 now = clock.nsecsElapsed();
		owedNs += now - lastNs;
		lastNs = now;

		// A late wake-up (the window was being dragged, say) is caught up a
		// few frames at most; the rest is let go rather than raced through.
		const qint64 limit = framePeriodNs * 4;
		if (owedNs > limit)
		{
			owedNs = limit;
		}

		if (owedNs < framePeriodNs || !engine)
		{
			return;
		}

		try
		{
			while (owedNs >= framePeriodNs)
			{
				engine->step();
				owedNs -= framePeriodNs;
				++frameCount;
			}

			// Only the last of them is ever on the screen: drawn once.
			engine->render();
		}
		catch (const std::exception& e)
		{
			fail(QString::fromUtf8(e.what()));
			return;
		}

		emit frameAdvanced();
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
