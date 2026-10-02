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
	class GameStage;
	class Inspector;

	// The application window: the game on the left, the Inspector (controls
	// and the tree of game data) on the right, a File menu to
	// choose the game, and the Options dialog (the video library and the XML
	// parser) behind the menu's Options item.
	class MainWindow : public QMainWindow
	{
		Q_OBJECT

	public:
		explicit MainWindow(QWidget* parent = nullptr);

		// Loads a game file, telling the user if it will not load.
		bool openGame(const QString& file);

	protected:
		// Frees the game's window before Qt destroys the widget it draws into.
		void closeEvent(QCloseEvent* event) override;

	public slots:
		// Asks which game file to open.
		void chooseGame();

		// Shows the Options dialog and uses what is chosen in it.
		void showOptions();

	private:
		GameStage* stage;
		GameSession* session;
		Inspector* inspector;
		QString lastDirectory;
	};
}
