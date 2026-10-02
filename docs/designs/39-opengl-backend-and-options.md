# 39. OpenGL backend, window replacement and the Options dialog

**Status:** built (`lib/source/window_opengl.cpp`, `Engine::replaceWindow()`, `gui/`). On 2026-10-01 all four libraries were built on Linux (raylib 6.0 configured the way vcpkg builds it, against GLFW 3.4; SFML 3.1; Qt 6.4) and XGEGUI was run under a virtual X server with a software OpenGL, switching a running Pong, Space Invaders, Lunar Lander and Frogger between libraries. Windows is where the author runs it; the agent has not. The way those libraries are shown in XGEGUI was changed on 2026-10-02: see [40](40-split-windows-in-xgegui.md), which replaced the embedding this design first described.

## Decision

### The OpenGL backend

`WindowBackend::OpenGL` (`-w opengl`) is a fourth `Window` backend. GLFW makes the window and the OpenGL context and reads the keyboard. Everything is drawn with OpenGL itself: a projection of the window in pixels and one textured rectangle for each object, the same bake-once-then-rebuild-on-`visualDirty` shape as the other backends.

- **No loader library.** The drawing is plain OpenGL 1.1, which the system's OpenGL library exports as it is (`opengl32` on Windows, `libGL` on Linux, linked through `OpenGL::GL`), so there is no glad or GLEW to depend on.
- **Pictures are made on the CPU.** A circle or rectangle is filled in, a sprite of lines is already a `Bitmap`, and each is uploaded as a texture. Text and images are decoded by SDL2_ttf and SDL2_image, which the library already links for the SDL2 backend, so text looks the same as the SDL2 backend's; with no font file, text falls back to the built-in font as everywhere else. An image that cannot be loaded throws `std::runtime_error`, as it now does in every backend (they used to end the program).
- **Frame time.** A window of its own waits out the frame time itself, and does not wait for the display's refresh: waiting for both would make a 60 frame a second game run at whatever the refresh divides into. Because of that it can tear.
- **GLFW** is found with `find_package(glfw3)`. It is the same GLFW raylib is built on (vcpkg installs it for raylib); with a raylib built from source, raylib's own `glfw` target is used. Either way XGELIB links one target called `glfw`.

### Starting and failing

Every backend opens a window of its own, and throws `std::runtime_error` if the library cannot start (`WindowFactory::create`). `Engine::replaceWindow(create)` swaps the window of a running engine: the old one is destroyed first (raylib and GLFW can only have one window), the new one is made by `create`, and every object's picture is built again; the game itself, including the state it is in, is untouched. (The `Engine` constructor cannot do this: it pushes the first state.)

### The Options dialog

XGEGUI has Options in the File menu (there is no toolbar). The dialog has a **Video** dropdown (the Qt renderer from design 38, SFML 3, SDL2, raylib, OpenGL) and an **XML parser** dropdown (Xerces, TinyXML2, PugiXML, RapidXML); the Qt renderer and Xerces are what is used until something else is chosen. Nothing changes until OK. It also has the switch for the question asked before the game moves to a window of its own ([40](40-split-windows-in-xgegui.md)).

- The game **waits while the dialog is open** (it cannot be played from there: the dialog has the keyboard), and when the dialog closes it carries on if it was playing, whatever was chosen.
- A new **video** library is given the game as it is: the old window is destroyed, the new one made, every picture built again, and the picture drawn.
- A new **XML parser** has to read the file again, so the game starts over from the beginning.
- If a library will not start the game is drawn with the Qt renderer instead, and a message says why.

### What was tried first, and removed

The first version showed every library inside the one XGEGUI window: SFML 3 and SDL2 adopted a platform window Qt made, and raylib and GLFW drew to a hidden window and an off-screen buffer that Qt read back and drew again, with `WindowTarget`, `Window::backBuffer()` and `Window::activate()` in the interface. The lesson that stayed: two users of OpenGL on one thread behind each other's back go wrong. Qt trusts its own record of which context is current, so a library that makes its own current leaves Qt drawing into the library's context and the library into Qt's; on Linux that was noise over the picture and a blank raylib frame, and on Windows a crash soon after switching to raylib or OpenGL. A guard around every call into a library fixed it, at the cost of most of the front end's complexity. Design 40 took the libraries out of the main window instead.

One thing from that work is still true and still matters: the swap of a running engine's window must tolerate anything that reports an edit in the middle of it. The inspector's text boxes report a finished edit when they lose the focus, and showing a page of the stage moved the focus, so the inspector redrew the game with no window. `GameSession` ignores step, reset and redraw while `load()` or `applyOptions()` is under way, and text boxes only count a real change.

## Approximations

- **Run on Linux only by the agent**, and not at all since the change of 2026-10-02 for SFML 3 and raylib.
- **vcpkg's raylib 6.0 is built with every optional feature on.** Its port configures raylib with `CUSTOMIZE_BUILD=ON`, and raylib 6.0's `ParseConfigHeader.cmake` reads every `#define SUPPORT_... 0` in `config.h` as on. That is why `SUPPORT_CUSTOM_FRAME_CONTROL` is set in the author's build (see `RaylibWindow::display()`), and also why its raylib can read JPEG, which a default raylib cannot (`assets/paddle.jpg`, used by Pong). A raylib built some other way may not load Pong's paddles; the backend then throws instead of ending the program.
- The OpenGL backend draws with straight (not premultiplied) alpha, so the edges of text can differ from the other backends by a shade.
- Only the question about two windows is remembered between runs ([40](40-split-windows-in-xgegui.md)); the options themselves are not.
