// main_window.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "main_window.h"

#include "game_session.h"
#include "game_stage.h"
#include "inspector.h"
#include "options_dialog.h"

#include <QAction>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenuBar>
#include <QMessageBox>
#include <QSizePolicy>
#include <QSplitter>
#include <QToolBar>

namespace xge
{
	MainWindow::MainWindow(QWidget* parent) :
		QMainWindow(parent),
		stage(new GameStage),
		session(new GameSession(*stage, this)),
		inspector(new Inspector(*session)),
		lastDirectory(QDir("games").exists() ? QDir("games").absolutePath() : QDir::currentPath())
	{
		setWindowTitle(tr("XML Game Engine"));

		// The game takes whatever room the inspector does not need, drawn as
		// large as fits with its own proportions.
		stage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		inspector->setMinimumWidth(340);

		auto* splitter = new QSplitter;
		splitter->addWidget(stage);
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
		auto* optionsAction = fileMenu->addAction(tr("&Options..."), this, &MainWindow::showOptions);
		fileMenu->addSeparator();
		auto* quit = fileMenu->addAction(tr("&Quit"), this, &QWidget::close);
		quit->setShortcut(QKeySequence::Quit);

		auto* toolbar = addToolBar(tr("Main"));
		toolbar->setMovable(false);
		toolbar->addAction(open);
		toolbar->addAction(optionsAction);

		resize(1400, 760);

		connect(session, &GameSession::failed, this, [this](const QString& message)
			{
				QMessageBox::critical(this, tr("The game stopped"), message);
			});

		// Queued: this arrives while a game is still being set up.
		connect(session, &GameSession::videoFellBack, this, [this](const QString& message)
			{
				QMessageBox::warning(this, tr("Video library"),
					tr("The video library chosen would not start, so the game is drawn with the Qt renderer instead.\n\n%1").arg(message));
			}, Qt::QueuedConnection);
	}

	void MainWindow::chooseGame()
	{
		const QString file = QFileDialog::getOpenFileName(this, tr("Open Game"), lastDirectory, tr("Game files (*.xml)"));
		if (!file.isEmpty())
		{
			openGame(file);
		}
	}

	void MainWindow::showOptions()
	{
		OptionsDialog dialog(session->options(), this);
		if (dialog.exec() != QDialog::Accepted)
		{
			return;
		}

		if (!session->applyOptions(dialog.options()) && !session->error().isEmpty())
		{
			QMessageBox::critical(this, tr("Could not use the options"), session->error());
		}

		stage->focusGame();
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
		stage->focusGame();
		return true;
	}
}
