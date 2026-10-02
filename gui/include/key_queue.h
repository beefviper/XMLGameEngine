// key_queue.h
// XML Game Engine
// author: beefviper
// date: Oct 1, 2026

#pragma once

#include "keycode.h"

#include <array>
#include <cstddef>
#include <utility>
#include <vector>

class QKeyEvent;

namespace xge
{
	// The keys pressed in the game's part of the window since the engine last
	// asked, as the engine knows them (xge::KeyCode). The GameView feeds the
	// one queue of the GameStage that holds it; the QtWindow backend hands them
	// to the engine. (A video library's own window reads its own keys.)
	class KeyQueue
	{
	public:
		// Take a key event from a widget. Return true for a key the engine
		// knows (the event is then used up); a key held down repeats as press
		// events, and only the first is queued.
		bool press(const QKeyEvent& event);
		bool release(const QKeyEvent& event);

		// Releases every key still down: its release would go to whatever has
		// the focus now, and the paddle would be left moving.
		void releaseAll();

		// Forgets everything: for a new game or window.
		void clear();

		// The keys that changed since the last call, as {key, pressed}.
		std::vector<std::pair<KeyCode, bool>> take();

	private:
		std::vector<std::pair<KeyCode, bool>> pending;
		std::array<bool, static_cast<std::size_t>(KeyCode::Count)> down{};

		void queue(KeyCode key, bool pressed);
	};
}
