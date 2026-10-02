// game_stage.h
// XML Game Engine
// author: beefviper
// date: Oct 2, 2026

#pragma once

#include "game_view.h"
#include "key_queue.h"

#include <QPoint>
#include <QString>
#include <QWidget>

#include <optional>

class QCloseEvent;
class QVBoxLayout;

namespace xge
{
	// A window of its own that holds the game's picture, for the two window
	// layout. Closing it is reported, not refused.
	class GameWindow : public QWidget
	{
		Q_OBJECT

	public:
		GameWindow();

		QVBoxLayout* contents() const noexcept { return layout; }

	signals:
		void closed();

	protected:
		void closeEvent(QCloseEvent* event) override;

	private:
		QVBoxLayout* layout;
	};

	// Where the Qt renderer's picture (the GameView) is shown. A video library
	// has a window of its own and needs none of this; only the Qt renderer
	// draws into a widget of the application.
	//
	//   one window   the stage is the left side of the main window, and the
	//                picture is in it
	//   two windows  the stage is hidden, and the picture is in a GameWindow
	//
	// The picture is one widget that lives for as long as the stage and is
	// moved between the two places; it is hidden while a library draws.
	class GameStage : public QWidget
	{
		Q_OBJECT

	public:
		explicit GameStage(QWidget* parent = nullptr);
		~GameStage() override;

		// Shows the picture, sized for a game window of this size, in whichever
		// window the layout says; any key held is forgotten.
		GameView& showView(int width, int height, const QString& title);

		// Hides the picture, and the game window with it.
		void hideView();

		// The text on the title bar of the game's own window (the Qt
		// renderer's, in two windows); remembered for when it opens.
		void setGameTitle(const QString& title);

		bool isSplit() const noexcept { return split; }

		// One window or two. Takes the picture, if there is one, to where the
		// layout now has it.
		void setSplit(bool twoWindows);

		// Where the game window is, or is to open: the screen position of its
		// top left corner. None until it has been placed.
		std::optional<QPoint> gameWindowPosition() const;
		void setGameWindowPosition(const QPoint& position);

		// Puts the keyboard focus on the picture.
		void focusGame();

		QSize sizeHint() const override;
		QSize minimumSizeHint() const override;

	signals:
		// The user closed the game window.
		void gameWindowClosed();

	private:
		KeyQueue keys;
		GameView* view;
		GameWindow* gameWindow{ nullptr };
		QVBoxLayout* layout;
		QString title;
		bool split{ false };
		std::optional<QPoint> wantedPosition;

		// Whether the picture is wanted (showView, not hideView).
		bool shown{ false };

		// Puts the picture and the game window where `split` says, and shows them.
		void place();
		void moveViewTo(QVBoxLayout* target);
	};
}
