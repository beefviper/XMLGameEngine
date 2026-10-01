// game_stage.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include "game_view.h"
#include "key_queue.h"
#include "native_surface.h"

#include <QStackedWidget>

#include <utility>
#include <vector>

namespace xge
{
	class NativeHolder;

	// The left side of the window: where the game is shown. It holds the two
	// ways the game can be shown and one of them is up at a time.
	//
	//   the view      a picture the program drew (the Qt renderer's, or the
	//                 pixels a window library drew to a back buffer)
	//   the surface   a window the window library draws into itself (SFML3,
	//                 SDL2), the size of the game, in the middle of the pane
	//
	// and owns the KeyQueue they both put the keyboard in.
	class GameStage : public QStackedWidget
	{
	public:
		explicit GameStage(QWidget* parent = nullptr);

		// Sizes both to the game's window; this also forgets every key.
		void setGameSize(int width, int height);

		GameView& view() noexcept { return *gameView; }

		// Shows the view.
		void showView();

		// Replaces the surface with a new one (a window library should never
		// be given a window another has used: the first one's choice of pixel
		// format would stay on it) and shows it. Returns its platform window
		// handle. The old surface's window must already be gone.
		void* showFreshSurface();

		// The keys that changed since the last call, as {key, pressed}.
		std::vector<std::pair<KeyCode, bool>> takeKeyEvents();

		// Puts the keyboard focus on whatever shows the game.
		void focusGame();

		QSize sizeHint() const override;
		QSize minimumSizeHint() const override;

	private:
		KeyQueue keys;
		GameView* gameView;
		NativeHolder* holder;
		NativeSurface* surface{ nullptr };
		int gameWidth{ 640 };
		int gameHeight{ 360 };
	};
}
