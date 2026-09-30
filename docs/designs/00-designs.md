# XMLGameEngine design index

Each row is one design topic: the options that were considered (a few words each), which one was chosen (if any), and a link to the full write-up. The current, working behavior of the engine is described in [../readme.md](../readme.md). These files record *why*, and what is still an idea.

Status key: **Built** = in the code today. **Decided** = chosen, not built. **Leaning** = a direction stated but not final. **Open** = undecided. **Idea** = floated, not adopted.

| # | Topic | Options considered | Chosen / status | Write-up |
|---|---|---|---|---|
| 01 | Vision and scope | purely declarative / embedded scripting / declarative + triggers | Declarative + `condition` triggers. **Built** (basic) | [01](01-vision-and-scope.md) |
| 02 | File format | XML+XSD / academic text VGDL / YAML / JSON; one file vs rules + level file | XML with XSD, one file. **Built** | [02](02-file-format.md) |
| 03 | Expression syntax | function-call strings / one element per call / structure for real trees / functions to elements, arithmetic stays | Function strings are what exists; removing them is the stated intent. **Leaning** (toward elements) | [03](03-expression-syntax.md) |
| 04 | Arithmetic and XSLT code generation | paste strings / XSLT parses / parse to AST-as-XML then XSLT / remove arithmetic from XML | None yet; AST-as-XML suggested, author prefers no extra tooling. **Open** | [04](04-arithmetic-and-xslt-codegen.md) |
| 05 | Variables and evaluation order | declaration order / two-pass symbol collection; topological sort vs lazy | Two-pass up-front registration. **Built** (chained references: lazy suggested) | [05](05-variables-and-evaluation-order.md) |
| 06 | Names and classes | HTML-style `name` + `class`; compound selectors; class inheritance; unique instance names | `name` + `class` with filters. **Built**; the rest **Idea** | [06](06-names-and-classes.md) |
| 07 | Object variables and references | polling / listeners / copies / raw pointers; `float` vs `Value` variant | Polling by named reference, float variables. **Built**; `Value` variant and handle references **Decided** | [07](07-object-variables-and-references.md) |
| 08 | Entity storage, handles, type identity | vector by value / `unique_ptr` / indices / `{index, generation}` handle / deque; enum vs data types | Generational handles, data-driven types. **Decided**, not built (code uses a vector and name search) | [08](08-entity-storage-and-handles.md) |
| 09 | States and screens | state = screen; flat list / stack; file-prefix naming | Stack of states with shows, inputs, conditions. **Built** | [09](09-states-and-screens.md) |
| 10 | Input and named actions | keys bound directly / key to named action to behavior; held keys across state changes: latched / live / cancelled | Three-layer indirection; held-key combining; a release always reaches what its press started. **Built**. Held keys across state changes: latched to press (built) / live held keys / cancel on state change. **Open** for B and C | [10](10-input-and-actions.md) |
| 11 | Collision detection and response | tangled / split detect, dispatch, execute; edge-only vs swept | Split into detector, typed commands, executor; edge-only. **Built** (rough spots listed) | [11](11-collision-detection-and-response.md) |
| 12 | Collision escalation | per-type-pair rules / class hierarchy / TCG-style verbs and phases | Verb protocol idea from Mario model. **Idea** | [12](12-collision-escalation.md) |
| 13 | Verb vocabulary | small closed set now; static vs dynamic jump; AI targeting modes | Grow only when a game needs it. Small set **Built**; jump and AI **Decided in outline**, not built | [13](13-verb-vocabulary.md) |
| 14 | Conditions and win conditions | `if` / trigger; state-level vs object-level; `>=` only vs richer | State-level threshold `condition`. **Built**, minimal | [14](14-conditions-and-win-conditions.md) |
| 15 | Backend abstraction | concrete node vs interface; pointers vs values; read-only vs mutable; strong vs weak XSD | Interfaces + factories: 4 XML and 3 window backends, Strong/Weak validation. **Built** | [15](15-backend-abstraction.md) |
| 16 | Code layout and pipeline | layered directories / flat files; parse-validate-evaluate-generate | Pipeline concept kept, tree flattened; no `generate` step. **Built** (partial) | [16](16-code-layout-and-pipeline.md) |
| 17 | Reference Pong in C++ | tunables, generic helpers, swept vs simple collision | Kept as the yardstick; swept collision not yet in the engine. **Prototype** | [17](17-reference-pong.md) |
| 18 | Build system | CMake per directory / one root file + modules; find vs FetchContent | One root file, modules in `scripts/cmake/`. **Built** | [18](18-build-system.md) |
| 19 | The VGDL landscape | GDL, RBG, Ludii, PuzzleScript, Griddly, GVGAI | Reference only | [19](19-vgdl-landscape.md) |
| 20 | Ideas parking lot | machine-readable state export; growth components; more | **Idea** | [20](20-ideas-parking-lot.md) |

## Reading order

New to the project: [readme](../readme.md), then 01, 02, 03, 09, 10, 11. Working on the language: 03, 04, 12, 13, 14. Working on the C++: 07, 08, 15, 16.

## Where this came from

Compiled by Claude (Sonnet 5.5) on 2026-09-29 from the author's exported conversation history (see [../agents/sources.md](../agents/sources.md)) and checked against the source on the `claude` branch.
