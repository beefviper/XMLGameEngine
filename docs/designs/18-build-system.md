# 18. Build system

**Status:** implemented

## Decision

One top-level `CMakeLists.txt`, with the logic split into modules in `scripts/cmake/` (`assets`, `dependencies`, `executables`, `options`, `platform`, `targets`, `tests`). Source, include and assets directories stay clean; there is no `CMakeLists.txt` in every directory. The `XGELIB` target is a library and `executables.cmake` defines the programs built on it ([35](35-library-and-front-ends.md)).

The alternative proposed first was a `CMakeLists.txt` in each subdirectory (main, source, data). The author preferred a single root file plus a `cmake/` (or `scripts/cmake/`) directory.

## How the modules behave

A `.cmake` file included from the main file runs immediately. A file can both run some code at once and define functions the main file calls later, when it wants control of timing. Repeated logic was factored into per-file helper macros and functions.

## Dependencies

- Libraries: Xerces-C, exprtk, SFML 3 (and the other backends: Raylib, GLFW and the system OpenGL, SDL2 with SDL2_image and SDL2_ttf, TinyXML2, PugiXML, RapidXML), lunasvg (for `<svg>` sprites, used inside the engine and linked `PRIVATE`; [43](43-svg-sprites.md)), Catch2 for tests, and Qt 6 for XGEGUI (found only; without it XGEGUI is left out).
- Each is found (vcpkg or the system) or fetched and built with FetchContent. `FORCE_LOCAL_<NAME>` options force a fetched copy.
- `BUILD_TESTING` is off by default, so the test suite (Catch2) is opt-in.
- Platform shell scripts exist for `apt`, `dnf` and `pacman` prerequisites.

## A build gotcha worth remembering

On Windows, when SFML is fetched from GitHub but finds FreeType through vcpkg, a malformed `optimized`/`debug` entry can show up in FreeType's `INTERFACE_LINK_LIBRARIES`. It is a leftover of CMake's older keyword convention and is not a valid path. The fix used is a small guarded loop that strips those keywords (right where it is consumed, so it survives dependency updates). An untried alternative: `set(CMAKE_DISABLE_FIND_PACKAGE_Freetype TRUE)` so SFML builds its own FreeType.

## Other notes

- C++ standard: the target requires C++20 (`cxx_std_20` in `scripts/cmake/targets.cmake`); some later scratch sketches used C++23.
- The generated build directory is `build/` (ignored by git).

## Second batch: alternatives

- **Per-backend options.** An option for each backend, with defaults on for the window, XML and expression libraries the engine needs first, and a clear error naming what is missing rather than a quiet fallback ([15](15-backend-abstraction.md)).

## Sources

- "CMake file review" (2026-02-06).
- "Organizing CMakeLists.txt with logical chunks" (2026-07-20).
- "Adding Xerces linking to CMake" (2026-07-19), "Setting up SFML3 with CMake" (2026-07-19), "C++23 SFML 3 CMake setup" (2026-09-26).
- Second batch: "Game Object Pipeline Design" (2026-07-21).
