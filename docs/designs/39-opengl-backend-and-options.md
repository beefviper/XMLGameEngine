# 39. OpenGL backend, window targets and the Options dialog

**Status:** built (`lib/source/window_opengl.cpp`, `lib/include/window.h`, `Engine::replaceWindow()`, `gui/`; `tests/test_window_target.cpp`). The OpenGL backend was run on Linux with a software OpenGL (title screen through the back buffer, and in a window of its own); the SDL2 path into a window it did not make was run against a plain X11 window. Nothing was run on Windows, and SFML3 and raylib were only written, not compiled (the sandbox has neither).

## Decision

### The OpenGL backend

`WindowBackend::OpenGL` (`-w opengl`) is a fourth `Window` backend. GLFW makes the window and the OpenGL context and reads the keyboard. Everything is drawn with OpenGL itself: a projection of the window in pixels and one textured rectangle for each object, the same bake-once-then-rebuild-on-`visualDirty` shape as the other backends.

- **No loader library.** The drawing is plain OpenGL 1.1, which the system's OpenGL library exports as it is (`opengl32` on Windows, `libGL` on Linux, linked through `OpenGL::GL`), so there is no glad or GLEW to depend on. The five framebuffer-object functions the back buffer needs (below) are looked up through `glfwGetProcAddress`.
- **Pictures are made on the CPU.** A circle or rectangle is filled in, a sprite of lines is already a `Bitmap`, and each is uploaded as a texture. Text and images are decoded by SDL2_ttf and SDL2_image, which the library already links for the SDL2 backend, so text looks the same as the SDL2 backend's; with no font file, text falls back to the built-in font as everywhere else. An image that cannot be loaded throws `std::runtime_error` (the SDL2 backend exits).
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

- A new **video** library is given the game as it is: the old window is destroyed, the new one made, every picture built again, and the picture drawn. The game is left **paused**; the user plays it again.
- A new **XML parser** has to read the file again, so the game starts over from the beginning, also paused.
- If a library will not start (for instance SFML3 where its window cannot be adopted) the game is drawn with the Qt renderer instead, and a message says why.

`GameStage` holds the two ways of showing the game: the `GameView` (a picture: the Qt renderer's, or a back buffer's pixels via `EmbeddedWindow`) and a `NativeSurface` (a widget with a platform window, scaled to the largest size in the game's proportions that fits the pane, and kept in the middle). A new surface is made for every window because a library's choice of pixel format stays on the window it was made for. The keyboard is read by Qt in both (`KeyQueue`) and passed on by `EmbeddedWindow`, which wraps a library's window and ignores that window's own keys, because Qt is what has the keyboard focus. In the native case the library does the scaling: SFML 3 is given a view of the whole game and SDL2 a logical size, so each draws the whole game into a window of any size.

## Approximations

- **Not run on Windows.** The SFML3 and SDL2 paths into a Qt-made HWND, and raylib's hidden window, are the parts most likely to need a fix. Each is also the first thing to try again if a picture does not appear: the Qt renderer is always there to switch back to.
- **Native windows are scaled by the library, not by Qt.** The surface changes size with the pane, so the picture is the library's own scaling (a stretch, but the proportions are kept).
- **raylib's context** is made current again through GLFW (`glfwGetCurrentContext()` right after raylib starts), which assumes raylib and the library share one GLFW (true with vcpkg's dynamic raylib). If not, `activate()` does nothing for raylib and it can draw into Qt's context.
- The OpenGL backend draws with straight (not premultiplied) alpha, so the edges of text can differ from the other backends by a shade.
- The options are not remembered between runs.
