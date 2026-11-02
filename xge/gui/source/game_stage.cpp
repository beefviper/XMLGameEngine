// game_stage.cpp
// XML Game Engine
// author: beefviper
// date: Oct 2, 2026

#include "game_stage.h"

#include <QCloseEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMargins>
#include <QRect>
#include <QScreen>
#include <QSizePolicy>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <array>
#include <cmath>

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
		holder(new QWidget),
		sizeLabel(new QLabel),
		layout(new QVBoxLayout(this))
	{
		// The picture with the bar under it.
		auto* bar = new QHBoxLayout;
		bar->setContentsMargins(6, 2, 6, 2);
		bar->addWidget(sizeLabel);
		bar->addStretch(1);

		const auto button = [this, bar](const QString& text, const QString& tip, auto act)
			{
				auto* made = new QToolButton;
				made->setText(text);
				made->setToolTip(tip);
				made->setAutoRaise(true);
				made->setFocusPolicy(Qt::NoFocus);
				connect(made, &QToolButton::clicked, this, act);
				bar->addWidget(made);
			};
		button(QStringLiteral("\u2212"), tr("Smaller"), [this] { zoomBy(-1); });
		button(QStringLiteral("100%"), tr("The game's own size"), [this] { zoomTo(100); });
		button(QStringLiteral("+"), tr("Bigger"), [this] { zoomBy(1); });

		auto* column = new QVBoxLayout(holder);
		column->setContentsMargins(0, 0, 0, 0);
		column->setSpacing(0);
		column->addWidget(view, 1);
		column->addLayout(bar);

		connect(view, &GameView::shownChanged, this, &GameStage::showSize);

		layout->setContentsMargins(0, 0, 0, 0);
		layout->addWidget(holder);
		holder->hide();
		setFocusPolicy(Qt::NoFocus);
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	}

	GameStage::~GameStage()
	{
		// Not closed: nothing is listening any more.
		delete gameWindow;
	}

	GameView& GameStage::showView(int width, int height, int scale, const QString& windowTitle)
	{
		keys.clear();
		view->setGameSize(width, height, scale);
		title = windowTitle;
		shown = true;
		place();
		return *view;
	}

	void GameStage::setGameTitle(const QString& text)
	{
		title = text;

		if (gameWindow)
		{
			gameWindow->setWindowTitle(title);
		}
	}

	void GameStage::hideView()
	{
		shown = false;
		holder->hide();

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
		if (QWidget* from = holder->parentWidget(); from && from->layout())
		{
			from->layout()->removeWidget(holder);
		}

		target->addWidget(holder);
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
			holder->show();
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
		holder->show();

		// Opens at the size and place it was last left, or else at the game's
		// own size beside the main window if there is room, and stays the
		// size and place the user gives it from game to game.
		if (first)
		{
			gameWindow->resize(wantedSize ? *wantedSize : holder->sizeHint());

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

	std::optional<QSize> GameStage::gameWindowSize() const
	{
		return gameWindow ? std::optional<QSize>(gameWindow->size()) : wantedSize;
	}

	void GameStage::setGameWindowSize(const QSize& size)
	{
		wantedSize = size;

		if (gameWindow)
		{
			gameWindow->resize(size);
		}
	}

	void GameStage::zoomBy(int step)
	{
		// Steps in percent of the game's defined size.
		constexpr std::array<double, 15> steps{ 25, 33, 50, 67, 75, 100, 125, 150, 200, 250, 300, 400, 500, 600, 800 };
		const double now = view->shownPercent();

		if (step > 0)
		{
			const auto next = std::find_if(steps.begin(), steps.end(), [now](double percent) { return percent > now * 1.01; });
			zoomTo(next != steps.end() ? *next : steps.back());
		}
		else
		{
			const auto next = std::find_if(steps.rbegin(), steps.rend(), [now](double percent) { return percent < now * 0.99; });
			zoomTo(next != steps.rend() ? *next : steps.front());
		}
	}

	void GameStage::zoomTo(double percent)
	{
		if (!split || !gameWindow)
		{
			// Within the room the main window gives it; as large as fits once
			// the zoom would fill it.
			view->setZoom(0);
			if (percent < view->shownPercent() * 0.999)
			{
				view->setZoom(percent);
			}
			return;
		}

		// The game window is made the picture's size, as far as the screen
		// has room, and the picture fills it.
		QSize wanted = view->pictureSize(percent) + QSize(0, holder->height() - view->height());

		if (const QScreen* screen = gameWindow->screen())
		{
			const QMargins frame = QMargins(
				gameWindow->geometry().left() - gameWindow->frameGeometry().left(),
				gameWindow->geometry().top() - gameWindow->frameGeometry().top(),
				gameWindow->frameGeometry().right() - gameWindow->geometry().right(),
				gameWindow->frameGeometry().bottom() - gameWindow->geometry().bottom());
			const QSize room = screen->availableGeometry().size().shrunkBy(frame);
			wanted = wanted.boundedTo(room);
		}

		view->setZoom(0);
		gameWindow->resize(wanted);
	}

	void GameStage::showSize()
	{
		if (view->gameWidth() <= 0)
		{
			sizeLabel->clear();
			return;
		}

		QString text = QStringLiteral("%1 \u00d7 %2").arg(view->gameWidth()).arg(view->gameHeight());
		if (view->gameScale() > 1)
		{
			text += tr(", scale %1").arg(view->gameScale());
		}

		text += QStringLiteral("   %1%").arg(std::lround(view->shownPercent()));
		sizeLabel->setText(text);
	}

	void GameStage::focusGame()
	{
		view->setFocus();
	}

	QSize GameStage::sizeHint() const
	{
		return holder->sizeHint();
	}

	QSize GameStage::minimumSizeHint() const
	{
		return QSize(160, 90);
	}
}
