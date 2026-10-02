// game_session.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "game_session.h"

#include "embedded_window.h"
#include "game_stage.h"
#include "qt_window.h"

#include <exception>
#include <functional>

namespace xge
{
	GameSession::GameSession(GameStage& stage, QObject* parent) :
		QObject(parent),
		stage(stage)
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
		unload();
	}

	void GameSession::unload()
	{
		timer.stop();
		playing = false;

		// The engine holds a reference to the game: it goes first.
		engine.reset();
		game.reset();
	}

	bool GameSession::load(const QString& file, bool startPlaying)
	{
		pause();

		emit aboutToUnload();
		engine.reset();
		game.reset();
		frameCount = 0;
		lastError.clear();

		try
		{
			game = std::make_unique<Game>(file.toStdString(), currentOptions.xml);

			const WindowDesc& desc = game->getWindowDesc();
			stage.setGameSize(static_cast<int>(desc.width), static_cast<int>(desc.height));

			engine = std::make_unique<Engine>(*game, makeWindow(desc));

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

		currentFile = file;

		emit loaded();

		if (startPlaying)
		{
			play();
		}

		return true;
	}

	bool GameSession::applyOptions(const SessionOptions& next)
	{
		const bool xmlChanged = next.xml != currentOptions.xml;
		const bool videoChanged = next.video != currentOptions.video;
		currentOptions = next;

		if (!game || (!xmlChanged && !videoChanged))
		{
			return true;
		}

		// A new parser means reading the file again; that also builds the
		// window with the new video library.
		if (xmlChanged)
		{
			return load(currentFile, false);
		}

		pause();
		lastError.clear();

		try
		{
			engine->replaceWindow([this] { return makeWindow(game->getWindowDesc()); });
			engine->render();
		}
		catch (const std::exception& e)
		{
			// The new library could not take the game: the Qt renderer can.
			try
			{
				currentOptions.video = VideoBackend::Qt;
				engine->replaceWindow([this] { return makeWindow(game->getWindowDesc()); });
				engine->render();
				emit videoFellBack(QString::fromUtf8(e.what()));
			}
			catch (const std::exception& again)
			{
				fail(QString::fromUtf8(again.what()));
				return false;
			}
		}

		stage.focusGame();
		emit frameAdvanced();
		return true;
	}

	std::unique_ptr<Window> GameSession::makeWindow(const WindowDesc& desc)
	{
		if (currentOptions.video != VideoBackend::Qt)
		{
			try
			{
				return makeLibraryWindow(desc, currentOptions.video);
			}
			catch (const std::exception& e)
			{
				currentOptions.video = VideoBackend::Qt;
				emit videoFellBack(QString::fromUtf8(e.what()));
			}
		}

		stage.showView();
		return std::make_unique<QtWindow>(stage.view());
	}

	std::unique_ptr<Window> GameSession::makeLibraryWindow(const WindowDesc& desc, VideoBackend video)
	{
		const WindowBackend backend = libraryBackend(video);

		// A library that can draw into a window of someone else's is given
		// one; one that cannot draws to a back buffer, and the stage shows what
		// it drew.
		WindowTarget target;
		if (WindowFactory::embedding(backend) == Embedding::NativeWindow)
		{
			target.kind = WindowTarget::Kind::NativeWindow;
			target.nativeHandle = stage.showFreshSurface();
		}
		else
		{
			target.kind = WindowTarget::Kind::BackBuffer;
			stage.showView();
		}

		// Starting the library makes its context current; Qt's is put back.
		QtContextKeeper keeper;
		return std::make_unique<EmbeddedWindow>(WindowFactory::create(desc, backend, target), stage);
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
