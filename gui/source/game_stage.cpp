// game_stage.cpp
// XML Game Engine
// author: beefviper
// date: Oct 2, 2026

#include "game_stage.h"

#include <QCloseEvent>
#include <QRect>
#include <QScreen>
#include <QSizePolicy>
#include <QVBoxLayout>

namespace xge
{
	GameWindow::GameWindow() :
		layout(new QVBoxLayout(this))
	{
		layout->setContentsMargins(0, 0, 0, 0);
	}

	void GameWindow::closeEvent(QCloseEvent* event)
	{
		QWidget::closeEvent(event);
		emit closed();
	}

	GameStage::GameStage(QWidget* parent) :
		QWidget(parent),
		view(new GameView(keys)),
		layout(new QVBoxLayout(this))
	{
		layout->setContentsMargins(0, 0, 0, 0);
		layout->addWidget(view);
		view->hide();
		setFocusPolicy(Qt::NoFocus);
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	}

	GameStage::~GameStage()
	{
		// Not closed: nothing is listening any more.
		delete gameWindow;
	}

	GameView& GameStage::showView(int width, int height, const QString& windowTitle)
	{
		keys.clear();
		view->setGameSize(width, height);
		title = windowTitle;
		shown = true;
		place();
		return *view;
	}

	void GameStage::hideView()
	{
		shown = false;
		view->hide();

		if (gameWindow)
		{
			gameWindow->hide();
		}
	}

	void GameStage::setSplit(bool twoWindows)
	{
		split = twoWindows;
		setVisible(!split);

		if (shown)
		{
			place();
		}
	}

	void GameStage::moveViewTo(QVBoxLayout* target)
	{
		// A widget is in one layout at a time.
		if (QWidget* from = view->parentWidget(); from && from->layout())
		{
			from->layout()->removeWidget(view);
		}

		target->addWidget(view);
	}

	void GameStage::place()
	{
		if (!split)
		{
			if (gameWindow)
			{
				gameWindow->hide();
			}

			moveViewTo(layout);
			view->show();
			return;
		}

		const bool first = !gameWindow;
		if (first)
		{
			gameWindow = new GameWindow;
			connect(gameWindow, &GameWindow::closed, this, &GameStage::gameWindowClosed, Qt::QueuedConnection);
		}

		gameWindow->setWindowTitle(title);
		moveViewTo(gameWindow->contents());
		view->show();

		// Opens at the game's own size, where it was last left or else beside
		// the main window if there is room, and stays where the user puts it.
		if (first)
		{
			gameWindow->resize(view->sizeHint());

			const QRect beside = window()->frameGeometry();
			const QScreen* available = window()->screen();

			if (wantedPosition)
			{
				gameWindow->move(*wantedPosition);
			}
			else if (available && beside.right() + gameWindow->width() <= available->availableGeometry().right())
			{
				gameWindow->move(beside.right() + 1, beside.top());
			}
		}

		gameWindow->show();
	}

	std::optional<QPoint> GameStage::gameWindowPosition() const
	{
		return gameWindow ? std::optional<QPoint>(gameWindow->pos()) : wantedPosition;
	}

	void GameStage::setGameWindowPosition(const QPoint& position)
	{
		wantedPosition = position;

		if (gameWindow)
		{
			gameWindow->move(position);
		}
	}

	void GameStage::focusGame()
	{
		view->setFocus();
	}

	QSize GameStage::sizeHint() const
	{
		return view->sizeHint();
	}

	QSize GameStage::minimumSizeHint() const
	{
		return QSize(160, 90);
	}
}
