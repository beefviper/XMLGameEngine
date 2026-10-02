// game_session.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include "engine.h"
#include "game.h"
#include "session_options.h"

#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QTimer>

#include <memory>

namespace xge
{
	class GameStage;

	// One loaded game and the engine running it, driven from a Qt timer
	// instead of Engine::loop(), so the application keeps its own event loop
	// (and the user can pause, step and edit the game between frames).
	class GameSession : public QObject
	{
		Q_OBJECT

	public:
		explicit GameSession(GameStage& stage, QObject* parent = nullptr);
		~GameSession() override;

		// Loads a game file and, by default, starts it playing. On failure the
		// previous game is gone and error() says why.
		bool load(const QString& file, bool startPlaying = true);
		const QString& error() const noexcept { return lastError; }

		// Stops and frees the game and its window. The window must go while
		// the widget it draws into is still alive: a library's graphics
		// objects cannot be freed once their window has been destroyed.
		void unload();

		bool isLoaded() const noexcept { return static_cast<bool>(game); }
		bool isPlaying() const noexcept { return playing; }
		unsigned long frames() const noexcept { return frameCount; }

		// Null when no game is loaded.
		Game* currentGame() noexcept { return game.get(); }

		// The video library and XML parser in use, and in use for the next game
		// loaded.
		const SessionOptions& options() const noexcept { return currentOptions; }

		// Uses the new options. With a game loaded: a new video library gets
		// the game just as it is (every object, every value, the state it is in)
		// and draws it again. A new XML parser has to read the game file again,
		// so the game starts over. Either way the game is paused afterwards;
		// whoever asked (the Options dialog, MainWindow::showOptions) plays it
		// again if it was playing. Returns false if that failed (error() says
		// why).
		bool applyOptions(const SessionOptions& next);

	public slots:
		void play();
		void pause();
		void togglePlay();

		// One frame of the simulation, drawn. Meant for while paused.
		void step();

		// Puts every object back as the game loaded and returns to the first state.
		void reset();

		// Draws the current picture without moving anything: after an edit.
		void redraw();

	signals:
		// The game was replaced; anything holding its objects must let go
		// (aboutToUnload) and then look again (loaded).
		void aboutToUnload();
		void loaded();

		void playingChanged(bool playing);
		void frameAdvanced();
		void failed(const QString& message);

		// The video library asked for would not start, and the Qt renderer is
		// being used instead.
		void videoFellBack(const QString& message);

	private:
		GameStage& stage;
		std::unique_ptr<Game> game;
		std::unique_ptr<Engine> engine;
		QTimer timer;
		QElapsedTimer clock;
		qint64 framePeriodNs{ 16'666'667 };
		qint64 lastNs{ 0 };
		qint64 owedNs{ 0 };
		bool playing{ false };
		unsigned long frameCount{ 0 };
		QString lastError;
		QString currentFile;
		SessionOptions currentOptions;

		// The timer's slot: plays however many frames the clock says are due.
		void advance();

		// One frame of the simulation, drawn.
		void tick();

		// A Window for the game with the video library chosen: a library's own
		// window drawing into the stage, or the Qt renderer. If the library
		// will not start, the Qt renderer, and videoFellBack().
		std::unique_ptr<Window> makeWindow(const WindowDesc& desc);
		std::unique_ptr<Window> makeLibraryWindow(const WindowDesc& desc, VideoBackend video);
		void fail(const QString& message);
	};
}
