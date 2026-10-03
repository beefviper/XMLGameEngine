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
		// the widget it draws into is still alive.
		void unload();

		bool isLoaded() const noexcept { return static_cast<bool>(game); }
		bool isPlaying() const noexcept { return playing; }
		unsigned long frames() const noexcept { return frameCount; }

		// Frames a second actually played, measured over half a second at a
		// time and smoothed (see advance()); 0 while paused and until the first
		// measure is in.
		double fps() const noexcept { return framesPerSecond; }

		// Null when no game is loaded.
		Game* currentGame() noexcept { return game.get(); }

		// The video library, XML parser and sound library in use, and in use for
		// the next game loaded.
		const SessionOptions& options() const noexcept { return currentOptions; }

		// Uses the new options. With a game loaded: a new video library gets
		// the game just as it is (every object, every value, the state it is in)
		// and draws it again, in a window of its own unless it is the Qt
		// renderer; a new sound library gets the game's sounds, and nothing
		// else changes. A new XML parser has to read the game file again, so the
		// game starts over. Either way the game is paused afterwards; whoever
		// asked (the Options dialog, MainWindow::showOptions) plays it again if
		// it was playing. Returns false if that failed (error() says why).
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

		// The title the game's windows now have (see updateTitle()).
		void titleChanged(const QString& title);
		void frameAdvanced();
		void failed(const QString& message);

		// The video library asked for would not start, and the Qt renderer is
		// being used instead.
		void videoFellBack(const QString& message);

		// The sound library asked for would not start, and the game is silent
		// (the choice is None) instead.
		void audioFellBack(const QString& message);

		// The window the game is drawn in was closed by the user. The game is
		// paused and has no window until it is given one (applyOptions).
		void windowClosed();

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

		// True while load() or applyOptions() is replacing the game or its
		// window. Qt can deliver events in the middle of that (showing the
		// other page of the stage moves the keyboard focus, and an editor
		// losing it reports an edit), and for part of it the engine has no
		// window at all: step(), reset() and redraw() do nothing meanwhile.
		bool changing{ false };
		unsigned long frameCount{ 0 };

		// The frames played since fpsStartNs, for fps().
		double framesPerSecond{ 0 };
		qint64 fpsStartNs{ 0 };
		unsigned long fpsFrames{ 0 };
		QString lastError;
		QString currentFile;
		SessionOptions currentOptions;

		// Whether the game's window is one of a video library's. The Qt
		// renderer's window is the stage's.
		bool libraryWindow{ false };

		// Tells the stage where a library's window is now, so that whatever
		// window comes next opens there.
		void rememberPosition();

		// Works out the title every window the game is shown in carries: the
		// game's name, whether it is playing (and how fast) or paused, and the
		// video library and XML parser in use, as in "Space Invaders (Playing:
		// 59.9fps) (SFML3, Xerces)". It goes to a library's own window, to the
		// stage (for the Qt renderer's window in two windows) and out as
		// titleChanged() (for the main window, which is the game's window in
		// one). Does nothing when the title is already that: `shownTitle` is
		// what it last set, and is cleared when there is a new window to put it on.
		void updateTitle();
		QString shownTitle;

		// The timer's slot: plays however many frames the clock says are due
		// (while paused, only lets a library's window deal with its events and
		// draws it again).
		void advance();

		// The timer runs while the game is playing, and while it is paused in a
		// library's window, which has to be looked after to stay responsive.
		// The Qt renderer's picture needs nothing while the game is paused.
		void updateTimer();

		// One frame of the simulation, drawn.
		void tick();

		// A Window for the game with the video library chosen: the library's
		// own window, or the Qt renderer drawing into the stage. If the library
		// will not start, the Qt renderer, and videoFellBack().
		std::unique_ptr<Window> makeWindow(const WindowDesc& desc);

		// An Audio with the sound library chosen. If it will not start,
		// NullAudio, the choice becomes None, and audioFellBack().
		std::unique_ptr<Audio> makeAudio();
		void fail(const QString& message);
	};
}
