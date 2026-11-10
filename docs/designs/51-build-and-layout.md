# Build system and code layout

**Status:** Built. Reference: [the engine](../reference/engine.md#building).

## Build system

- **One top-level `CMakeLists.txt`, the logic in modules in `scripts/cmake/`** (`assets`, `dependencies`, `executables`, `options`, `output`, `platform`, `targets`, `tests`, `deploy_libraries`). Rejected: a `CMakeLists.txt` per folder.
- **Dependencies** are found through vcpkg or the system, or fetched; `FORCE_LOCAL_<NAME>` forces a fetch. One macro, `xge_dependency()`, does the finding, status line and fetch for every library (it was written out twelve times before). Qt 6 and libxslt are only found: without Qt there is no `xgegui`, without libxslt `--generate` says so.
- **Only the backends built are looked for.** All eight were once always built, so one game fetched SDL2, raylib, GLFW and three XML libraries. **Chosen:** SFML 3 and Xerces by default, the rest by `XGE_WITH_<NAME>` or `XGE_ALL_BACKENDS`, and `BUILD_TESTING` implies all, since the tests play every game through every backend. The OpenGL window needs SDL2_image and SDL2_ttf (`XGE_NEEDS_SDL2`). Rejected: plug-in libraries, not worth it for four small wrappers.
- **Warnings.** A fetched library once passed `-w` on its interface, silencing the engine itself. **Chosen:** `xge_warnings()` gives the engine's targets one strict set (GCC and Clang: all, extra, pedantic, shadow, conversion, old-style-cast and more; MSVC `/W4` and extras); third-party headers are system headers; fetched sources build with `-w`. No `-Werror`: a new compiler should not break a user's build.
- **Sanitizers.** **Chosen:** `XGE_SANITIZE` (`address`, which brings `undefined`, or `thread`, the two cannot be mixed), off by default, on the engine's own targets through `xge_warnings()`, which then drops the strict warnings: with both, GCC 13 took over fifteen minutes on each large file, seconds without. GCC and Clang only; MSVC has `/fsanitize=address` alone, a possible later addition. Rejected: instrumenting the fetched libraries (a rebuild of everything for little).
- **`output/<Config>/`** holds the programs, a copy of `games/` and `libraries/`. Directories are set before any target so fetched libraries follow.
- **DLLs in `libraries/` on Windows** by a private assembly: each program embeds a manifest naming the assembly `libraries`, whose manifest lists every DLL. Delay-loading was out (Qt and SFML export data). Linux and macOS use the run path `$ORIGIN/libraries`. Qt plug-ins go through `windeployqt`.
- C++20. Shell scripts install prerequisites for apt, dnf and pacman.

### Gotchas

- SFML fetched with FreeType from vcpkg can leave malformed `optimized`/`debug` link entries; `platform.cmake` strips them.
- Fetching SFML and raylib both static fails on Linux with duplicate `stbi_*` symbols; `-DBUILD_SHARED_LIBS=ON` links.
- vcpkg's raylib turns on every feature (JPEG for Pong's paddles); another raylib may not load them.
- A target named `m` collides with plutovg's `-lm`.
- Nothing builds the default configuration automatically, so a change to the `#ifdef`s needs a build with no options as well as with tests.

## Layout

- **Two groups at the top:** `xge/` is the engine and its programs (`lib/`, `cli/`, `gui/`, `tests/`); `games/` is what a game is made of (`xgedef.xsd`, `descriptions/`, `assets/`, `generators/`). The root keeps `docs/`, `scripts/` and the build files.
- **The programs run in `games/`,** so asset paths in a game (`assets/paddle.jpg`) stay short and a generated program has `assets/` beside it. Rejected: the repository root as working directory.
- **The schema sits beside the games,** so `../xgedef.xsd` is right in the repository and in the output alike. When it is not there, `findSchema` takes the nearest one above.
- **Engine folders:** a folder per part, header beside source, a short name inside (`game/`, `play/`, `load/`, `xml/`, `window/`, `audio/`, `picture/`, `engine/`). Includes name the folder (`"window/sfml.h"`). Names avoid Windows macros (`near`, `min`, `small`, `interface`).
- **Generated games apart:** `xgetest-generated` is excluded from the default build, and building it generates every game. `--generate` reads the engine from the repository (`XGE_ENGINE_FOLDER`), so a generating `xgecli` needs the repository it was built from.

## Pipeline

- A description is treated like a program: **parse, validate, evaluate, (generate)**. `load/xml` parses and validates into a `GameData`; `load/expr` evaluates it into objects and states. Generating is the parse stage done ahead of time by XSLT, so a generated program starts at evaluate.
- An early skeleton had many layers (`core/engine` with system and service tiers); too layered for the code. Kept from it: update can run without a window, and what is truly live is the current state and objects.
- Open: whether a live snapshot of a game is its own type.
