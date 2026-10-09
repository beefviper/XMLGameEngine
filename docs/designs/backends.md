# Backends and the library

**Status:** Built: 4 XML, 4 window and 3 audio backends (plus silent), library plus two programs.

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
- **Audio: `Audio`:** `load()`, `play(name)`, `stopAll()`; SFML3, Raylib, SDL2, `NullAudio` ([sound](sound.md)).
- **Factories** (`XmlDocumentFactory`, `WindowFactory`, `AudioFactory`) are the only places that know every implementation; a new library is one branch and one pair of files. Selection is a run-time factory; CMake options only decide which libraries get built.
- The word "backend" stayed over implementation/module/service/provider/adapter: it says "interchangeable lower layer" and still fits as subsystems grow (window, graphics, input, audio, network, XML, expressions, filesystem). The engine owns the loop and knows only interfaces; the game is plain data.
- Not an interface: lunasvg draws SVG inside the engine ([pictures](pictures.md)).
- Ideas: a software renderer and null implementations of every subsystem so the engine never checks for absence ([ideas](ideas.md)); forward declarations to keep third-party types out of headers (they are `PUBLIC` today, so programs including `engine.h` need them).


## Library and programs

- The engine is a library, `xgelib` (`lib/`). `xgecli` ([cli](cli.md)) and `xgegui` are programs; `xgetest` links the library. `xgedata` copies `games/` and `assets/`. A new front end is a `main()` and a few lines in `executables.cmake`.
- Layout: one folder per project each with `source/` and `include/` (`lib/`, `cli/`, `gui/`); `tests/`, `games/`, `assets/` and `xgedef.xsd` stay at the top. The schema is the definition of the language, not media a game consumes, so it sits at the root and not in `assets/`; games name it as `../xgedef.xsd`. (Before, the engine's `source/`/`include/` sat at the top so the programs looked like extras.)
- **Static by default, shared with `-DXGE_BUILD_SHARED=ON`.** Static: `xgecli` is one file that runs anywhere beside its games and assets; nothing to mismatch. Shared: one copy, replaceable without relinking, but the library must be found at run time and on Windows every crossing class needs exporting (`WINDOWS_EXPORT_ALL_SYMBOLS`). With two small programs always built together, nothing is gained from sharing. Tests were run both ways.
- Rejected: one executable with a `--gui` switch; a `CMakeLists.txt` in every folder; third-party libraries `PRIVATE` (the engine's headers include exprtk, Xerces, SFML and others; hiding them behind interfaces is a separate change).
- Targets are all `XGE`-named; the CMake project is `XMLGameEngine`.
