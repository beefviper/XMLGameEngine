// engine.h
// XML Game Engine
// author: beefviper
// date: Sept 28, 2026

#pragma once

#include "audio/audio.h"
#include "game/command.h"
#include "play/executor.h"
#include "game/game.h"
#include "engine/keys.h"
#include "game/object.h"
#include "window/window.h"

#include <array>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace xge
{
	class Engine
	{
	public:
		// A window that cannot be made throws; sound that cannot start is
		// warned of, and the game plays silent.
		explicit Engine(Game& game, WindowBackend backend = WindowFactory::defaultBackend(), AudioBackend audioBackend = AudioFactory::defaultBackend());

		// Same, but with an already-built Window (and Audio) - what lets a test
		// drive Engine's key handling with a fake window instead of opening a
		// real one. No audio (nullptr) is NullAudio: silent.
		Engine(Game& game, std::unique_ptr<Window> window, std::unique_ptr<Audio> audio = nullptr);

		// Writes what the game keeps (see keepIn) one last time.
		~Engine();

		Engine(const Engine&) = delete;
		Engine& operator=(const Engine&) = delete;

		// Plays the game until the window closes: each frame input(), then
		// update() by the time that passed (at most kLongestStep), then
		// render() (the window waits for the screen's refresh). At most
		// <framerate> frames a second, or what setFramerate gave.
		void run(void);

		// The most frames a second run() plays, in place of the game's
		// <framerate> (xgecli --framerate).
		void setFramerate(int framesASecond) noexcept { mostFrames = framesASecond; }

		// Records the game being played, for its attract mode: a new game
		// starts now, with random numbers drawn from a seed picked here, and
		// every key pressed and let go from now on is noted with the frame it
		// happened in. recorded() is what was played so far, as a <demo>
		// (xgecli --record writes it into the game file when it closes).
		void record(void);
		WrittenDemo recorded(void) const;

		// Plays a recording's keys back (xgecli --keys): a new game starts
		// now from the recording's seed, as record() started the one it was
		// made from, and every frame from here presses and lets go each of
		// its keys in the frame it was recorded in, beside any the window
		// reports. Frames the game's own demo plays in are not counted, as
		// they were not while recording.
		void replay(const WrittenDemo& demo);

		// Whether the game's <demo> is playing (a <demo /> asked for it):
		// a new game started from the demo's seed, played by its recorded
		// keys, until a key is pressed or the recording ends. Then a new game
		// starts, with what the game keeps as it was before the demo.
		bool isPlayingDemo(void) const noexcept { return demoPlaying; }

		// What the game <keeps> lasts in that file from one run of the program
		// to the next: it is read now, if it is there, and written whenever a
		// new game starts (a full reset) and when the engine is done, its
		// folder made if need be. A game that keeps nothing reads and writes
		// nothing. Without a call, nothing is read or written (the tests).
		// xgecli and xgegui use keptFileFor (engine/folder.h).
		void keepIn(const std::string& file);

		// False once the window has been closed (the user closed it, or
		// close() was called), and while a replaceWindow() has left the engine
		// without one.
		bool isWindowOpen(void) const;

		// Null while there is no window.
		Window* currentWindow(void) noexcept { return window.get(); }

		// Never null: NullAudio when there is no sound.
		Audio* currentAudio(void) noexcept { return audio.get(); }

		// The old one goes first. If `create` throws or gives null, the game
		// plays silent and the exception is passed on.
		void replaceAudio(const std::function<std::unique_ptr<Audio>()>& create);

		// Stops every sound that is playing: for a front end pausing the game.
		void silence(void);

		// The old one goes first (some libraries allow one window at a time);
		// the game and the keys held are untouched. If `create` throws, there
		// is no window, and input(), update() and render() throw
		// std::logic_error until another is given.
		void replaceWindow(const std::function<std::unique_ptr<Window>()>& create);

		// What run() does each frame, for a front end that owns the event
		// loop (the Qt application, xgegui) and runs the engine from its own
		// timer, and for the tests. input() takes what came from outside: the
		// window's keys, the mouse and pads, a demo's or a replay's keys.
		// update() is the game's step of `seconds`: the computer's presses and
		// held keys, everything moved and checked, the sounds it asked for,
		// what it keeps. render() draws the current picture without moving
		// anything, so a paused game can be redrawn after its values are edited.
		void input(void);
		void update(float seconds);
		void render(void);

		// Lets the window deal with its own events (move, resize, close)
		// without playing a frame: for a front end that has paused the game
		// but keeps its window alive. A key pressed meanwhile is dropped, so
		// it never reaches the game; one let go is held back for the next
		// input(), so a key held when the game paused is not left down.
		void pump(void);

		// A key going down or up, as the window reports it; public so the
		// tests can press keys the same way.
		void handleKeyPressed(KeyCode key);
		void handleKeyReleased(KeyCode key);

		// The mouse and the pads as the window sees them now (input() asks it
		// every frame): a button that went down or up is pressed or released
		// as a key is (KeyCode::MouseLeft, Pad1A...; a pad's direction is its
		// d-pad or its stick past halfway), and a control that moved runs the
		// current state's <input axis="..."> for it. The first call only
		// notes where everything is, so a mouse resting on the window does
		// not move anything when the game starts.
		void handleControls(const Controls& controls);

	private:
		Game& game;
		CommandExecutor commandExecutor;
		std::unique_ptr<Window> window;
		std::unique_ptr<Audio> audio;

		// When the window's pictures are drawn again at its size (render).
		SettledFit settledFit;

		// Keys let go while pumped that input() has not handled yet.
		std::vector<std::pair<KeyCode, bool>> pumpedKeys;

		// Which keys are down; a repeated press of a held key is not a new one.
		HeldKeys heldKeys;

		// What handleControls last saw: whether it has seen anything yet,
		// each mouse and pad button, and each control's place (0 to 1;
		// negative when it was not there).
		bool controlsSeen = false;
		Controls lastControls;
		std::array<std::array<bool, padButtonCount>, padCount> lastPadDown{};
		std::array<float, axisCount> lastAxes{};
		// The mouse buttons the game has been told are down: a button
		// pressed off the playfield (a title bar, a menu, the bars beside a
		// stretched picture) is not, nor is its release.
		std::array<bool, 3> mouseDown{};

		// The commands each held key is currently driving: what its press ran,
		// or, after a state change, what the new state's binding for it
		// resumed. Releasing a key sends the release to exactly these, so it
		// always reaches whatever it started, even across a state change.
		std::array<std::vector<Command>, static_cast<std::size_t>(KeyCode::Count)> heldCommands{};

		// Game::stateChangeCount() when each key last went down. A key's
		// on="hold" and on="release" bindings answer only a press made in
		// the state that is current now: Space held through the title does
		// not charge the game's shot, nor shoot it when let go.
		std::array<unsigned long, static_cast<std::size_t>(KeyCode::Count)> pressedInState{};

		// Runs the current state's on="hold" bindings of the keys down.
		void playHeldKeys(void);

		// Runs commands as one press each, as a key's press runs its own,
		// stopping at a refused one or a turn's end.
		void runAsPress(const std::vector<Command>& commands);
		// runAsPress's commands, between CellMaps::pressBegins and pressEnds.
		void runPress(const std::vector<Command>& commands);

		// Sends the release to everything the held keys are driving, and
		// forgets it; the keys stay down.
		void stopHeldCommands(void);

		// Game::stateChangeCount() as of the last syncHeldKeysToState().
		unsigned long syncedStateChanges = 0;

		// After a state change: what held keys drove in the old state is let
		// go, and the new state's continuous bindings (moves) for keys still
		// down resume, so a paddle moves again on return with no new press.
		// One-shot commands only ever run on a real press, or Space held
		// through a menu would pause the game as it began.
		void syncHeldKeysToState();

		// Throws std::logic_error when there is no window (see replaceWindow).
		void requireWindow(void) const;

		// Plays what the game asked for this frame (Sounds::request), stops
		// what it silenced, and keeps its looping sounds playing.
		void playRequestedSounds(void);

		// setFramerate's, or 0 for the game's own.
		int mostFrames = 0;

		// The buttons the state's <computer> is holding down, which it presses
		// and lets go as a player would, and is not recorded doing.
		std::set<KeyCode> computerHeld;
		bool computerPressing = false;
		void playComputer(float seconds);

		// Recording (record()): its seed, the time played since, and the keys.
		bool recording = false;
		WrittenDemo recordedSoFar;

		// A recording played: the time of it reached and the next key of it
		// to press or let go.
		struct Playback
		{
			const WrittenDemo* demo{ nullptr };
			double seconds{ 0 };
			std::size_t next{ 0 };
		};

		// Plays `seconds` of the game, split at each key of the recording
		// that falls inside them, so a key lands at its own time whatever
		// steps the window makes. A key within half a millisecond of a step's
		// start is pressed before it, so on the steps it was recorded on it
		// plays out as recorded.
		void playRecording(Playback& playback, float seconds);

		// The game's own demo (startDemo), and what the game kept before it.
		bool demoPlaying = false;
		Playback demoPlayback;
		std::string keptBeforeDemo;

		// A recording played back by replay().
		bool replaying = false;
		WrittenDemo replayed;
		Playback replayPlayback;

		void startDemo(void);
		void stopDemo(void);
		void releaseHeldKeys(void);

		// One step of the game, with no recording played: the computer's and
		// the held keys, everything moved and checked, sounds played.
		void step(float seconds);

		// Where what the game keeps is written (empty: nowhere), and
		// Game::resetCount() when it last was.
		std::string keptFile;
		unsigned long keptResets = 0;
		void writeKept(void) const;

		// The game's looping sounds the Audio was last told to play.
		std::set<std::string> playingLoops;
	};
}
