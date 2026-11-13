# Backends and the library

**Status:** Built: 4 XML, 4 window and 3 sound libraries plus a null window and silence; a library and two programs. Reference: [the engine](../reference/engine.md#backends).

## Why backends

The prototype was hard-wired to Xerces and SFML. Each library sits behind an interface so the core never names one, licensing or weight can be dodged per library, and one can be swapped without touching game logic. A compiling two-library example proved it before adoption.

**Backends are a feature (chosen by the author).** Wide support for libraries is part of what the project is, as an emulator front end supports many.

- Dropping some to save upkeep was proposed and rejected.
- Lower-level ones are wanted next (OpenGL is there; Direct3D and Vulkan to come), and Xerces as the full validator with a lighter built-in one behind the other parsers is intended.
- What follows is making the next backend cheap: text and image files drawn by the engine, so a backend only opens a window, shows bitmaps and reads keys ([plan](../plan.md#a7-the-next-backend-is-cheap)).

## XML: `XmlDocument` and `XmlNode`

| Decision | Alternative | Why |
|---|---|---|
| A node is an **interface** | one struct everything converts into | libraries store data very differently; a struct copies on every access |
| Children come as fresh `unique_ptr`s | raw pointers into a cache | several nodes alive while a subtree is walked; allocation is irrelevant at load |
| Unfiltered traversal, callers filter by name | positional | element order is not guaranteed |
| Read-only, UTF-8 `std::string` | mutable; `wstring` | RapidXML cannot mutate; library-agnostic |

- **Validation follows the library:** only Xerces validates fully ("strong"). The others use `load/xsd`, which reads the same `.xsd` and checks the subset the schema uses ("weak"), deliberately permissive about values. It is kept at the constructs the schemas actually use (the author's wish): one it does not read stops the schema loading, naming it, so a new construct is taught to it rather than avoided in the schema. `test_xml_format.cpp` pins what both must reject.

## Window and sound

- **`Window`** speaks only in engine terms (`Object`, `KeyCode`, `Color`): `init`, `pollEvents`, `controls`, `clear`, `prepare`, `draw`, `display`, `setTitle`. The one thing only a backend can measure is a text's or image's size. Every library opens a window of its own and throws if it cannot start.
- **`Audio`**: `load`, `play`, `loop`, `stop`, `stopAll`, `update` ([sound](33-sound.md)).
- **Pace** is the engine's, not a library's: each library once kept time its own way (SFML's frame limit, raylib's target, a clock in the OpenGL window, only vsync in SDL2, which ran a game too fast on a fast screen).
  - Now `Engine::loop` steps whole frames by the clock and every window waits for the refresh, so the picture never tears and the speed never depends on the screen.
  - A variable time step was not taken: every number in a game counts frames, and play, demos and tests must come out the same on every machine.
  - Drawing between two frames on a screen faster than the game is open.
- **Factories** are the only places that know every library; a new one is one branch and a pair of files. Choice is at run time; CMake decides only what is built.
- "Backend" stayed over module, provider or adapter: it says "interchangeable lower layer".
- lunasvg is not a backend: it draws inside the engine ([pictures](30-pictures.md)).

## Scale

- **Window `<scale>`:** a game at an arcade's 256 by 224 looked empty in a modern window. The game keeps its size and every library draws it N times bigger (an SFML view, `SDL_RenderSetScale`, raylib's `DrawTexturePro`, the OpenGL viewport); the mouse maps back. Rejected: scaling every number in the file, a free resizable window (a whole scale keeps square pixels). Text is drawn unsmoothed, so a pixel font at multiples of 8 stays sharp.
- **Resized freely:** `xgegui` and `xgecli`'s SFML window fit the game to the window by any factor with its proportions, centred on whole pixels. Whole-number scales only were tried first: a window a little too small for 2x showed 1x with wide bars. Not yet in the SDL2, raylib and OpenGL windows, which stretch.

## Library and programs

- The engine is a library, `xgelib`; `xgecli` and `xgegui` are programs; `xgetest` links the library. A new front end is a `main()` and a few lines in `executables.cmake`.
- **Static by default,** `-DXGE_BUILD_SHARED=ON` for shared. Static `xgecli` is one file that runs beside its games; with two small programs always built together, sharing gains nothing.
- Rejected: one executable with a `--gui` switch; third-party libraries `PRIVATE` (the engine's headers include them; hiding them is a separate change).
- **Null backends** (the author's idea, as in the classic media players): `-a none` (`NullAudio`) and `-w none` (`NullWindow`, `window/null.h`).
  - The null window takes the game's own size, measures every object as a window would (text in the built-in 8x8 font, since it has no font file's letters; image files by their pixels) and draws nothing, so a game runs headless with the same sizes and positions.
  - It is what the tests play through (24 hand-written fake windows went), and what `--record`, the long tests and stepping from the command line run on.
  - Always built, never the default.
- Ideas: a null XML library is not wanted (a game needs reading); forward declarations to keep library types out of headers.
