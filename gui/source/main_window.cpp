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
#include <QCheckBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenuBar>
#include <QMessageBox>
#include <QCloseEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QSizePolicy>
#include <QSplitter>

namespace xge
{
	namespace
	{
		// Where the windows were left, in the settings file.
		const QString kOneWindowKey = QStringLiteral("Windows/one_window");
		const QString kControlsKey = QStringLiteral("Windows/controls");
		const QString kGameKey = QStringLiteral("Windows/game");

		// What was last in use: one window or two, and the choices of the
		// Options dialog.
		const QString kTwoWindowsKey = QStringLiteral("Windows/two_windows");
		const QString kVideoKey = QStringLiteral("Session/video");
		const QString kXmlKey = QStringLiteral("Session/xml");
		const QString kAudioKey = QStringLiteral("Session/audio");
		const QString kGameFileKey = QStringLiteral("Session/game");

		// The width of the controls until the user has shown otherwise.
		constexpr int kControlsWidth = 440;

		// Whether a window with its top left corner here can be got at.
		bool onScreen(const QPoint& corner)
		{
			const QRect grip(corner, QSize(100, 50));
			for (const QScreen* screen : QGuiApplication::screens())
			{
				if (screen->availableGeometry().intersects(grip))
				{
					return true;
				}
			}

			return false;
		}
	}

	bool MainWindow::placeAt(const QString& key)
	{
		const QRect rect = settings.value(key).toRect();
		if (!rect.isValid() || !onScreen(rect.topLeft()))
		{
			return false;
		}

		setGeometry(rect);
		return true;
	}

	void MainWindow::rememberPlace(const QString& key)
	{
		settings.setValue(key, isMaximized() ? normalGeometry() : geometry());
	}

	void MainWindow::closeEvent(QCloseEvent* event)
	{
		session->unload();
		stage->hideView();

		// Where everything was left, for next time.
		if (stage->isSplit())
		{
			rememberPlace(kControlsKey);
			settings.setValue(kOneWindowKey, oneWindowRect);
		}
		else
		{
			rememberPlace(kOneWindowKey);
		}

		if (const auto position = stage->gameWindowPosition())
		{
			settings.setValue(kGameKey, *position);
		}

		settings.setValue(kTwoWindowsKey, stage->isSplit());
		settings.setValue(kVideoKey, videoBackendKey(session->options().video));
		settings.setValue(kXmlKey, xmlBackendKey(session->options().xml));
		settings.setValue(kAudioKey, audioBackendKey(session->options().audio));

		QMainWindow::closeEvent(event);
	}

	MainWindow::MainWindow(QWidget* parent) :
		QMainWindow(parent),
		stage(new GameStage),
		session(new GameSession(*stage, this)),
		inspector(new Inspector(*session)),
		lastDirectory(QDir("games").exists() ? QDir("games").absolutePath() : QDir::currentPath())
	{
		controlsTitle = tr("XML Game Engine");
		setWindowTitle(controlsTitle);

		// The game takes whatever room the inspector does not need, drawn as
		// large as fits with its own proportions (in one window).
		stage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		inspector->setMinimumWidth(340);

		auto* splitter = new QSplitter;
		splitter->addWidget(stage);
		splitter->addWidget(inspector);
		splitter->setStretchFactor(0, 1);
		splitter->setStretchFactor(1, 0);
		splitter->setCollapsible(0, false);
		splitter->setCollapsible(1, false);
		splitter->setSizes({ 1400 - kControlsWidth, kControlsWidth });
		setCentralWidget(splitter);

		auto* fileMenu = menuBar()->addMenu(tr("&File"));
		auto* open = fileMenu->addAction(tr("&Open Game..."), this, &MainWindow::chooseGame);
		open->setShortcut(QKeySequence::Open);
		fileMenu->addSeparator();
		fileMenu->addAction(tr("&Options..."), this, &MainWindow::showOptions);
		fileMenu->addSeparator();
		auto* quit = fileMenu->addAction(tr("&Quit"), this, &QWidget::close);
		quit->setShortcut(QKeySequence::Quit);

		auto* viewMenu = menuBar()->addMenu(tr("&View"));
		twoWindows = viewMenu->addAction(tr("&Game in Its Own Window"));
		twoWindows->setCheckable(true);
		connect(twoWindows, &QAction::triggered, this, [this](bool checked)
			{
				if (checked) { enterTwoWindows(); }
				else { leaveTwoWindows(); }
			});

		resize(1400, 760);
		placeAt(kOneWindowKey);

		if (const QVariant game = settings.value(kGameKey); game.isValid() && onScreen(game.toPoint()))
		{
			stage->setGameWindowPosition(game.toPoint());
		}

		// As it was left: the choices first, then the layout (a video library
		// needs two windows whatever the file says). No game is loaded yet, so
		// nothing is asked or drawn.
		SessionOptions last;
		last.video = videoBackendFromKey(settings.value(kVideoKey).toString()).value_or(last.video);
		last.xml = xmlBackendFromKey(settings.value(kXmlKey).toString()).value_or(last.xml);
		last.audio = audioBackendFromKey(settings.value(kAudioKey).toString()).value_or(last.audio);
		session->applyOptions(last);

		if (settings.value(kTwoWindowsKey).toBool() || last.video != VideoBackend::Qt)
		{
			enterTwoWindows();
		}

		connect(session, &GameSession::titleChanged, this, &MainWindow::showTitle);

		connect(session, &GameSession::failed, this, [this](const QString& message)
			{
				QMessageBox::critical(this, tr("The game stopped"), message);
			});

		// Queued: both arrive from code that is still using the window.
		connect(stage, &GameStage::gameWindowClosed, this, &MainWindow::gameWindowClosed, Qt::QueuedConnection);
		connect(session, &GameSession::windowClosed, this, &MainWindow::gameWindowClosed, Qt::QueuedConnection);

		// Queued: this arrives while a game is still being set up.
		connect(session, &GameSession::videoFellBack, this, [this](const QString& message)
			{
				QMessageBox::warning(this, tr("Video library"),
					tr("The video library chosen would not start, so the game is drawn with the Qt renderer instead.\n\n%1").arg(message));
			}, Qt::QueuedConnection);

		connect(session, &GameSession::audioFellBack, this, [this](const QString& message)
			{
				QMessageBox::warning(this, tr("Sound library"),
					tr("The sound library chosen would not start, so the game is silent.\n\n%1").arg(message));
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
		// The game cannot be played while the dialog is up (it has the
		// keyboard), so it waits, and carries on afterwards the way it was.
		const bool wasPlaying = session->isPlaying();
		session->pause();

		OptionsDialog dialog(session->options(), settings.warnBeforeTwoWindows(), settings.startGameOnLoad(), this);
		bool done = true;

		if (dialog.exec() == QDialog::Accepted)
		{
			settings.setWarnBeforeTwoWindows(dialog.warnBeforeTwoWindows());
			settings.setStartGameOnLoad(dialog.startGameOnLoad());
			done = changeOptions(dialog.options());
		}

		if (done && wasPlaying)
		{
			session->play();
		}
	}

	bool MainWindow::changeOptions(const SessionOptions& next)
	{
		SessionOptions wanted = next;
		bool movesToTwoWindows = wanted.video != VideoBackend::Qt && !stage->isSplit();

		// Saying no keeps the video library as it was; the rest still counts.
		if (movesToTwoWindows && settings.warnBeforeTwoWindows() && !confirmTwoWindows())
		{
			wanted.video = session->options().video;
			movesToTwoWindows = false;
		}

		if (!session->applyOptions(wanted))
		{
			if (!session->error().isEmpty())
			{
				QMessageBox::critical(this, tr("Could not use the options"), session->error());
			}
			return false;
		}

		// If the library would not start the Qt renderer is drawing the game,
		// and it belongs in this window.
		if (movesToTwoWindows && session->options().video != VideoBackend::Qt)
		{
			enterTwoWindows();
		}

		return true;
	}

	bool MainWindow::confirmTwoWindows()
	{
		QMessageBox box(QMessageBox::Question, tr("Two windows"),
			tr("This video library draws the game in a window of its own, so the game will move out of this window into a second one.\n\nSwitch to two windows?"),
			QMessageBox::Ok | QMessageBox::Cancel, this);
		box.setDefaultButton(QMessageBox::Ok);

		auto* remember = new QCheckBox(tr("Don't ask me again"));
		remember->setChecked(true);
		box.setCheckBox(remember);

		if (box.exec() != QMessageBox::Ok)
		{
			return false;
		}

		if (remember->isChecked())
		{
			settings.setWarnBeforeTwoWindows(false);
		}

		return true;
	}

	void MainWindow::enterTwoWindows()
	{
		if (stage->isSplit())
		{
			return;
		}

		// This window keeps the width the controls had, where it was last left.
		const int controlsWidth = isVisible() ? inspector->width() : kControlsWidth;
		oneWindowRect = isMaximized() ? normalGeometry() : geometry();

		if (!placeAt(kControlsKey))
		{
			resize(controlsWidth, height());
		}

		stage->setSplit(true);
		twoWindows->setChecked(true);
		applyTitle();
	}

	void MainWindow::leaveTwoWindows()
	{
		if (!stage->isSplit())
		{
			return;
		}

		rememberPlace(kControlsKey);
		stage->setSplit(false);
		twoWindows->setChecked(false);
		setGeometry(oneWindowRect);
		applyTitle();

		// Only the Qt renderer draws in this window.
		SessionOptions options = session->options();
		if (options.video != VideoBackend::Qt)
		{
			const bool wasPlaying = session->isPlaying();
			options.video = VideoBackend::Qt;

			if (changeOptions(options) && wasPlaying)
			{
				session->play();
			}
		}
	}

	void MainWindow::showTitle(const QString& title)
	{
		gameTitle = title;
		applyTitle();
	}

	void MainWindow::applyTitle()
	{
		setWindowTitle(gameTitle.isEmpty() ? controlsTitle : gameTitle);
	}

	void MainWindow::gameWindowClosed()
	{
		session->pause();
		leaveTwoWindows();
	}

	bool MainWindow::openGame(const QString& file)
	{
		lastDirectory = QFileInfo(file).absolutePath();

		if (!session->load(file, settings.startGameOnLoad()))
		{
			QMessageBox::critical(this, tr("Could not load the game"), session->error());
			return false;
		}

		settings.setValue(kGameFileKey, QFileInfo(file).absoluteFilePath());
		controlsTitle = tr("XML Game Engine - %1").arg(QFileInfo(file).fileName());
		applyTitle();
		stage->focusGame();
		return true;
	}

	bool MainWindow::openLastGame()
	{
		const QString file = settings.value(kGameFileKey).toString();
		return !file.isEmpty() && QFileInfo::exists(file) && openGame(file);
	}
}
