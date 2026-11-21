# Notes: backends, build, generator, xgegui

Tips for changing the libraries behind the engine, the build, the generator and the Qt app. Back to [the notes](readme.md).

## Backends and build

- Never destroy a library's window after creating the next (raylib, GLFW). A front end that pauses calls `Engine::pump()`.
- Pace is `Engine::loop`'s alone: no window sets a frame rate or waits out a frame (vsync on is fine). `xgetest "[pace]"` checks it in real time.
- raylib: `pollEvents()` must not call `PollInputEvents()`; Escape is not its exit key; a texture drawn into a render texture must live until `EndTextureMode()`.
- SDL is shared: each of window and sound starts only its subsystem; the last out calls `SDL_Quit()`.
- Backend files compile only behind `XGE_WITH_<NAME>`: build with no options as well as with tests when touching factories, `cli.cpp` or `session_options.cpp`.
- SFML and raylib both static from source fail to link on Linux; `-DBUILD_SHARED_LIBS=ON` links. A target named `m` collides with plutovg's `-lm`.

## Generator

- `engine-cpp` writes only the file's tree (`tree.xsl`); a new tag needs nothing there. The tree is static arrays, so a big game costs no stack.
- XSLT comments cannot hold `--`; libxml2 refuses the stylesheet.
- A new engine source a generated program needs goes in `engine.xml` and `assets.cmake`; one that reads XML or chooses libraries stays out (keep `game/game.cpp` and `engine/engine.cpp` free of `load/xml.h` and the factories).
- After a generator or reader change, `cmake --build build --target xgetest-generated`.

## xgegui

- No OpenGL through Qt: do not bring back a `QOpenGLWidget` without a guard around every library call.
- Anything running while `GameSession` swaps game or window checks its `changing` guard.
- Do not style `QSpinBox` borders (the arrows vanish).
- Left and right Shift come from the native scan code (`key_queue.cpp`).
