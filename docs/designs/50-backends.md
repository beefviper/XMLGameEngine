# Backends and the library

**Status:** Built: 4 XML, 4 window and 3 sound libraries plus a null window and silence; a library and two programs. Reference: [the engine](../reference/engine.md#backends).

## Why backends

The prototype was hard-wired to Xerces and SFML. Each library sits behind an interface so the core never names one, licensing or weight can be dodged per library, and one can be swapped without touching game logic. A compiling two-library example proved it before adoption.

**Backends are a feature (chosen by the author).** Wide support for libraries is part of what the project is, as an emulator front end supports many.

- Dropping some to save upkeep was proposed and rejected.
- Lower-level ones are wanted next (OpenGL and Direct3D 9 and 11 are there; Vulkan and Direct3D 12 to come), and Xerces as the full validator with a lighter built-in one behind the other parsers is intended.
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

- **`Window`** speaks only in engine terms (`Object`, `KeyCode`, `Color`): `init`, `pollEvents`, `controls`, `clear`, `prepare`, `draw`, `display`, `setTitle`. Every picture is made by the engine (`Object::drawnPicture`; plutovg reads fonts and image files and fills circles), so a window shows only bitmaps. Every library opens a window of its own and throws if it cannot start. What a window must do is a numbered contract in [the engine](../reference/engine.md#what-a-window-must-do).
- **Text and shapes are drawn by the engine**, so they look and measure the same in every window and a window holds no font or drawing code ([text](32-text.md)). Circles had been filled four ways: SFML's polygon, Qt's smoothed ellipse, and a hard pixel test in SDL2 and OpenGL.
- **A window measures nothing.** Every size is known when the game loads, so a window only turns finished bitmaps into textures and changes nothing on an object but `visualDirty`.
- **`Audio`**: `load`, `play`, `loop`, `stop`, `stopAll`, `update` ([sound](33-sound.md)).
- **Pace** is the engine's, not a library's: each library once kept time its own way (SFML's frame limit, raylib's target, a clock in the OpenGL window, only vsync in SDL2, which ran a game too fast on a fast screen).
  - Now `Engine::loop` steps whole frames by the clock and every window waits for the refresh, so the picture never tears and the speed never depends on the screen.
  - A variable time step was not taken: every number in a game counts frames, and play, demos and tests must come out the same on every machine.
  - Drawing between two frames on a screen faster than the game is open.
- **Direct3D 11** (the author wants the low-level libraries too): one file, a Win32 window of its own, every object one textured rectangle drawn by two small shaders compiled when the window opens (no shader files to ship), pads from XInput. It resizes like the SFML window. Built only on Windows, where its SDK comes with Visual Studio; `BUILD_TESTING` turns it on there and nowhere else.
- **Direct3D 9** (older machines): the same rectangles drawn by the fixed pipeline, already in the window's pixels, so no shaders and no compiler. Managed textures outlive a reset, so a resize or a lost device needs only `Reset`. The device is made with `FPU_PRESERVE`: without it Direct3D 9 drops the whole program's sums to single precision.
- **One Win32 window** (`window/win32.cpp`) serves both: its messages, keys, XInput pads, and where the game is fitted in it. A Direct3D backend adds only its drawing. Next: Vulkan, Direct3D 12, each the same shape.
- **One test for every window:** opening each library for real, drawing known pictures and reading the pixels back (`capture()`), with a key pressed as a person would. Hidden from ordinary runs, since it opens windows; under Xvfb on Linux. Rejected: a fake of each library (it tests the fake).
- **Factories** are the only places that know every library; a new one is one branch and a pair of files. Choice is at run time; CMake decides only what is built.
- "Backend" stayed over module, provider or adapter: it says "interchangeable lower layer".
- lunasvg is not a backend: it draws inside the engine ([pictures](30-pictures.md)).

## Scale

- **Window `<scale>`:** a game at an arcade's 256 by 224 looked empty in a modern window. The game keeps its size and every library draws it N times bigger (an SFML view, `SDL_RenderSetScale`, raylib's `DrawTexturePro`, the OpenGL viewport); the mouse maps back. Rejected: scaling every number in the file, a free resizable window (a whole scale keeps square pixels). A pixel font at multiples of 8 has every edge on a whole pixel, so it stays sharp.
- **Sharp at any size** (the author's idea): a window says how many of its pixels show one of the game's (`fit()`), and once that has held still a quarter of a second (`SettledFit`) the engine has it draw text, circles and rectangles again with that many pixels each (`pictureScale()`). While a window is dragged it stretches what it has, so a resize does not redraw every frame.
  - Positions, sizes and touches stay in the game's pixels; only the drawing has more pixels, stretched back over the object's size. Play never notices.
  - Bitmaps and image files keep their own pixels, shown hard edged. Games scale by any factor, so a pixel font at an odd scale has slightly uneven pixels but sharp edges.
  - Open: an SVG is a bitmap made at load, so it stretches; redrawing it would mean keeping how it was made, turned and recoloured.
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
