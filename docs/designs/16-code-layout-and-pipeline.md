# 16. Code layout and the compile pipeline

**Status:** concept adopted; the current tree is flat

## The compiler-pipeline model

A game description is treated like a program in a language with a front end and back end: **parse, validate, evaluate, generate**. Validation as its own explicit stage makes it much easier to catch a bug in one stage leaking into another (an earlier Pong condition bug was of that kind). The current code has this shape: `game_xml` parses and validates, `game_expr` evaluates, and the results are `Object`s and `State`s.

## The July 2026 skeleton

A restructure drafted this tree (empty files), reviewed in conversation:

- `core/engine`: `loop_input`, `loop_update`, `loop_render` (three loop phases, so update can be driven without a window in tests) and a two-tier layer of `system_*` (thin OS pieces: clock, console, file, memory, thread) below `service_*` (logger, resource, timer), where services depend on systems and never the reverse.
- `core/game`: `compile_parse/validate/evaluate/generate` plus data types (`model_object`, `model_states`).
- `window`: interface, factory, SDL2 and SFML3 backends.
- `xml`: interface, factory, TinyXML2 and Xerces backends.

Feedback given: the layering and the three-phase loop are sound; things worth deciding were whether `types.h` becomes a dumping ground, where the collection of live objects and the currently active state live, whether the build wiring uses `#ifdef` or a run-time factory, and that audio and configuration were absent (the author confirmed both were intentionally missing for now).

## What the tree looks like now

The layered directories were later flattened into `include/` and `source/` files with the same responsibilities (`game_xml`, `game_expr`, `game`, `engine`, `command`, `command_executor`, `collision_detector`, `window_*`, `xml_*`, `xsd_lite`). The command line (`cli.cpp`, `main.cpp`) later moved out of the engine into `cli/`, and the engine became a library ([35](35-library-and-front-ends.md)). See the source map in [docs/readme.md](../readme.md).

## Open

- The flat tree has grown to 23 source and 23 header files and needs sorting into folders again. The earlier attempt (the `rewrite` branch, mostly empty stubs, reviewed and retired 2026-09-30) was too layered for the code that existed; a lighter grouping by responsibility (game loading and evaluation, engine loop, collision and commands, window backends, XML backends), fits what is there now. The Visual Studio filters are to be built from the directories, so this is only about the layout on disk.
- Audio service, configuration service.
- A `generate` step: nothing emits code from the description yet ([04](04-arithmetic-and-xslt-codegen.md)).
- Whether a "live snapshot" (current state plus current object instances) should be its own type instead of living inside `Game` ([09](09-states-and-screens.md)).

## Second batch: alternatives

- **Load-time construction.** The game object runs parse, validate, evaluate and generate on creation; if any step fails, no object exists. Caveat: keep the constructor small by delegating to a builder or factory that returns a valid game or an error.
- **Names for the stages and layers.** Builder versus factory versus compiler; prefixes for phases or managers. The compiler view: XML to raw model, verified model, resolved model, runtime.
- **Tables resolved once.** Verb names could be turned into handlers when the file loads, so the running game never looks up strings ([25](25-targets-and-capability-profiles.md)).
- **Phase order.** Input, update, render in the engine; collisions between update and render.

## Sources

- "XML game engine project structure review" (2026-07-20).
- "Project layout conversation B" (2026-07-20).
- Second batch: "Game Object Pipeline Design" (2026-07-21), "XML Architecture Review" (2026-07-21), "Data-driven tables versus code" (2025-09-04).
