# 15. Backend abstraction: XML libraries and windowing libraries

**Status:** implemented (4 XML backends, 3 window backends)

## Why

The prototype was hard-wired to Xerces and SFML. The rewrite puts each behind an interface so the engine core never names a library, licensing or dependency-weight problems can be dodged per backend, and a backend can be swapped without touching game logic.

## XML: `XmlDocument` / `XmlNode`

Started from the author's own sketch: a node with a name, attributes as key/value pairs, and child nodes, plus functions to walk the tree (root node, next child, next sibling).

Design decisions and the alternatives:

| Decision | Alternatives | Why |
|---|---|---|
| Node type is an **interface**, not a concrete struct | One concrete `Node` struct everything converts into | Libraries store data very differently (Xerces has a full DOM with reference counting, pugixml a flat memory pool with light handles, RapidXML parses in place over a mutable buffer). A concrete struct means copying on every access. |
| `getFirstChild()` / `getNextSibling()` return a fresh `unique_ptr` node | Raw pointers into a shared cache; lightweight value handles | Several nodes are alive at once while a subtree is walked; plain ownership is simpler. A new allocation per call is irrelevant because the file is read once at load. |
| Traversal is unfiltered (any tag name); callers filter by name | Name-filtered traversal, fixed positions | The schema's elements are not guaranteed to stay in a fixed order; lookup by name is robust. |
| **Read-only** interface | One interface with mutation | Some libraries (RapidXML in non-owning mode) cannot mutate. If writing XML is ever wanted, use a separate interface. |
| `std::string` (UTF-8) at the boundary | `std::wstring` or `XMLCh*` | Keeps the interface library-agnostic. Xerces uses UTF-16 internally, so its adapter pays a transcode cost per string; irrelevant at load time. |

Future extras that were suggested: a range-for iterator over children built on first-child/next-sibling, and a pool inside each backend so nodes are not allocated per call.

A compiling two-backend example (pugixml and Xerces walking the same tree through the same `main.cpp`) was built during the design conversation to prove the approach before it was adopted.

Interface hygiene: an abstract base class gets a `virtual ~X() = default;` and nothing else: no data members means nothing for a constructor to do, and deleting through a base pointer without a virtual destructor is undefined behavior. Copying through the base type risks slicing.

### Validation follows the backend

Only Xerces can do real XSD validation. For the other three backends the project has its own small validator (`xsd_lite`) that reads the same `.xsd` file through the same abstract interface (an XSD is just XML) and checks only the subset the schema uses (sequence, element, complexType, attribute). It is deliberately permissive: an attribute type it does not recognize passes unchecked, so it never reports a false failure, but a pass is weaker evidence than a Xerces pass. The load reports **Strong** (Xerces) or **Weak** (built-in) so it is always clear which one ran.

## Windows: `Window`

A window backend provides lifecycle, `init()` (build and measure each object's visual), `pollEvents()` (key changes as `{key, pressed}` pairs), `clear`, `draw`, `display`. Everything is in engine terms (`WindowDesc`, `Object`, `KeyCode`, an engine `Color` and `Vector2f`), so no library type leaks upward. Backends: **SFML3**, **Raylib**, **SDL2**, **OpenGL** (GLFW), and in `XGEGUI` a fifth that draws with Qt; a window can also draw into a front end's window or to a back buffer instead of opening its own ([39](39-opengl-backend-and-options.md)). `Object::size` is the one thing only a backend can measure (text and image sizes depend on real fonts and files); collision code reads that field and never a backend type.

## Both use the same factory pattern

`XmlDocumentFactory::create(backend)` and `WindowFactory::create(desc, backend)` are the only places that know every implementation. Adding a fifth XML library or fourth window library is one branch and one new pair of files.

## Open

- A command-line switch for the backends (defaults are Xerces and SFML3).
- Audio has no interface yet.
- Compile-time selection with `#ifdef` versus this run-time factory: the factory is used; the CMake options only decide which libraries get built.

## Second batch: alternatives

- **The word backend.** Alternatives: implementation, module, service, provider, adapter. Backend stayed because it says what it is (an interchangeable lower layer), and it still fits when the subsystem list grows: window, graphics, input, audio, network, XML, expressions, filesystem.
- **Null and software backends.** A null implementation of each subsystem removes checks for absence; a software renderer is the last fallback ([25](25-targets-and-capability-profiles.md)).
- **Audio.** No audio vocabulary exists; when there is one, an audio interface with a play-by-name call and a null implementation would follow the same pattern.
- **Validator at the engine layer.** Put schema validation in the engine, use the parser's own when it exists, fall back otherwise, so a new parser only has to parse.
- **Selection.** A build option per backend and an error when none is chosen, in place of silently searching until something is found; optionally build everything and choose at runtime ([18](18-build-system.md)).
- **Forward declarations.** Keep third-party types out of headers with forward declarations so they do not leak.
- **Engine owns the loop.** The main loop lives in the engine and knows only an interface; the game holds plain data and never touches a library; the engine turns updated state into draw instructions for the current backend.

## Sources

- "C++ interface conversation A" (2026-07-27).
- "C++ interface conversation B" (2026-07-21).
- "C++ interface conversation C" (2026-07-19).
- "XML game engine project structure review" (2026-07-20).
- Second batch: "Engine Abstraction Design" (2026-07-20), "Engine and Game Separation" (2026-01-29), "Game Object Pipeline Design" (2026-07-21).
