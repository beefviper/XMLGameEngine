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
// none, a file dialog opens. Like XGECLI it looks for assets/ and games/ in
// the working directory, so run it from the build directory.

#include "main_window.h"

#include <QApplication>
#include <QFileInfo>
#include <QString>
#include <QTimer>

namespace
{
	QString findGame(const QString& name)
	{
		for (const QString& candidate : { name, name + ".xml", "games/" + name, "games/" + name + ".xml" })
		{
			if (QFileInfo(candidate).isFile())
			{
				return candidate;
			}
		}
		return name;
	}
}

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);

	xge::MainWindow window;
	window.show();

	if (argc > 1)
	{
		window.openGame(findGame(QString::fromLocal8Bit(argv[1])));
	}
	else
	{
		QTimer::singleShot(0, &window, &xge::MainWindow::chooseGame);
	}

	return app.exec();
}
