# 38. Qt front end

**Status:** built (`gui/`, `scripts/cmake/executables.cmake`; `Engine::step()`/`render()`, `Game::setVariable()`/`getStates()`)

## Decision

`XGEGUI` is a Qt 6 Widgets application. The game is on the left; on the right, from the top, are the **Play/Pause**, **Step** and **Reset** buttons, a line saying the current state, the frame number and whether it is running, and a tree of everything in the game.

### Showing the game

Options were to embed one of the existing windows (SFML and SDL2 can draw into a window handle Qt gives them; raylib cannot take a foreign window at all), or to write one more `Window` backend that draws the picture itself. The second is built, and the first was tried and later removed ([39](39-opengl-backend-and-options.md), [40](40-split-windows-in-xgegui.md)): `QtWindow` (`gui/source/qt_window.cpp`) implements `Window` like the SFML, raylib and SDL2 backends, draws every object with QPainter into an image, and `GameView` shows that image as large as fits with the game's own proportions. One backend then works the same on every platform and is resized by Qt, rather than three platform-specific embeddings, only one of which would be complete. It is in `gui/`, not `lib/`, so the engine library needs no Qt. The other backends open windows of their own ([40](40-split-windows-in-xgegui.md)): the Options dialog chooses between them and this one.

The engine is driven from a Qt timer instead of `Engine::loop()`, at the game's own frame rate (a game moves a fixed amount a frame, so that is also its speed). The timer wakes every 2 ms and plays however many frames the clock says are due (at most four, so a window drag does not make the game race to catch up), then draws once; a timer of the frame's own length (16 ms for 60) drifted against the clock and ran frames early, late and in pairs. The frame is drawn with one `QPainter`, not one per object. `GameView` is an ordinary `QWidget`; it was a `QOpenGLWidget` (which presents on the display's vertical blank) until a video library's OpenGL context turned out not to share the thread with Qt's ([40](40-split-windows-in-xgegui.md)). `loop()` is now `step()` (read the keys, move, check conditions) followed by `render()` (draw without moving anything), so a frozen game can be redrawn after an edit. Pause really stops the simulation, which is separate from a game's own pause state; Step advances one frame while frozen; Reset is `Game::resetAll()`.

Keys go to the game only while the game view has the focus (click it); the buttons do not take the focus, so pressing one does not stop the keyboard working. Keys held when the focus leaves are released.

### Choosing the libraries

The Options dialog (File menu) picks the video library, the Qt renderer by default, and the XML parser, Xerces by default, and can change the video library of a running game; see [39](39-opengl-backend-and-options.md). The Qt renderer above is one of the choices; the others draw in a window of their own, and the View menu and the Options dialog move the game there ([40](40-split-windows-in-xgegui.md)).

### The tree

The tree is laid out like the XML:

- **Window**: name, size, background (editable), full screen, frame rate.
- **Objects**: one entry per object. A `<group>` and a `<grid>` are each shown once, with their members under them (a count is on the row), so how the game was written can be seen. Each object has class, visible, sprite, position, velocity, acceleration, collision (enabled, type, the rules, and the edge commands), variables and actions.
- **States**: each state's shown objects, key bindings and conditions, written the way `printGame()` writes them.

A value that can change has an editor on its own row: a spin box with up and down arrows for a number (type a value or click), a check box for yes/no, a text box for text. A change is made on the running object at once, and the picture is redrawn straight away when the game is frozen. Editable: class, visible, a sprite's size, color and text, position, velocity, acceleration, collision on/off, every object variable (through `Game::setVariable()`, so a text showing it follows), and the window background. Values read from the game every tenth of a second keep the tree current while it runs; a box being typed in is left alone.

Editors are only made when their parent row is first opened, because a game has thousands of values (Frogger has 110 objects) and most are never looked at.

### How it looks

The controls were the platform's flat default, one color throughout, which made the rows of the tree hard to follow across and the buttons hard to tell from the background. `gui/source/theme.cpp` (`applyTheme`, called once from `main()`) sets the **Fusion** style, which draws the same raised, shaded controls and sunken, bordered number boxes everywhere, then adds a style sheet: shaded buttons with a hover, pressed and disabled look, shaded column headings with dividers, taller tree rows tinted on every other row, a hover tint and a shaded selection bar, a visible splitter grip, and a bordered text box for text values. The sections of the tree (Window, Objects, States) are bold. Every color is worked out from the application's palette, lighter or darker as the theme is, so a dark theme gets a dark version. The accent is a neutral grey (dark grey on a light theme, light grey on a dark one), not the system accent color: selection, hover, focus, and the row stripes are all shades of grey. The Playing (green) and Paused (red) label keeps its color, since it is easier to read at a glance. Number boxes are left to the style on purpose: a style sheet that touches the border of a spin box takes its arrows away. The look was checked by drawing a mock of the panel under a virtual X server with Qt 6.4 in a light and a dark palette; the real application was not run, and Windows was not tried.

### Finding the games and assets

The engine reads `assets/` (the font, images) relative to the working directory, and the games are in `games/`. `XGEGUI` and `XGECLI` look for a folder that has both in the working directory, then next to the program, then in the folder above it (a Visual Studio build puts the program in `build/Debug` and the copies of `games/` and `assets/` in `build/`). The first one found becomes the working directory, so the program can be started from anywhere, and the file dialog opens in its `games/`. A game named on the command line is found the way `XGECLI` finds one (a bare name gets `.xml`; then as given, its file name alone, then in `games/`). The search is one piece of code in the library (`lib/include/data_folder.h`, tested in `tests/test_data_folder.cpp`), so the two programs cannot drift apart; `main.cpp` of each only calls it.

## Not done

- States are shown but not editable: a state on the stack is a copy of the one in the game's list, so a change would have to reach both.
- A text bound to a variable shows that variable (the row says what it shows); editing its text is overwritten the next time the variable changes.
- Sound (added later, [45](45-sound.md)): the Options dialog picks the sound library, which can be changed while a game is loaded (`Engine::replaceAudio`), and pausing silences what is playing (`Engine::silence`). A library that will not start leaves the game silent, with a message.
- Edits are not saved back to the XML.
- Built and run offscreen against Qt 6.4 with Xerces on Linux, with a smoke test that loaded Pong and Frogger, ran frames and edited a value; later (2026-10-01) run under a virtual X server switching between every video library ([39](39-opengl-backend-and-options.md)). The Windows build against vcpkg's Qt 6.11 follows from the same source but has not been run by an agent.
- On Windows, `XGEGUI` needs Qt's DLLs and its `platforms` plug-in next to it. vcpkg copies the DLLs after a build (it reads what the program links to) but not plug-ins, which Qt loads at run time, and the Qt 6 port has no hook for them. A post-build step in `executables.cmake` runs `windeployqt` through the `Qt6::windeployqt` target, which vcpkg points at `windeployqt.debug.bat` in Debug; the plain `windeployqt.exe` looks for release DLLs and fails on a debug build (vcpkg issues 36250 and 17840). It needs `qtbase[windeployqt]`. Qt's own install-time deploy functions are not used: they pick the release tool under Visual Studio's multi-configuration generator (vcpkg issue 39534).
