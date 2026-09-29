# AGENTS.md

XMLGameEngine is a video game description language in XML plus a C++ engine that runs it.

Start here, in order:

1. [docs/readme.md](docs/readme.md): how the engine works today (file format, verbs, collisions, backends, known limitations).
2. [docs/designs/00-designs.md](docs/designs/00-designs.md): index of design decisions, the options considered, and what is still only an idea.
3. [docs/agents/notes.md](docs/agents/notes.md): gotchas and suggested next steps for agents.

Conventions:

- Every C++ source or header file begins with this comment block, with the real filename and date:

  ```
  // main.cpp
  // XML Game Engine
  // author: beefviper
  // date: Sept 18, 2020
  ```

- Files end with exactly one trailing newline.
- Keep `docs/readme.md` and `docs/designs/` in step with code changes.

Build: CMake, see `scripts/cmake/` (dependencies are found through vcpkg or fetched). Tests are Catch2 and opt-in: configure with `-DBUILD_TESTING=ON`.
