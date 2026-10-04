# 11. Backends, library, build system and code layout

**Status:** built: 4 XML, 4 window and 3 audio backends (plus silent), library + two programs, CMake modules, `output/` layout. The source tree is flat and wants folders.

## Backends

- Why: the prototype was hard-wired to Xerces and SFML. Each library sits behind an interface so the core never names one, licensing or weight problems can be dodged per backend, and a library can be swapped without touching game logic. A compiling two-backend example (pugixml and Xerces through the same `main.cpp`) proved it before adoption.
- **XML: `XmlDocument` / `XmlNode`** (read-only):

| Decision | Alternative | Why |
|---|---|---|
| Node is an **interface** | One concrete struct everything converts into | Libraries store data very differently (Xerces DOM, pugixml pool, RapidXML in place); a struct copies on every access |
| `getFirstChild()` / `getNextSibling()` return a fresh `unique_ptr` | Raw pointers into a cache; value handles | Several nodes alive while a subtree is walked; allocation is irrelevant at load |
| Unfiltered traversal, callers filter by name | Name-filtered or positional | Element order is not guaranteed |
| Read-only | Mutable | RapidXML non-owning cannot mutate; writing would be a separate interface |
| UTF-8 `std::string` | `wstring`, `XMLCh*` | Library-agnostic; Xerces transcodes at load |

- Interface hygiene: abstract base gets only `virtual ~X() = default;`. Ideas: a range-for iterator over children, a node pool per backend.
- **Validation follows the backend:** only Xerces does real XSD validation ("Strong"). The other three use `xsd_lite`, which reads the same `.xsd` through the same interface and checks only the subset the schema uses (sequence, element, complexType, attribute); deliberately permissive (unknown attribute types pass). `printGame()` says which ran. `tests/test_xml_format.cpp` pins what both must reject. Alternative: put validation in the engine layer so a new parser only has to parse.
- **Window: `Window`:** lifecycle, `init()` (build and measure each object's visual), `pollEvents()` (`{key, pressed}`), `clear`, `draw`, `display`, `setTitle`, `position`/`setPosition`. Everything in engine terms (`WindowDesc`, `Object`, `KeyCode`, `Color`, `Vector2f`); no library type leaks. `Object::size` is the one thing only a backend can measure (text and image). Backends: SFML3, Raylib, SDL2, OpenGL (GLFW), plus xgegui's Qt renderer. Every backend opens a window of its own and throws `std::runtime_error` if the library cannot start.
- **Audio: `Audio`:** `load()`, `play(name)`, `stopAll()`; SFML3, Raylib, SDL2, `NullAudio` ([09](09-sound.md)).
- **Factories** (`XmlDocumentFactory`, `WindowFactory`, `AudioFactory`) are the only places that know every implementation; a new library is one branch and one pair of files. Selection is a run-time factory; CMake options only decide which libraries get built.
- The word "backend" stayed over implementation/module/service/provider/adapter: it says "interchangeable lower layer" and still fits as subsystems grow (window, graphics, input, audio, network, XML, expressions, filesystem). The engine owns the loop and knows only interfaces; the game is plain data.
- Not an interface: lunasvg draws SVG inside the engine ([07](07-pictures-and-text.md)).
- Ideas: a software renderer and null implementations of every subsystem so the engine never checks for absence ([13](13-ideas.md)); forward declarations to keep third-party types out of headers (they are `PUBLIC` today, so programs including `engine.h` need them).

## Library and programs

- The engine is a library, `xgelib` (`lib/`). `xgecli` ([12](12-front-ends.md)) and `xgegui` are programs; `xgetest` links the library. `xgedata` copies `games/` and `assets/`. A new front end is a `main()` and a few lines in `executables.cmake`.
- Layout: one folder per project each with `source/` and `include/` (`lib/`, `cli/`, `gui/`); `tests/`, `games/`, `assets/` stay at the top. (Before, the engine's `source/`/`include/` sat at the top so the programs looked like extras.)
- **Static by default, shared with `-DXGE_BUILD_SHARED=ON`.** Static: `xgecli` is one file that runs anywhere beside its games and assets; nothing to mismatch. Shared: one copy, replaceable without relinking, but the library must be found at run time and on Windows every crossing class needs exporting (`WINDOWS_EXPORT_ALL_SYMBOLS`). With two small programs always built together, nothing is gained from sharing. Tests were run both ways.
- Rejected: one executable with a `--gui` switch; a `CMakeLists.txt` in every folder; third-party libraries `PRIVATE` (the engine's headers include exprtk, Xerces, SFML and others; hiding them behind interfaces is a separate change).
- Targets are all `XGE`-named; the CMake project is `XMLGameEngine`.

## Build system

- **One top-level `CMakeLists.txt`, logic in modules in `scripts/cmake/`:** `assets`, `dependencies`, `executables`, `options`, `output`, `platform`, `targets`, `tests`, `deploy_libraries`. Source and asset folders stay clean. An included `.cmake` runs at once and can also define functions called later. Rejected: a `CMakeLists.txt` per subdirectory.
- **Dependencies:** Xerces-C, exprtk, SFML 3, Raylib, GLFW + system OpenGL, SDL2 (+ SDL2_image, SDL2_ttf), TinyXML2, PugiXML, RapidXML, lunasvg, Catch2 (tests), Qt 6 (xgegui; found only). Each is found (vcpkg or system) or fetched with FetchContent; `FORCE_LOCAL_<NAME>` forces a fetched copy. `BUILD_TESTING` is off by default. Shell scripts install prerequisites for apt, dnf, pacman. C++20 (`cxx_std_20`).
- **`output/` layout:** programs, copies of `games/` and `assets/`, and `libraries/` (DLLs, or `.so` on Linux, and Qt plug-ins) go in `output/Debug` or `output/Release` (ignored by git). `output.cmake` sets `CMAKE_RUNTIME_OUTPUT_DIRECTORY` and `CMAKE_LIBRARY_OUTPUT_DIRECTORY` to `output/$<CONFIG>/libraries` before any target exists (so fetched dependencies follow); `xge_place_program()` moves each program up; the generator expression stops Visual Studio / Ninja Multi-Config adding a config folder of their own; `assets.cmake` copies into the same path; `platform.cmake` points SFML and Xerces back at `libraries/`. A single-config build with no `CMAKE_BUILD_TYPE` becomes Debug. Static libs and import `.lib`s stay in the build directory.
- **DLLs in `libraries/` on Windows:** Windows loads a program's DLLs before `main()` from its own folder, system folders and `PATH`; delay-loading is out (Qt and SFML export data). Chosen: a private assembly. Each program embeds a manifest (MSVC links `xge_libraries.manifest`, made by `output.cmake`) that needs the assembly `libraries`; Windows finds `libraries/libraries.manifest`; every DLL listed there loads from `libraries/`, for the whole process (built and run on Windows with Visual Studio and vcpkg). On Linux and macOS the programs carry a relative run path (`$ORIGIN/libraries`), so a copied `output/<config>` still runs. After each build `deploy_libraries.cmake` moves the DLLs vcpkg copied next to the program (vcpkg follows each DLL's imports) into `libraries/` and writes the list from the folder, under a lock because the programs build in parallel. Qt: `windeployqt` through `Qt6::windeployqt` (vcpkg points it at the `.debug.bat` in Debug; the plain tool fails on debug builds; needs `qtbase[windeployqt]`), plug-ins in `libraries/plugins`, a `qt.conf` beside the program.
- **Gotchas:**
  - On Windows, SFML fetched from GitHub with FreeType from vcpkg can leave a malformed `optimized`/`debug` entry in FreeType's `INTERFACE_LINK_LIBRARIES`; `platform.cmake` strips them where consumed. Untried: `CMAKE_DISABLE_FIND_PACKAGE_Freetype`.
  - Fetching both SFML and raylib static (no vcpkg, no system packages) fails to link on Linux with duplicate `stbi_*` (and miniaudio) symbols; vcpkg DLLs do not. Building one as shared avoids it; not done.
  - vcpkg's raylib 6.0 is configured with `CUSTOMIZE_BUILD=ON`, which turns on every optional feature (`SUPPORT_CUSTOM_FRAME_CONTROL`, JPEG for `assets/paddle.jpg`). A raylib built another way may not load Pong's paddles; the backend then throws.
  - A CMake target named `m` collides with plutovg's `-lm`.

## Code layout and pipeline

- The description is treated like a program with a front end: **parse, validate, evaluate, (generate)**. An explicit validation stage makes it easy to see a bug in one stage leaking into another. The code has this shape: `game_xml` parses and validates, `game_expr` evaluates into `Object`s and `State`s; `generate` does not exist ([01](01-vision-and-format.md)).
- A July 2026 skeleton (empty files, branch `rewrite`, retired 2026-09-30) layered `core/engine` (loop phases input/update/render, a `system_*` tier under a `service_*` tier), `core/game` (`compile_*`, `model_*`), `window`, `xml`. Too layered for the code that existed; the tree was flattened into `lib/source` and `lib/include` with the same responsibilities. Keep its good ideas: three loop phases so update can run without a window; `model_object` and `model_states` are parallel definitions; what is genuinely runtime is the current state and live objects.
- **Open:** `lib/` is flat and growing (31 source and 32 header files). Group by responsibility: game loading and evaluation, engine loop, collision and commands, window backends, audio backends, XML backends. Visual Studio filters are built from directories, so this is only the layout on disk. Also open: a configuration service, whether a "live snapshot" (current state plus live objects) is its own type, rename `collisionData.basic` (a leftover of the old `basic="basic"` spelling).
- Second-batch ideas: construct the game in one step (parse, validate, evaluate; no object on failure; keep the constructor small by delegating to a builder); resolve verb names to handlers once at load so the running game never looks up strings ([13](13-ideas.md)); phase order input, update, collisions, render.
