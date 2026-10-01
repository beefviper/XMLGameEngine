// game_session.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include "engine.h"
#include "game.h"

#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QTimer>

#include <memory>

namespace xge
{
	class GameView;

	// One loaded game and the engine running it, driven from a Qt timer
	// instead of Engine::loop(), so the application keeps its own event loop
	// (and the user can pause, step and edit the game between frames).
	class GameSession : public QObject
	{
		Q_OBJECT

	public:
		explicit GameSession(GameView& view, QObject* parent = nullptr);
		~GameSession() override;

		// Loads a game file and starts it playing. On failure the previous game
		// is gone and error() says why.
		bool load(const QString& file);
		const QString& error() const noexcept { return lastError; }

		bool isLoaded() const noexcept { return static_cast<bool>(game); }
		bool isPlaying() const noexcept { return playing; }
		unsigned long frames() const noexcept { return frameCount; }

		// Null when no game is loaded.
		Game* currentGame() noexcept { return game.get(); }

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

	private:
		GameView& view;
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

		// The timer's slot: plays however many frames the clock says are due.
		void advance();

		// One frame of the simulation, drawn.
		void tick();
		void fail(const QString& message);
	};
}
