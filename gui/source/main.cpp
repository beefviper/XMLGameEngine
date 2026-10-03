// main.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

// XGEGUI: the graphical front end. The game runs in the view on the left; on
// the right are the play, pause and step controls and a tree of everything in
// the game, with editors for the values that can be changed while it runs.
// The View menu moves the game into a window of its own, which is where the
// video libraries draw.
//
//   XGEGUI [game]
//
// `game` is a game file, or the name of one in games/ (pong, or pong.xml). With
// none, a file dialog opens, in games/.
//
// The engine looks for assets/ (the font, the images) relative to the working
// directory, and games/ is where the games are. Both are looked for in the
// working directory, then in the folder the program is in, then in the folder
// above that: a Visual Studio build puts the program in build/Debug and the
// copies of games/ and assets/ in build/. The working directory is then set to
// the folder they were found in, so the program can be started from anywhere.

#include "data_folder.h"
#include "main_window.h"
#include "theme.h"

#include <QApplication>
#include <QString>
#include <QTimer>

#include <filesystem>
#include <optional>

// The lookup itself is shared with XGECLI: see data_folder.h.

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	xge::applyTheme(app);

	// The game named on the command line is relative to where the program was
	// started, so it is found before the working directory is changed.
	const std::filesystem::path dataFolder = xge::findDataFolder(std::filesystem::current_path(), xge::programDirectory());

	QString game;
	if (argc > 1)
	{
		const std::string name = QString::fromLocal8Bit(argv[1]).toStdString();
		const std::filesystem::path gamesDirectory = dataFolder.empty() ? std::filesystem::path("games") : dataFolder / "games";
		const std::optional<std::filesystem::path> found = xge::locateGameFile(name, gamesDirectory);

		// With no such file the name goes on as given and the load says so.
		game = QString::fromStdU16String((found ? std::filesystem::absolute(*found) : xge::gameFileGiven(name)).u16string());
	}

	std::error_code ignored;
	if (!dataFolder.empty())
	{
		std::filesystem::current_path(dataFolder, ignored);
	}

	xge::MainWindow window;
	window.show();

	if (argc > 1)
	{
		window.openGame(game);
	}
	else if (!window.openLastGame())
	{
		QTimer::singleShot(0, &window, &xge::MainWindow::chooseGame);
	}

	return app.exec();
}
