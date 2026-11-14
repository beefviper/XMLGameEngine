# Backends and the library

**Status:** Built: 4 XML, 4 window and 3 sound libraries plus a null window and silence; a library and two programs. Reference: [the engine](../reference/engine.md#backends).

## Why backends

The prototype was hard-wired to Xerces and SFML. Each library sits behind an interface so the core never names one, licensing or weight can be dodged per library, and one can be swapped without touching game logic. A compiling two-library example proved it before adoption.

**Backends are a feature (chosen by the author).** Wide support for libraries is part of what the project is, as an emulator front end supports many.

- Dropping some to save upkeep was proposed and rejected.
- Lower-level ones are wanted next (OpenGL is there; Direct3D and Vulkan to come), and Xerces as the full validator with a lighter built-in one behind the other parsers is intended.
- What follows is making the next backend cheap: with text and image files made by the engine, a backend only opens a window, shows bitmaps and reads keys ([plan](../plan.md#a7-the-next-backend-is-cheap)).

## XML: `XmlDocument` and `XmlNode`

| Decision | Alternative | Why |
|---|---|---|
| A node is an **interface** | one struct everything converts into | libraries store data very differently; a struct copies on every access |
| Children come as fresh `unique_ptr`s | raw pointers into a cache | several nodes alive while a subtree is walked; allocation is irrelevant at load |
| Unfiltered traversal, callers filter by name | positional | element order is not guaranteed |
| Read-only, UTF-8 `std::string` | mutable; `wstring` | RapidXML cannot mutate; library-agnostic |

- **Validation follows the library:** Xerces validates through its own code ("strong"). The others use `load/xsd`, which reads the same `.xsd` ("weak").
  - It covers only the constructs and built-in types the schemas actually use (the author's wish), but within them it is held to XSD and to Xerces: undeclared attributes, attributes on a text-only element, whitespace rules, integer bounds of any size, float and decimal forms, length in characters and bounds facets.
  - A construct it does not read, an unknown type, stray text or an attribute it does not read inside the schema stops the schema loading, naming it, so a new construct is taught to it rather than avoided in the schema.
  - `test_schema_checker.cpp` keeps the two in step: small schemas judged by both, and a sweep of every game changed seven ways (attribute added or changed, element taken out, doubled, renamed, moved, value changed) where the verdicts must match.
  - Open: XSD's schema-wide rules (Unique Particle Attribution, Element Declarations Consistent) are checked by neither: Xerces does so only with full schema checking, which the engine leaves off. A schema that broke them would have to be found by hand.

## Window and sound

- **`Window`** speaks only in engine terms (`Object`, `KeyCode`, `Color`): `init`, `pollEvents`, `controls`, `clear`, `prepare`, `draw`, `display`, `setTitle`. Text and image files are made by the engine (plutovg reads both), so a window shows only bitmaps, circles and rectangles. Every library opens a window of its own and throws if it cannot start. What a window must do is a numbered contract in [the engine](../reference/engine.md#what-a-window-must-do).
- **Text is drawn by the engine**, so it looks and measures the same in every window and a window holds no font code ([text](32-text.md)).
- **A window measures nothing.** Every size is known when the game loads, so a window only turns finished bitmaps into textures and changes nothing on an object but `visualDirty`.
- **`Audio`**: `load`, `play`, `loop`, `stop`, `stopAll`, `update` ([sound](33-sound.md)).
- **Pace** is the engine's, not a library's: each library once kept time its own way (SFML's frame limit, raylib's target, a clock in the OpenGL window, only vsync in SDL2, which ran a game too fast on a fast screen).
  - Now `Engine::loop` steps whole frames by the clock and every window waits for the refresh, so the picture never tears and the speed never depends on the screen.
  - A variable time step was not taken: every number in a game counts frames, and play, demos and tests must come out the same on every machine.
  - Drawing between two frames on a screen faster than the game is open.
- **Factories** are the only places that know every library; a new one is one branch and a pair of files. Choice is at run time; CMake decides only what is built.
- "Backend" stayed over module, provider or adapter: it says "interchangeable lower layer".
- lunasvg is not a backend: it draws inside the engine ([pictures](30-pictures.md)).

## Scale

- **Window `<scale>`:** a game at an arcade's 256 by 224 looked empty in a modern window. The game keeps its size and every library draws it N times bigger (an SFML view, `SDL_RenderSetScale`, raylib's `DrawTexturePro`, the OpenGL viewport); the mouse maps back. Rejected: scaling every number in the file, a free resizable window (a whole scale keeps square pixels). A pixel font at multiples of 8 has every edge on a whole pixel, so it stays sharp.
- **Resized freely:** `xgegui` and `xgecli`'s SFML window fit the game to the window by any factor with its proportions, centred on whole pixels. Whole-number scales only were tried first: a window a little too small for 2x showed 1x with wide bars. Not yet in the SDL2, raylib and OpenGL windows, which stretch.

## Library and programs

- The engine is a library, `xgelib`; `xgecli` and `xgegui` are programs; `xgetest` links the library. A new front end is a `main()` and a few lines in `executables.cmake`.
- **Static by default,** `-DXGE_BUILD_SHARED=ON` for shared. Static `xgecli` is one file that runs beside its games; with two small programs always built together, sharing gains nothing.
- Rejected: one executable with a `--gui` switch; third-party libraries `PRIVATE` (the engine's headers include them; hiding them is a separate change).
- **Null backends** (the author's idea, as in the classic media players): `-a none` (`NullAudio`) and `-w none` (`NullWindow`, `window/null.h`).
  - The null window takes the game's own size, opens nothing and draws nothing; sizes and positions are the game's own, so a game runs headless exactly as in a window.
  - It is what the tests play through (24 hand-written fake windows went), and what `--record`, the long tests and stepping from the command line run on.
  - Always built, never the default.
- Ideas: a null XML library is not wanted (a game needs reading); forward declarations to keep library types out of headers.
