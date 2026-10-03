// game_session.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "game_session.h"

#include "game_stage.h"
#include "qt_window.h"

#include <QPoint>

#include <cmath>
#include <exception>
#include <functional>
#include <utility>

namespace xge
{
	namespace
	{
		// Sets a flag for as long as it lives, and puts back what it was.
		class Changing
		{
		public:
			explicit Changing(bool& flag) : flag(flag), was(std::exchange(flag, true)) {}
			~Changing() { flag = was; }

			Changing(const Changing&) = delete;
			Changing& operator=(const Changing&) = delete;

		private:
			bool& flag;
			bool was;
		};

		// The short name of a video library, for a title bar.
		QString videoName(VideoBackend video)
		{
			switch (video)
			{
			case VideoBackend::Qt:     return QStringLiteral("Qt");
			case VideoBackend::SFML3:  return QStringLiteral("SFML3");
			case VideoBackend::SDL2:   return QStringLiteral("SDL2");
			case VideoBackend::Raylib: return QStringLiteral("raylib");
			case VideoBackend::OpenGL: return QStringLiteral("OpenGL");
			}

			return QString();
		}
	}

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
		clock.start();
	}

	GameSession::~GameSession()
	{
		unload();
	}

	void GameSession::unload()
	{
		rememberPosition();
		timer.stop();
		playing = false;

		// The engine holds a reference to the game: it goes first.
		engine.reset();
		game.reset();
	}

	bool GameSession::load(const QString& file, bool startPlaying)
	{
		pause();
		Changing guard(changing);

		emit aboutToUnload();
		rememberPosition();
		engine.reset();
		game.reset();
		frameCount = 0;
		lastError.clear();

		try
		{
			game = std::make_unique<Game>(file.toStdString(), currentOptions.xml);

			const WindowDesc& desc = game->getWindowDesc();

			engine = std::make_unique<Engine>(*game, makeWindow(desc), makeAudio());

			// The frame pace: the game moves a fixed amount a frame, so this is
			// also its speed.
			framePeriodNs = 1'000'000'000 / (desc.framerate > 0 ? desc.framerate : 60);

			engine->render();
			shownTitle.clear();
			updateTitle();
		}
		catch (const std::exception& e)
		{
			lastError = QString::fromUtf8(e.what());
			engine.reset();
			game.reset();
			updateTimer();
			emit loaded();
			return false;
		}

		currentFile = file;

		updateTimer();
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
		const bool audioChanged = next.audio != currentOptions.audio;
		currentOptions = next;

		if (!game || (!xmlChanged && !videoChanged && !audioChanged))
		{
			return true;
		}

		Changing guard(changing);

		// A new parser means reading the file again; that also builds the
		// window with the new video library.
		if (xmlChanged)
		{
			return load(currentFile, false);
		}

		pause();
		lastError.clear();

		// The sounds go to the new library; nothing else about the game changes.
		if (audioChanged)
		{
			engine->replaceAudio([this] { return makeAudio(); });
		}

		if (!videoChanged)
		{
			return true;
		}

		rememberPosition();

		try
		{
			engine->replaceWindow([this] { return makeWindow(game->getWindowDesc()); });
			engine->render();
			shownTitle.clear();
			updateTitle();
		}
		catch (const std::exception& e)
		{
			// The new library could not take the game: the Qt renderer can.
			try
			{
				currentOptions.video = VideoBackend::Qt;
				engine->replaceWindow([this] { return makeWindow(game->getWindowDesc()); });
				engine->render();
				shownTitle.clear();
				updateTitle();
				emit videoFellBack(QString::fromUtf8(e.what()));
			}
			catch (const std::exception& again)
			{
				fail(QString::fromUtf8(again.what()));
				return false;
			}
		}

		updateTimer();
		stage.focusGame();
		emit frameAdvanced();
		return true;
	}

	std::unique_ptr<Window> GameSession::makeWindow(const WindowDesc& desc)
	{
		if (currentOptions.video != VideoBackend::Qt)
		{
			// A library draws in a window of its own, and the Qt renderer's
			// picture is not wanted while it does.
			stage.hideView();

			try
			{
				auto library = WindowFactory::create(desc, libraryBackend(currentOptions.video));

				if (const auto position = stage.gameWindowPosition())
				{
					library->setPosition(position->x(), position->y());
				}

				libraryWindow = true;
				return library;
			}
			catch (const std::exception& e)
			{
				currentOptions.video = VideoBackend::Qt;
				emit videoFellBack(QString::fromUtf8(e.what()));
			}
		}

		libraryWindow = false;
		return std::make_unique<QtWindow>(
			stage.showView(static_cast<int>(desc.width), static_cast<int>(desc.height), QString::fromStdString(desc.name)));
	}

	std::unique_ptr<Audio> GameSession::makeAudio()
	{
		try
		{
			return AudioFactory::create(currentOptions.audio);
		}
		catch (const std::exception& e)
		{
			currentOptions.audio = AudioBackend::None;
			emit audioFellBack(QString::fromUtf8(e.what()));
			return std::make_unique<NullAudio>();
		}
	}

	void GameSession::updateTitle()
	{
		if (!game || !engine)
		{
			return;
		}

		QString state = tr("Paused");
		if (playing)
		{
			state = framesPerSecond > 0 ? tr("Playing: %1fps").arg(framesPerSecond, 0, 'f', 1) : tr("Playing");
		}

		const QString title = QStringLiteral("%1 (%2) (%3, %4)")
			.arg(QString::fromStdString(game->getWindowDesc().name), state,
				videoName(currentOptions.video), xmlBackendTitle(currentOptions.xml));

		if (title == shownTitle)
		{
			return;
		}

		shownTitle = title;
		stage.setGameTitle(title);
		emit titleChanged(title);

		// A library's window has a title bar of its own, and only one that is
		// still open can be given a title (a closed one is gone).
		if (libraryWindow && engine->isWindowOpen())
		{
			engine->currentWindow()->setTitle(title.toStdString());
		}
	}

	void GameSession::rememberPosition()
	{
		if (!libraryWindow || !engine || !engine->isWindowOpen())
		{
			return;
		}

		const auto [x, y] = engine->currentWindow()->position();
		stage.setGameWindowPosition(QPoint(x, y));
	}

	void GameSession::play()
	{
		if (!engine || playing)
		{
			return;
		}

		playing = true;
		lastNs = clock.nsecsElapsed();
		fpsStartNs = lastNs;
		fpsFrames = 0;
		owedNs = 0;
		updateTimer();
		updateTitle();
		emit playingChanged(true);
	}

	void GameSession::pause()
	{
		if (!playing)
		{
			return;
		}

		playing = false;
		framesPerSecond = 0;

		// A tune that was playing stops with the game.
		if (engine)
		{
			engine->silence();
		}
		updateTimer();
		updateTitle();
		emit playingChanged(false);
	}

	void GameSession::updateTimer()
	{
		const bool needed = engine && (playing || currentOptions.video != VideoBackend::Qt);

		if (needed && !timer.isActive())
		{
			lastNs = clock.nsecsElapsed();
			owedNs = 0;
			timer.start();
		}
		else if (!needed)
		{
			timer.stop();
		}
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
		if (!engine || changing)
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
		if (!engine || changing)
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
		if (!engine || changing)
		{
			return;
		}

		if (!engine->isWindowOpen())
		{
			// The user closed the game's window.
			pause();
			timer.stop();
			emit windowClosed();
			return;
		}

		// Kept up to date while it can be asked: once the user closes the
		// window it can't be.
		rememberPosition();

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

		if (owedNs < framePeriodNs)
		{
			return;
		}

		try
		{
			while (owedNs >= framePeriodNs)
			{
				if (playing)
				{
					engine->step();
					++frameCount;
					++fpsFrames;
				}
				else
				{
					engine->pump();
				}

				owedNs -= framePeriodNs;
			}

			// Only the last of them is ever on the screen: drawn once.
			engine->render();
		}
		catch (const std::exception& e)
		{
			fail(QString::fromUtf8(e.what()));
			return;
		}

		if (playing)
		{
			if (const qint64 span = now - fpsStartNs; span >= 500'000'000)
			{
				// An exponential moving average evens out the ripple of a
				// half second's count (a frame more or less is 0.2 to 0.3 of
				// a frame a second); a measure that is far from the average
				// is a real change, and replaces it at once.
				const double measured = static_cast<double>(fpsFrames) * 1e9 / static_cast<double>(span);
				constexpr double kSnap = 1.0;
				constexpr double kWeight = 0.25;

				framesPerSecond = (framesPerSecond > 0 && std::abs(measured - framesPerSecond) < kSnap)
					? framesPerSecond + kWeight * (measured - framesPerSecond)
					: measured;
				fpsStartNs = now;
				fpsFrames = 0;
				updateTitle();
			}

			emit frameAdvanced();
		}
	}

	void GameSession::tick()
	{
		if (!engine || changing)
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
		timer.stop();
		lastError = message;
		emit failed(message);
	}
}
