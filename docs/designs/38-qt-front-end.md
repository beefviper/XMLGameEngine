# 38. Qt front end

**Status:** built (`gui/`, `scripts/cmake/executables.cmake`; `Engine::step()`/`render()`, `Game::setVariable()`/`getStates()`)

## Decision

`XGEGUI` is a Qt 6 Widgets application. The game is on the left; on the right, from the top, are the **Play/Pause**, **Step** and **Reset** buttons, a line saying the current state, the frame number and whether it is running, and a tree of everything in the game.

### Showing the game

Options were to embed one of the existing windows (SFML and SDL2 can draw into a window handle Qt gives them; raylib cannot take a foreign window at all), or to write one more `Window` backend that draws the picture itself. The second is built: `QtWindow` (`gui/source/qt_window.cpp`) implements `Window` like the SFML, raylib and SDL2 backends, draws every object with QPainter into an image, and `GameView` shows that image as large as fits with the game's own proportions. One backend then works the same on every platform and is resized by Qt, rather than three platform-specific embeddings, only one of which would be complete. It is in `gui/`, not `lib/`, so the engine library needs no Qt. Embedding the other backends stays possible later; the window choice is not exposed in the application yet.

The engine is driven from a Qt timer instead of `Engine::loop()`, at the game's own frame rate (a game moves a fixed amount a frame, so that is also its speed). `loop()` is now `step()` (read the keys, move, check conditions) followed by `render()` (draw without moving anything), so a frozen game can be redrawn after an edit. Pause really stops the simulation, which is separate from a game's own pause state; Step advances one frame while frozen; Reset is `Game::resetAll()`.

Keys go to the game only while the game view has the focus (click it); the buttons do not take the focus, so pressing one does not stop the keyboard working. Keys held when the focus leaves are released.

### The tree

The tree is laid out like the XML:

- **Window**: name, size, background (editable), full screen, frame rate.
- **Objects**: one entry per object. A `<group>` and a `<grid>` are each shown once, with their members under them (a count is on the row), so how the game was written can be seen. Each object has class, visible, sprite, position, velocity, acceleration, collision (enabled, type, the rules, and the edge commands), variables and actions.
- **States**: each state's shown objects, key bindings and conditions, written the way `printGame()` writes them.

A value that can change has an editor on its own row: a spin box with up and down arrows for a number (type a value or click), a check box for yes/no, a text box for text. A change is made on the running object at once, and the picture is redrawn straight away when the game is frozen. Editable: class, visible, a sprite's size, color and text, position, velocity, acceleration, collision on/off, every object variable (through `Game::setVariable()`, so a text showing it follows), and the window background. Values read from the game every tenth of a second keep the tree current while it runs; a box being typed in is left alone.

Editors are only made when their parent row is first opened, because a game has thousands of values (Frogger has 110 objects) and most are never looked at.

### Finding the games and assets

The engine reads `assets/` (the font, images) relative to the working directory, and the games are in `games/`. `XGEGUI` and `XGECLI` look for a folder that has both in the working directory, then next to the program, then in the folder above it (a Visual Studio build puts the program in `build/Debug` and the copies of `games/` and `assets/` in `build/`). The first one found becomes the working directory, so the program can be started from anywhere, and the file dialog opens in its `games/`. A game named on the command line is found the way `XGECLI` finds one (a bare name gets `.xml`; then as given, its file name alone, then in `games/`). The search is one piece of code in the library (`lib/include/data_folder.h`, tested in `tests/test_data_folder.cpp`), so the two programs cannot drift apart; `main.cpp` of each only calls it.

## Not done

- States are shown but not editable: a state on the stack is a copy of the one in the game's list, so a change would have to reach both.
- A text bound to a variable shows that variable (the row says what it shows); editing its text is overwritten the next time the variable changes.
- No sound exists in the engine, so the application has none.
- Edits are not saved back to the XML.
- Built and run offscreen against Qt 6.4 with Xerces on Linux, with a smoke test that loaded Pong and Frogger, ran frames and edited a value; the Windows build against vcpkg's Qt 6.11 follows from the same source but has not been run by the agent that wrote this.
- On Windows, `XGEGUI` needs Qt's DLLs and its `platforms` plug-in next to it. vcpkg copies the DLLs after a build (it reads what the program links to) but not plug-ins, which Qt loads at run time, and the Qt 6 port has no hook for them. A post-build step in `executables.cmake` runs `windeployqt` through the `Qt6::windeployqt` target, which vcpkg points at `windeployqt.debug.bat` in Debug; the plain `windeployqt.exe` looks for release DLLs and fails on a debug build (vcpkg issues 36250 and 17840). It needs `qtbase[windeployqt]`. Qt's own install-time deploy functions are not used: they pick the release tool under Visual Studio's multi-configuration generator (vcpkg issue 39534).
