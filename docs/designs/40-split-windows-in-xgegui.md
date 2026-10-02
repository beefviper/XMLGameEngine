# 40. One window or two in XGEGUI

**Status:** built (`gui/`, `lib/source/window*.cpp`, `Engine::pump()`, `Engine::isWindowOpen()`; `tests/test_engine_input.cpp`). Checked on 2026-10-02 on Linux under a virtual X server with Mesa's software OpenGL: the Qt renderer, the SDL2 backend and the OpenGL (GLFW) backend were switched between, in one window and in two, many times over, with the game playing, paused, stepped and closed from the game window's side. SFML 3 and raylib could not be built in that environment and were only compiled by reading; Windows is where the author runs it, and the agent has not.

## Background

[38](38-qt-front-end.md) and [39](39-opengl-backend-and-options.md) first put every video library inside the one XGEGUI window, next to the tree. Only the Qt renderer fits there naturally. SFML 3 and SDL2 were made to adopt a platform window that Qt made, and raylib and GLFW, which always make a window of their own, were made to draw to a buffer that was read back and drawn again by Qt. Sharing one OpenGL context between Qt and a library then went wrong in ways that took a context keeper around every call to fix. That embedding was the most expensive part of the front end and the least like what each library is meant for.

## Decision

XGEGUI shows the game in one of two layouts, and the user chooses.

- **One window (the default).** The game is drawn by the Qt renderer ([38](38-qt-front-end.md)) in the main window, on the left, with the controls and the tree on the right. No other video library is used in this layout.
- **Two windows.** The main window holds only the controls and the tree, and the game has a window of its own. A video library (SFML 3, SDL2, raylib, OpenGL) opens that window itself, exactly as `XGECLI` makes it. The Qt renderer can also be shown in two windows, in a plain window of the application's; that is what choosing the split by hand gives until another library is picked.

### Moving between them

- **View > Game in Its Own Window** switches to two windows and back. Switching to two keeps the video library as it is (the Qt renderer, if nothing else was picked), and the game moves out into its own window. Switching back to one window returns the game to the Qt renderer in the main window, and it carries on as it was.
- **Picking a video library other than the Qt renderer in the Options dialog** while there is one window switches to two windows. The user is asked first (OK and Cancel). Cancel keeps the video library as it was and still applies the rest of the dialog (the XML parser).
- The question has a check box, **Don't ask me again**, checked by default; OK with it checked turns the question off. The same switch is in the Options dialog as **Ask before the game moves to a window of its own**, so it can be turned back on.
- **The setting is kept in `xgegui.ini`**, in the folder the program is in, under `[Window]` as `warn_before_two_windows` (`true` or `false`; asking is the default). The file is written only when the setting changes, so a program that has never had it changed leaves no file; if the folder cannot be written, the setting still holds until the program ends. (`AppSettings`, `gui/include/app_settings.h`.) Nothing else is remembered between runs yet.
- **Closing the game's window** (the library's own window, or the application's) pauses the game and folds it back into the main window, drawn by the Qt renderer. **Closing the main window** ends the program.

### What is gone

The library backends have one way of opening: a window of their own. Everything that existed only to embed them was removed from `lib/` and `gui/`:

- `WindowTarget`, `Embedding` and `WindowFactory::embedding()`; the constructors of the four backends take only the window description.
- `Window::backBuffer()` and every read-back: raylib's render texture and `LoadImageFromTexture`, the OpenGL backend's framebuffer object, its hidden window and the framebuffer calls looked up through GLFW. `Bitmap` is no longer something a window hands to a front end.
- Adopting a foreign window: `sf::RenderWindow(handle)`, `SDL_CreateWindowFrom`, the handle conversion, SDL's logical size and SFML's view that scaled a game into a pane, and the vsync and frame limit that were switched off for an embedded window.
- `Window::activate()`, raylib's stored GLFW window, SFML's destructor that made its context current, and raylib's `close()` doing the same.
- In `gui/`: `EmbeddedWindow`, `QtContextKeeper`, `NativeSurface`, the native-widget attributes in `main.cpp`, `GameView::showFrame`, and the second page of the stage.
- `tests/test_window_target.cpp`, which only pinned the targets.

The engine and the `Window` interface are otherwise the same. `Engine::replaceWindow()` is still how the video library changes in a running game.

### What was added

- `Engine::isWindowOpen()` and `Engine::pump()`. A library's window needs its events handled even when the game is paused (it cannot be moved, resized or closed otherwise, and on Windows it is marked not responding), but pausing must stop the simulation. `pump()` lets the window poll without playing a frame, and keeps the key changes it reports for the next `step()`, so none is lost and none reaches the game while it is stopped. While paused in a library's window, the session pumps and draws the same picture once a frame; while paused in the Qt renderer it does nothing.
- `GameSession` reports `windowClosed()` when the window it draws in is closed by the user.
- `GameStage` is now only the place where the Qt renderer's picture is: the left pane in one window, a `GameWindow` in two. One `GameView` lives for as long as the stage and is moved between them.
- `AppSettings` and the Options and confirmation dialogs described above.

## Why the picture is an ordinary widget

The game view used to be a `QOpenGLWidget` when Qt had OpenGL, which presents each frame on the display's vertical blank. It is now an ordinary widget, drawn by Qt's raster engine, and the application uses no OpenGL through Qt at all. The reason is the one that caused the embedding code: Qt keeps its own record of which OpenGL context is current and trusts it, and a library makes its own context current on the same thread. What was seen on Linux: after a library had run, a `QOpenGLWidget` created or moved into the main window drew nothing, and a controls window that had once held one showed noise. A window that has held a `QOpenGLWidget` appears to go on flushing through OpenGL, which would explain both. Keeping the two apart would mean a guard around every call into a library again. With no Qt OpenGL there is nothing to guard.

The price is that the Qt renderer no longer waits for the display's refresh. On Windows the desktop compositor presents every window, so a raster window does not tear, but a frame can be shown a refresh late or twice; the game's own clock decides when it steps either way. If the smoothness matters, the way back is a `QOpenGLWidget` in the one-window layout only, with a guard around library calls; that is the code this design removed.

## Settled points

- **Pumping events.** The session's 2 ms timer plays the frames the clock says are due (`step()` reads the library's events), and pumps and redraws while paused in a library's window. A window that is being dragged on Windows runs the system's own loop inside the library's event call, which stops the game for as long as it is held, as it does in `XGECLI`.
- **Pacing.** A library's window waits out the frame time itself (SFML's frame limit, SDL's vertical sync, raylib's target FPS, GLFW's own wait), as it does in `XGECLI`. The session's clock decides which frames are due and the window's wait agrees with it. The wait is inside the Qt timer's call, so the controls answer between frames and can lag by up to a frame. If that is felt, the fix is a way to tell a backend not to wait; there is none yet.
- **Pause, Step and Reset** are in the controls window. Pausing stops the simulation whichever window has the focus.
- **Window size and place.** The game window, whichever video library draws it, opens where the last one was (a library's window is asked its place with `Window::position`, and placed with `setPosition`), so changing library keeps it in one spot; failing that, the library's window opens as it does in `XGECLI` and the application's own opens at the game's size beside the main window when there is room. The main window shrinks to the width of the controls in two windows and gets its width back in one.
- **Remembered places.** `xgegui.ini` keeps where the main window was in one window, where it was in two, and where the game window was (`[Windows]` one_window, controls, game), written when the program closes. A place that is no longer on a screen is ignored. It also keeps whether it was in one window or two (`two_windows`) and the video library and XML parser chosen (`[Session]` video, xml, by fixed names such as `raylib`), and starts that way; a video library other than the Qt renderer always starts in two windows. The last game opened (`[Session]` game) is opened again when the program is started with no game named; the file picker only appears if there is none or it will not load. The two places of the main window and the game window's corner are kept; the game window's size is not.
- **Keys.** A library's window reads its own keyboard. In the Qt renderer the game view reads Qt's keys (`KeyQueue`), and takes the focus when clicked.
- **Failure.** If a library will not start, the game is drawn with the Qt renderer and a message says why; when that happened while switching from one window to two, the layout stays one window.

## Approximations

- Not run on Windows, nor with SFML 3 or raylib, by the agent. The paths most likely to differ are raylib's single-window rule (the old window is destroyed before the new one is made, which `replaceWindow` does) and SFML's window being created from Qt's timer.
- A library's window is placed by the system, so on a desktop with no window manager (a bare virtual X server) it can open over the controls.
- Only the warning setting, the window places and how the program was left are kept between runs.
- A library's window position is the system's idea of it (its frame for some, its drawing area for others), so changing library can move the window by the height of a title bar.
