# 35. Library and front ends

**Status:** built (`CMakeLists.txt`, `scripts/cmake/executables.cmake`, `cli/`, `gui/`)

## Decision

The engine is a library, the `XMLGameEngine` target. Everything that loads, runs, draws and tests a game is in it. What was in `main.cpp` and `cli.cpp` (read the arguments, find the game file, build `Game` and `Engine`, run the loop) moved to `cli/` and is a program of its own, `XGECLI`. A second program, `XGEGUI` (`gui/main.cpp`), is a stub that returns at once, to be the graphical front end later. The test suite links the library too, so it no longer compiles the engine's files a second time.

A new front end is now a `main()` and a few lines in `executables.cmake`; none of the engine changes.

## Static or shared

- **Static** (the default): the engine's code is copied into each program when it is linked, so `XGECLI` is one file that runs anywhere it is put (next to its games and assets). Nothing to lose or mismatch, and the linker drops what a program does not use. The cost is that two programs each carry a copy, and a fixed engine means rebuilding every program to change it.
- **Shared** (a `.dll` on Windows, `.so` on Linux): one copy of the engine that programs load when they start. It saves disk and memory only when several programs share it, and it lets the engine be replaced without relinking the programs, but the library file has to be found at run time (next to the program, or on the system's library path), a program and a library of different versions can fail in odd ways, and on Windows every class that crosses the boundary has to be exported.

With two small programs from one repository that are always built together, nothing is gained from sharing, so static is the default. `-DXGE_BUILD_SHARED=ON` builds a shared library instead, with no source changes: on Windows it exports every symbol (`WINDOWS_EXPORT_ALL_SYMBOLS`) rather than marking each class by hand, and puts the DLL beside the programs. The tests were run both ways.

## Options considered

- **Keep one executable and add a `--gui` switch.** Simplest, but the command-line code and a window front end would live in one program for good.
- **A `CMakeLists.txt` in each folder.** Rejected earlier ([18](18-build-system.md)); the programs are defined in one file, `executables.cmake`, in the top-level scope so they land in the build directory next to the copied `games/` and `assets/`.
- **Third-party libraries `PRIVATE` to the engine.** Not possible yet: the engine's own headers include exprtk, Xerces, SFML and the others, so a program including `engine.h` needs them too. They are `PUBLIC`. Hiding them behind interfaces would be a separate change.
- **Rename the library target.** `XMLGameEngine` stays the project and library name; the programs are named for what they are.

## Approximations

`XGEGUI` links the library but uses nothing from it. `cli.h` is not in the library's include directory, only `XGECLI` sees it. The run was checked with the window backends replaced by a stub, because the sandbox that built it has no SFML, raylib or SDL2 libraries: configure, build, link and the whole test suite passed for both the static and the shared library, but `XGECLI` has not been run against a real window from this layout.
