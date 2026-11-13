# Notes for AI coding agents

Tips that are hard to work out from the code quickly. This page is for every change; the others are for the part being changed.

| Page | Read when changing |
|---|---|
| [play.md](play.md) | values, objects and states, frames and touches, spawning, caches, loading |
| [cells.md](cells.md) | grid puzzles, board games, the computer's search |
| [pictures.md](pictures.md) | looks and pictures |
| [tools.md](tools.md) | backends, the build, the generator, `xgegui` |

Found something that cost time and is not here? Add a line to the page for its part.

Rules, build and test commands: [AGENTS.md](../../AGENTS.md). How the engine works: [the reference](../reference/). Why: [the designs](../designs/00-designs.md). What is next: [the plan](../plan.md).

## The repository

- The games are in `games/descriptions/`, one test file per area in `xge/tests/engine/` and per game in `xge/tests/games/`. New work lands on `working` ahead of `master`: check `git branch -r` and `git log` first.
- The author builds on Windows (Visual Studio, vcpkg). Agents build on Linux, often without Qt, and cannot watch or listen.
- Windows headers define macros a Linux build never sees: `near`, `far`, `min`, `max`, `small`, `interface`, `ERROR`. Never name anything one of them (`-Dnear= -Dfar=` on a syntax-only compile catches it).
- MSVC flags what GCC misses, in tests too: C4702 (unreachable code; also a loop whose body always throws, as `fail` does: test the first child with an `if`), C4459 (a local hiding a file-level name).

## Building in a Linux sandbox

- The recipe that works with every backend on: `cmake -S . -B build -G Ninja -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release -DFORCE_LOCAL_GLFW=ON -DBUILD_SHARED_LIBS=ON`, `cmake --build build --parallel`, `cd build && ctest -j4`.
- Never commit a build folder: build in `build/` (ignored) or outside the repository, and read `git status` before `git add`.
- Build and test before and after a change. After any change to the reader, `RawObject`, `RawCommand` or the generator, also `cmake --build build --target xgetest-generated && output/Release/xgetest-generated`.

## Changing the language

A language change is made everywhere in one commit:

| What | Where |
|---|---|
| the schema | `games/xgedef.xsd` |
| the reader | `xge/lib/load/xml.cpp`, and the verb's file in `xge/lib/verbs/` |
| pieces and `from=`, if touched | `xge/lib/load/merge.cpp` and `games/generators/windows-cpp/merge.xsl`, step for step |
| the generator | `games/generators/windows-cpp/data.xsl`, `modules/game_parts.h`, and the describer in `xge/tests/generated/test_generated_games.cpp` |
| the games | every game that uses the old form |
| the tests | the area's `test_<topic>.cpp` |
| the docs | the `docs/reference/` page, the word index in `docs/readme.md`, the matching `docs/designs/` file |

## Tests

- A new schema word needs a game that uses it and a mention in `docs/reference/` (`xgetest "[vocabulary]"`); one no game uses goes in `kUnused` in `test_vocabulary.cpp` with why it stays.
- A test that builds a `Game` without an `Engine` calls `setCurrentState(0)` and `measureShapeSize`, or nothing is shown and sizes are 0.
- A command run between frames leaves its copies waiting: call `settleSpawns()` first. Never hold a reference to a copy across frames; look it up by name.
- The working directory is `games/`, so a test names `descriptions/<game>.xml` and finds the schema with `findSchema("descriptions/pong.xml", "../xgedef.xsd")`. ctest runs from `output/<config>/games`, a plain run from the repository's `games/`: check a new schema test both ways.
- A scratch file is named with `scratchName()`, so `ctest -j` never has two cases writing one file.
- Before touching `circleOverlapsBox`, add a test to `test_collision_geometry.cpp`.
- `invaders_fixture.h` is a frozen copy of the first Space Invaders.

## Docs

- The rules for writing docs are in [AGENTS.md](../../AGENTS.md#writing-docs).
- A change only eyes, ears or MSVC can check adds a line to [Waiting for a look on Windows](../plan.md#waiting-for-a-look-on-windows) in the plan; the author deletes it once seen.

## Privacy

This repository is public. Nothing personal from a chat export goes in it: no health, identity, contact or business details, no conversation titles, and no names beyond the `beefviper` handle in the code headers. Design files call the owner "the author".
