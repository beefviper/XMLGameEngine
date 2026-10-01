// main_window.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include <QMainWindow>
#include <QString>

namespace xge
{
	class GameSession;
	class GameView;
	class Inspector;

	// The application window: the game on the left, the Inspector (controls
	// and the tree of game data) on the right, and a File menu to choose the
	// game.
	class MainWindow : public QMainWindow
	{
		Q_OBJECT

	public:
		explicit MainWindow(QWidget* parent = nullptr);

		// Loads a game file, telling the user if it will not load.
		bool openGame(const QString& file);

	public slots:
		// Asks which game file to open.
		void chooseGame();

	private:
		GameView* view;
		GameSession* session;
		Inspector* inspector;
		QString lastDirectory;
	};
}
