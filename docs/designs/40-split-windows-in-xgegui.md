# 40. One window or two in XGEGUI

**Status:** idea (nothing built; describes a later version of `gui/` and what it would let `lib/` drop)

## Background

[38](38-qt-front-end.md) and [39](39-opengl-backend-and-options.md) put every video library inside the one XGEGUI window, next to the tree. Only the Qt renderer fits there naturally. The others had to be adapted: SFML 3 and SDL2 adopt a platform window that Qt made (`NativeWindow`), and raylib and GLFW, which always make a window of their own, draw to a buffer that is read back and shown by Qt (`BackBuffer`). For raylib and OpenGL that means the picture is drawn, copied to the CPU and drawn again by Qt, which is more work than the original window would have taken, and it is where the sharing of one OpenGL context between Qt and a library went wrong ([39](39-opengl-backend-and-options.md), "Sharing the thread with Qt's OpenGL"). The embedding is the part of the front end that needs the most care and is the least like what each library is meant for.

## Idea

Give XGEGUI two ways of showing the game and let the user choose:

- **One window (the default).** The game is drawn in the main window by Qt's own surface: the Qt renderer of [38](38-qt-front-end.md) (`QtWindow` drawing into `GameView`), with the controls and tree beside it. No other video library is offered in this layout, because the Qt surface is the simplest thing that works and the one that is already scaled, focused and redrawn by Qt.
- **Split.** An option separates the two: one Qt window holds only the controls and the tree, and the game runs in a second window that belongs to the chosen video library (SFML 3, SDL2, raylib, OpenGL) and is made with `WindowTarget::OwnWindow`, exactly as `XGECLI` makes it. Every backend then works in XGEGUI, because none has to be embedded.

## What it would remove

The split layout has the library own its window from creation to destruction, so most of what exists only to embed them is not needed on that path:

- **No `BackBuffer`** copy for raylib and OpenGL (no read-back, no `EmbeddedWindow` for them), and no framebuffer object just for XGEGUI.
- **No `NativeWindow`**: no platform handle from Qt, no `NativeSurface`, no scaling the library does inside a pane, no per-window pixel-format workaround, no `Qt::WA_DontCreateNativeAncestors` / `AA_DontCreateNativeWidgetSiblings`.
- **No `QtContextKeeper` and no `activate()`.** The cause of that code was two users of OpenGL on one thread behind each other's back. In the split layout the controls window needs no OpenGL (the `QOpenGLWidget` game view is not in it), so Qt's context and the library's no longer meet.
- **The keyboard** goes to whichever window has the focus, so the game's keys are read by the library's own window as in `XGECLI`; `KeyQueue` and the focus bookkeeping (keys held when the focus leaves, buttons that must not take the focus) apply only to the one-window layout.
- **Switching library** is closing one window and opening another (`Engine::replaceWindow`, which already does that), with no surface to rebuild.

The engine and the `Window` interface do not change. What is not removed: `WindowTarget` and the two embedding paths may stay in the library for a front end that wants them, or be dropped once nothing uses them; that is a separate choice.

## Things to settle

- **Pumping the library's events.** SFML, SDL2, raylib and GLFW each need their events polled, which each backend already does in `step()`; with the engine driven from the Qt timer ([38](38-qt-front-end.md)) that keeps working, but a second window that is dragged or blocked by the system must not stall the Qt timer or the reverse.
- **Closing either window.** Closing the game window should pause (or end the session and offer to reopen it); closing the controls window ends the program.
- **Pause, Step and Reset** are in the controls window, so the game window is never clicked to use them; pausing while the game window has the focus still has to stop the simulation, not just the drawing.
- **Window size and place.** The game window opens at the game's own size, as in `XGECLI`; the layout would remember where it was put (options are not remembered yet, [39](39-opengl-backend-and-options.md)).
- **Tearing.** A window of its own waits out the frame time itself and does not wait for the display ([39](39-opengl-backend-and-options.md)); that is the same as `XGECLI` and is not new, but it is visible in a way it is not under Qt's OpenGL widget.
- **One window, other libraries.** If a library should still be tried inside the main window, the embedded paths would have to stay; the idea here is to drop them from the user's choices rather than from the code at first.

## Why

One path for each layout, and each path as direct as it can be: the Qt surface in one window, and the libraries' own windows in the other. The embedding was the larger cost of showing four libraries in one window, and a second window removes the need for it instead of working around it.
