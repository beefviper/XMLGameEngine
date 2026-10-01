// game_stage.cpp
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#include "game_stage.h"

#include <QPalette>
#include <QResizeEvent>
#include <QSizePolicy>

namespace xge
{
	// The page the surface sits on: it keeps the surface (which is the size of
	// the game, not of the pane) in the middle, cut off at the edges when the
	// pane is smaller than the game.
	class NativeHolder : public QWidget
	{
	public:
		explicit NativeHolder(QWidget* parent = nullptr) :
			QWidget(parent)
		{
			setAutoFillBackground(true);

			QPalette palette = this->palette();
			palette.setColor(QPalette::Window, QColor(0x20, 0x20, 0x20));
			setPalette(palette);
		}

		void center(QWidget* child)
		{
			if (child)
			{
				child->move((width() - child->width()) / 2, (height() - child->height()) / 2);
			}
		}

		QSize minimumSizeHint() const override { return QSize(160, 90); }

	protected:
		void resizeEvent(QResizeEvent* event) override
		{
			QWidget::resizeEvent(event);

			for (QWidget* child : findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly))
			{
				center(child);
			}
		}
	};

	GameStage::GameStage(QWidget* parent) :
		QStackedWidget(parent),
		gameView(new GameView(keys)),
		holder(new NativeHolder)
	{
		addWidget(gameView);
		addWidget(holder);
		setFocusPolicy(Qt::NoFocus);
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	}

	void GameStage::setGameSize(int width, int height)
	{
		gameWidth = width;
		gameHeight = height;

		keys.clear();
		gameView->setGameSize(width, height);

		if (surface)
		{
			surface->setGameSize(width, height);
			holder->center(surface);
		}
	}

	void GameStage::showView()
	{
		setCurrentWidget(gameView);
	}

	void* GameStage::showFreshSurface()
	{
		delete surface;

		surface = new NativeSurface(keys, holder);
		surface->setGameSize(gameWidth, gameHeight);
		holder->center(surface);
		surface->show();

		setCurrentWidget(holder);
		return surface->nativeHandle();
	}

	std::vector<std::pair<KeyCode, bool>> GameStage::takeKeyEvents()
	{
		return keys.take();
	}

	void GameStage::focusGame()
	{
		if (currentWidget() == holder && surface)
		{
			surface->setFocus();
		}
		else
		{
			gameView->setFocus();
		}
	}

	QSize GameStage::sizeHint() const
	{
		return gameView->sizeHint();
	}

	QSize GameStage::minimumSizeHint() const
	{
		return QSize(160, 90);
	}
}
