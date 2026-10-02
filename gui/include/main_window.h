// main_window.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include "app_settings.h"
#include "session_options.h"

#include <QMainWindow>
#include <QRect>
#include <QString>

class QAction;

namespace xge
{
	class GameSession;
	class GameStage;
	class Inspector;

	// The application window: the Inspector (controls and the tree of game
	// data) with the game beside it, a File menu to choose the game, and the
	// Options dialog (the video library and the XML parser) behind the menu's
	// Options item.
	//
	// One window is the default: the game is drawn by the Qt renderer, on the
	// left. The View menu splits it in two: this window keeps only the controls
	// and the tree, and the game has a window of its own, which is where a
	// video library draws (every library but the Qt renderer needs one). Asking
	// for such a library while there is one window moves to two, after asking
	// (see AppSettings::warnBeforeTwoWindows).
	class MainWindow : public QMainWindow
	{
		Q_OBJECT

	public:
		explicit MainWindow(QWidget* parent = nullptr);

		// Loads a game file, telling the user if it will not load.
		bool openGame(const QString& file);

		// Opens the game that was open when the program was last used. False
		// if there was none, or it is gone or would not load.
		bool openLastGame();

	protected:
		// Frees the game's window before Qt destroys the widget it draws into.
		void closeEvent(QCloseEvent* event) override;

	public slots:
		// Asks which game file to open.
		void chooseGame();

		// Shows the Options dialog and uses what is chosen in it.
		void showOptions();

		// The controls in this window, and the game in a window of its own.
		void enterTwoWindows();

		// The game back in this window, drawn by the Qt renderer.
		void leaveTwoWindows();

	private:
		GameStage* stage;
		GameSession* session;
		Inspector* inspector;
		QAction* twoWindows{ nullptr };
		AppSettings settings;
		QString lastDirectory;

		// Where this window was while the game was in it, put back on returning.
		QRect oneWindowRect;

		// Puts this window where the settings say it was left under `key`, if
		// that is still on a screen.
		bool placeAt(const QString& key);

		// Where this window is, to be kept in the settings under `key`.
		void rememberPlace(const QString& key);

		// Uses the options chosen, moving to two windows first if the video
		// library needs one and the user agrees. Returns false if that failed
		// (the user has been told).
		bool changeOptions(const SessionOptions& next);

		// Asks whether to move to two windows. OK, with the box to stop asking
		// checked, is remembered.
		bool confirmTwoWindows();

		// The game's window was closed: paused, and back in this window.
		void gameWindowClosed();
	};
}
