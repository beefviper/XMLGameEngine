# 39. OpenGL backend, window targets and the Options dialog

**Status:** built (`lib/source/window_opengl.cpp`, `lib/include/window.h`, `Engine::replaceWindow()`, `gui/`; `tests/test_window_target.cpp`). On 2026-10-01 all four libraries were built on Linux (raylib 6.0 configured the way vcpkg builds it, against GLFW 3.4; SFML 3.1; Qt 6.4) and XGEGUI was run under a virtual X server with a software OpenGL, switching a running Pong, Space Invaders, Lunar Lander and Frogger between SFML 3, raylib, OpenGL and the Qt renderer many times over. That is how the shared-context bug below was found and checked. Windows is where the author runs it; the agent has not.

## Decision

### The OpenGL backend

`WindowBackend::OpenGL` (`-w opengl`) is a fourth `Window` backend. GLFW makes the window and the OpenGL context and reads the keyboard. Everything is drawn with OpenGL itself: a projection of the window in pixels and one textured rectangle for each object, the same bake-once-then-rebuild-on-`visualDirty` shape as the other backends.

- **No loader library.** The drawing is plain OpenGL 1.1, which the system's OpenGL library exports as it is (`opengl32` on Windows, `libGL` on Linux, linked through `OpenGL::GL`), so there is no glad or GLEW to depend on. The five framebuffer-object functions the back buffer needs (below) are looked up through `glfwGetProcAddress`.
- **Pictures are made on the CPU.** A circle or rectangle is filled in, a sprite of lines is already a `Bitmap`, and each is uploaded as a texture. Text and images are decoded by SDL2_ttf and SDL2_image, which the library already links for the SDL2 backend, so text looks the same as the SDL2 backend's; with no font file, text falls back to the built-in font as everywhere else. An image that cannot be loaded throws `std::runtime_error`, as it now does in every backend (they used to end the program).
- **Frame time.** A window of its own waits out the frame time itself, and does not wait for the display's refresh: waiting for both would make a 60 frame a second game run at whatever the refresh divides into. Because of that it can tear in its own window; inside XGEGUI the picture is shown by Qt.
- **GLFW** is found with `find_package(glfw3)`. It is the same GLFW raylib is built on (vcpkg installs it for raylib); with a raylib built from source, raylib's own `glfw` target is used. Either way XGELIB links one target called `glfw`.

### Where a window draws

A `Window` used to always open a window of its own. `WindowFactory::create` now takes a `WindowTarget`:

| Kind | Meaning | Backends |
|---|---|---|
| `OwnWindow` (the default) | opens its own window, as before | all |
| `NativeWindow` | draws into a platform window (`nativeHandle`, an HWND on Windows) that the front end made and keeps | SFML3, SDL2 |
| `BackBuffer` | opens no visible window and draws to a buffer of the game's size, handed over after each frame by `Window::backBuffer()` | raylib, OpenGL |

`WindowFactory::embedding(backend)` says which of the last two a backend supports, and `create` throws `std::invalid_argument` for one it does not. The split is by what the library allows: SFML and SDL2 can adopt a window someone else made (`sf::RenderWindow(handle)`, `SDL_CreateWindowFrom`), while raylib and GLFW always make a window of their own, so they draw to a texture (raylib's `RenderTexture2D`, OpenGL's framebuffer object) that is read back and shown by the front end. This was chosen over reparenting raylib's or GLFW's window into Qt with Win32 calls, which is fragile (keyboard focus, sizing, and the window briefly showing on its own). The interface did not change for the engine, the same way raylib's polling did not: only the backends adapted.

Two small additions to `Window` go with it, both with a do-nothing default: `backBuffer()` (above) and `activate()`, which makes the window's graphics context current again. A front end that shares the thread with another user of OpenGL (Qt's own OpenGL widget) calls it before each frame, because the other one leaves its context current; SFML remembers which context it made current and would not notice, so its `activate()` lets go and takes it again. `Engine::replaceWindow(create)` swaps the window of a running engine: the old one is destroyed first (raylib and GLFW can only have one window), the new one is made by `create`, and every object's picture is built again; the game itself, including the state it is in, is untouched. (The `Engine` constructor cannot do this: it pushes the first state.)

### The Options dialog

XGEGUI has a toolbar (Open Game, Options) and Options in the File menu. The dialog has a **Video** dropdown (SFML 3, SDL2, raylib, OpenGL, and the Qt renderer from design 38) and an **XML parser** dropdown (Xerces, TinyXML2, PugiXML, RapidXML); like XGECLI, SFML 3 and Xerces are what is used until something else is chosen. Nothing changes until OK.

- The game **waits while the dialog is open** (it cannot be played from there: the dialog has the keyboard), and when the dialog closes it carries on if it was playing, whatever was chosen. Until 2026-10-01 it ran on under the dialog and was paused only after a change of library.
- A new **video** library is given the game as it is: the old window is destroyed, the new one made, every picture built again, and the picture drawn.
- A new **XML parser** has to read the file again, so the game starts over from the beginning.
- If a library will not start (for instance SFML3 where its window cannot be adopted) the game is drawn with the Qt renderer instead, and a message says why.

`GameStage` holds the two ways of showing the game: the `GameView` (a picture: the Qt renderer's, or a back buffer's pixels via `EmbeddedWindow`) and a `NativeSurface` (a widget with a platform window, scaled to the largest size in the game's proportions that fits the pane, and kept in the middle). A new surface is made for every window because a library's choice of pixel format stays on the window it was made for. The keyboard is read by Qt in both (`KeyQueue`) and passed on by `EmbeddedWindow`, which wraps a library's window and ignores that window's own keys, because Qt is what has the keyboard focus. In the native case the library does the scaling: SFML 3 is given a view of the whole game and SDL2 a logical size, so each draws the whole game into a window of any size.

### Sharing the thread with Qt's OpenGL

When Qt has OpenGL, the game view is a `QOpenGLWidget` and the whole main window is drawn by Qt with OpenGL. raylib and GLFW also draw with OpenGL, on the same thread, and every library makes its own context current behind Qt's back. Qt keeps its own record of which context is current and trusts it: when it believes its context is still current it skips making it current, and draws into whatever is. The first version only made the library's context current before each frame (`activate()`) and left it current afterwards, so Qt then drew its widgets into raylib's or GLFW's context and they drew into Qt's (texture and buffer names are small numbers in every context, so each used the other's). On Linux that showed as noise over the picture and a blank raylib frame; on Windows it crashed soon after switching to raylib or OpenGL. SFML 3 and SDL2 were not seen to be affected (SDL2's renderer on Windows is Direct3D, not OpenGL); the same rule covers them now anyway.

The rule now: every call into a library's window (making it, each frame, freeing it) is wrapped by `QtContextKeeper` (`gui/include/embedded_window.h`), which notes the context Qt has current, and makes it current again afterwards on the surface it was on; the library's own context is made current before the call. A frame keeps the library's context from `clear()` to `display()`, so the objects in between are not each paid for. `RaylibWindow::close()` makes raylib's context current before it frees its textures (the OpenGL and SFML backends already did), which matters to a program without Qt too.

A second, smaller fault went with it. `NativeSurface` asked Qt for a platform window, and Qt then gave one to every widget above it and beside them (the splitter, the inspector, ...). A main window drawn with OpenGL cannot make its context current on those plain windows (Qt warned *Failed to make context current. Expect bad things to happen.*), and drew into whatever context was current instead. The surface now sets `Qt::WA_DontCreateNativeAncestors` and the application `Qt::AA_DontCreateNativeWidgetSiblings`, so only the surface itself has a platform window.

The crash the author actually saw on switching to raylib had a third cause. `Engine::replaceWindow` destroys the old window before it makes the new one, and making a back-buffer window first shows the stage's picture page. Showing it moves the keyboard focus, and an inspector text box that loses the focus reports a finished edit, so the inspector redrew the game in the middle of the switch, with no window: a null pointer. Text boxes now only count a real change, and `GameSession` ignores step, reset and redraw while `load()` or `applyOptions()` is under way. A test that drives `MainWindow::showOptions()` through the real dialog found it; the earlier tests called `applyOptions()` directly and missed it.

## Approximations

- **Run on Linux only by the agent.** The SFML3 and SDL2 paths into a Qt-made HWND and raylib's and GLFW's hidden windows are the parts most likely to differ on Windows. The Qt renderer is always there to switch back to.
- **SDL2 inside XGEGUI on Linux** drew nothing under the virtual X server, and left the rest of the screen garbled when it closed; SDL2 on Windows draws with Direct3D, where the author has it working. Not looked into further.
- **vcpkg's raylib 6.0 is built with every optional feature on.** Its port configures raylib with `CUSTOMIZE_BUILD=ON`, and raylib 6.0's `ParseConfigHeader.cmake` reads every `#define SUPPORT_... 0` in `config.h` as on. That is why `SUPPORT_CUSTOM_FRAME_CONTROL` is set in the author's build (see `RaylibWindow::display()`), and also why its raylib can read JPEG, which a default raylib cannot (`assets/paddle.jpg`, used by Pong). A raylib built some other way may not load Pong's paddles; the backend then throws instead of ending the program.
- **Native windows are scaled by the library, not by Qt.** The surface changes size with the pane, so the picture is the library's own scaling (a stretch, but the proportions are kept).
- **raylib's context** is made current again through GLFW (`glfwGetCurrentContext()` right after raylib starts), which assumes raylib and the library share one GLFW (true with vcpkg's dynamic raylib). If not, `activate()` does nothing for raylib and it can draw into Qt's context.
- The OpenGL backend draws with straight (not premultiplied) alpha, so the edges of text can differ from the other backends by a shade.
- The options are not remembered between runs.
