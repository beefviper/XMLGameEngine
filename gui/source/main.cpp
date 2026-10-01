// main.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

// XGEGUI: the graphical front end. The game runs in the view on the left; on
// the right are the play, pause and step controls and a tree of everything in
// the game, with editors for the values that can be changed while it runs.
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

#include "main_window.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QTimer>

namespace
{
	// The first folder that has both games/ and assets/ in it, or empty.
	QString findDataFolder()
	{
		const QDir program(QApplication::applicationDirPath());
		const QString candidates[] = {
			QDir::currentPath(),
			program.absolutePath(),
			program.absoluteFilePath(".."),
		};

		for (const QString& candidate : candidates)
		{
			const QDir folder(candidate);
			if (QFileInfo(folder.filePath("games")).isDir() && QFileInfo(folder.filePath("assets")).isDir())
			{
				return folder.canonicalPath();
			}
		}

		return QString();
	}

	// Finds a game the way XGECLI does: a bare name gets .xml added, then it is
	// looked for as given, by its file name alone, and in games/ (here, the
	// games/ of the data folder). Returns the full path, or the name as given
	// when there is no such file (the load will say so).
	QString findGame(const QString& name, const QString& dataFolder)
	{
		QString given = name;
		if (QFileInfo(given).suffix().isEmpty())
		{
			given += ".xml";
		}

		const QString fileName = QFileInfo(given).fileName();

		QStringList candidates{ given, fileName };
		if (!dataFolder.isEmpty())
		{
			candidates << QDir(dataFolder).filePath("games/" + fileName);
		}

		for (const QString& candidate : candidates)
		{
			if (QFileInfo(candidate).isFile())
			{
				return QFileInfo(candidate).absoluteFilePath();
			}
		}

		return name;
	}
}

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);

	// The game named on the command line is relative to where the program was
	// started, so it is found before the working directory is changed.
	const QString dataFolder = findDataFolder();
	const QString game = argc > 1 ? findGame(QString::fromLocal8Bit(argv[1]), dataFolder) : QString();

	if (!dataFolder.isEmpty())
	{
		QDir::setCurrent(dataFolder);
	}

	xge::MainWindow window;
	window.show();

	if (argc > 1)
	{
		window.openGame(game);
	}
	else
	{
		QTimer::singleShot(0, &window, &xge::MainWindow::chooseGame);
	}

	return app.exec();
}
