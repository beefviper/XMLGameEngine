# xgegui

**Status:** Built (Qt 6 Widgets). Reference: [programs](../reference/programs.md#xgegui).

## Showing the game

- Options were embedding a library's window (SFML and SDL2 can adopt a Qt handle; raylib cannot) or one more `Window` that draws the picture itself. **Built:** `QtWindow` draws every object with QPainter into an image, shown by `GameView`, an ordinary widget. It lives in `xge/gui/` so the library needs no Qt.
- **One window or two.** Embedding every library in the main window was built first and **removed**: two users of OpenGL on one thread went wrong (Qt trusts its own record of the current context), giving noise, blank frames and a crash on Windows. **Chosen:** one window by default, drawn by Qt, controls on the right; or two, the game in its own window opened by the chosen library exactly as `xgecli` does. Closing the game window folds it back.
- **Qt uses no OpenGL at all** (no `QOpenGLWidget`), so it cannot fight a library's context. Price: the Qt renderer does not wait for vertical blank. Do not bring one back without a guard around every library call.
- `Engine::pump()` handles a library window's events while paused, or Windows calls it "not responding".

## Size

- The windows keep their size between games and runs; a game is drawn as large as fits, any factor, with its proportions, in screen pixels (`devicePixelRatioF`, so a 125% display does not enlarge it). Whole-number steps were tried for sharp cards and dropped: they left much of the window empty. Cards and text are sharp anyway now: the frame is drawn at the size it is shown once that settles ([backends](50-backends.md#scale)).
- A bar under the picture shows the game's size, `<scale>`, and the percent shown, with smaller, 100% and bigger buttons (25 to 800%). In one window they zoom inside its room; in two they resize the game window. Rejected: resizing the main window to fit each game.

## Driving the engine

- A Qt timer wakes every 2 ms and plays the frames the clock says are due (at most 4, so a drag does not race to catch up), then draws once. A timer of the frame's own length drifted. `loop()` split into `step()` and `render()`, so a frozen game can be redrawn after an edit.
- Keys go to the game only while the view has focus; keys held when focus leaves are let go. Qt names Shift, Control and Alt once for both hands, so `key_queue.cpp` reads left and right from the system's own code.

## The inspector

- A tree like the XML: window, objects (a group once, its members under it), states. A value that can change has an editor acting on the running object at once (through `Game::setVariable()` so bound text follows). Editors are made only when a row opens (Frogger has thousands). Not done: editing states, saving edits to XML.
- A swap in progress must tolerate edits: a text box reporting an edit on losing focus once redrew a game with no window. `GameSession` ignores step, reset and redraw while loading or applying options.

## Options and settings

- File > Options: video, XML and sound libraries (only those built), the two-window question, start on load. A new video library is given the running game (`Engine::replaceWindow`, old one destroyed first since raylib and GLFW allow only one); a new XML parser rereads and restarts; one that will not start falls back to Qt.
- Every window shows the same title, `Space Invaders (Playing: 59.9fps) (SFML3, Xerces)`.
- **`xgegui.ini`** beside the program, written only when something changes: the window places and sizes, one or two windows, the libraries, the last game.
- Look: Fusion with a palette-based style sheet. Do not style `QSpinBox` borders: the arrows disappear.
