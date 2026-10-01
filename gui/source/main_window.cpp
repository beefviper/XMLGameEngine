// main_window.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "main_window.h"

#include "game_session.h"
#include "game_view.h"
#include "inspector.h"

#include <QAction>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenuBar>
#include <QMessageBox>
#include <QSizePolicy>
#include <QSplitter>

namespace xge
{
	MainWindow::MainWindow(QWidget* parent) :
		QMainWindow(parent),
		view(new GameView),
		session(new GameSession(*view, this)),
		inspector(new Inspector(*session)),
		lastDirectory(QDir("games").exists() ? QDir("games").absolutePath() : QDir::currentPath())
	{
		setWindowTitle(tr("XML Game Engine"));

		// The game takes whatever room the inspector does not need, drawn as
		// large as fits with its own proportions.
		view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		inspector->setMinimumWidth(340);

		auto* splitter = new QSplitter;
		splitter->addWidget(view);
		splitter->addWidget(inspector);
		splitter->setStretchFactor(0, 1);
		splitter->setStretchFactor(1, 0);
		splitter->setCollapsible(0, false);
		splitter->setCollapsible(1, false);
		splitter->setSizes({ 960, 440 });
		setCentralWidget(splitter);

		auto* fileMenu = menuBar()->addMenu(tr("&File"));
		auto* open = fileMenu->addAction(tr("&Open Game..."), this, &MainWindow::chooseGame);
		open->setShortcut(QKeySequence::Open);
		fileMenu->addSeparator();
		auto* quit = fileMenu->addAction(tr("&Quit"), this, &QWidget::close);
		quit->setShortcut(QKeySequence::Quit);

		resize(1400, 760);

		connect(session, &GameSession::failed, this, [this](const QString& message)
			{
				QMessageBox::critical(this, tr("The game stopped"), message);
			});
	}

	void MainWindow::chooseGame()
	{
		const QString file = QFileDialog::getOpenFileName(this, tr("Open Game"), lastDirectory, tr("Game files (*.xml)"));
		if (!file.isEmpty())
		{
			openGame(file);
		}
	}

	bool MainWindow::openGame(const QString& file)
	{
		lastDirectory = QFileInfo(file).absolutePath();

		if (!session->load(file))
		{
			QMessageBox::critical(this, tr("Could not load the game"), session->error());
			return false;
		}

		setWindowTitle(tr("XML Game Engine - %1").arg(QFileInfo(file).fileName()));
		view->setFocus();
		return true;
	}
}
